// Header file for data structure representing a collection of lists
// defined over integers 1,2,... Each item can be on at most one list.
// Lists are also numbered 1,2,...
//

#ifndef LISTSET_H
#define LISTSET_H

#include "stdinc.h"
#include "misc.h"

typedef int item;
typedef int alist;


class listset {
public:		listset(int=26,int=5);
		virtual ~listset();
	void	enq(item, alist);	// enqueue item on list
	item	deq(alist);		// remove and return first item on list
	void	push(item,alist);	// push item onto front of list
	bool	mbr(item) const;	// return true if member of some list
	bool	empty(alist) const;	// return true if list is empty
	item	suc(item) const;	// return successor
	item	head(alist) const;	// return first item on list
	item	tail(alist) const;	// return last item on list
	void	print(ostream&, alist) const; // print a single list
	friend ostream& operator<<(ostream&, const listset&); // output listset
private:
	int	nI;			// listset defined on ints in {1..nI}
	int	nL;			// lists 1..nL
	struct listhdr {
		alist first, last;
	};
	listhdr	*lh;			// array of list headers
	item	*next;			// next[i] is successor of i
};

// Return last item on listset.
inline item listset::head(alist L) const { return lh[L].first; }

// Return last item on listset.
inline item listset::tail(alist L) const { return lh[L].last; }

// Return true if listset is empty.
inline bool listset::empty(alist L) const { return lh[L].first == Null; }

// Return true if i in listset, else false.
inline bool listset::mbr(item i) const { return next[i] != -1; }

// Return the successor of i.
inline item listset::suc(item i) const { return next[i]; }

#endif
