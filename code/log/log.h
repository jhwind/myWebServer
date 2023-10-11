#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>				// vastart va_end
#include <assert.h>
#include <sys/stat.h>         	// mkdir
#include "blockqueue.h"
#include "../buffer/buffer.h"
#include "../config/config.h"

class Log {
private:
	static const int LOG_PATH_LEN = 256;
	static const int LOG_NAME_LEN = 256;
	static const int MAX_LINES = 50000;

	const char *path_;
	const char *suffix_;

	Buffer buff_;
	int maxLines_;
	int lineCount_;
	int lastday_;	// 当前日志文件日期
	bool isOpen_;
	int level_;
	bool isAsync_;

	FILE *fp_;
	std::unique_ptr<BlockDeque<std::string>> deque_;
	std::unique_ptr<std::thread> writeThread_;
	std::mutex mtx_;

private:
	Log();
	virtual ~Log();
	void AppendLogLevelTitle_(int level);
	void AsyncWrite_();

public:
	void Init(int level, 
		   	  const char *path = Config::LOG_PATH.c_str(), 
			  const char *suffix = Config::LOG_SUFFIX.c_str(), 
			  int maxQueueCapacity = Config::LOG_MAXCAPACITY);
	static Log* Instance();
	static void FlushLogThread();

	void Write(int level, const char *format, ...);
	void Flush();

	int Level();
	void SetLevel(int level);
	bool IsOpen() { return isOpen_; }
};

// level = 1
// LOG_DEBUG  0 <= 1  y
// LOG_INFO   1 <= 1  y
// LOG_WARN   2 <= 1  n
// LOG_ERROR  3 <= 1  n
#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->Level() <= level) {\
            log->Write(level, format, ##__VA_ARGS__); \
            log->Flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...)  do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...)  do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H

