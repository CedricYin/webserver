#ifndef CONFIG_H
#define CONFIG_H

#include <sys/types.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

class Config {
public:
    // 服务器默认配置
    Config();
    ~Config(){}

    // 解析命令行，实现个性化运行
    void ParseCmd(int argc, char *argv[]);
    // 后台化
    int SetDaemon();

    int port;
    int trigMode;
    int timeoutMS;
    bool OptLinger;
    int sqlPort;
    const char* sqlUser;
    const  char* sqlPwd;
    const char* dbName;
    int connPoolNum;
    int threadNum;
    bool openLog;
    int logLevel;
    int logQueSize;
    int actor;
};

#endif