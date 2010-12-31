// Header file for data structure representing a collection of lists that
// partition a set of small positive integers. The lists are circularly
// linked in both directions. There is no concept of a first or last
// node of a list. We can refer to a list by referring to any of its
// elements.

#ifndef CLIST_H
#define CLIST_H

#include "stdinc.h"
#include "misc.h"

typedef int item;

class clist {
public:		clist(int=26);
		clist(const clist&);
		~clist();
	void	remove(item);		// remove an item from its list
	void	join(item,item);	// join two lists
	int	suc(item);	 	// return successor
	int	pred(item); 		// return predecessor
	clist&	operator=(const clist&); // list assignment
	friend ostream& operator<<(ostream&, const clist&);
					// print all the lists
private:
	int	N;			// list defined on ints in {1,...,N}
	struct lnode {
	int	next;			// index of successor
	int	prev;			// index of predecessor
	} *node;
	void	makeSpace();		// allocate dynamic storage
	void	freeSpace();		// free dynamic storage
	void	copyFrom(const clist&);	// copy in from another clist
};

inline item clist::suc(item i) {
// Return successor of item i
	assert(0 <= i <= N); return node[i].next;
}

inline item clist::pred(item i) {
// Return predecessor of item i
	assert(0 <= i <= N); return node[i].prev;
}

#endif
