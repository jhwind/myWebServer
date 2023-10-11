#include "webserver.h"

using namespace std;

/**
 * @brief 初始化 sql连接池 httpconn 线程池 日志 定时器 epoller
 */
WebServer::	WebServer(
	int port, int trigMode, int timeoutMS, bool OPTLinger, 
	int sqlPort, const char *sqlUser, const char *sqlPwd, const char *dbName, int connPoolNum, int threadNum,
	bool openLog, int logLevel, int logQueSize) :
	port_(port), openLinger_(OPTLinger), timeoutMS_(timeoutMS), isClose_(false),
	timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
{
	// httpconn 配置
	srcDir_ = getcwd(nullptr, 256);	// 动态给 srcDir_ 分配内存	
	assert(srcDir_);
	strncat(srcDir_, "/resources/", 16);

	HttpConn::userCount = 0;
	HttpConn::srcDir = srcDir_;

	// sql 初始化
	SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

	InitEventMode_(trigMode);
	// socket 初始化
	if (!InitSocket_()) { isClose_ = true; }

	// log 初始化
	if (openLog) {
		Log::Instance()->Init(logLevel, "./log", ".log", logQueSize);
        if(isClose_) { LOG_ERROR("========== Server init error!=========="); }
        else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, openLinger_? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent_ & EPOLLET ? "ET": "LT"),
                            (connEvent_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
	}
}
/**
 * @brief 1. 关闭监听事件文件描述符
 *        2. 关闭 sql 连接池
 */
WebServer::~WebServer() {
	close(listenFd_);
	isClose_ = true;
	free(srcDir_);
	SqlConnPool::Instance()->ClosePool();
}

/**
 * @brief 初始化事件模式
 */
void WebServer::InitEventMode_(int trigMode) {
	listenEvent_ = EPOLLRDHUP;	// EPOLLRDHUP 判断对端关闭
	// EPOLLONESHOT 只触发一次，再次触发需要 socket  重新加入队列 Epoller
	connEvent_ = EPOLLONESHOT | EPOLLRDHUP;	
	switch (trigMode)
	{
	case 0:
		break;
	case 1:
		connEvent_ |= EPOLLET;
		break;
	case 2:
		listenEvent_ |= EPOLLET;
		break;
	case 3:
		listenEvent_ |= EPOLLET;
		connEvent_ |= EPOLLET;
		break;
	default:
		listenEvent_ |= EPOLLET;
		connEvent_ |= EPOLLET;
		break;
	}
	HttpConn::isET = (connEvent_ & EPOLLET);
}

/**
 * @brief 循环监听，处理事件
 */
void WebServer::Start() {
	int timeMS = -1;
    if(!isClose_) { LOG_INFO("========== Server start =========="); }
    while (!isClose_) {
    	// 第二次进入循环了
    	if (timeoutMS_ > 0) {
    		timeMS = timer_->GetNextTick();
    	}
    	int eventCnt = epoller_->Wait(timeMS);
    	for (int i = 0; i < eventCnt; ++i) {
    		int fd = epoller_->GetEventFd(i);
 			uint32_t events = epoller_->GetEvents(i);
 			if (fd == listenFd_) {
 				DealListen_();
 			}
 			// 对端关闭 | Fd 挂断 | Fd 发生错误
 			else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
 				assert(users_.count(fd) > 0);
 				CloseConn_(&users_[fd]);
 			}
 			// Fd 可写
 			else if (events & EPOLLIN) {
 				assert(users_.count(fd) > 0);
 				DealRead_(&users_[fd]);
 			} else if (events & EPOLLOUT) {
 				assert(users_.count(fd) > 0);
 				DealWrite_(&users_[fd]);
 			} else {
 				LOG_ERROR("Unexpected event!");
 			}
    	}
    }
}

bool WebServer::InitSocket_() {
	int ret;
	struct sockaddr_in addr;	// 结构体处理网络通信地址 存放 ip 和 port

	if (port_ > 65535 || port_ < 1024) {
		LOG_ERROR("Port:%d error!", port_);
		return false;
	}
	addr.sin_family = AF_INET;
	// INADDR_ANY 转换过来就是0.0.0.0，泛指本机的意思，也就是表示本机的所有IP
	// 因为有些机子不止一块网卡，多网卡的情况下，这个就表示所有网卡ip地址的意思
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port_);

	// 设置优雅关闭
	struct linger OPTLinger = {0};
	if (openLinger_) {
		OPTLinger.l_onoff = 1;
		OPTLinger.l_linger = 1;
	}
	// 流 socket
	listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd_ < 0) {	
        LOG_ERROR("Create socket error!", port_);
        return false;
    }
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &OPTLinger, sizeof(OPTLinger));
    if (ret < 0) {
        LOG_ERROR("Init linger error!", port_);
        close(listenFd_);
        return false;
    }

    int optval = 1;
    // 端口复用 只有最后一个套接字会正常接收数据。
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if (ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }
    // 绑定端口
	ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr)); 
	if (ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }
    // 将套接字设置为监听模步等待连接请求
    ret = listen(listenFd_, 6);
	if (ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }
    // 将监听 socket 加入 Epoller 中
	ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }
    // 设置非阻塞监听
    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true; 
}

