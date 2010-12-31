// Lazy Collection of leftist heaps
// This version uses implicit deletion. That is, the user provides a
// pointer to a function that accepts a single item argument and returns
// true if that item has been removed from a heap. Deleted items should not
// be re-inserted into another heap.

#ifndef LLHEAPS_H
#define LLHEAPS_H

#include "lheaps.h"
#include "list.h"

typedef bool (*delftyp)(item);

class llheaps : public lheaps {
public:		llheaps(int=26,delftyp=NULL);
		~llheaps();
	item	findmin(lheap);		// return item with smallest key
	lheap	lmeld(lheap,lheap);	// lazy version of meld
	lheap	insert(item,lheap);	// insert item into heap
	lheap	makeheap(list&);	// create and return heap from items
	friend	ostream& operator<<(ostream&, const llheaps&);	// print heaps
	void	tprint(ostream&, lheap,int) const; // print heap as tree
private:
	int	dummy;			// head of free dummy node list
	delftyp	delf;			// pointer to deleted function
	list	*tmpL;			// pointer to temporary list
	void	purge(lheap,list&);	// build list of non-deleted subtrees
	lheap	heapify(list&);		// combine list of heaps
	void	sprint(ostream&, lheap) const;	// print one heap
};
#endif
