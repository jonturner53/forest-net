// Header file for data structure representing a collection of reversible
// lists on the integers 1..n. The lists are circularly linked in both
// directions. A list is referred to by its LAST item.

#ifndef RLIST_H
#define RLIST_H

#include "stdinc.h"
#include "misc.h"

typedef int item;

class rlist {
public:		rlist(int=26);
		~rlist();
	item	first(item);		// return first item on list
	item	pop(item);		// remove first item from a list
	item	join(item,item);	// join two lists
	item	reverse(item);		// reverse a list
	void	print(ostream&, item);	// print one list in collection
	///////////////////////////////////////////////////////////////////
		rlist(const rlist&);	// stubbed, not implemented
	rlist&	operator=(const rlist&);// stubbed, not implemented
private:
	int	N;			// lists defined on ints in {1,...,N}
	struct lnode {
	int	next;			// index of successor
	int	prev;			// index of predecessor
	} *node;
};

// Return the first item on the list whose last item is t.
inline item rlist::first(item t) { return node[t].next; }

// Stubs for copy constructor and assignment operator
inline rlist::rlist(const rlist& h) {
	fatal("rlist: no copy constructor available");
}
inline rlist& rlist::operator=(const rlist& L) {
	fatal("rlist: no assignment operator available");
}

#endif
