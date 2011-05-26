/** @file UiList.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef UILIST_H
#define UILIST_H

#include "stdinc.h"
#include "Misc.h"

typedef int item;

/** Data structure representing a UiList of unique integers.
 *
 *  Used to represent a list of integers from a defined range 1..n,
 *  where each integer may appear on the list at most one time.
 *  Allows fast membership tests in addition to the usual list
 *  operations.
 *
 *  Note: UiList may be extended by other classes, but it was
 *  not designed for use in contexts where  polymorphism is
 *  required. To make it polymorphic, many of its methods
 *  would have to be made virtual.
 */
class UiList {
public:		UiList(int=26);
		~UiList();

	// item access
	item 	get(int) const;
	item	next(item) const;
	item	first() const;
	item	last() const;
	item	n() const;

	// predicates
	bool	valid(item) const;
	bool	member(item) const;
	bool	empty() const;
	bool	equals(UiList&) const;

	// modifiers
	virtual bool insert(item,item);
	virtual bool removeNext(item);
	bool	addFirst(item);
	bool	addLast(item);
	bool	removeFirst();
	void	copyFrom(const UiList&);
	void	clear();

	// input/output
	// bool	read(istream&);
	void	add2string(string&) const;
	void	write(ostream&);

protected:
	// managing dynamic storage
        void    makeSpace();   
	void	freeSpace();

private:
	int	nn;			///< list defined on ints in {1,...,nn}
	item	head;			///< first item in list
	item	tail;			///< last item in list
	item	*nxt;			///< nxt[i] is successor of i in list
};

/** Return the successor of i. */
inline item UiList::next(item i) const {
        assert(member(i)); return nxt[i];
}

/** Return first item on list. */
inline item UiList::first() const { return head; }

/** Return last item on list. */
inline item UiList::last() const { return tail; }

/** Return the largest value that can be stored in list. */
inline item UiList::n() const { return nn; }

/** Test if a given item is valid for this UiList */
inline bool UiList::valid(item i) const { return 1 <= i && i <= n(); }

/** Return true if list is empty. */
inline bool UiList::empty() const { return first() == 0; }

/** Return true if specified item in list, else false. */
inline bool UiList::member(item i) const {
	return valid(i) && nxt[i] != -1;
}

/** Add item to the front of the list.
 *  @param item to be added.
 *  @return true if the list was modified, else false
 */
inline bool UiList::addFirst(item i) { return insert(i,0); }

/** Add item to the end of the list.
 *  @param item to be added.
 *  @return true if the list was modified, else false
 */
inline bool UiList::addLast(item i) { return insert(i,last()); }

/** Remove the first item in the list.
 *  @return true if the list was modified, else false
 */
inline bool UiList::removeFirst() { return removeNext(0); }

#endif
