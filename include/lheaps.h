// Collection of leftist heaps

#ifndef LHEAPS_H
#define LHEAPS_H

#include "stdinc.h"
#include "misc.h"

typedef int keytyp;
typedef int lheap;
typedef int item;

class lheaps {
public:		lheaps(int=100);
		~lheaps();
	keytyp	key(item) const;		// return key of item
	void	setkey(item,keytyp);		// set key of item
	lheap	findmin(lheap) const;		// return smallest item
	lheap	meld(lheap,lheap);		// combine two heaps
	lheap	insert(item,lheap);		// insert item into heap
	item	deletemin(lheap);		// delete smallest item
	friend	ostream& operator<<(ostream&, const lheaps&); // print all heaps
	void	tprint(ostream&,lheap,int) const;// print one heap as tree
protected:
	int	n;				// partition on {1,...,n}
	struct hnode {
	keytyp	kee;				// kee[i] = key of fitem i
	int	rank;				// rank[i] = rank of item i
	int	left;				// left[i] = left child of i
	int	right;				// right[i] = right child of i
	} *node;
	void	sprint(ostream&, lheap) const;	// print one heap
};

// Return the key of item i.
inline keytyp lheaps::key(item i) const { return node[i].kee; };
	
// Make key(i) = k.
inline void lheaps::setkey(item i,keytyp k) { node[i].kee = k; };

// Find and return the smallest item in s.
inline lheap lheaps::findmin(lheap h) const { return h; };

#endif
