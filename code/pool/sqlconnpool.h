#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
public:
    // 单例懒汉
    static SqlConnPool *Instance();  
    // 从连接池取走一个连接
    MYSQL *GetConn();  
    // 释放一个连接，还给连接池
    void FreeConn(MYSQL * conn);  
    // 获取可用连接数量（connQue_队列长度）
    int GetFreeConnCount();  

    // 初始化，设置主机，端口，用户，密码，库名，连接池大小
    void Init(const char* host, int port, const char* user,const char* pwd, const char* dbName, int connSize);
    // 关闭连接池
    void ClosePool();  

private:
    SqlConnPool();
    ~SqlConnPool();
    // 最大连接数
    int MAX_CONN_;  
    // 当前连接数
    int useCount_;  
    // 空闲连接数
    int freeCount_; 
    // 空闲连接队列
    std::queue<MYSQL *> connQue_;  
    // 互斥锁
    std::mutex mtx_;  
    // 信号量
    sem_t semId_;  
};


#endif // SQLCONNPOOL_H