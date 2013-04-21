/** \file Queue.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include "stdinc.h"
#include "Util.h"

namespace forest {


/** This class implements a simple thread-safe queue for communication
 *  among threads.
 */
class Queue {
public:		Queue(int=10);
		~Queue();
	bool	init();
	void	reset();

	bool	empty() const;

	void	enq(int);
	int	deq();	
	int	deq(uint32_t);	
	static	int const TIMEOUT = (1 << 31); // largest 32 bit negative number
private:
	int	qMax;			///< max number of items in queue

	int	count;			///< number of items in queue
	int	head;			///< index of first item in buf
	int 	tail;			///< index of first empty space in buf

	int	*buf;			///< where values are stored

	pthread_mutex_t lock;		///< used to ensure mutual exclusion
	pthread_cond_t emptyQ;		///< condition variable for empty queue
	pthread_cond_t fullQ;		///< condition variable for full queue
};

/** Determine if queue is empty.
 *  @return true if the queue is empty, else false
 */
inline bool Queue::empty() const { return count == 0; }

} // ends namespace


#endif
