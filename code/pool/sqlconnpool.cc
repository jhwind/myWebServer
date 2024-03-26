#include "sqlconnpool.h"
using namespace std;

SqlConnPool::SqlConnPool() {
	useCount_ = 0;
	freeCount_ = 0;
}

SqlConnPool::~SqlConnPool() {
	ClosePool();
}

SqlConnPool* SqlConnPool::Instance() {
	static SqlConnPool instance;
	return &instance;
}

void SqlConnPool::Init(const char *host, int port,
					   const char *user, const char *pwd,
					   const char *dbName, int connSize) {
	assert(connSize > 0);
	for (int i = 0; i < connSize; ++i) {
		MYSQL *sql = nullptr;
		sql = mysql_init(sql);
		if (!sql) {
			LOG_ERROR("MySql init error!");
			return;
		}
		sql = mysql_real_connect(sql, host, user, pwd, dbName, port,
								 nullptr, 0);
		if (!sql) {
			LOG_ERROR("MySql connect error!");
			return;
		}

		connQue_.push(sql);
	}
	MAX_CONN_ = connSize;
	sem_init(&poolSem_, 0, MAX_CONN_);
}

MYSQL* SqlConnPool::GetConn() {
	MYSQL *sql = nullptr;
	if (connQue_.empty()) {
		LOG_WARN("SqlConnPool busy!");
		return nullptr;
	}
	sem_wait(&poolSem_);
	{
		lock_guard<mutex> locker(mtx_);
		sql = connQue_.front();
		connQue_.pop();
	}
	return sql;
}

void SqlConnPool::FreeConn(MYSQL *sql) {
	assert(sql);
	lock_guard<mutex> locker(mtx_);
	connQue_.push(sql);
	sem_post(&poolSem_);
}

void SqlConnPool::ClosePool() {
	lock_guard<mutex> locker(mtx_);
	while (!connQue_.empty()) {
		auto item = connQue_.front();
		connQue_.pop();
		mysql_close(item);
	}
	// 终止使用 MySQL 库，释放内存
	mysql_library_end();
}

int SqlConnPool::FreeConnCount() {
	lock_guard<mutex> locker(mtx_);
	return connQue_.size();
}