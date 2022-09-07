#include "config.h"
#include "../log/log.h"
#include <stdio.h>

Config::Config() {
    // 端口
    port = 9006;
    // LT+ET模式
    trigMode = 1;
    // timeoutMs
    timeoutMS = 60000;
    // 优雅退出
    OptLinger = false;
    // Mysql配置
    sqlPort = 3306;
    sqlUser = "ysq";
    sqlPwd = "123";
    dbName = "ysqdb";
    // 连接池数量
    connPoolNum = 12;
    // 线程池数量
    threadNum = 8;
    // 日志开关
    openLog = true;
    // 日志等级
    logLevel = 1;
    // 阻塞队列容量
    logQueSize = 1024;
    // 事件处理模式默认为reactor
    actor = 0;
}

void Config::ParseCmd(int argc, char *argv[]) {
    int opt;
    const char *str = "p:l:m:o:s:t:e:a:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt)
        {
            case 'p':
                port = atoi(optarg);
                break;
            case 'l':
                openLog = atoi(optarg);
                break;
            case 'm':
                timeoutMS = atoi(optarg);
                break;
            case 'o':
                OptLinger = atoi(optarg);
                break;
            case 's':
                connPoolNum = atoi(optarg);
                break;
            case 't':
                threadNum = atoi(optarg);
                break;
            case 'e':
                logLevel = atoi(optarg);
                break;
            case 'a':
                actor = atoi(optarg);
                break;
            default:
                break;
        }
    } 
}