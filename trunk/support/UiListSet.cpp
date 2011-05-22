/** \file UiListSet.cpp */

#include "UiListSet.h"

// Allocate and initialize space for listset.
UiListSet::UiListSet(int nI1, int nL1) : nI(nI1), nL(nL1) {
	int i;
	nxt = new item[nI+1]; lh = new listhdr[nL+1];
	for (i = 0; i <= nL; i++) {
		lh[i].head = lh[i].tail = 0;
	}
	for (i = 0; i <= nI; i++) nxt[i] = -1;
}

UiListSet::~UiListSet() { delete [] nxt; delete [] lh; }

// Add i to the end of list j.
void UiListSet::addLast(item i, alist j) {
	if (i == 0) return;
	if (lh[j].head == 0) lh[j].head = i;
	else nxt[lh[j].tail] = i;
	lh[j].tail = i; nxt[i] = 0;
}

// Remove first item on list and return it.
int UiListSet::removeFirst(alist j) {
	int i = lh[j].head;
	if (i == 0) return 0;
	lh[j].head = nxt[i]; nxt[i] = -1;
	return i;
}

// Add i to the front of list j.
void UiListSet::addFirst(item i, alist j) {
	if (i == 0) return;
	if (lh[j].head == 0) lh[j].tail = i;
	nxt[i] = lh[j].head;
	lh[j].head = i;
}

// Print the contents of one list.
void UiListSet::write(ostream& os, alist j) const {
	int i;
	os << setw(2) << j << ": ";
	for (i = first(j); i != 0; i = next(i)) {
		Misc::writeNode(os,i,nI); os << " ";
	}
	os << endl;
}

// Print the contents of the listset.
void UiListSet::write(ostream& os) const {
	item i; alist j;
	for (j = 1; j <= nL; j++) {
		if (lh[j].head != 0) write(os,j);
	}
}
