#include "mheap.h"

#define p(x) (((x)+(d-2))/d)
#define left(x) (d*((x)-1)+2)
#define right(x) (d*(x)+1)

// above(a,b) is true if key value a belongs above b in the heap
#define above(a,b) ((a) != (b) && (minFlag ? \
			((b)-(a)) < (1 << 31) : ((a)-(b)) < (1 << 31) ) )

mheap::mheap(int N1, int d1, bool minFlag1) : N(N1), d(d1), minFlag(minFlag1) {
// Initialize a heap to store items in {1,...,N}.
	n = 0;
	h = new item[N+1]; pos = new int[N+1]; kee = new keytyp[N+1];
	for (int i = 1; i <= N; i++) pos[i] = Null;
	h[Null] = pos[Null] = Null; kee[Null] = 0;
}

mheap::~mheap() { delete [] h; delete [] pos; delete [] kee; }

void mheap::insert(item i, keytyp k) {
// Add i to heap.
	kee[i] = k; n++; siftup(i,n);
}

void mheap::remove(item i) {
// Remove item i from heap. Name remove is used since delete is C++ keyword.
	int j = h[n--];
	if (i != j) {
		if (above(kee[j],kee[i]) || kee[j] == kee[i]) siftup(j,pos[i]);
		else siftdown(j,pos[i]);
	}
	pos[i] = Null;
}

void mheap::siftup(item i, int x) {
// Shift i up from position x to restore heap order.
	int px = p(x);
	while (x > 1 && above(kee[i],kee[h[px]])) {
		h[x] = h[px]; pos[h[x]] = x;
		x = px; px = p(x);
	}
	h[x] = i; pos[i] = x;
}

void mheap::siftdown(item i, int x) {
// Shift i down from position x to restore heap order.
	int cx = topchild(x);
	while (cx != Null && above(kee[h[cx]],kee[i])) {
		h[x] = h[cx]; pos[h[x]] = x;
		x = cx; cx = topchild(x);
	}
	h[x] = i; pos[i] = x;
}

// Return the position of the child of the item at position x
// having the top-most key.
int mheap::topchild(int x) {
	int y, topc;
	if ((topc = left(x)) > n) return Null;
	for (y = topc + 1; y <= right(x) && y <= n; y++) {
		if (above(kee[h[y]],kee[h[topc]])) topc = y;
	}
	return topc;
}

void mheap::changekey(item i, keytyp k) {
// Change the key of i and restore heap order.
	keytyp ki = kee[i]; kee[i] = k;
	if (k == ki) return;
	if (above(k,ki)) siftup(i,pos[i]);
	else siftdown(i,pos[i]);
}

// Print the contents of the heap.
ostream& operator<<(ostream& os, const mheap& H) {
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
