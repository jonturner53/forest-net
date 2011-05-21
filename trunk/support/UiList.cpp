/** \file UiList.cpp */

#include "UiList.h"

/** Create a list that can hold items in 1..nn1 */
UiList::UiList(int nn1) : nn(nn1) {
	assert(nn >= 0); makeSpace();
	first = last = 0;
}

UiList::~UiList() { freeSpace(); }

/** Allocate and initialize space for list.
 *  Assumes n has been initialized.
 */
void UiList::makeSpace() {
	nxt = new item[n()+1];
	for (item i = 1; i <= n(); i++) nxt[i] = -1;
	nxt[0] = 0;
}

/** Free dynamic storage used by list. */
void UiList::freeSpace() { delete [] nxt; }

/** Get an item based on its position in the list.
 *  @param i is position of item to be returned
 *  @return item at position i, or 0 if no such item
 */
item UiList::get(int i) const {
	if (i == 1) return first;
	item j;
	for (j = first; j != 0 && --i; j = nxt[j]) {}
	return j;
}

/** Insert an item into the list, relative to another.
 *  @param i is item to insert; if i is 0 or already in
 *  the list, no change is made
 *  @param j is item after which i is to be inserted;
 *  if zero, i is inserted at the front of the list
 *  @return true if list was modified, else false
 */
bool UiList::insert(item i, item j) {
	assert((i == 0 || valid(i)) && (j == 0 || valid(j)));
	if (i == 0 || member(i)) return false;
	if (j == 0) {
		if (empty()) last = i;
		nxt[i] = first; first = i;
		return true;
	}
	nxt[i] = nxt[j]; nxt[j] = i;
	if (last == j) last = i;
	return true;
}

/** Remove the item following a specified item.
 *  @param i is item whose successor is to be removed;
 *  if zero, the first item is removed
 *  @return true if the list was modified, else false
 */
bool UiList::removeNext(item i) {
	assert(i == 0 || valid(i));
	if (empty() || (i == last) || (i != 0 && !member(i)))
	    	return false;
	item j;
	if (i == 0) { j = first;   first = nxt[j]; }
	       else { j = nxt[i]; nxt[i] = nxt[j]; }
	nxt[j] = -1;
	return true;
}

/** Copy into list from src. */
void UiList::copyFrom(const UiList& src) {
	if (src.n() > n()) {
		freeSpace(); nn = src.n(); makeSpace();
	}
	item i;
	for (i = 1; i <= src.n(); i++) nxt[i] = src.nxt[i];
	for (i = src.n()+1; i <= n(); i++) nxt[i] = -1;
	first = src.first; last = src.last;
}

/** Remove all elements from list. */
void UiList::clear() {
	while (first != 0) {
		item i = first; first = nxt[i]; nxt[i] = -1;
	}
	last = 0;
}

/** Write the contents of the list. */
void UiList::write(ostream& os) {
	os << "[ ";
	for (item i = head(); i != 0; i = next(i))
		Misc::writeNode(os,i,n()); os << ' ';
	os << "]";
}
