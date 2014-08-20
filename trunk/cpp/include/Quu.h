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

	void	enq(T);
	T	deq();	
private:
	int	qMax;			///< max number of items in queue

	int	count;			///< number of items in queue
	int	head;			///< index of first item in buf
	int 	tail;			///< index of first empty space in buf

	T	*buf;			///< where values are stored

	mutex mtx;			///< used to ensure mutual exclusion
	condition_variable emptyQ;	///< condition variable for empty queue
	condition_variable fullQ;	///< condition variable for full queue
};

/** Constructor for Quu objects.
 *  @param qMax1 is the maximum number of elements that can be queued.
 */
template<class T>
Quu<T>::Quu(int qMax1) {
	qMax = qMax1;
	count = head = tail = 0;
	buf = new T[qMax];
}

/** Destructor for Quu objects. */
template<class T>
Quu<T>::~Quu() { delete [] buf; }

/** Reset the queue, discarding any contents.
 *  This should only be used in contexts where there is a single writer,
 *  and only the writing thread should do it.
 */
template<class T>
inline void Quu<T>::reset() {
	unique_lock<mutex> lck(mtx);
	count = tail = head = 0;
}

/** Resize the queue, discarding any contents.
 *  This should only be used when the queue is empty and preferably
 *  before any threads are using the Quu.
 */
template<class T>
inline void Quu<T>::resize(int nuSiz) {
	unique_lock<mutex> lck(mtx);
	qMax = nuSiz; delete [] buf; buf = new T[qMax+1];
}

/** Determine if queue is empty.
 *  @return true if the queue is empty, else false
 */
template<class T>
inline bool Quu<T>::empty() const { return count == 0; }

/** Add value to the end of the queue.
 *  The calling thread is blocked if the queue is full.
 *  @param i is the value to be added.
 */
template<class T>
void Quu<T>::enq(T x) {
	unique_lock<mutex> lck(mtx);
	while (count == qMax) { fullQ.wait(lck); }

	buf[tail] = x;
	count++;
	tail = (tail + 1) % qMax;

	lck.unlock();
	emptyQ.notify_one();
}

/** Remove and return first item in the queue.
 *  The calling thread is blocked if the queue is empty.
 *  @return the next item in the queue
 */
template<class T> T Quu<T>::deq() {
	unique_lock<mutex> lck(mtx);
	while (count == 0) { emptyQ.wait(lck); }

	T x = buf[head];
	count--;
	head = (head + 1) % qMax;

	lck.unlock();
	fullQ.notify_one();
	return x;
}

} // ends namespace

#endif
