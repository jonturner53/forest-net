#include "clist.h"

#define next(x) node[x].next
#define prev(x) node[x].prev

void clist::makeSpace() {
// Allocate dynamic storage and initialize. Assumes N has been initialized.
	node = new lnode[N+1];
	for (item i = 0; i <= N; i++) { next(i) = prev(i) = i; }
	next(Null) = prev(Null) = Null;
}
clist::clist(int N1) : N(N1) {
// Construct clist with space for N1 items
	assert(N >= 0);
	makeSpace();
}

// Free dynamic storage
void clist::freeSpace() { delete [] node; }

clist::~clist() { freeSpace(); }

void clist::copyFrom(const clist& C) {
// Copy in from another clist.
	assert(N >= C.N);
	item i;
	for (i = 1; i <= C.N; i++) {
		next(i) = C.next(i); prev(i) = C.prev(i);
	}
	for (i = C.N+1; i <= N; i++) { next(i) = prev(i) = i; }
}

clist::clist(const clist& C) : N(C.N) {
// Construct copy of given clist
	assert(N >= 0);
	makeSpace(); copyFrom(C);
}

// Assign value of right operand to left operand.
// Reallocate space if necessary.
clist& clist::operator=(const clist& L) {
        if (&L == this) return *this;
        if (N < L.N) { freeSpace(); N = L.N; makeSpace(); }
        copyFrom(L);
        return (*this);
}

void clist::remove(item i) {
// Remove i from its list.
	assert(0 <= i && i <= N);
	next(prev(i)) = next(i);
	prev(next(i)) = prev(i);
	next(i) = prev(i) = i;
}

void clist::join(item i, item j) {
// Join the lists containing i and j.
	assert(0 <= i && i <= N && 0 <= j && j <= N);
	if (i == Null || j == Null) return;
	prev(next(i)) = prev(j);
	next(prev(j)) = next(i);
	next(i) = j; prev(j) = i;
}

ostream& operator<<(ostream& os, const clist& cl) {
// Print the contents of all the lists
	item i, j;
	int *mark = new int[cl.N+1];
	for (i = 1; i <= cl.N; i++) mark[i] = 0;
	for (i = 1; i <= cl.N; i++) {
		if (mark[i]) continue; 
		mark[i] = 1;
		if (i > 1) { os << ", "; }
		os << "("; misc::putNode(os,i,cl.N);
		for (j = cl.next(i); j != i; j = cl.next(j)) {
			mark[j] = 1; 
			os << " "; misc::putNode(os,j,cl.N);
		}
		os << ")";
	}
	delete [] mark;
	return os;
}
