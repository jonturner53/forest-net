#include "listset.h"

// Allocate and initialize space for listset.
listset::listset(int nI1, int nL1) : nI(nI1), nL(nL1) {
	int i;
	next = new item[nI+1]; lh = new listhdr[nL+1];
	for (i = 0; i <= nL; i++) {
		lh[i].first = lh[i].last = Null;
	}
	for (i = 0; i <= nI; i++) next[i] = -1;
}

listset::~listset() { delete [] next; delete [] lh; }

// Add i to the end of list j.
void listset::enq(item i, alist j) {
	if (i == Null) return;
	if (lh[j].first == Null) lh[j].first = i;
	else next[lh[j].last] = i;
	lh[j].last = i; next[i] = Null;
}

// Remove first item on list and return it.
int listset::deq(alist j) {
	int i = lh[j].first;
	if (i == Null) return Null;
	lh[j].first = next[i]; next[i] = -1;
	return i;
}

// Add i to the front of list j.
void listset::push(item i, alist j) {
	if (i == Null) return;
	if (lh[j].first == Null) lh[j].last = i;
	next[i] = lh[j].first;
	lh[j].first = i;
}

// Print the contents of one list.
void listset::print(ostream& os, alist j) const {
	int i;
	os << setw(2) << j << ": ";
	for (i = head(j); i != Null; i = next[i]) {
		misc::putNode(os,i,nI); os << " ";
	}
	os << endl;
}

// Print the contents of the listset.
ostream& operator<<(ostream& os, const listset& L) {
	item i; alist j;
	for (j = 1; j <= L.nL; j++) {
		if (L.lh[j].first != Null) L.print(os,j);
	}
	return os;
}
