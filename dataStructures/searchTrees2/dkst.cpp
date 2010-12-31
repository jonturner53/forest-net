#include "dkst.h"

#define left(x) node[x].left
#define right(x) node[x].right
#define p(x) node[x].p
#define kee1(x) node[x].kee
#define dmin(x) dmin[x]
#define dkey(x) dkey[x]

dkst::dkst(int N) : sass(N) { 
	dmin = new keytyp[N+1]; dkey = new keytyp[N+1];
	for (int i = 0; i <= N; i++) dmin(i) = dkey(i) = 0; 
} 

dkst::~dkst() { delete [] dmin; delete [] dkey; }

item dkst::access(keytyp k, sset s)  {
// Return the item in s with largest key1 value that is <=k.
// If there is no such item, return Null.
	assert (0 <= s && s <= n);
	// your code here
}

sset dkst::join(sset s1, item i, sset s2) {
// Join the sets s1 and s2 at i. S1 is assumed to have items with key1
// values less than i's and s2 is assumed to have larger key2 values
	// your code here
}

spair dkst::split(item i, sset s) {
// Split s at item i, yielding two ssets, s1 containing items with keys smaller
// than key(i), s2 with keys larger than key(i) and leaving i in a singleton.
// Return the pair [s1,s2]
	// your code here
}

keytyp dkst::key2(item i) {
	assert(1 <= i && i <= n);
	splay(i);
	return dmin(i) + dkey(i);
}

item dkst::first(sset s) const {
// Return first item in s, based on key1 values
// Note: does not do a splay.
	while (left(s) != Null) s = left(s);
	return s;
}

item dkst::next(item i) const {
// Return next item after i based on key1 values. Null if i is last item.
// Note: does not do a splay.
	if (right(i) != Null) {
		for (i = right(i); left(i) != Null; i = left(i)) {}
	} else {
		item c = i; i = p(i); 
		while (i != Null && right(i) == c) { c = i; i = p(i); }
	}
	return i;
}

void dkst::rotate(item x) {
// Perform a rotation at the parent of x,
// moving x up to take its parent's place.
	item y = p(x); if (y == Null) return;
	item a, b, c;
	if (x == left(y)) { a = left(x);  b = right(x); c = right(y); }
	else 		  { a = right(x); b = left(x);  c = left(y);  }
	ssets::rotate(x);

	dmin(a) += dmin(x); dmin(b) += dmin(x);

	dkey(x) = dkey(x) + dmin(x);
	keytyp dmx = dmin(x);
	dmin(x) = dmin(y);

	dmin(y) = dkey(y);
	if (b != Null) dmin(y) = min(dmin(y),dmin(b)+dmx);
	if (c != Null) dmin(y) = min(dmin(y),dmin(c));
	dkey(y) = dkey(y) - dmin(y);

	dmin(b) -= dmin(y); dmin(c) -= dmin(y);
}

item dkst::insert(item i, sset s) {
// Insert item i into sset s; i is assumed to belong to a singleton
// sset before the operation. Return the new set.
	assert (1 <= i && i <= n && 1 <= s && s <= n && i != s);
	assert (left(0) == 0 && right(0) == 0 && p(0) == 0);
	sset x = s; keytyp key2i = dmin(i);
	// save key2 value of i and correct dmin, dkey values
	// of i after splay brings it to the root
	while (true) {
		     if (kee1(i) < kee1(x) &&  left(x) != Null) x = left(x);
		else if (kee1(i) > kee1(x) && right(x) != Null) x = right(x);
		else break;
	}
	     if (kee1(i) < kee1(x))  left(x) = i;
	else if (kee1(i) > kee1(x)) right(x) = i;
	else fatal("dkst::insert: inserting item with duplicate key");
	p(i) = x;
	splay(i); // note: apparent key value of i is >= that of any node on path
		  // back to root; this ensures correct dmin, dkey values assigned
		  // to other nodes during rotations
	item l = left(i); item r = right(i);
	keytyp dmi = key2i;
	if (l != Null && dmin(l) + dmin(i) < dmi) dmi = dmin(l) + dmin(i);
	if (r != Null && dmin(r) + dmin(i) < dmi) dmi = dmin(r) + dmin(i);
	if (l != Null) dmin(l) += (dmin(i) - dmi);
	if (r != Null) dmin(r) += (dmin(i) - dmi);
	dmin(i) = dmi;
	dkey(i) = key2i - dmi;
	return i;
}

item dkst::remove(item i, sset s) {
// Remove item i from set s. Return the new set.
// Uses non-standard procedure to simplify updating of dmin, dkey values.
	assert(1 <= i && i <= n && 1 <= s && s <= n);
	assert (left(0) == 0 && right(0) == 0 && p(0) == 0);

	// search for i in the tree to determine its key2 value
	item x = s; keytyp key2i = dmin(x);
	while (x != i) {
		assert(x != Null);
		if (kee1(i) < kee1(x)) x = left(x);
		else x = right(x);
		key2i += dmin(x);
	}
	key2i += dkey(i);

	item j;
	if (left(i) == Null) 	   j = right(i);
	else if (right(i) == Null) j = left(i);
	else {
		for (j = left(i); right(j) != Null; j = right(j)) {}
		p(left(i)) = Null; splay(j);
		right(j) = right(i); p(right(j)) = j;
	}
	// j is now child of i that will take i's position in tree
	if (j != Null) { dmin(j) += dmin(i); p(j) = p(i); }
	if (p(i) != Null) {
		     if (i ==  left(p(i)))  left(p(i)) = j;
		else if (i == right(p(i))) right(p(i)) = j;
	}
	p(i) = left(i) = right(i) = Null;
	return splay(j);
}

ostream& operator<<(ostream& os, const dkst& T) {
// Print the contents of all the ssets in the collection of dual key search trees
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
	os << " key1:";
	for (sset i = 1; i <= T.n; i++) os << " " << setw(2) << T.kee1(i);
	os << endl;
	os << " key2:";
	for (sset i = 1; i <= T.n; i++) {
		keytyp minkey2i = 0; item j = i;
		do { minkey2i += T.dmin(j); j = T.p(j); } while (j != Null);
		os << " " << setw(2) << minkey2i + T.dkey(i);
	}
	os << endl;
	os << " dmin:";
	for (sset i = 1; i <= T.n; i++) os << " " << setw(2) << T.dmin(i);
	os << endl;
	os << " dkey:";
	for (sset i = 1; i <= T.n; i++) os << " " << setw(2) << T.dkey(i);
	os << endl;
}
