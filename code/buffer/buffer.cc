#include "buffer.h"

char* Buffer::BeginPtr_() {
	return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
	return &*buffer_.begin();
}

/**
 * @param
 * @return
 * @brief
 */
void Buffer::MakeSpace_(size_t len) {
	if (WritableBytes() + CoverableBytes() < len) {
		buffer_.resize(writePos_ + len + 1);
	} else {
		size_t readableBytes = ReadableBytes();
		std::copy(ReadPtr(), WritePtr(), BeginPtr_());
		readPos_ = 0;
		writePos_ = readableBytes;
		assert(readableBytes == ReadableBytes());
	}
}

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

size_t Buffer::ReadableBytes() const {
	return writePos_ - readPos_;
}

size_t Buffer::WritableBytes() const {
	return buffer_.size() - writePos_;
}

size_t Buffer::CoverableBytes() const {
	return ReadPtrConst() - BeginPtr_();
}

void Buffer::EnsureWriteable(size_t len) {
	if (WritableBytes() < len) {
		MakeSpace_(len);
	}
	assert(WritableBytes() >= len);
}

void Buffer::HasRead(size_t len) {
	assert(len <= ReadableBytes());
	readPos_ += len;
}
void Buffer::HasWrite(size_t len) {
	// assert(len <= WritableBytes());
	writePos_ += len;
}

void Buffer::ReadUntil(const char *end) {
	assert(ReadPtr() <= end);
	HasRead(end - ReadPtr());
}

void Buffer::ReadAll() {
	bzero(&buffer_[0], buffer_.size());
	readPos_ = 0;
	writePos_ = 0;
}

std::string Buffer::ReadAllToStr() {
	std::string str(ReadPtr(), ReadableBytes());
	// ReadAll();
	return str;
}

char* Buffer::ReadPtr() {
	return BeginPtr_() + readPos_;
}

const char* Buffer::ReadPtrConst() const {
	return BeginPtr_() + readPos_;
}

char* Buffer::WritePtr() {
	return BeginPtr_() + writePos_;
}

const char* Buffer::WritePtrConst() const {
	return BeginPtr_() + writePos_;
}

void Buffer::Append(const char *str, size_t len) {
	assert(str);
	EnsureWriteable(len);
	std::copy(str, str + len, WritePtr());
	HasWrite(len);
}

void Buffer::Append(const std::string &str) {
	Append(str.data(), str.length());
}

void Buffer::Append(const void *data, size_t len) {
	assert(data);
	Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const Buffer &buffer) {
	Append(buffer.ReadPtrConst(), buffer.ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int *saveErrno) {
	char buffer[65535];
	struct iovec iov[2];
	const size_t writableBytes = WritableBytes();

	iov[0].iov_base = BeginPtr_() + writePos_;
	iov[0].iov_len = writableBytes;
	iov[1].iov_base = buffer;
	iov[1].iov_len = sizeof(buffer);

	const ssize_t len = readv(fd, iov, 2);
	if (len < 0) {
		*saveErrno = errno;
	}
	else if (static_cast<size_t>(len) <= writableBytes) {
		HasWrite(len);
	} else {
		HasWrite(writableBytes);
		Append(buffer, len - writableBytes);
	}
	return len;
}

ssize_t Buffer::WriteFd(int fd, int *saveErrno) {
	ssize_t readableBytes = ReadableBytes();
	ssize_t len = write(fd, ReadPtr(), readableBytes);
	if (len < 0) {
		*saveErrno = errno;
		return len;
	}
	HasRead(len);
	return len;
}




