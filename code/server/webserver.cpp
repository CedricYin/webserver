#include "webserver.h"

using namespace std;

WebServer::WebServer(
            int port, int trigMode, int timeoutMS, bool OptLinger,
            int sqlPort, const char* sqlUser, const  char* sqlPwd,
            const char* dbName, int connPoolNum, int threadNum,
            bool openLog, int logLevel, int logQueSize, int actor):
            port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMS), isClose_(false), sigutils_(),
            timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller()), actor_(actor)
    {
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    uploadDir_ = getcwd(nullptr, 256);
    strcat(uploadDir_, "/resources/fromclients/");
    HttpConn::RequestCount = 0;
    HttpConn::srcDir = srcDir_;
    HttpConn::uploadDir = uploadDir_;
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    InitEventMode_(trigMode);
    if(!InitSocket_() || !InitPipe_()) { isClose_ = true;}

    if(openLog) {
        Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        if(isClose_) { LOG_ERROR("========== Server init error!=========="); }
        else {
            time_t timer = time(nullptr);
            struct tm *t = localtime(&timer);
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Date: %04d-%02d-%02d, Time: %02d:%02d:%02d", 
                    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
            LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                        (listenEvent_ & EPOLLET ? "ET": "LT"),
                        (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("Actor Mode: %s", actor_ ? "Proactor" : "Reactor");
            LOG_INFO("LogSys Status: %s", openLog ? "Open" : "Close");
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
            LOG_INFO("TimeOut: %ds", timeoutMS / 1000);
            LOG_INFO("DataBase: %s, SqlUser: %s, SqlPort: %d", dbName, sqlUser, sqlPort);
        }
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
    close(pipefd_[0]);
    close(pipefd_[1]);
}

void WebServer::InitEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::Start() {
    int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    if(!isClose_) { LOG_INFO("========== Server start =========="); }
    while(!isClose_) {
        if(timeoutMS_ > 0) {
            timeMS = timer_->GetNextTick();
        }
        // 第一次调用是阻塞的（timeMS为-1），接下来每次调用timeMS为时间堆顶超时时长，返回0说明超时，不会调用下面的for循环
        int eventCnt = epoller_->Wait(timeMS);
        for(int i = 0; i < eventCnt; i++) {
            /* 处理事件 */
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if(fd == listenFd_) {  // 新的连接
                DealListen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {  // 异常事件
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if(fd == pipefd_[0] && (events & EPOLLIN)) {  // 有信号处理事件
                DealSignal_();
            } 
            else if(events & EPOLLIN) {  // 有数据可读
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }
            else if(events & EPOLLOUT) {  // 有数据可写
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            }
            else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::SendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    // LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if(timeoutMS_ > 0) {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock(fd);
    // LOG_INFO("Client[%d] in!", users_[fd].GetFd());
    LOG_INFO("Client[%d](%s:%d) in, RequestCount:%d", fd, users_[fd].GetIP(), users_[fd].GetPort(), (int)users_[fd].RequestCount);
}

void WebServer::DealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { return;}
        else if(HttpConn::RequestCount >= MAX_FD) {
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd, addr);
    } while(listenEvent_ & EPOLLET);
}

void WebServer::DealSignal_() {
    int ret = 0;
    char signals[1024];
    ret = recv(pipefd_[0], signals, sizeof(signals), 0);
    if(ret == 0 || ret == -1) return ;

    for(int i = 0; i < ret; ++ i) {
        switch (signals[i])
        {
            case SIGINT:
            case SIGTERM:
                isClose_ = true;
            default:
                break;
        }
    }
}

void WebServer::DealRead_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);
    if(actor_ == 0)  // reactor
        threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
    else {  // proactor
        int readErrno = 0;
        int ret = client->read(&readErrno);
        if(ret <= 0 && readErrno != EAGAIN) {
            CloseConn_(client);
            return;
        }
        threadpool_->AddTask(std::bind(&WebServer::OnProcess, this, client));
    }
}

void WebServer::DealWrite_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);
    if(actor_ == 0)  // reactor
        threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
    else  // proactor
        OnWrite_(client);
}

void WebServer::ExtentTime_(HttpConn* client) {
    assert(client);
    if(timeoutMS_ > 0) { timer_->adjust(client->GetFd(), timeoutMS_); }
}

void WebServer::OnRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnProcess(HttpConn* client) {
    if(client->process()) {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::OnWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0) {
        /* 传输完成 */
        if(client->IsKeepAlive()) {
            // OnProcess(client);
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);  // 换成监测读事件
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

/* Create listenFd */
bool WebServer::InitSocket_() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = { 0 };
    if(openLinger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }
    ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }
    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

bool WebServer::InitPipe_() {
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd_);
    assert(ret != -1);
    ret = epoller_->AddFd(pipefd_[0], EPOLLRDHUP | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add pipefd[0] error!");
        close(pipefd_[0]);
        close(pipefd_[1]);
        return false;
    }
    SetFdNonblock(pipefd_[1]);
    sigutils_.AddSig_(SIGPIPE, SIG_IGN);
    sigutils_.AddSig_(SIGINT, SigUtils::SigHandler_, false);
    sigutils_.AddSig_(SIGTERM, SigUtils::SigHandler_, false);
    SigUtils::u_pipefd = pipefd_;
    return true;
}

int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}


