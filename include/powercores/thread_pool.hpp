/**This file is part of powercores, released under the terms of the Unlicense.
See LICENSE in the root of the powercores repository for details.*/
#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <atomic>
#include <future>
#include <type_traits>
#include "exceptions.hpp"
#include "threadsafe_queue.hpp"

namespace powercores {

/**A pool of threads.  Accepts tasks in a fairly obvious manner.*/

class ThreadPool {
	public:
	
	ThreadPool(int threadCount): thread_count(threadCount) {}
	
	~ThreadPool() {
		if(running.load()) stop();
	}
	
	void start() {
		auto worker = [&](){workerThreadFunction();};
		running.store(1);
		for(int i = 0; i < thread_count; i++) threads.emplace_back(worker);
	}
	
	void stop() {
		running.store(0);
		for(auto &i: threads) i.join();
		threads.clear();
	}
	
	/**Submit a job, which will be called in the future.*/
	void submitJob(std::function<void(void)> job) {
		job_queue.enqueue(job);
	}
	
	/**Submit a job represented by a function with arguments and a return value, obtaining a future which will later contain the result of the job.*/
	template<class FuncT, class... ArgsT>
	std::future<typename std::result_of<FuncT(ArgsT...)>::type> submitJobWithResult(FuncT callable, ArgsT... args) {
		//The task is not copyable, so we keep a pointer and delete it after we execute it.
		auto task = new std::packaged_task<typename std::result_of<FuncT(ArgsT...)>::type(ArgsT...)>(callable);
		auto job = [task, args...] () {
			(*task)(args...);
			delete task;
		};
		auto retval = task->get_future();
		submitJob(job);
		return retval;
	}
	
	private:
	
	void workerThreadFunction() {
		std::function<void(void)> job;
		while(running	.load()) {
			try {
				job = job_queue.dequeueWithTimeout(10);
			}
			catch(TimeoutException e) {
				//Nothing. We allow timeouts so that we keep evaluating the loop condition.
				continue;
			}
			job();
		}
	}
	
	int thread_count = 0;
	std::vector<std::thread> threads;
	ThreadsafeQueue<std::function<void(void)>> job_queue;
	std::atomic<int> running;
};

}