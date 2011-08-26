/** \file Heap.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef HEAP_H
#define HEAP_H

#include "stdinc.h"
#include "Misc.h"

typedef uint64_t keytyp;
typedef int item;

/** This class implements a heap data structure.
 *  In this heap, keys are 64 bit unsigned integers.
 */
class Heap {
public:		Heap(int);
		~Heap();

	// access methods
	item	findmin();
	keytyp	key(item);

	// predicates
	bool	member(item);
	bool	empty();	

	// modifiers
	void	insert(item,keytyp);
	void	remove(item);
	item 	deletemin();
	void	changekey(item,keytyp);	

	string& toString(string&) const;
private:
	const static int D = 8;		// base of heap
	int	N;			// max number of items in heap
	int	n;			// number of items in heap

	item	*h;			// {h[1],...,h[n]} is set of items
	int	*pos;			// pos[i] gives position of i in h
	keytyp	*kee;			// kee[i] is key of item i

	item	minchild(item);		// return topmost child of item
	void	siftup(item,int);	// move item up to restore heap order
	void	siftdown(item,int);	// move item down to restore heap order
};

// Return item at top of heap
inline int Heap::findmin() { return n == 0 ? Null : h[1]; }

// Delete and return item at top of heap
inline int Heap::deletemin() {
	if (n == 0) return Null;
	item i = h[1]; remove(h[1]);
	return i;
}

// Return key of i.
inline keytyp Heap::key(item i) { return kee[i]; }

// Return true if i in heap, else false.
inline bool Heap::member(item i) { return pos[i] != Null; }

// Return true if heap is empty, else false.
inline bool Heap::empty() { return n == 0; };

#endif
