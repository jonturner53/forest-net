/** @file UiListSet.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef UILISTSET_H
#define UILISTSET_H

#include "stdinc.h"
#include "Misc.h"

typedef int item;
typedef int alist;

/**  Header file for data structure representing a collection of lists
 *  defined over integers 1,2,... Each item can be on at most one list.
 *  Lists are also numbered 1,2,...
 */
class UiListSet {
public:		UiListSet(int=26,int=5);
		~UiListSet();

	/** access items */
	item	next(item) const;	
	item	first(alist) const;
	item	last(alist) const;	

	/** predicates */
	bool	member(item) const;
	bool	empty(alist) const;

	/** modifiers */
	void	addLast(item, alist);
	void	addFirst(item,alist);
	item	removeFirst(alist);

	/** input/output */
	void	write(ostream&, alist) const;
	void	write(ostream&) const;
private:
	int	nI;			// listset defined on ints in {1..nI}
	int	nL;			// lists 1..nL
	struct listhdr {
		alist head, tail;
	};
	listhdr	*lh;			// array of list headers
	item	*nxt;			// next[i] is successor of i
};

// Return first item on listset.
inline item UiListSet::first(alist lst) const { return lh[lst].head; }

// Return last item on listset.
inline item UiListSet::last(alist lst) const { return lh[lst].tail; }

// Return true if listset is empty.
inline bool UiListSet::empty(alist lst) const { return lh[lst].head == Null; }

// Return true if i in listset, else false.
inline bool UiListSet::member(item i) const { return nxt[i] != -1; }

// Return the successor of i.
inline item UiListSet::next(item i) const { return nxt[i]; }

#endif
