#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log {
public:
    // 初始化日志实例，创建日志文件
    void init(int level, const char* path = "./log", const char* suffix =".log",int maxQueueCapacity = 1024);
    // 单例懒汉
    static Log* Instance();  
    // 写线程函数必须为静态的（因为类内函数自带this参数），调用AsyncWrite_()
    static void FlushLogThread();  

    // 写进日志文件（传入日志等级，格式format，可变参数va_list）
    void write(int level, const char *format,...);  
    // 刷新缓冲区
    void flush();  
    // 获取日志等级
    int GetLevel();  
    // 设置日志等级
    void SetLevel(int level);  
    // 返回日志是否开启
    bool IsOpen() { return isOpen_; }  
    
private:
    Log();
    // 为一条日志信息添加类型
    void AppendLogLevelTitle_(int level);  
    virtual ~Log();
    // 从阻塞队列取出一条信息并写入日志文件
    void AsyncWrite_();  

private:
    // 日志目录路径最大长度 
    static const int LOG_PATH_LEN = 256;
    // 日志文件名最大长度
    static const int LOG_NAME_LEN = 256;  
    // 日志文件最大行数
    static const int MAX_LINES = 50000;  

    // 日志目录的路径
    const char* path_;  
    // 日志文件名后缀
    const char* suffix_;  

    int MAX_LINES_;
    // 当前行数
    int lineCount_;  
    // 日期
    int toDay_;  
    // 是否开启日志
    bool isOpen_;  
    // 日志缓冲区
    Buffer buff_;  
    // 日志等级
    int level_;  
    // 是否异步写入
    bool isAsync_;  
    // 文件指针
    FILE* fp_;  
    // 阻塞队列
    std::unique_ptr<BlockDeque<std::string>> deque_;  
    // 写线程（日志系统是单线程模式）
    std::unique_ptr<std::thread> writeThread_;  
    // 日志互斥🔓
    std::mutex mtx_;  
};

/*
 * 系统中有4种类型的日志，分别是LOG_DEBUG、LOG_INFO、LOG_WARN与LOG_ERROR，它们共同使用LOG_BASE，以level来区分不同级别的日志
 * 初始设置的日志等级可以控制不同级别的日志是否被记录
*/
// 写入日志文件（日志类型必须小于等于系统的日志等级才能输出，比如日志等级设置为1，那么DEBUG类型的日志无法输出）
#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H