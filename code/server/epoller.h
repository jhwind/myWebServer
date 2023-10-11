#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller {
private:
	int epollFd_;	// epoller 对象的文件描述符，不是 socket
	std::vector<struct epoll_event> events_;
/*
typedef union epoll_data {
    void		*ptr;
    int         fd;
    uint32_t    u32;
    uint64_t    u64;
} epoll_data_t;

struct epoll_event {
	// events 这个参数是一个字节的掩码构成的
	// 例如 EPOLLIN | EPOLLHUP 
    uint32_t    	events;	// Epoll events 
    epoll_data_t 	data;   // User data variable
};
*/

public:
	explicit Epoller(int maxEvent = 512);
	~Epoller();

	bool AddFd(int fd, uint32_t events);
	bool ModFd(int fd, uint32_t events);
	bool DelFd(int fd);
	int Wait(int timeoutMS = -1);
	int GetEventFd(size_t i) const;
	uint32_t GetEvents(size_t i) const;
};

#endif //EPOLLER_H
