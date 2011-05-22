/** \file UiList.cpp */

#include "UiList.h"

/** Create a list that can hold items in 1..nn1 */
UiList::UiList(int nn1) : nn(nn1) {
	assert(nn >= 0); makeSpace();
	head = tail = 0;
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
	if (i == 1) return first();
	item j;
	for (j = first(); j != 0 && --i; j = nxt[j]) {}
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
		if (empty()) tail = i;
		nxt[i] = head; head = i;
		return true;
	}
	nxt[i] = nxt[j]; nxt[j] = i;
	if (tail == j) tail = i;
	return true;
}

/** Remove the item following a specified item.
 *  @param i is item whose successor is to be removed;
 *  if zero, the first item is removed
 *  @return true if the list was modified, else false
 */
bool UiList::removeNext(item i) {
	assert(i == 0 || valid(i));
	if (empty() || (i == last()) || (i != 0 && !member(i)))
	    	return false;
	item j;
	if (i == 0) { j = head;   head = nxt[j]; }
	else	    { j = nxt[i]; nxt[i] = nxt[j]; }
	if (tail == j) tail = i;
	nxt[j] = -1;
	return true;
}

/** Copy into list from src. */
void UiList::copyFrom(const UiList& src) {
	if (&src == this) return;
	if (src.n() > n()) {
		freeSpace(); nn = src.n(); makeSpace();
	}
	item i;
	for (i = 1; i <= src.n(); i++) nxt[i] = src.nxt[i];
	for (i = src.n()+1; i <= n(); i++) nxt[i] = -1;
	head = src.head; tail = src.tail;
}

/** Remove all elements from list. */
void UiList::clear() {
	while (head != 0) {
		item i = head; head = nxt[i]; nxt[i] = -1;
	}
	tail = 0;
}

/** Compare two lists for equality.
 *
 *  @param other is the list to be compared to this one
 *  @return true if they are the same list or have the
 *  same contents; they need not have the same storage
 *  capacity to be equal
 */
bool UiList::equals(UiList& other) const {
	if (this == &other) return true;
	item i = first(); item j = other.first();
	while (i == j) {
		if (i == 0) return true;
		i = next(i); j = other.next(j);
	}
	return false;
}

/** Add a representation of the list to a given string.
 *
 *  @param s points to string to be extended
 */
void UiList::add2string(string& s) const {
	s += "[ ";
	for (item i = first(); i != 0; i = next(i)) {
		Misc::addNode2string(s,i,n()); s += " ";
	}
	s += "]";
}

/** Write the contents of the list. */
void UiList::write(ostream& os) {
	string s; add2string(s); os << s;
}
