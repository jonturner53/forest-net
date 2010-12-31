// Header file for data structure representing a collection of lists.
// In this version, each list node has a 12 bit value field and a
// a 20 bit link to the next node, limiting the number of list nodes
// to just over 1 million. The class does not maintain head and tail
// pointers to the list. The calling program must maintain a pointer
// to the head of each list to maintain access.
//

#ifndef LISTS_H
#define LISTS_H

#include "stdinc.h"
#include "misc.h"

typedef uint32_t lnode;
typedef int lvalu;
typedef int item;
typedef int alist;

const uint32_t IMSK = 0xfff00000;
const uint32_t ISHFT = 20;
const uint32_t PMSK = ~IMSK;

class lists {
public:		lists(int=26);
		virtual ~lists();
	alist	insert(lvalu, alist);	// insert new item at front of list
	alist	remove(alist);		// remove first item
	alist	remove(lvalu,alist);	// remove item with given value
	alist	clear(alist);		// remove all items from given list
	lvalu	value(item) const;	// return value of given list element
	item	suc(item) const;	// return successor
	bool	mbr(lvalu,alist);	// return true if list contains value

	void 	print(ostream&, alist) const;	// print one list
	friend	ostream& operator<<(ostream&, const lists&); // print all lists
private:
	int	N;			// number of list nodes in space
	lnode	*node;			// node[i] is i-th node in the space
	int	free;			// first unused list node
};

// Return value of given item.
inline lvalu lists::value(item i) const { return node[i] >> ISHFT; }

// Return the successor of i.
inline item lists::suc(item i) const { return node[i] & PMSK; }

#endif
