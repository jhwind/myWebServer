#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;		// 回调函数
typedef std::chrono::high_resolution_clock Clock;	// 高精度时间
typedef std::chrono::milliseconds MS;	// 定义时间单位 毫秒
typedef Clock::time_point TimeStamp;	// 时间点

struct TimerNode {
	int id;
	TimeStamp expires;
	TimeoutCallBack cb;
	bool operator<(const TimerNode &t) {
		return expires < t.expires;
	}
};

class HeapTimer {
private:
	std::vector<TimerNode> heap_;
	std::unordered_map<int, size_t> ref_;

	void Del_(size_t i);
	void Siftup_(size_t i);
	bool Siftdown_(size_t index, size_t n);
	void SwapNode_(size_t i, size_t j);
	// 超时处理
	void Tick_();

public:
	HeapTimer() { heap_.reserve(64); }
	~HeapTimer() { Clear(); }

	void Adjust(int id, int newExpires);
	void Add(int id, int timeout, const TimeoutCallBack &cb);
	void DoWork(int id);
	void Clear();
	void Pop();
	// 超时处理+ 获取等待时间
	int GetNextTick();
};

#endif // HEAP_TIMER_H
