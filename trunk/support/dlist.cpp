#include "dlist.h"

// Allocate and initialize dynamic storage space for this object,
void dlist::makeSpace() {
        prev = new item[N+1];
        for (item i = 1; i <= N; i++) prev[i] = -1;
        prev[Null] = Null;
}
void dlist::mSpace() { list::mSpace(); makeSpace(); }

// Construct list with space for N1 items
dlist::dlist(int N1) : list(N1) {
	assert(N >= 0); makeSpace();
}

// Free space for object.
void dlist::freeSpace() { delete [] prev; }
void dlist::fSpace() { freeSpace(); list::fSpace(); }
dlist::~dlist() { freeSpace(); }

// Copy into list from L.
void dlist::copyFrom(const dlist& L) {
        assert(N >= L.N);
        item i;
        for (i = 1; i <= L.N; i++) prev[i] = L.prev[i];
        for (i = L.N+1; i <= N; i++) prev[i] = -1;;
}
void dlist::cFrom(const dlist& L) { list::cFrom(L); copyFrom(L); }

// Construct copy of list L
dlist::dlist(const dlist& L) : list(L) {
	assert(N >= 0); makeSpace(); copyFrom(L);
}

// Assign value of right operand to left operand.
dlist& dlist::operator=(const dlist& L) {
        if (&L == this) return *this;
        if (N < L.N) { fSpace(); N = L.N; mSpace(); }
        cFrom(L);
        return (*this);
}

// Remove all elements from list.
void dlist::clear() {
	while (first != Null) {
		int i = first; first = next[i]; prev[first] = Null;
		next[i] = prev[i] = -1;
		
	}
}

// Print the raw data structure, including non-list elements.
void dlist::dump() const {
	int i;
	if (N <= 26)
		cout << "first=" << char(first+('a'-1))
		     << " last=" << char(last+('a'-1)) << endl;
	else
		cout << "first=" << first << " last=" << last << endl;
	cout << "next: ";
	for (i = 1; i <= N; i++) {
		if (N <= 26 && next[i] == -1)
			cout << ", ";
		else if (N <= 26 && next[i] == Null)
			cout << "- ";
		else if (N <= 26)
			cout << char(next[i]+('a'-1)) << ' ';
		else {
			cout.width(2); cout << next[i]+('a'-1) << ' ';
		}
	}
	cout << "\nprev: ";
	for (i = 1; i <= N; i++) {
		if (N <= 26 && prev[i] == -1)
			cout << ", ";
		else if (N <= 26 && prev[i] == Null)
			cout << "- ";
		else if (N <= 26)
			cout << char(prev[i]+('a'-1)) << ' ';
		else {
			cout.width(2); cout << prev[i]+('a'-1) << ' ';
		}
	}
	printf("\n");
}


// Add i to the front of the list.
void dlist::push(item i) {
	assert(i == 0 || (1 <= i && i <= N && next[i] == -1));
	if (i == Null) return;
	if (first == Null) last = i;
	next[i] = first; prev[i] = Null;
	prev[first] = i;
	first = i;
}

// Insert item i after item j.
void dlist::insert(item i, item j) {
	assert(	0 <= i && i <= N && next[i] == -1 &&
		0 <= j && j <= N && 0 <= next[j] && next[j] <= N
		);
	if (i == Null) return;
	if (j == Null) push(i);
	next[i] = next[j]; prev[i] = j;
	next[j] = i;
	if (last == j) last = i;
}

// Return the i-th element, where the first is 1.
// Negative numbers indicate selection from end of list.
item dlist::operator[](int i) {
	item j;
	     if (i == 0) return Null;
	else if (i > 0) {
		for (j = first; j != Null && --i; j = next[j]) {}
	} else if (i < 0) {
		for (j = last; j != Null && ++i; j = prev[j]) {}
	}
	return j;
}

// Add i to the end of the list.
dlist& dlist::operator&=(item i) {
if (i < 1 || i > N || next[i] != -1)
cerr << "i=" << i << " next[i]=" << next[i] << endl;
	assert(1 <= i && i <= N && next[i] == -1);
	if (i < 1 || i > N) fatal("dlist::operator&=: item out of range");
	if (next[i] != -1) fatal("dlist::operator&=: item already in list");
	if (first == Null) {
		first = i; prev[i] = Null;
	} else {
		next[last] = i; prev[i] = last;
	}
	next[i] = Null; last = i;
	return (*this);
}

// Remove i from list (note, i need not be i-th item in list).
dlist& dlist::operator-=(item i) {
	assert(1 <= i && i <= N);
	if (next[i] != -1) {
		next[prev[i]] = next[i];
		prev[next[i]] = prev[i];
		if (first == i) first = next[i];
		if (last == i) last = prev[i];
		next[i] = prev[i] = -1;
		next[Null] = prev[Null] = Null;
	}
	return (*this);
}

// Remove the first i items.
dlist& dlist::operator<<=(int i) {
	while (first != Null && i--) {
		item f = first; first = next[f]; next[f] = prev[f] = -1;
	}
	prev[first] = Null;
	if (first == Null) last = Null;
	return (*this);
}
