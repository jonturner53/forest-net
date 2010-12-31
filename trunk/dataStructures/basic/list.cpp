#include "list.h"

// Allocate and initialize space for list. Assumes N has been initialized.
void list::makeSpace() {
	next = new item[N+1];
	for (item i = 1; i <= N; i++) next[i] = -1;
	next[Null] = Null;
}
void list::mSpace() { makeSpace(); }

// Create a list that can hold up to N1 items
list::list(int N1) : N(N1) {
	assert(N >= 0); makeSpace();
	first = last = Null;
}

// free dynamic storage
void list::freeSpace() { delete [] next; }
void list::fSpace() { freeSpace(); }

list::~list() { freeSpace(); }

// Copy into list from L.
void list::copyFrom(const list& L) {
	assert(N >= L.N);
	item i;
	for (i = 1; i <= L.N; i++) next[i] = L.next[i];
	for (i = L.N+1; i <= N; i++) next[i] = -1;;
	first = L.first; last = L.last;
}
void list::cFrom(const list& L) { copyFrom(L); }

// Create a copy of the list L
list::list(const list& L) : N(L.N) {
	assert(N >= 0);
	makeSpace(); copyFrom(L);
}

// Assign value of right operand to left operand.
// Reallocate space if necessary.
list& list::operator=(const list& L) {
	if (&L == this) return *this;
	if (N < L.N) { fSpace(); N = L.N; mSpace(); }
	cFrom(L);
	return (*this);
}

// Remove all elements from list.
void list::clear() {
	while (first != Null) {
		item i = first; first = next[i]; next[i] = -1;
	}
	last = Null;
}

// Add i to the front of the list.
void list::push(item i) {
	assert(i == 0 || (1 <= i && i <= N && next[i] == -1));
	if (i == Null) return;
	if (first == Null) last = i;
	next[i] = first;
	first = i;
}

// Insert item i after item j.
void list::insert(item i, item j) {
	assert(	0 <= i && i <= N && next[i] == -1 &&
		0 <= j && j <= N && 0 <= next[j] && next[j] <= N
		);
	if (i == Null) return;
	if (j == Null) {
		if (first == Null) last = i;
		next[i] = first; first = i;
		return;
	}
	next[i] = next[j]; next[j] = i;
	if (last == j) last = i;
}

// Return the i-th element, where the first is 1.
item list::operator[](int i) const {
	item j;
	if (i == 1) return first;
	for (j = first; j != Null && --i; j = next[j]) {}
	return j;
}


// Add i to the end of the list.
list& list::operator&=(item i) {
	assert(1 <= i && i <= N && next[i] == -1);
	if (first == Null) first = i;
	else next[last] = i;
	next[i] = Null; last = i;
	return (*this);
}

// Remove the first i items.
list& list::operator<<=(int i) {
	while (first != Null && i--) {
		item f = first; first = next[f]; next[f] = -1;
	}
	if (first == Null) last = Null;
	return (*this);
}

// Print the contents of the list.
ostream& operator<<(ostream& os, const list& L) {
	for (item i = L.first; i != Null; i = L.next[i]) {
		misc::putNode(os,i,L.N); os << ' ';
	}
	return os;
}
