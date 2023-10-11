#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <vector>
#include <unistd.h>		// 符号常量
#include <sys/uio.h>	// 矢量 I/O 操作
#include <atomic>
#include <assert.h>
#include <cstring>

class Buffer {
private:
	std::vector<char> buffer_;
	// 声明为 atomic 类型，变相的线程安全
	std::atomic<std::size_t> readPos_;
	std::atomic<std::size_t> writePos_;

	// 返回初始位o置
	char* BeginPtr_();
	const char* BeginPtr_() const;
	void MakeSpace_(size_t len);

public:
	Buffer(int initBuffSize = 10240);
	~Buffer() = default;

	size_t ReadableBytes() const;
	size_t WritableBytes() const;
	size_t CoverableBytes() const;

	void EnsureWriteable(size_t len);

	void HasRead(size_t len);
	void HasWrite(size_t len);

	void ReadUntil(const char* end);
	void ReadAll();
	std::string ReadAllToStr();

	// return read ptr
	char* ReadPtr();
	const char* ReadPtrConst() const;
	char* WritePtr();
	const char* WritePtrConst() const;

	void Append(const char *str, size_t len);
	void Append(const std::string &str);
	void Append(const void *data, size_t len);
	void Append(const Buffer &buff);

	ssize_t ReadFd(int fd, int *Errno);
	ssize_t WriteFd(int fd, int *Errno);
};

#endif //BUFFER_H
