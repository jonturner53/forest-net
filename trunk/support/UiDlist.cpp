/** @file UiDlist.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "UiDlist.h"

/** Construct list with space for nn1 items */
UiDlist::UiDlist(int nn1) : UiList(nn1) {
	assert(n() >= 0); makeSpace();
}

UiDlist::~UiDlist() { freeSpace(); }

/** Allocate and initialize dynamic storage space for this object. */
void UiDlist::makeSpace() {
        prv = new item[n()+1];
        for (item i = 1; i <= n(); i++) prv[i] = -1;
        prv[0] = 0;
}

/** Release dynamic storage used by this object. */
void UiDlist::freeSpace() { delete [] prv; }

/** Get an item based on its position in the lsit.
 *  @param i is position of item to be returned; negative values
 *  are interpreted relative to the end of the list.
 *  @return the item at the specfied position
 */
item UiDlist::get(int i) const {
	if (i >= 0) return UiList::get(i);
	item j;
	for (j = last(); j != 0 && ++i; j = prv[j]) {}
	return j;
}

/** Insert an item into the list, relative to another.
 *  @param i is item to insert; if i is 0 or already in
 *  the list, no change is made
 *  @param j is item after which i is to be inserted;
 *  if j == 0, i is inserted at the front of the list
 *  @return true if list was modified, else false
 */
bool UiDlist::insert(item i, item j) {
	if (!UiList::insert(i,j)) return false;
	// now update prv
	prv[i] = j;
	if (next(i) != 0) prv[next(i)] = i;
        return true;
}

/** Remove a specified item.
 *  @param i is item to be removed
 *  @return true if the list was modified, else false
 */
bool UiDlist::remove(item i) {
	if (i == 0 || !UiList::removeNext(prev(i)))
		return false;
	if (prv[i] == 0) prv[first()] = 0;
	else {
		item j = prv[i];
		if (next(j) != 0) prv[next(j)] = j;
	}
	prv[i] = -1;
	return true;
}

/** Replace the content of the list with that of another.
 *  @param src is the list whose value is to replace this list's
 */
void UiDlist::copyFrom(const UiDlist& src) {
	bool resize = (src.n() > n());
	UiList::copyFrom(src);
	if (resize) { freeSpace(); makeSpace(); }

        item i;
        for (i = 1; i <= src.n(); i++) prv[i] = src.prv[i];
        for (i = src.n()+1; i <= n(); i++) prv[i] = -1;
}

/** Remove all items from list. */
void UiDlist::clear() {
	item t = last();
	while (t != 0) {
		int i = t; t = prv[t]; prv[i] = -1;
	}
	UiList::clear();
}
