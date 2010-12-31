// Header file for a specialized mlist data structure with a fast 
// membership test. Supports a mlist of integers in a fixed range [1..n]
// where each integer appears at most once.
// 

#ifndef MLIST_H
#define MLIST_H

#include "stdinc.h"
#include "misc.h"


class mlist {
public:		mlist(int=26);
		mlist(const mlist&);
		virtual ~mlist();
	typedef int item;
	bool	enq(item);		// add item to end of list
	item	deq();			// remove an item from the front
	void	push(item);		// push item onto front of mlist
	bool	insert(item,item);	// insert one item following another
	bool	remove(item);		// remove an item from the list
	bool	mbr(item) const;	// return true if item is on mlist
	bool	empty() const;		// return true if mlist is empty
	item	suc(item) const;	// return successor
	item	pred(item) const;	// return predecessor
	item	tail() const;		// return last item on mlist
	void	clear();		// remove everything in the list
	mlist& 	operator=(const mlist&);// mlist assignment
	friend ostream& operator<<(ostream&, const mlist&); // output mlist
protected:
	int	N;			// mlist defined on ints in {1,...,N}
// rework with iterators
	item	first;			// first item in mlist
	item	last;			// last mlist in mlist
	vector<item> next;		// next[i] is successor of i in mlist
	vector<item> prev;		// prev[i] is successor of i in mlist
        void    virtual makeSpace();    // allocate dynamic storage
	void	virtual mSpace();		 
	void	virtual freeSpace();	// free dynamic storage
	void	virtual fSpace();
        void    virtual copyFrom(const mlist&); 	// copy-in from given mlist
        void    virtual cFrom(const mlist&); 	
};

// Return last item on mlist.
inline item mlist::tail() const { return last; }

// Return true if mlist is empty.
inline bool mlist::empty() const { return first == Null; }

// Return true if i in mlist, else false.
inline bool mlist::mbr(item i) const { return 1<=i && i<=N && next[i] != -1; }

// Return the successor of i.
inline item mlist::suc(item i) const {
        assert(1 <= i && i <= N && 0 <= next[i] && next[i] <= N);
        return next[i];
}

#endif
