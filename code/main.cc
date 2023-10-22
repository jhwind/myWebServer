#include <unistd.h>
#include "server/webserver.h"

int main() {
    WebServer server(
        1316,                           // 端口
        3,                              // ET模式
        60000,                          // timeoutMs
        false,                          // 优雅退出
        3306,
        Config::SQL_USERNAME.c_str(),   // mysql用户名
        Config::SQL_PSW.c_str(),        // mysql密码
        Config::SQL_DBNAME.c_str(),     // 数据库名
        8,                              // 连接池数量
        16,                             // 线程池数量
        false,                          // 日志开关
        1,                              // 日志等级
        1024);                          // 日志异步队列容量
    
    server.Start();
}