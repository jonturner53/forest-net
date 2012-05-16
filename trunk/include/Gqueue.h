/** \file Gqueue.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef GQUEUE_H
#define GQUEUE_H

#include <pthread.h>
#include "stdinc.h"
#include "Util.h"

/** This class implements a simple thread-safe queue for communication
 *  among threads. It uses templates, but is not intended to support
 *  arbitrary classes. Can be used with any simple type or simple
 *  structs.
 */
template<class T> class Gqueue {
public:		Gqueue(int=10);
		~Gqueue();
	bool	init();
	void	reset();

	bool	empty() const;

	void	enq(T);
	T	deq();	
	T	deq(uint32_t);	
	static	int const TIMEOUT = (1 << 31); // largest 32 bit negative number
private:
	int	qMax;			///< max number of items in queue

	int	count;			///< number of items in queue
	int	head;			///< index of first item in buf
	int 	tail;			///< index of first empty space in buf

	T	*buf;			///< where values are stored

	pthread_mutex_t lock;		///< used to ensure mutual exclusion
	pthread_cond_t emptyQ;		///< condition variable for empty queue
	pthread_cond_t fullQ;		///< condition variable for full queue
};

/** Determine if queue is empty.
 *  @return true if the queue is empty, else false
 */
template<class T> inline bool Gqueue<T>::empty() const { return count == 0; }

#endif
