// usage:
//	testLfq
//
// Simple test of Lfq data structure

#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include "Quu.h"

using std::thread;
using std::mutex;
using std::atomic;
using std::unique_lock;

using namespace forest;

Quu<int> q(20);

void f() {
	for (int i = 0; i < 333333; i++) {
		int x;
		x = q.deq();
		q.enq(x);
	}
}

int main() {
	for (int i = 1; i <= 10; i++) q.enq(i);

	thread t[10];
	for (int i = 0; i < 3; i++) t[i] = thread(f);
	for (int i = 0; i < 3; i++) t[i].join();
}
