#include <unistd.h>
#include "server/webserver.h"
#include "config/config.h"

int main(int argc, char *argv[]) {
    /* 守护进程 后台运行 */
    //daemon(1, 0); 
    
    Config config;
    config.ParseCmd(argc, argv);

    WebServer server(
        config.port, config.trigMode, config.timeoutMS, config.OptLinger,
        config.sqlPort, config.sqlUser, config.sqlPwd, config.dbName,
        config.connPoolNum, config.threadNum, config.openLog, config.logLevel, config.logQueSize, config.actor);

    server.Start();

    return 0;
}
  