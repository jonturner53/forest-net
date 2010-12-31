#include "lheaps.h"

#define kee(x) node[x].kee
#define rank(x) node[x].rank
#define left(x) node[x].left
#define right(x) node[x].right

lheaps::lheaps(int N) : n(N) {
	node = new hnode[n+1];
	for (int i = 1; i <= n ; i++) {
		left(i) = right(i) = Null;
		rank(i) = 1; kee(i) = 0;
	}
	rank(Null) = 0;
	left(Null) = right(Null) = Null;
}

lheaps::~lheaps() { delete [] node; }

// Return the heap formed by combining h1 and h2.
lheap lheaps::meld(lheap h1, lheap h2) {
	assert(0 <= h1 && h1 <= n && 0 <= h2 && h2 <= n);
	     if (h1 == Null) return h2;
	else if (h2 == Null) return h1;
	if (kee(h1) > kee(h2)) {
		lheap t = h1; h1 = h2; h2 = t;
	}
	right(h1) = meld(right(h1),h2);
	if (rank(left(h1)) < rank(right(h1))) {
		lheap t = left(h1); left(h1) = right(h1); right(h1) = t;
	}
	rank(h1) = rank(right(h1)) + 1;
	return h1;
}

// Insert i in h and return the resulting heap.
lheap lheaps::insert(item i, lheap h) {
	assert(0 <= i && i <= n && 0 <= h && h <= n);
	assert(left(i) == Null && right(i) == Null && rank(i) ==1);
	return meld(i,h);
}

// Delete and return the item in h with the smallest key.
item lheaps::deletemin(lheap h) {
	(void) meld(left(h),right(h));
	left(h) = right(h) = Null; rank(h) = 1;
	return h;
}

// Print all the heaps.
ostream& operator<<(ostream& os, const lheaps& F) {
	int i; bool *mark = new bool[F.n+1];
	for (i = 1; i <= F.n; i++) mark[i] = true;
	for (i = 1; i <= F.n; i++)
		mark[F.left(i)] = mark[F.right(i)] = false;
	for (i = 1; i <= F.n; i++)
		if (mark[i]) { F.sprint(os,i); os << endl; }
	delete [] mark;
	return os;
}

// Print the heap h.
void lheaps::sprint(ostream& os, lheap h) const {
	if (h == Null) return;
	os << "(";
	if (n <= 26) os << misc::nam(h);
	os << "," << setw(2) << kee(h) << ") ";
	sprint(os,left(h)); sprint(os,right(h));
}

// Print the heap h as a tree.
void lheaps::tprint(ostream& os, lheap h, int i) const {
	const int PRINTDEPTH = 20;
	static char tabstring[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	i = max(0,i); i = min(PRINTDEPTH,i);
	if (h == Null) return;
	tprint(os,right(h),i+1);
	os << (tabstring+PRINTDEPTH-i) << "(";
	misc::putNode(os,h,n);
	os << " " << kee(h) << " " << rank(h) << ")\n";
	tprint(os,left(h),i+1);
}
