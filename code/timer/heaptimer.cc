#include "heaptimer.h"

void HeapTimer::Siftup_(size_t i) {
	assert(i >= 0 && i < heap_.size());
	if (i == 0) return;
	size_t j = (i-1) / 2;
	while (j >= 0) {
		if (heap_[j] < heap_[i]) { break; }
		SwapNode_(i, j);
		i = j;
		j = (i-1) / 2;
	}
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
	assert(i >= 0 && i < heap_.size());
	assert(j >= 0 && j < heap_.size());
	std::swap(heap_[i], heap_[j]);
	ref_[heap_[i].id] = i;
	ref_[heap_[j].id] = j;
}

bool HeapTimer::Siftdown_(size_t index, size_t n) {
	assert(index >= 0 && index < heap_.size());
	assert(n >= 0 && n <= heap_.size());
	size_t i = index;
	size_t j = i*2 + 1;
	while (j < n) {
		if (j+1 < n && heap_[j+1] < heap_[j]) j++;
		if (heap_[i] < heap_[j]) break;
		SwapNode_(i, j);
		i = j;
		j = i*2 + 1;
	}
	return i > index;
}

void HeapTimer::Add(int id, int timeout, const TimeoutCallBack &cb) {
	assert(id >= 0);
	size_t i;
	if (ref_.count(id) == 0) {
		i = heap_.size();
		ref_[id] = i;
		heap_.push_back({id, Clock::now() + MS(timeout), cb});
		Siftup_(i);
	} else {
		i = ref_[id];
		heap_[i].expires = Clock::now() + MS(timeout);
		heap_[i].cb = cb;
		if (!Siftdown_(i, heap_.size())) {
			Siftup_(i);
		}
	}
}

void HeapTimer::DoWork(int id) {
	if (heap_.empty() || ref_.count(id) == 0) { return; }

	size_t i = ref_[id];
	TimerNode node = heap_[i];
	node.cb();
	Del_(i);
}

void HeapTimer::Del_(size_t index) {
	assert(!heap_.empty() && index >= 0 && index < heap_.size());
	ssize_t i = index;
	// del n = size()-1
	ssize_t n = heap_.size() - 1;

	assert(i <= n);
	if (i < n) {
		SwapNode_(i, n);
		if (!Siftdown_(i, n)) {
			Siftup_(i);
		}
	}
	ref_.erase(heap_.back().id);
	heap_.pop_back();
}

void HeapTimer::Adjust(int id, int timeout) {
	assert(!heap_.empty() && ref_.count(id) > 0);
	heap_[ref_[id]].expires = Clock::now() + MS(timeout);
	Siftdown_(ref_[id], heap_.size());
}

void HeapTimer::Tick_() {
	if (heap_.empty()) { return; }
	int i = 0;
	while (!heap_.empty()) {
		TimerNode node = heap_.front();
		if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
			break;
		}
		LOG_INFO("pop: ", i++);
		node.cb();
		Pop();
	}
}

void HeapTimer::Pop() {
	assert(!heap_.empty());
	Del_(0);
}

void HeapTimer::Clear() {
	ref_.clear();
	heap_.clear();
}

int HeapTimer::GetNextTick() {
	Tick_();
	size_t ret = -1;
	if (!heap_.empty()) {
		ret = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
		ret = ret < 0 ? 0 : ret;
	}
	LOG_INFO("GetNextTick: ", ret);
	return ret;
}

