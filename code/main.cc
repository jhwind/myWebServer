#include <unistd.h>
#include "server/webserver.h"

int main() {
    WebServer server(
        1316, 3, 60000, false,                  // 端口 ET模式 timeoutMs 优雅退出
        3306,
        Config::SQL_USERNAME.c_str(), 
        Config::SQL_PSW.c_str(), 
        Config::SQL_DBNAME.c_str(), 
        12,                                     // 连接池数量
        6,                                      // 线程池数量
        true, 3, 1024);                         // 日志开关 日志等级 日志异步队列容量
    server.Start();
}