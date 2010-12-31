#include "fheaps.h"

#define left(x) sibs->pred(x)
#define right(x) sibs->suc(x)
#define kee(x) node[x].kee
#define rank(x) node[x].rank
#define mark(x) node[x].rank
#define p(x) node[x].p
#define c(x) node[x].c

fheaps::fheaps(int N) : n(N) {
	node = new fnode[n+1]; sibs = new clist(n); tmpq = new list(n);
	for (int i = 1; i <= n ; i++) {
		c(i) = p(i) = Null;
		rank(i) = 0; kee(i) = 0;
		mark(i) = false;
	}
	for (int i = 0; i <= MAXRANK; i++) rvec[i] = Null;
	kee(Null) = rank(Null) = mark(Null) = 0;
	p(Null) = c(Null) = Null;
}

fheaps::~fheaps() {
	delete [] node; delete tmpq; delete sibs;
}

// Return the heap formed by combining h1 and h2.
fheap fheaps::meld(fheap h1, fheap h2) {
	assert(0 <= h1 && h1 <= n && 0 <= h2 && h2 <= n);
	if (h1 == Null) return h2;
	if (h2 == Null) return h1;
	sibs->join(h1,h2);
	return kee(h1) <= kee(h2) ? h1 : h2;
}

// Insert i into h with given key value and return resulting heap.
fheap fheaps::insert(item i, fheap h, keytyp x) {
	assert(0 <= i && i <= n && 0 <= h && h <= n);
	assert(left(i) == i && right(i) == i && c(i) == Null && p(i) == Null);
	kee(i) = x;
	return meld(i,h);
}

// Decrease the key of item i by delta. Return new heap.
fheap fheaps::decreasekey(item i, keytyp delta, fheap h) {
	assert(0 <= i && i <= n && 0 <= h && h <= n && delta >= 0);
	fheap pi = p(i);
	kee(i) -= delta;
	if (pi == Null) return kee(i) < kee(h) ? i : h;
	c(pi) = rank(pi) == 1 ? Null: left(i);
	rank(pi)--;
	sibs->remove(i);
	p(i) = Null;
	h = meld(i,h);
	if (p(pi) == Null) return h;
	if (!mark(pi))	mark(pi) = true;
	else		h = decreasekey(pi,0,h);
	return h;
}

// Delete the item in h with the smallest key.
// Return the new heap.
fheap fheaps::deletemin(fheap h) {
	assert(1 <= h && h <= n);
	item i,j; int k,mr;

	// Merge h's children into root list and remove it
	sibs->join(h,c(h)); c(h) = Null; rank(h) = 0;
	if (left(h) == h) return Null;
	i = left(h);
	sibs->remove(h);
	
	// Build queue of roots and find root with smallest key
	h = i; *tmpq &= i; p(i) = Null;
	for (j = right(i); j != i; j = right(j)) {
		if (kee(j) < kee(h)) h = j;
		*tmpq &= j; p(j) = Null;
	}
	mr = -1;
	while ((*tmpq)[1] != Null) {
		i = (*tmpq)[1]; *tmpq <<= 1;
		if (rank(i) > MAXRANK) fatal("deletemin: rank too large");
		j = rvec[rank(i)];
		if (mr < rank(i)) {
			for (mr++; mr < rank(i); mr++) rvec[mr] = Null;
			rvec[rank(i)] = i;
		} else if (j == Null) {
			rvec[rank(i)] = i;
		} else if (kee(i) < kee(j)) {
			sibs->remove(j);
			sibs->join(c(i),j); c(i) = j;
			rvec[rank(i)] = Null;
			rank(i)++;
			p(j) = i; mark(j) = false;
			*tmpq &= i;
		} else {
			sibs->remove(i);
			sibs->join(c(j),i); c(j) = i;
			rvec[rank(i)] = Null;
			rank(j)++;
			p(i) = j; mark(i) = false;
			if (h == i) h = j;
			*tmpq &= j;
		}
	}
	for (k = 0; k <= mr; k++) rvec[k] = Null;
	return h;
}

// Delete i from h and return the modified heap.
fheap fheaps::remove(item i, fheap h) {
	assert(1 <= i && i <= n && 1 <= h && h <= n);
	keytyp k;
	k = kee(i);
	h = decreasekey(i,(kee(i)-kee(h))+1,h);
	h = deletemin(h);
	kee(i) = k;
	return h;
}

// Print all the heaps.
ostream& operator<<(ostream& os, const fheaps& F) {
	item i, j;
	bool *pmark = new bool[F.n+1];
	for (i = 1; i <= F.n; i++) pmark[i] = false;
	for (i = 1; i <= F.n; i++) {
		if (F.p(i) == Null && !pmark[i]) {
			F.print(os,i); os << endl;
			pmark[i] = true;
			for (j = F.sibs->suc(i); j != i; j = F.sibs->suc(j))
				pmark[j] = true;
		}
	}
	delete [] pmark;
	return os;
}

// Print the heap h.
void fheaps::print(ostream& os, fheap h) const {
	if (h == Null) return;
	os << "("; misc::putNode(os,h,n); os << "," << kee(h) << ") ";
	print(os,c(h));
	for (item i = right(h); i != h; i = right(i)) {
		os << "("; misc::putNode(os,i,n); os << "," << kee(i) << ")";
		print(os,c(i));
	}
}

// Print the heap h as a tree.
void fheaps::tprint(ostream& os, fheap h, int i) const {
	const int PRINTDEPTH = 20;
	static char tabstring[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	i = max(0,i); i = min(PRINTDEPTH,i);
	if (h == Null) return;
	os << &tabstring[PRINTDEPTH-i];
	if (n <= 26) os << misc::nam(h); else os << h;
	os << " " << kee(h) << " " << rank(h) << (mark(h) ? '*' : ' ') << endl;
	tprint(os,c(h),i+1);
	for (item j = right(h); j != h; j = right(j)) {
		os << &tabstring[PRINTDEPTH-i];
		misc::putNode(os,j,n);
		os << " " << kee(j) << " " << rank(j)
		   << (mark(j) ? '*' : ' ') << endl;
		tprint(os,c(j),i+1);
	}
}
