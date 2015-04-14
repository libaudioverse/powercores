/**This file is part of Lambdatask, released under the terms of the Unlicense.
See LICENSE in the root of the Lambdatask repository for details.*/
#pragma once
#include "monitor.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <atomic>

namespace powercores {
/**A threadsafe queue supporitng any number of readers and writers.

Note: T must be default constructible, copy assignable and copy constructible.*/
template <typename T>
class ThreadsafeQueue {
	public:
	/**Enqueue an item.*/
	void enqueue(T item) {
		auto l = std::unique_lock<std::mutex>(lock);
		internal_queue.push_front(item);
		_contains++;
		l.unlock();
		enqueued_notify.notify_one();
	}

	/**Dequeue an item.
When called with no parameters, this function will sleep forever.

Othererwise, call it with 3 parameters: `true`, a timeout in milliseconds, and a default item to return if the queue is empty.*/
	T dequeue(bool sleepForever = true, unsigned int timeoutInMs = 0, T returnIfEmptyAfterTimeout = T()) {
		auto l = std::unique_lock<std::mutex>(lock);
		if(internal_queue.empty() == false) {
			return actualDequeue();
		}

		if(sleepForever == false) {
			if(enqueued_notify.wait_for(l, std::chrono::milliseconds(timeoutInMs), [this]()->bool {return internal_queue.empty() == false;}) == true) {
				return actualDequeue();
			}
			else {
				return returnIfEmptyAfterTimeout;
			}
		}
		else {
			enqueued_notify.wait(l, [this]() {return internal_queue.empty() == false;});
			return actualDequeue();
		}
	}

	bool empty() {
		auto l = std::lock_guard<std::mutex>(lock);
		return internal_queue.empty();
	}

/**Get the current number of items in the queue.*/
	unsigned int contains() {
		auto l = std::lock_guard<std::mutex>(lock);
		return _contains;
	}
	private:
	T actualDequeue() {
		auto res = internal_queue.back();
		internal_queue.pop_back();
		_contains--;
		return res;
	}
	std::mutex lock;
	std::deque<T> internal_queue;
	std::condition_variable enqueued_notify;
	unsigned int _contains;
};

}
