// Header file for d-heap data structure. Maintains a subset
// of items in {1,...,m}, where item has a key.

#ifndef DHEAP_H
#define DHEAP_H

#include "stdinc.h"
#include "misc.h"

typedef int keytyp;
typedef int item;

class dheap {
public:		dheap(int=100,int=2);
		~dheap();
	item	findmin();		// return the item with smallest key
	keytyp	key(item);		// return the key of item
	bool	member(item);		// return true if item in heap
	bool	empty();		// return true if heap is empty
	void	insert(item,keytyp);	// insert item with specified key
	void	remove(item);		// remove item from heap
	item 	deletemin();		// delete and return smallest item
	void	changekey(item,keytyp);	// change the key of an item
	friend  ostream& operator<<(ostream&, const dheap&); // output dheap
private:
	int	N;			// max number of items in heap
	int	n;			// number of items in heap
	int	d;			// base of heap
	item	*h;			// {h[1],...,h[n]} is set of items
	int	*pos;			// pos[i] gives position of i in h
	keytyp	*kee;			// kee[i] is key of item i

	item	minchild(item);		// returm smallest child of item
	void	siftup(item,int);	// move item up to restore heap order
	void	siftdown(item,int);	// move item down to restore heap order
};

// Return item with smallest key.
inline int dheap::findmin() { return n == 0 ? Null : h[1]; }

// Return key of i.
inline keytyp dheap::key(item i) { return kee[i]; }

// Return true if i in heap, else false.
inline bool dheap::member(item i) { return pos[i] != Null; }

// Return true if heap is empty, else false.
inline bool dheap::empty() { return n == 0; };

#endif
