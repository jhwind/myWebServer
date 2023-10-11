#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
private:
	int MAX_CONN_;
	int useCount_;	// 已使用数量
	int freeCount_; // 空闲数量

	std::queue<MYSQL*> connQue_;
	std::mutex mtx_;
	sem_t poolSem_;

	SqlConnPool();
	~SqlConnPool();

public:
	static SqlConnPool* Instance();

	MYSQL* GetConn();
	void FreeConn(MYSQL *conn);
	int FreeConnCount();
	void Init(const char *host, int port,
			  const char *user, const char* pwd,
			  const char *dbName, int connSize = 10);
	void ClosePool();
};

#endif // SQLCONNPOOL_H
