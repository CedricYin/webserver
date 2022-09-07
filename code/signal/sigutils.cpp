#include "sigutils.h"

void SigUtils::SigHandler_(int sig) {
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
}

void SigUtils::AddSig_(int sig, void(handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;

    if(restart) sa.sa_flags |= SA_RESTART;

    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

int *SigUtils::u_pipefd = 0;