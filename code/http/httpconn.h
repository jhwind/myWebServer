#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
private:
	int fd_; 
	struct sockaddr_in addr_;

	bool isClose_;

	int iovCnt_;
	struct iovec iov_[2];

	Buffer readBuff_;
	Buffer writeBuff_;

	HttpRequest request_;
	HttpResponse response_;

public:
	static bool isET;
	static const char* srcDir;
	static std::atomic<int> userCount;

	HttpConn();
	~HttpConn();

	void Init(int sockFd, const sockaddr_in &addr);
	ssize_t Read(int *saveErrno);
	ssize_t Write(int *saveErrno);

	void Close();
	int GetFd() const { return fd_; }
	int GetPort() const { return addr_.sin_port; }
	const char* GetIP() const { return inet_ntoa(addr_.sin_addr); }
	sockaddr_in GetAddr() const { return addr_; }

	bool Process();
	// 返回要写的数据量
	int ToWriteBytes() { return iov_[0].iov_len + iov_[1].iov_len; }

	bool IsKeepAlive() const { return request_.IsKeepAlive(); }
};

#endif //HTTP_CONN_H