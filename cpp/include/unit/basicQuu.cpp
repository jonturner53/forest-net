#include <iostream>					 
#include <thread>					
#include <mutex>				
#include <chrono>				
#include "Quu.h"

using std::thread;
using std::mutex;
using std::unique_lock;
using namespace std::chrono;
using std::cout;
using std::endl;
using namespace forest;

Quu<pair<int,int>> myq(8);

void prod(int i) { 
	pair<int,int> p; p.first = i;
	for (int j = 0; j < 10; j++) {
		p.second = j; myq.enq(p);
	}
}


void cons() {
	for (int i = 0; i < 100; i++) {
		pair<int,int> p = myq.deq();
		cout << p.first << " " << p.second << endl;
	}
}

int main () {
	thread consumer(cons);
	thread producer[10];

	for (int i = 0; i < 10; i++)
		producer[i] = thread(prod,i);

	for (int i = 0; i < 10; i++)
		producer[i].join();
	consumer.join();

	return 0;
}
