// usage:
//	testLfq
//
// Simple test of Lfq data structure

#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include "Lfq.h"

using std::thread;
using std::mutex;
using std::atomic;
using std::unique_lock;

using namespace forest;

Lfq<int> q(4);

void f() {
	for (int i = 0; i < 3333333; i++) {
		int x;
		do {
			x = q.deq();
			if (x == 0) this_thread::yield();
		} while(x == 0);
		while (!q.enq(x)) { this_thread::yield(); }
	}
}

int main() {
	for (int i = 1; i <= 10; i++) q.enq(i);

	thread t[10];
	for (int i = 0; i < 3; i++) t[i] = thread(f);
	for (int i = 0; i < 3; i++) t[i].join();
}