/**
 * @brief 向 fd 发送错误信息
 */
void WebServer::SendError_(int fd, const char *info) {
	assert(fd > 0);
	int ret = send(fd, info, strlen(info), 0);
	if (ret < 0) {
		LOG_WARN("send error to client[%d] error!", fd);
	}
	close(fd);
}

/**
 * @brief 关闭 http | 连接 socket
 */
void WebServer::CloseConn_(HttpConn *client) {
	assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
	epoller_->DelFd(client->GetFd());
	client->Close();	
}

/**
 * @brief  添加新的 http | 连接 socket
 */
void WebServer::AddClient_(int fd, sockaddr_in addr) {
	assert(fd > 0);
	users_[fd].Init(fd, addr);
	if (timeoutMS_ > 0) {
		timer_->Add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
	}
	epoller_->AddFd(fd, EPOLLIN | connEvent_);
	SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

/**
 * @brief 处理监听事件
 */
void WebServer::DealListen_() {
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	do {
		// 返回新的 连接 socket，从连接请求队列中，一直取，直到没有
		int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
		if (fd <= 0) { return; }
		else if (HttpConn::userCount >= MAX_FD) {
			SendError_(fd, "Server busy!");
			LOG_WARN("Clients is full!");
			return;
		}
		AddClient_(fd, addr);
	} while (listenEvent_ & EPOLLET);	// 循环一直进行知道 accept 队列没有数据
}

/**
 * @brief 处理读事件，调用线程池
 */
void WebServer::DealRead_(HttpConn *client) {
	assert(client);
	ExtentTime_(client);
	threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}
/**
 * @brief 处理写事件，调用线程池
 */
void WebServer::DealWrite_(HttpConn *client) {
	assert(client);
	ExtentTime_(client);
	threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}
/**
 * @brief 更新时间
 */
void WebServer::ExtentTime_(HttpConn *client) {
	assert(client);
	if (timeoutMS_ > 0) {
		timer_->Adjust(client->GetFd(), timeoutMS_);
	}
}

/**
 * @brief 1. 从 fd 中读数据到 buffer
 *        2. 调用处理请求数据
 */
void WebServer::OnRead_(HttpConn *client) {
	assert(client);
	int ret = -1;
	int readErrno = 0;
	ret = client->Read(&readErrno);
	if (ret <= 0 && readErrno != EAGAIN) {
		CloseConn_(client);
		return;
	}
	OnProcess(client);
}

/**
 * @param  
 * @brief 处理请求数据
 */
void WebServer::OnProcess(HttpConn *client) {
	if (client->Process()) {
		// 处理成功，注册可写
		epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
	} else {
		// OnWrite_ 调用，没有可写的，注册可读
		epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
	}
}

/**
 * @param  
 * @brief 处理写事件
 */
void WebServer::OnWrite_(HttpConn *client) {
	assert(client);
	int ret = -1;
	int writeErrno = 0;
	ret = client->Write(&writeErrno);
	if (client->ToWriteBytes() == 0) {
		// 长连接重置 http 类实例，注册读事件，不关闭连接
		if (client->IsKeepAlive()) {
			OnProcess(client);
			return;
		}
	} 
	// 单次发送不成功
	else if (ret < 0) {
		// EAGAIN 一般用于非阻塞的系统调用 
		// 这里表示缓冲区满了再次尝试
		// 更新 iovec 结构体的指针和长度，并注册写事件，等待下一次写事件触发
		if (writeErrno == EAGAIN) {
			// 特殊情况 注册可写
			epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
			return;
		}
	}
	// 不是因为缓冲区满了而失败，取消 mmap 映射，关闭连接
	CloseConn_(client);
}

/**
 * @param   
 * @return
 * @brief 设置非阻塞
 */
int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    // fcntl 系统调用对已打开的文件描述符设置读写操作为 O_NONBLOCK 非堵塞
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}