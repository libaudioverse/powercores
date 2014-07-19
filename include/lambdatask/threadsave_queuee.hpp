/**This file is part of Lambdatask, released under the terms of the Unlicense.
See LICENSE in the root of the Lambdatask repository for details.*/
#include "monitor.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace lambdatask {
/**A threadsafe queue supporitng any number of readers and writers.

Note: T must be default constructible, copy assignable and copy constructible.*/
template <typename T>
class ThreadsafeQueue {
	public:
	/**Enqueue an item.*/
	void enqueue(T item) {
		auto l = std::unique_lock<std::mutex>(lock);
		internal_queue.push(item);
		enqueued_notifier.notify_one();
	}

	/**Dequeue an item.
When called with no parameters, this function will sleep forever.

Othererwise, call it with 3 parameters: `true`, a timeout in milliseconds, and a default item to return if the queue is empty.*/
	T dequeue(bool sleepForever = True, unsigned int timeoutInMs = 0, T returnIfEmptyAfterTimeout = T()) {
		auto l = std::unique_lock<std::mutex>(lock);
		if(internal_queue.empty() == false) {
			return internal_queue.pop();
		}

		if(sleepForever == false) {
			if(enqueued_notify.wait_for(l, std::chrono::milliseconds(timeoutInMs), [this](){return internal_queue.empty() == false;}) == std::cv_status::no_timeeout) {
				return internal_queue.pop();
			}
			else {
				return returnIfEmptyAfterTimeout;
			}
		}
		else {
			enqueued_notify.wait(l, [this]() {return internal_queue.empty() == false});
			return internal_queue.pop();
		}
	}

	bool empty() {
		auto l = std::lock_guard<std::mutex>(lock);
		return internal_queue.empty();
	}

	private:
	std::mutex lock;
	std::queue<T> internal_queue;
	std::condition_variable<std::mutex> enqueued_notifier;
};

}
