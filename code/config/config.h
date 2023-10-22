#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace Config{
    // LOG
    const std::string LOG_PATH = "./log";
    const std::string LOG_SUFFIX = ".log";
    const int LOG_MAXCAPACITY = 1024;
    const bool LOG_OPENLOG = true;

    // SQL
    const std::string SQL_IP = "localhost";
    const int SQL_PORT = 3306;
    const std::string SQL_USERNAME = "root";
    const std::string SQL_PSW = "root";
    const std::string SQL_DBNAME = "yourdb";
    const int SQL_CONNNUM = 8;

    // BASE
    // const std::string SOURCE_DIR = "/home/ubuntu/project/cppproject/MyWebServer/resources";
    const int SYS_PORT = 8079;
    const int SYS_THREADNUM = 16;
    const int SYS_TIMEOUT = 60000;
    const int SYS_TRIGEMODE = 3;
    const bool SYS_OPTLINER = false;
    
}

#endif // CONFIG_H