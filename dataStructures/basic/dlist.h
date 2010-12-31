// Header file for data structure representing list of small positive integers.
// This version is doubly linked and supports efficient deletion of any item.
//
// Invariant properties in addition to those from list
// prev[Null] = Null
// -1 <= prev[i] <= N
// if last != Null then prev[last] != -1
// if first != Null then prev[first] = Null
// if prev[i] > 0 then prev[prev[i]] != -1
// if next[i] = j > 0 then prev[j] = i
// if prev[i] = j > 0 then next[j] = i

#ifndef DLIST_H
#define DLIST_H

#include "stdinc.h"
#include "misc.h"
#include "list.h"

class dlist : public list {
public:		dlist(int=26);
		dlist(const dlist&);
		~dlist();
	int	pred(item) const;	// return predecessor
	void	clear();		// remove everything
	void	dump() const;		// dump the raw data structure contents

	dlist& operator=(const dlist&);	// list assignment
	void push(item);		// push item on front of list
	void insert(item,item);		// push item after another
	item operator[](int);		// access list item
	dlist& operator&=(item);	// append item
	dlist& operator<<=(int);	// remove initial items
	dlist& operator-=(item);	// remove a specific item
private:
	item	*prev;			// prev[i] is predecessor of i in list
        void    makeSpace();            // allocate dynamic storage
	void	mSpace();
	void	freeSpace();		// free dynamic storage
	void	fSpace();
        void    copyFrom(const dlist&);  // copy-in from given list
	void	cFrom(const dlist&);
};

// Return the predecessor of i.
inline item dlist::pred(item i) const {
        assert(1 <= i && i <= N && 0 <= prev[i] && prev[i] <= N);
        return prev[i];
}

#endif
