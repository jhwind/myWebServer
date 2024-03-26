#include "epoller.h"

// maxEvent 的值不能大于创建 epoll_create () 时的 size
Epoller::Epoller(int maxEvent) : epollFd_(epoll_create(512)), events_(maxEvent) {
	assert(epollFd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller() {
	close(epollFd_);
}

bool Epoller::AddFd(int fd, uint32_t events) {
	// if (fd < 0) return false;

	epoll_event ev = {0};
	ev.data.fd = fd;
	ev.events = events;

	return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events) {
	// if (fd < 0) return false;
	
	epoll_event ev = {0};
	ev.data.fd = fd;
	ev.events = events;

	return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);	
}

bool Epoller::DelFd(int fd) {
	// if (fd < 0) return false;

	epoll_event ev = {0};
	return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}
// 需要处理的事件数目，如返回 0 表示已超时，-1 将不确定，也有说法说是永久阻塞
int Epoller::Wait(int timeoutMS) {
	return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMS);
}

int Epoller::GetEventFd(size_t i) const {
	assert(i >= 0 && i < events_.size());
	return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const {
	assert(i >= 0 && i < events_.size());
	return events_[i].events;
}