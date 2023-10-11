#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H
#include "sqlconnpool.h"

// RAII 资源获取即初始化 C++ 语言局部对象自动销毁的特性来控制资源的生命周期
class SqlConnRAII {
private:
	MYSQL *sql_;
	SqlConnPool* connpool_;

public:
	SqlConnRAII(MYSQL **sql, SqlConnPool *connpool) {
		assert(connpool);
		*sql = connpool->GetConn();
		sql_ = *sql;
		connpool_ = connpool;
	}

	~SqlConnRAII() {
		if (sql_) { connpool_->FreeConn(sql_); }
	}
};

#endif //SQLCONNRAII_H