#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller {
public:
    // 创建内核事件表 以及 初始化就绪事件数组
    explicit Epoller(int maxEvent = 1024); 
    ~Epoller();

    // 注册事件
    bool AddFd(int fd, uint32_t events);  
    // 修改（重置）事件
    bool ModFd(int fd, uint32_t events);  
    // 删除事件
    bool DelFd(int fd);  
    // 开始监测
    int Wait(int timeoutMs = -1);  
    // 获取第i个事件的文件描述符
    int GetEventFd(size_t i) const;  
    // 获取第i个事件的事件类型
    uint32_t GetEvents(size_t i) const;  
        
private:
    // 内核事件表的文件描述符
    int epollFd_;  
    // 就绪事件数组
    std::vector<struct epoll_event> events_;  
};

#endif //EPOLLER_H