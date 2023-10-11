#include "httpconn.h"
using namespace std;

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() {
	fd_ = -1;
	addr_ = {0};
	isClose_ = true;
}

HttpConn::~HttpConn() {
	Close();
}

void HttpConn::Init(int fd, const sockaddr_in &addr) {
	assert(fd > 0);
	userCount++;
	addr_ = addr;
	fd_ = fd;
	writeBuff_.ReadAll();
	readBuff_.ReadAll();
	isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close() {
	response_.UnmapFile();
	if (isClose_ == false) {
		isClose_ = true;
		userCount--;
		close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
	}
}

ssize_t HttpConn::Read(int *saveErrno) {
	ssize_t len = -1;
	// 如果还是 ET 触发状态，那就一直读，直到没有可读内容，break
	// Level 为只要处于水平，那么就一直触发，而 Edge 则为上升沿和下降沿的时候触发
	do {
		len = readBuff_.ReadFd(fd_, saveErrno);
		if (len <= 0) break;
	} while (isET);
	return len;
}

ssize_t HttpConn::Write(int *saveErrno) {
	ssize_t len = -1;
	do {
		// 分散写数据
		len = writev(fd_, iov_, iovCnt_);
		if (len <= 0) {
			*saveErrno = errno;
			break;
		}
		// 这种情况下所有数据都传输结束
		if (iov_[0].iov_len + iov_[1].iov_len == 0) { break; }
		// 用到了第二块内存
		else if (static_cast<size_t>(len) > iov_[0].iov_len) {
			iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
			iov_[1].iov_len -= (len - iov_[0].iov_len);
			if (iov_[0].iov_len) {
				writeBuff_.ReadAll();
				iov_[0].iov_len = 0;
			}
		} 
		// 还没用到第二块内存
		else {
			iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
			iov_[0].iov_len -= len;
			writeBuff_.HasRead(len);
		}
	// > 10240 说明是大文件，需要多次调用 wtritev
	} while (isET || ToWriteBytes() > 10240);
	return len;
}
// 已经读完，重置 request 后，对内容进行解析，生成结果 response
bool HttpConn::Process() {
	request_.Init();
	if (readBuff_.ReadableBytes() <= 0) {
		return false;	
	}	
	else if (request_.Parse(readBuff_)) {
		LOG_DEBUG("%s", request_.Path().c_str());
		response_.Init(srcDir, request_.Path(), request_.IsKeepAlive(), 200);
	} else {
		response_.Init(srcDir, request_.Path(), false, 400);
	}
	// 状态行和响应头
	response_.MakeResponse(writeBuff_);
	iov_[0].iov_base = const_cast<char*>(writeBuff_.ReadPtr());
	iov_[0].iov_len = writeBuff_.ReadableBytes();
	iovCnt_ = 1;
	// 响应正文, 在本项目中是 .html 文件
	if (response_.FileLen() > 0 && response_.File()) {
		iov_[1].iov_base = response_.File();
		iov_[1].iov_len = response_.FileLen();
		iovCnt_ = 2;
	}
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;	
}
