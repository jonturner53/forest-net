/** \file Quu.h
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef QUU_H
#define QUU_H

#include <pthread.h>
#include "stdinc.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Util.h"

using std::mutex;
using std::unique_lock;
using std::condition_variable;

namespace forest {

/** This class implements a simple thread-safe queue for communication
 *  among threads.
 */
template<class T> class Quu {
public:		Quu(int=10);
		~Quu();

	void	reset();
	void	resize(int);
	bool	empty() const;
	bool	full() const;

	void	enq(T);
	T	deq();	
private:
	int	N;			///< max number of items in queue

	atomic<int> count;		///< number of items in queue
	int	rp;			///< index of first item in buf
	int 	wp;			///< index of first empty space in buf

	T	*buf;			///< where values are stored

	mutex emtx, dmtx;		///< used to ensure mutual exclusion
	condition_variable notEmpty;	///< condition variable for empty queue
	condition_variable notFull;	///< condition variable for full queue
};

/** Constructor for Quu objects.
 *  @param x is the log2 of the maximum number of elements that can be queued
 */
template<class T>
Quu<T>::Quu(int x) : N(1 << x) {
	buf = new T[N];
	count.store(0); rp = wp = 0;
}

/** Destructor for Quu objects. */
template<class T>
Quu<T>::~Quu() { delete [] buf; }

/** Resize the queue, discarding any contents.
 *  This should only be used before any threads are using the Quu.
 */
template<class T>
inline void Quu<T>::resize(int x) {
	N = 1 << x; delete [] buf; buf = new T[N];
}

/** Determine if queue is empty.
 *  @return true if the queue is empty, else false
 */
template<class T>
inline bool Quu<T>::empty() const { return count.load() == 0; }

/** Determine if queue is full.
 *  @return true if the queue is full, else false
 */
template<class T>
inline bool Quu<T>::full() const { return count.load() == N; }

/** Add value to the end of the queue.
 *  The calling thread is blocked if the queue is full.
 *  @param i is the value to be added.
 */
template<class T>
void Quu<T>::enq(T x) {
	unique_lock<mutex> elck(emtx);
	notFull.wait(elck,[=]{return !full();});

	buf[wp] = x;
	wp = (wp+1)%N;

	bool wakeup = (count++ == 0 ? true : false);
	elck.unlock();

	if (wakeup) {
		unique_lock<mutex> dlck(dmtx);
		notEmpty.notify_all();
	}
}

/** Remove and return first item in the queue.
 *  The calling thread is blocked if the queue is empty.
 *  @return the next item in the queue
 */
template<class T> T Quu<T>::deq() {
	unique_lock<mutex> dlck(dmtx);
	notEmpty.wait(dlck,[=]{return !empty();});

	T x = buf[rp];
	rp = (rp+1)% N;

	bool wakeup = (count-- == N ? true : false);
	dlck.unlock();

	if (wakeup) {
		unique_lock<mutex> elck(emtx);
		notFull.notify_all();
	}
	return x;
}

} // ends namespace

#endif
