#include "lists.h"

// Allocate and initialize space for lists.
lists::lists(int N1) : N(N1) {
	int i;
	if (N >= (1 << ISHFT)) fatal("lists::lists: specified size too large");
	node = new lnode[N+1];
	for (i = 1; i < N; i++) { node[i] = i+1; }
	node[0] = Null; node[N] = Null;
	free = 1;
}

lists::~lists() { delete [] node; }

// Add an item to the front to of the list that starts with j.
// Return the first item on the new list or Null if out of space.
// If j is Null, insert effectively creates a new list with one item.
alist lists::insert(lvalu v, alist j) {
	if (free == Null) return Null;
	item i = free;
	free = suc(free);
	node[i] = (v << ISHFT) | j;
	return i;
}

// Remove first item on list and return its successor.
// The list is assumed to be non-empty (j != Null).
int lists::remove(alist j) {
	int i = suc(j);
	node[j] = free; free  = j;
	return i;
}

// Remove first item on list with value field matching v.
// Return first item on the modified list.
// The list is assumed to be non-empty (j != Null).
int lists::remove(lvalu v, alist j) {
	alist f, p;
	if ((node[j] >> ISHFT) == v) return remove(j);
	f = p = j; j = suc(j);
	while (j != Null && value(j) != v) {
		p = j; j = suc(j);
	}
	if (j == Null) return f;
	node[p] = (node[p] & IMSK) | remove(j);
	return f;
}

bool lists::mbr(lvalu v,alist j) {
// Return true if list contains item with given value.
	for (; j != Null; j = suc(j))
		if (value(j) == v) return true;
	return false;
}

int lists::clear(alist j) {
// Remove all elements on the list that starts with j.
	while (j != Null) j = remove(j);
}

void lists::print(ostream& os, alist j) const {
// Print the values in the list starting with node j.
	if (j == Null) return;
	while (j != Null) {
		os << value(j);
		j = suc(j);
		if (j != Null) os << ",";
	}
}

ostream& operator<<(ostream& os, const lists& L) {
	int i, j;
	bool *mark = new bool[L.N+1];
	for (i = 1; i <= L.N; i++) mark[i] = false;
	for (i = 1; i <= L.N; i++) {
		j = L.suc(i);
		if (j <= L.N) mark[j] = true;
	}
	mark[L.free] = true;
	// only unmarked nodes are list heads
	for (i = 1; i <= L.N; i++) {
		if (!mark[i]) {
			os << setw(2) << j << ": ";
			L.print(os,i);
			os << endl;
		}
	}
	delete [] mark;
	return os;
}
