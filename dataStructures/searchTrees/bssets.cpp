#include "bssets.h"

#define left(x) node[x].left
#define right(x) node[x].right
#define p(x) node[x].p
#define kee(x) node[x].kee
#define rank(x) rvec[x]

// The partition is defined on {1,...,N}.
bssets::bssets(int N) : ssets(N) {
	rvec = new int[n+1];
	rank(0) = 0;
	for (item i = 1; i <= n; i++) rank(i) = 1;
}

bssets::~bssets() { delete [] rvec; }

void bssets::swap(item i, item j) {
// Swap items i and j with each other, where it is assumed that
// item j is not the parent of item i
	ssets::swap(i,j);
	int r = rvec[i]; rvec[i] = rvec[j]; rvec[j] = r;
}

// Insert item i into sset s; i is assumed to belong to a singleton
// sset before the operation. Return the resulting set.
sset bssets::insert(item i, sset s) {
	assert(rank(0) == 0);
	ssets::insert(i,s);
	// rank(i) = 1;
	// go up the tree correcting rank violations using promotion
	item x = i; item gpx = p(p(x));
	while (gpx != Null && rank(x) == rank(gpx) &&
	       rank(left(gpx)) == rank(right(gpx))) {
		rank(gpx)++; x = gpx; gpx = p(p(x));
	}
	if (gpx == Null || rank(x) != rank(gpx)) return s;
	// finish off with a rotation or two
	if (x == left(left(gpx)) || x == right(right(gpx))) rotate(p(x));
	else { rotate(x); rotate(x); }
	return (p(s) == Null ? s : p(s));
}

// Remove item i from sset s. This leaves i in a singleton sset.
// Return the item that now represents the set (that is, the root).
sset bssets::remove(item i, sset s) {
	assert(rank(0) == 0);
	item r, x, px, y, z;
	r = (s != i ? s : (right(s) != Null ? right(s) : left(s)));
	px = ssets::remove(i); rank(i) = 1;
	// px is now i's former parent just before i was removed
	// or the child of i, if i had no parent
	if (px == Null) return Null;  // only arises if i is only node in s
	     if (rank(left(px))  < rank(px)-1) x = left(px);
	else if (rank(right(px)) < rank(px)-1) x = right(px);
	else return find(r);
	// if we reach here x is a child of px and rank(x) < rank(px)-1
	// note: x may be Null
	// now move up the tree checking for and fixing
	// rank violations between x and its parent
	y = sibling(x,px);
	// note: rank(x) >= 0, so rank(px) >= 2 and rank(y) >= 1
	while (px != Null && rank(x) < rank(px)-1 && rank(y) < rank(px) &&
		rank(left(y)) < rank(y) && rank(right(y)) < rank(y)) {
		rank(px)--; // creates no violations with y or y's children
		x = px; px = p(x); y = sibling(x,px);
	}
	if (px == Null) return x;
	// note: x can still be null
	if (rank(x) >= rank(px)-1)
		return find(r);
	// now, do a few rotations to finish up
	if (rank(y) == rank(px)) {
		rotate(y); y = sibling(x,px);
		if (left(y) == Null && right(y) == Null) {
			rank(px)--; return find(r);
		}
	}
	z = (x == right(px) ? left(y) : right(y)); // z is furthest nephew of x
	if (rank(z) == rank(y)) {
		rotate(y);
		rank(y) = rank(px); rank(px)--;
	} else {
		z = sibling(z,y);
		// now z is closest nephew of x
		rotate(z); rotate(z);
		rank(z) = rank(px); rank(px)--;
	}
	return find(r);
}

// Return the sset formed by joining s1, i and s2; i is assumed to be
// a singleton and all keys in s1 are assumed to be less than kee(i)
// while all keys in s2 are assumed to be greater than kee(i).
sset bssets::join(sset s1, item i, sset s2) {
	fatal("bssets::join not implemented");
}

// Split s on i, yielding two bssets, s1 containing items with keys smaller
// than kee(i), s2 with keys larger than kee(i) and leaving i in a singleton.
// Return the pair (s1,s2)
spair bssets::split(item i, sset s) {
	fatal("split::join not implemented");
}

ostream& operator<<(ostream& os, const bssets& T) {
// Print the contents of all the ssets in the collection
	// print all non-singletons, showing the tree structure
	for (sset i = 1; i <= T.n; i++) {
		if (T.p(i) == Null && (T.left(i) != Null || T.right(i) != Null)) {
			T.print(os,i); os << endl;
		}
	}
	os << "      ";
	for (sset i = 1; i <= T.n; i++) {
		if (T.n <= 26) {
			os << "  "; misc::putNode(os,i,T.n);
		} else {	       
			os << " " << setw(2) << i;
		}
	}
	os << endl;
	os << " keys:";
	for (sset i = 1; i <= T.n; i++) os << " " << setw(2) << T.key(i);
	os << endl;
	os << "ranks:";
	for (sset i = 1; i <= T.n; i++) os << " " << setw(2) << T.rvec[i];
	os << endl;
}
