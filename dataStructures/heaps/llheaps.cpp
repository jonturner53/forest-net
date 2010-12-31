#include "llheaps.h"

#define kee(x) node[x].kee
#define rank(x) node[x].rank
#define left(x) node[x].left
#define right(x) node[x].right

#define deleted(x)((x > n) || (delf != Null && (*delf)(x)))

llheaps::llheaps(int N, delftyp f) : lheaps(2*N) {
// N is the maximum number of elements in the collection.
// f is a pointer to a "deleted" function that takes a single
// item as its argument and returns true if that item
// should be considered deleted from any heap in which
// it might appear.
//
// The form of an llheaps decarlartion is:
//
//	llheaps foo(100,delete_function)
//
// where delete_function is some function that looks like
//
//	bool delete_function(item i) {
//		if (item i should be considered deleted)
//			return true;
//		else
//			return false
//	}
	int i;
	n = N; // note lheaps constructor provides space for 2N nodes
	delf = f;
	// build list of dummy nodes linked using left pointers
	for (i = n+1; i <= 2*n ; i++) left(i) = i+1;
	dummy = n+1; left(2*n) = Null;
	rank(Null) = 0; left(Null) = right(Null) = Null;
	tmpL = new list(n);
}

llheaps::~llheaps() { delete tmpL; }

// Return the heap formed by combining h1 and h2 in a lazy way.
lheap llheaps::lmeld(lheap h1, lheap h2) {
	assert(0 <= h1 && h1 <= 2*n && 0 <= h2 && h2 <= 2*n && dummy != Null);
	int i = dummy; dummy = left(dummy);
	left(i) = h1; right(i) = h2;
	return i;
}

// Insert i in s and return the resulting heap.
// Item i is assumed(!) to belong to single item heap.
lheap llheaps::insert(item i, lheap h) {
	assert(0 <= i && i <= n && 0 <= h && h <= 2*n);
	assert(left(i) == Null && right(i) == Null && rank(i) ==1);
	tmpL->clear(); purge(h,*tmpL); h = heapify(*tmpL);
	return meld(i,h);
}

// Find and return the item with smallest key in h.
item llheaps::findmin(lheap h) {
	assert(0 <= h && h <= 2*n);
	tmpL->clear(); purge(h,*tmpL); return heapify(*tmpL);
}

// Create a heap obtained by melding the heaps in L.
// The heaps on L are assumed(!) to contain no Deleted nodes.
lheap llheaps::heapify(list& L) {
	if (L[1] == Null) return Null;
	while (L[2] != Null) {
		lheap h = meld(L[1],L[2]);
		L <<= 2; L &= h;
	}
	return L[1];
}

// Add to L, the roots of subtrees of h that are not Deleted.
// Recover Deleted nodes in the process.
void llheaps::purge(lheap h, list& L) {
	if (h == Null) return;
	if (!deleted(h)) {
		L &= h;
	} else {
		purge(left(h),L); purge(right(h),L);
		if (h > n) {
			left(h) = dummy; dummy = h; right(h) = Null;
		} else {
			left(h) = right(h) = Null; rank(h) = 1;
		}
	}
}

// Construct heap from items on list. Items are assumed(!) to be
// in single item heaps.
lheap llheaps::makeheap(list& L1) {
	tmpL->clear(); *tmpL = L1; return heapify(*tmpL);
}


// Print all the heaps.
ostream& operator<<(ostream& os, const llheaps& L) {
	int i; bool *mark = new bool[2*L.n+1];
	for (i = 1; i <= 2*L.n; i++) mark[i] = true;
	for (i = 1; i <= 2*L.n; i++)
		mark[L.left(i)] = mark[L.right(i)] = false;
	mark[L.dummy] = false;
	for (i = 1; i <= 2*L.n; i++)
		if (mark[i]) { L.sprint(os,i); os << endl; }
	delete [] mark;
}

// Print the heap h.
void llheaps::sprint(ostream& os, lheap h) const {
	if (h == Null) return;
	os << "("; misc::putNode(os,h,n); os << "," << kee(h) << ")";
	if (deleted(h)) os << "* "; else os << " ";
	sprint(os,left(h)); sprint(os,right(h));
}

// Print the heap h as a tree.
void llheaps::tprint(ostream& os, lheap h, int i) const {
	const int PRINTDEPTH = 20;
	static char tabstring[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	i = max(0,i); i = min(PRINTDEPTH,i);
	if (h == Null) return;
	tprint(os,right(h),i+1);
	os << tabstring+PRINTDEPTH-i; misc::putNode(os,h,n); 
	os << " " << kee(h);
	if (deleted(h)) os << " **\n"; else os << " " << rank(h) << endl;
	tprint(os,left(h),i+1);
}
