// Header file for a "modulo" heap heap data structure.
// In this heap, keys are 32 bit unsigned integers and
// are ordered circularly around the 32 bit key value
// space and are assumed to never occupy more than half
// the key space. The empty portion of the key space
// effectively defines the ordering of keys. What this
// means is that if a and b are distinct key values, then
// we consider a to be greater than b if (a-b) < 2^31,
// where the subtraction is performed modulo 2^32
// as is standard for unsigned arithmetic.
//
// This data structure is intended for use in applications
// where the key values represent times and these times
// are all clustered around some specific value representing
// the current time. It is up to the application to ensure
// that the items are removed from the heap before their
// key values become too "old" relative to the current time.
// Note that the data structure is oblivious to actual
// time values. It simply assumes that keys are ordered
// as described above.
//
// This heap is also designed so that it can be used as
// either a minheap or a maxheap (but not both at the
// same time). The choice is made by a boolean flag
// passed to the constructor. Note that no error checking
// is done for findmin, deletemin, findmax, deletemax.
// If you call findmin on a max-heap, you will actually
// get the item with the largest key, not the smallest.

#ifndef MHEAP_H
#define MHEAP_H

#include "stdinc.h"
#include "misc.h"

typedef uint32_t keytyp;
typedef int item;

class mheap {
public:		mheap(int,int,bool);
		~mheap();
	item	findmin();		// return the item at top of heap
	item	findmax();		// return the item at top of heap
	keytyp	key(item);		// return the key of item
	bool	member(item);		// return true if item in heap
	bool	empty();		// return true if heap is empty
	void	insert(item,keytyp);	// insert item with specified key
	void	remove(item);		// remove item from heap
	item 	deletemin();		// delete and return item at top of heap
	item 	deletemax();		// delete and return item at top of heap
	void	changekey(item,keytyp);	// change the key of an item
	friend  ostream& operator<<(ostream&, const mheap&); // output mheap
private:
	int	N;			// max number of items in heap
	int	n;			// number of items in heap
	int	d;			// base of heap
	bool	minFlag;		// true for minheaps
	item	*h;			// {h[1],...,h[n]} is set of items
	int	*pos;			// pos[i] gives position of i in h
	keytyp	*kee;			// kee[i] is key of item i

	item	topchild(item);		// return topmost child of item
	void	siftup(item,int);	// move item up to restore heap order
	void	siftdown(item,int);	// move item down to restore heap order
};

// Return item at top of heap
inline int mheap::findmin() { return n == 0 ? Null : h[1]; }
inline int mheap::findmax() { return n == 0 ? Null : h[1]; }

// Delete and return item at top of heap
inline int mheap::deletemin() {
	if (n == 0) return Null;
	item i = h[1]; remove(h[1]);
	return i;
}
inline int mheap::deletemax() {
	if (n == 0) return Null;
	item i = h[1]; remove(h[1]);
	return i;
}

// Return key of i.
inline keytyp mheap::key(item i) { return kee[i]; }

// Return true if i in heap, else false.
inline bool mheap::member(item i) { return pos[i] != Null; }

// Return true if heap is empty, else false.
inline bool mheap::empty() { return n == 0; };

#endif