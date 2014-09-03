// usage:
//	testLfq11
//
// Simple test of Lfq11 data structure

#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include "Lfq11.h"

using std::thread;
using std::mutex;
using std::atomic;
using std::unique_lock;

using namespace forest;

Lfq11<int> q0(4);
Lfq11<int> q1(4);

void f(int i) {
	int x;
	if (i == 0) {
		for (int j = 0; j < 5000000; j++) {
			do { x = q0.deq(); } while(x == 0);
			while (!q1.enq(x)) ;
		}
	} else {
		for (int j = 0; j < 5000000; j++) {
			do { x = q1.deq(); } while(x == 0);
			while (!q0.enq(x)) ;
		}
	}
}

int main() {
	for (int i = 1; i <= 8; i++) q0.enq(i);
	for (int i = 9; i <= 16; i++) q1.enq(i);

	thread t0 = thread(f,0);
	thread t1 = thread(f,1);
	t0.join(); t1.join();

	cerr << q0.toString() << endl;
	cerr << q1.toString() << endl;
}
