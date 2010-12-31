// Collection of Fibonacci heaps.

#ifndef FHEAPS_H
#define FHEAPS_H

#include "stdinc.h"
#include "misc.h"
#include "list.h"
#include "clist.h"

typedef int keytyp;
typedef int fheap;
typedef int item;
const int MAXRANK = 32;

class fheaps {
public:		fheaps(int=26);
		~fheaps();
	keytyp	key(item) const;		// return key of item
	fheap	findmin(fheap) const;		// return smallest item
	fheap	meld(fheap,fheap);		// combine two heaps
	fheap	decreasekey(item,keytyp,fheap); // decrease key of item
	fheap	deletemin(fheap);		// delete smallest item
	fheap	insert(item,fheap,keytyp);	// insert item in heap with key
	fheap	remove(item, fheap);		// remove specified item
	friend	ostream& operator<<(ostream&, const fheaps&); // print all heaps
	void	tprint(ostream&, fheap, int) const;	// print heap as tree
private:
	int	n;				// partition on {1,...,n}
	struct fnode {
	keytyp	kee;				// key values
	int	rank;				// rank values
	bool	mark;				// marks
	fheap	p, c;				// parent and child pointers
	} *node;
	clist	*sibs;				// collection of sibling lists
	int	rvec[MAXRANK+1];		// temporary vector of ranks
	list	*tmpq;				// temporary queue used
	void	print(ostream&, fheap) const;	// print one heap
};

// Return the key of item i.
inline keytyp fheaps::key(item i) const { return node[i].kee; };
	
// Find and return the smallest item in s.
inline fheap fheaps::findmin(fheap h) const { return h; };

#endif
