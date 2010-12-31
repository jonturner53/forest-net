#include "dheap.h"

#define p(x) (((x)+(d-2))/d)
#define left(x) (d*((x)-1)+2)
#define right(x) (d*(x)+1)

dheap::dheap(int N1, int d1) {
// Initialize a heap to store items in {1,...,N}.
	static int x = 0; x++;
	N = N1; d = d1; n = 0;
	h = new item[N+1];
	pos = new int[N+1];
	kee = new keytyp[N+1];
	for (int i = 1; i <= N; i++) pos[i] = Null;
	h[Null] = pos[Null] = Null; kee[Null] = 0;
}

dheap::~dheap() { delete [] h; delete [] pos; delete [] kee; }

void dheap::insert(item i, keytyp k) {
// Add i to heap.
	kee[i] = k; n++; siftup(i,n);
}

void dheap::remove(item i) {
// Remove item i from heap. Name remove is used since delete is C++ keyword.
	int j = h[n--];
	     if (i != j && kee[j] <= kee[i]) siftup(j,pos[i]);
	else if (i != j && kee[j] >  kee[i]) siftdown(j,pos[i]);
	pos[i] = Null;
}

int dheap::deletemin() {
// Remove and return item with smallest key.
	if (n == 0) return Null;
	item i = h[1];
	siftdown(h[n--],1);
	pos[i] = Null;
	return i;
}

void dheap::siftup(item i, int x) {
// Shift i up from position x to restore heap order.
	int px = p(x);
	while (x > 1 && kee[h[px]] > kee[i]) {
		h[x] = h[px]; pos[h[x]] = x;
		x = px; px = p(x);
	}
	h[x] = i; pos[i] = x;
}

void dheap::siftdown(item i, int x) {
// Shift i down from position x to restore heap order.
	int cx = minchild(x);
	while (cx != Null && kee[h[cx]] < kee[i]) {
		h[x] = h[cx]; pos[h[x]] = x;
		x = cx; cx = minchild(x);
	}
	h[x] = i; pos[i] = x;
}

// Return the position of the child of the item at position x
// having minimum key.
int dheap::minchild(int x) {
	int y, minc;
	if ((minc = left(x)) > n) return Null;
	for (y = minc + 1; y <= right(x) && y <= n; y++) {
		if (kee[h[y]] < kee[h[minc]]) minc = y;
	}
	return minc;
}

void dheap::changekey(item i, keytyp k) {
// Change the key of i and restore heap order.
	keytyp ki = kee[i]; kee[i] = k;
	     if (k < ki) siftup(i,pos[i]);
	else if (k > ki) siftdown(i,pos[i]);
}

// Print the contents of the heap.
ostream& operator<<(ostream& os, const dheap& H) {
	int x;
	os << "  h:";
	for (x = 1; x <= H.n; x++) {
		os << "  "; misc::putNode(os,H.h[x],H.N);
	}
	os << "\nkey:";
	for (x = 1; x <= H.n; x++) os << " " << setw(2) << H.kee[H.h[x]];
	os << endl;
	return os;
}
