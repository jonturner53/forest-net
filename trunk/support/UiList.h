/** \file UiList.h
 *  Data structure representing a UiList of unique integers.
 *
 *  Used to represent a UiList of integers from a defined range 1..n,
 *  where each integer may appear on the UiList at most one time.
 *  Allows fast membership tests in addition to the usual UiList
 *  operations.
 */

#ifndef UILIST_H
#define UILIST_H

#include "stdinc.h"
#include "Misc.h"

typedef int item;

class UiList {
public:		UiList(int=26);
		UiList(const UiList&);
		virtual ~UiList();

	// item access
	item 	get(int) const;
	item	next(item) const;
	item	head() const;
	item	tail() const;
	item	n() const;

	// predicates
	bool	valid(item) const;
	bool	member(item) const;
	bool	empty() const;

	// modifiers
	bool	insert(item,item);
	bool	removeNext(item);
	bool	addFirst(item);
	bool	addLast(item);
	bool	removeFirst();
	void	copyFrom(const UiList&);
	void	clear();

	// input/output
	// bool	read(istream&);
	void	write(ostream&);

protected:
	// managing dynamic storage
        void    makeSpace();   
	void	freeSpace();

private:
	int	nn;			///< list defined on ints in {1,...,nn}
	item	first;			///< first item in list
	item	last;			///< last UiList in UiList
	item	*nxt;			///< nxt[i] is successor of i in list
};

/** Return the successor of i. */
inline item UiList::next(item i) const {
        assert(valid(i)); return nxt[i];
}

/** Return first item on list. */
inline item UiList::head() const { return first; }

/** Return last item on list. */
inline item UiList::tail() const { return last; }

/** Return the largest value that can be stored in list. */
inline item UiList::n() const { return nn; }

/** Test if a given item is valid for this UiList */
inline bool UiList::valid(item i) const { return 1 <= i && i <= n(); }

/** Return true if list is empty. */
inline bool UiList::empty() const { return first == 0; }

/** Return true if specified item in list, else false. */
inline bool UiList::member(item i) const {
	return valid(i) && next(i) != -1;
}

/** Add item to the front of the list.
 *  @param item to be added.
 *  @return true if the list was modified, else false
 */
bool UiList::addFirst(item i) { return insert(i,0); }

/** Add item to the end of the list.
 *  @param item to be added.
 *  @return true if the list was modified, else false
 */
bool UiList::addLast(item i) { return insert(i,tail()); }

/** Remove the first item in the list.
 *  @return true if the list was modified, else false
 */
bool UiList::removeFirst() { return removeNext(0); }

#endif
