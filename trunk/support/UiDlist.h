/** \file UiDlist.h
 *  Data structure representing a list of unique integers.
 *
 *  Used to represent a list of integers from a defined range 1..n,
 *  where each integer may appear on the list at most one time.
 *  Allows fast membership tests in addition to the usual list
 *  operations. This class extends UiList and adds support for
 *  reverse traversal and general remove operation.
 */

#ifndef UIDLIST_H
#define UIDLIST_H

#include "stdinc.h"
#include "Misc.h"
#include "UiList.h"

class UiDlist : public UiList {
public:		UiDlist(int=26);
		~UiDlist();

	// item access
	item	get(item) const;
	int	prev(item) const;

	// modifiers
	bool    insert(item,item);
        bool    remove(item);
        bool    removeNext(item);
	bool	removeLast();
        void    copyFrom(const UiDlist&); 
	void	clear();

	void	dump() const;	

protected:
	// handle dynamic storage
        void    makeSpace();   
	void	freeSpace();
private:
	item	*prv;			// prv[i] is previous item in list
};

/** Return the predecessor of an item in the list.
 *  @param i is item whose predecessor is to be returned
 *  @return the item that precedes i or 0, if none
 */
inline item UiDlist::prev(item i) const {
        assert(valid(i) && member(i)); return prv[i];
}

/** Remove the item following a specified item.
 *  @param i is item whose successor is to be removed;
 *  if zero, the first item is removed
 *  @return true if the list was modified, else false
 */
inline bool UiDlist::removeNext(item i) {
        return remove(i == 0 ? first() : next(i));
}

/** Remove the last item on the list.
 *  @return true if the list was modified, else false
 */
inline bool UiDlist::removeLast() {
        return remove(last());
}

#endif
