/**This file is part of Lambdatask, released under the terms of the Unlicense.
See LICENSE in the root of the Lambdatask repository for details.*/
#include <lambdatask/monitor.hpp>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <stdio.h>

//test configuration values.
const unsigned int times = 10000;
const unsigned int threads = 100;
//time to sleep for the slow test.
const unsigned int sleep_time = 5;
class Counter {
	public:
	void count(int t) {
		std::this_thread::sleep_for(std::chrono::milliseconds(t));
		val ++;
	}
	void reset() {
		val = 0;
	}
	unsigned int val = 0;
};

lambdatask::Monitor<Counter> mon;
std::atomic<unsigned int> atom;

void test_fast_thread() {
	for(unsigned int i = 0; i < times; i++) {
		mon->count(0);
		atom.fetch_add(1);
	}
}

void test_slow_thread() {
	for(unsigned int i = 0; i < times; i++) {
		mon->count(sleep_time);
		atom.fetch_add(1);
	}
}

bool verify() {
	unsigned int atomic_value = atom.load();
	unsigned int monitor_value = mon->val;
	return (atomic_value == monitor_value) && atomic_value == (times*threads);
}

void reset() {
	atom.store(0);
	mon->reset();
}

bool do_test_fast() {
	reset();
	std::vector<std::thread> ts;
	for(unsigned int i = 0; i < threads; i++) {
		ts.emplace_back(test_fast_thread);
	}
	for(unsigned int i = 0; i < ts.size(); i++) {
		ts[i].join();
	}
	return verify();
}

bool do_test_slow() {
	reset();
	std::vector<std::thread> ts;
	for(unsigned int i = 0; i < threads; i++) {
		ts.emplace_back(test_slow_thread);
	}
	for(unsigned int i = 0; i < ts.size(); i++) {
		ts[i].join();
	}
	return verify();
}

void main() {
	printf("Running fast test...\n");
	if(do_test_fast() == false) {
		printf("failed\n");
	return;
	}
	printf("Fast test passed.  Doing slow test...\n");
	if(do_test_slow() == false) {
	printf("Slow test failed.\n");
	return;
		printf("Slow test passed.\n");
}
}