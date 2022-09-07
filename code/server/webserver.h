#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"
#include "../signal/sigutils.h"

class WebServer {
public:
    WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger, 
        int sqlPort, const char* sqlUser, const char* sqlPwd, const char* dbName, 
        int connPoolNum, int threadNum, bool openLog, int logLevel, int logQueSize, int actor);

    ~WebServer();
    // 运行server
    void Start();

private:
    // 初始化监听socket
    bool InitSocket_();
    // 初始化传递信号的管道
    bool InitPipe_();
    // 初始化触发组合模式
    void InitEventMode_(int trigMode);
    // 添加客户端
    void AddClient_(int fd, sockaddr_in addr);
    
    // 获取新连接，初始化客户端数据
    void DealListen_();
    // 处理信号事件
    void DealSignal_();
    // 调用ExtentTime_，并将“写”任务加入线程池的工作队列
    void DealWrite_(HttpConn* client);
    // 调用ExtentTime_，并将“读”任务加入线程池的工作队列
    void DealRead_(HttpConn* client);
    // 发送错误信息给客户端并关闭连接
    void SendError_(int fd, const char*info);
    // 延长client的定时器的超时时长
    void ExtentTime_(HttpConn* client);
    // 关闭连接
    void CloseConn_(HttpConn* client);

    // 读取数据，并调用OnProcess处理请求
    void OnRead_(HttpConn* client);
    // 向客户端发送响应
    void OnWrite_(HttpConn* client);
    // 调用process解析请求生成响应，然后修改监测事件：若生成了响应则改为监测写事件，否则说明没有解析请求，改为监测读事件
    void OnProcess(HttpConn* client);

    // 最大文件描述符数量
    static const int MAX_FD = 65536;
    // 设置非阻塞
    static int SetFdNonblock(int fd);

    // 监听端口
    int port_;
    // 是否优雅关闭
    bool openLinger_;
    // 毫秒MS
    int timeoutMS_;
    // 是否关闭服务器
    bool isClose_;
    // 监听文件描述符
    int listenFd_;
    // 文件根目录
    char* srcDir_;

    // 传递信号的管道
    int pipefd_[2];
    SigUtils sigutils_;
    
    // 监听fd上事件
    uint32_t listenEvent_;
    // 连接fd上的事件
    uint32_t connEvent_;
   
    // 时间堆
    std::unique_ptr<HeapTimer> timer_;
    // 线程池
    std::unique_ptr<ThreadPool> threadpool_;
    // 监听实例
    std::unique_ptr<Epoller> epoller_;
    // 客户端连接集合
    std::unordered_map<int, HttpConn> users_;

    // 事件处理模式（reactor:0 or proactor:1）
    int actor_;
};


#endif //WEBSERVER_H