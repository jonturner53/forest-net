/** @file Queue.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Queue.h"

Queue::Queue(int qMax1) {
	qMax = qMax1;
	count = head = tail = 0;
	buf = new int[qMax];
}

Queue::~Queue() {
	delete [] buf;
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&emptyQ);
	pthread_cond_destroy(&fullQ);
}

/** Reset the queue, discarding any contents.
 *  This must only be used by the single writer to the queue;
 */
void Queue::reset() {
	pthread_mutex_lock(&lock);
	count = tail = head = 0;
	pthread_mutex_unlock(&lock);
}

bool Queue::init() {
	int status = pthread_mutex_init(&lock,NULL);
	if (status != 0) return false;
	status = (pthread_cond_init(&emptyQ,NULL) == 0) &&
		 (pthread_cond_init(&fullQ,NULL) == 0);
	return status;
} 

/** Add value to the end of the queue.
 *  The calling thread is blocked if the queue is full.
 *  @param i is the value to be added.
 */
void Queue::enq(int i) {
	pthread_mutex_lock(&lock);
	while (count == qMax) {
		pthread_cond_wait(&fullQ,&lock);
	}

	buf[tail] = i;
	count++;
	tail = (tail + 1) % qMax;

	pthread_mutex_unlock(&lock);
	if (pthread_cond_signal(&emptyQ) != 0) 
		cerr << "pthread_cond_signal on emptyQ failed";
}

/** Remove and return first item in the queue.
 *  The calling thread is blocked if the queue is empty.
 *  @return the next item in the queue
 */
int Queue::deq() {
	pthread_mutex_lock(&lock);
	while (count == 0) {
		pthread_cond_wait(&emptyQ,&lock);
	}

	int value = buf[head];
	count--;
	head = (head + 1) % qMax;

	pthread_mutex_unlock(&lock);
	pthread_cond_signal(&fullQ);
	return value;
}

/** Remove and return first item in the queue, with timeout.
 *  The calling thread is blocked if the queue is empty,
 *  but the method returns early if the specified timeout expires.
 *  @param timeout is the number of nanoseconds after which the
 *  method should return, even if nothing has been added to the queue.
 *  @return the next item in the queue, or Queue::TIMEOUT if timeout occurs
 */
int Queue::deq(uint32_t timeout) {
	pthread_mutex_lock(&lock);

	// determine when timeout should expire
	timeval now; 
	gettimeofday(&now,NULL);
	timespec targetTime;
	targetTime.tv_sec = now.tv_sec;
	targetTime.tv_nsec = 1000 * now.tv_usec;
	int  dsec = timeout/1000000000;
	int dnsec = timeout%1000000000;
	targetTime.tv_sec += dsec;
	targetTime.tv_nsec += dnsec;
	if (targetTime.tv_nsec > 1000000000) {
		targetTime.tv_sec++; targetTime.tv_nsec -= 1000000000;
	}

	int status = 0;
	while (count == 0 && status != ETIMEDOUT) {
		status = pthread_cond_timedwait(&emptyQ,&lock,&targetTime);
	}
	int retVal;
	if (status == ETIMEDOUT) {
		retVal = Queue::TIMEOUT;
	} else {
		retVal = buf[head];
		count--;
		head = (head + 1) % qMax;
	}
	pthread_mutex_unlock(&lock);
	pthread_cond_signal(&fullQ);
	return retVal;
}
