#ifndef SIGUTILS_H
#define SIGUTILS_H
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>

#include <signal.h>

class SigUtils {
public:
    SigUtils(){}
    ~SigUtils(){}

    // 信号处理函数
    static void SigHandler_(int sig);
    // 设置信号函数
    void AddSig_(int sig, void(handler)(int), bool restart = true);
    // 传递信号到主循环的管道
    static int *u_pipefd;
};

#endif