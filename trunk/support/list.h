// Header file for data structure representing list of small positive integers.
//
// Invariant properties
// Null = 0, next[Null] = Null
// 0 <= first <= N, 0 <= last <= N, -1 <= next[i] <= N
// if first != Null then next[first] != -1
// if last != Null then next[last] = Null
// if next[i] > 0 then next[next[i]] != -1
// if next[i] = j then i != j, if next[next[i]] = j then i != j, etc.

#ifndef LIST_H
#define LIST_H

#include "stdinc.h"
#include "misc.h"

typedef int item;

class list {
public:		list(int=26);
		list(const list&);
		virtual ~list();
	void	push(item);		// push item onto front of list
	void	insert(item,item);	// insert one item following another
	bool	mbr(item) const;	// return true if member of list
	bool	empty() const;		// return true if list is empty
	item	suc(item) const;	// return successor
	item	tail() const;		// return last item on list
	void	clear();		// remove everything
	item 	operator[](int) const;	// access item
	list& 	operator=(const list&);	// list assignment
	list& 	operator&=(item);	// append item
	list& 	operator<<=(int);	// remove initial items
	friend ostream& operator<<(ostream&, const list&); // output list
protected:
	int	N;			// list defined on ints in {1,...,N}
	item	first;			// first item in list
	item	last;			// last list in list
	item	*next;			// next[i] is successor of i in list
        void    virtual makeSpace();    // allocate dynamic storage
	void	virtual mSpace();		 
	void	virtual freeSpace();	// free dynamic storage
	void	virtual fSpace();
        void    virtual copyFrom(const list&); 	// copy-in from given list
        void    virtual cFrom(const list&); 	
};

// Return last item on list.
inline item list::tail() const { return last; }

// Return true if list is empty.
inline bool list::empty() const { return first == Null; }

// Return true if i in list, else false.
inline bool list::mbr(item i) const { return 1<=i && i<=N && next[i] != -1; }

// Return the successor of i.
inline item list::suc(item i) const {
        assert(1 <= i && i <= N && 0 <= next[i] && next[i] <= N);
        return next[i];
}

#endif
