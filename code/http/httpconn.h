#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);

    // 读写数据
    ssize_t read(int* saveErrno);
    ssize_t write(int* saveErrno);

    // 关闭该连接
    void Close();  

    // 获取该连接的信息
    int GetFd() const;
    int GetPort() const;
    const char* GetIP() const;
    sockaddr_in GetAddr() const;
    
    // 解析请求并生成响应
    bool process();  

    // 返回需要写的数据长度
    int ToWriteBytes() {  
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    // 返回是否长连接
    bool IsKeepAlive() const {  
        return request_.IsKeepAlive();
    }

    // 是否是ET模式
    static bool isET;
    // 资源目录
    static const char* srcDir;
    //  当前需要处理的请求数量
    static std::atomic<int> RequestCount;
    
private:
    // 该连接的文件描述符
    int fd_;
    // 客户端地址信息
    struct  sockaddr_in addr_;
    // 该连接是否关闭
    bool isClose_;
    
    // 缓冲区块
    int iovCnt_;
    struct iovec iov_[2];
    
    // 读缓冲区
    Buffer readBuff_; 
    // 写缓冲区
    Buffer writeBuff_; 

    // 请求
    HttpRequest request_;  
    // 响应
    HttpResponse response_;  
};


#endif //HTTP_CONN_H