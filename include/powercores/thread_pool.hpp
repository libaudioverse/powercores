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

/**Throwing this on a ThreadPool thread causes the thread to die.*/
class ThreadPoolPoisonException {
};

/**A pool of threads.  Accepts tasks in a fairly obvious manner.*/
class ThreadPool {
	public:
	
	ThreadPool(int threadCount): thread_count(threadCount) {
		running.store(0);
	}
	
	~ThreadPool() {
		if(running.load()) stop();
	}
	
	void start() {
		running.store(1);
		job_queues.resize(thread_count);
		for(auto &i: job_queues) i = new ThreadsafeQueue<std::function<void(void)>>();
		for(int i = 0; i < thread_count; i++) threads.emplace_back([&, i] () {workerThreadFunction(i);});
	}
	
	void stop() {
		for(auto &i: job_queues) i->enqueue([] () {throw ThreadPoolPoisonException();});
		running.store(0);
		for(int i = 0; i < threads.size(); i++) {
			threads[i].join();
		}
		threads.clear();
		for(auto &i: job_queues) delete i;
		job_queues.clear();
		job_queue_pointer = 0;
	}

	void setThreadCount(int n) {
		bool wasRunning = running.load() == 1;
		if(wasRunning)  stop();
		thread_count = n;
		if(wasRunning) start();
	}
	
	/**Submit a job, which will be called in the future.*/
	void submitJob(const std::function<void(void)> &job) {
		auto &job_queue = job_queues[job_queue_pointer];
		job_queue->enqueue(job);
		job_queue_pointer = (job_queue_pointer+1)%thread_count;
	}

	/**Submit a job represented by a function with arguments and a return value, obtaining a future which will later contain the result of the job.*/
	template<class FuncT, class... ArgsT>
	std::future<typename std::result_of<FuncT(ArgsT...)>::type> submitJobWithResult(const FuncT &callable, ArgsT... args) {
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

	/**Submit a barrier.	
	A barrier ensures that all jobs enqueued before the barrier will finish execution before any job after the barrier begins execution.*/
	void submitBarrier() {
		//Promises are not copyable, so we save a pointer and delete it later, after the barrier.
		auto promise = new std::promise<void>();
		std::shared_future<void> future(promise->get_future());
		auto counter = new std::atomic<int>();
		counter->store(0);
		int goal = thread_count;
		auto barrierJob = [counter, promise, future, goal] () {
			int currentCounter = counter->fetch_add(1); //increment by one and get the current counter.
			if(currentCounter == goal-1) { //we're finished.
				promise->set_value();
				delete promise; //we're done, this isn't needed anymore.
				//We got here, so we're the last one to manipulate the counter.
				delete counter;
			}
			else {
				//Otherwise, we wait for all the other barriers.
				future.wait();
			}
		};
		for(int i = 0; i < thread_count; i++) submitJob(barrierJob);
	}
	
	private:
	
	void workerThreadFunction(int id) {
		ThreadsafeQueue<std::function<void(void)>> &job_queue = *job_queues[id];
		int jobsSize = 5;
		std::function<void(void)> jobs[5];
		try {
			while(true) {
				int got = job_queue.dequeueRange(jobsSize, jobs);
				for(int i = 0; i < got; i++) jobs[i]();
			}
		}
		catch(ThreadPoolPoisonException) {
			//Nothing, just a way to break out.
		}
	}
	
	//job_queue_pointer is the queue we're writing into.
	int thread_count = 0, job_queue_pointer = 0;
	std::vector<std::thread> threads;
	std::vector<ThreadsafeQueue<std::function<void(void)>>*> job_queues;
	std::atomic<int> running;
};

}