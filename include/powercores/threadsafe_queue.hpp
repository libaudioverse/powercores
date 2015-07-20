/**This file is part of powercores, released under the terms of the Unlicense.
See LICENSE in the root of the powercores repository for details.*/
#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <atomic>

namespace powercores {
/**A threadsafe queue supporting any number of readers and writers.

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
If there is no item in the queue, this function sleeps forever.*/
	T dequeue() {
		auto l = std::unique_lock<std::mutex>(lock);
		if(internal_queue.empty() == false) {
			return actualDequeue();
		}

		enqueued_notify.wait(l, [this]() {return internal_queue.empty() == false;});
		return actualDequeue();
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
