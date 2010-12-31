#include "ssets.h"

#define left(x) node[x].left
#define right(x) node[x].right
#define p(x) node[x].p
#define kee(x) node[x].kee

// The partition is defined on {1,...,N}.
ssets::ssets(int N) : n(N) {
	node = new snode[n+1];
	for (item i = 0; i <= n; i++) {
		left(i) = right(i) = p(i) = kee(i) = Null;
	}
}

ssets::~ssets() { delete [] node; }

void ssets::rotate(item x) {
// Perform a rotation at the parent of x,
// moving x up to take its parent's place.
// Provided for benefit of derived classes.
	item y = p(x);
	if (y == Null) return;
	p(x) = p(y);
	     if (y == left(p(y)))  left(p(x)) = x;
	else if (y == right(p(y))) right(p(x)) = x;
	if (x == left(y)) {
		left(y) = right(x);
		if (left(y) != Null) p(left(y)) = y;
		right(x) = y;
	} else {
		right(y) = left(x);
		if (right(y) != Null) p(right(y)) = y;
		left(x) = y;
	}
	p(y) = x;
}

// Return the sset containing item i.
sset ssets::find(item i) const {
	assert (0 <= i && i <= n);
	while (p(i) != Null) i = p(i);
	return i;
}

// Return the item in s with key k, if there is one, else Null.
item ssets::access(keytyp k, sset s) const {
	assert (0 <= s && s <= n);
	while (s != Null && k != kee(s)) {
		if (k < kee(s)) s = left(s);
		else s = right(s);
	}
	return s;
}

// Insert item i into sset s; i is assumed to belong to a singleton
// sset before the operation. Return the resulting tree.
item ssets::insert(item i, sset s) {
	assert (1 <= i && i <= n && 1 <= s && s <= n);
	assert (left(0) == 0 && right(0) == 0 && p(0) == 0);
	sset x = s;
	while (1) {
		     if (kee(i) < kee(x) &&  left(x) != Null) x = left(x);
		else if (kee(i) > kee(x) && right(x) != Null) x = right(x);
		else break;
	}
	     if (kee(i) < kee(x))  left(x) = i;
	else if (kee(i) > kee(x)) right(x) = i;
	else fatal("ssets::insert: inserting item with duplicate key");
	p(i) = x;
	return s;
}

// Swap items i and j with each other, where it is assumed that
// item j is not the parent of item i
void ssets::swap(item i, item j) {
	assert(1 <= i && i <= n && 1 <= j && j <= n && j != p(i));

	// save values in items i and j
	item li,ri,pi,lj,rj,pj; keytyp ki,kj;
	li = left(i); ri = right(i); pi = p(i); ki = kee(i);
	lj = left(j); rj = right(j); pj = p(j); kj = kee(j);

	// fixup fields in i's neighbors
	if (li != Null) p(li) = j;
	if (ri != Null) p(ri) = j;
	if (pi != Null) {
		if (i == left(pi)) left(pi) = j;
		else right(pi) = j;
	}
	// fixup fields in j's neighbors
	if (lj != Null) p(lj) = i;
	if (rj != Null) p(rj) = i;
	if (pj != Null) {
		if (j == left(pj)) left(pj) = i;
		else right(pj) = i;
	}

	// update fields in items i and j
	left(i) = lj; right(i) = rj; p(i) = pj; kee(i) = kj;
	left(j) = li; right(j) = ri; p(j) = pi; kee(j) = ki;

	// final fixup for the case that i was originally the parent of j
	if (j == li) { left(j) = i; p(i) = j; 
	} else if (j == ri) { right(j) = i; p(i) = j; }
}

// Remove item i from sset s. This leaves i in a singleton sset.
// Return the item that now represents the set s (that is, the tree root).
item ssets::remove(item i, sset s) {
	assert(1 <= i && i <= n && 1 <= s && s <= n);
	item c = (left(s) != Null ? left(s) : right(s));
	remove(i);
	return (i != s ? s : (p(c) == Null ? c : p(c)));
}

// Private helper function used to remove item from its set.
// Return the item that was the parent of i just before i was deleted.
// If i had no parent, but had a non-null child, return that child.
// If i was the only node in its tree, return i.
item ssets::remove(item i) {
	assert (left(0) == 0 && right(0) == 0 && p(0) == 0);
	item j;
	if (left(i) != Null && right(i) != Null) {
		for (j = left(i); right(j) != Null; j = right(j)) {}
		swap(i,j);
	}
	// now, i has at most one child
	j = (left(i) != Null ? left(i) : right(i));
	// j is now the index of the only child that could be non-null
	if (j != Null) p(j) = p(i);
	if (p(i) != Null) {
		     if (i ==  left(p(i)))  left(p(i)) = j;
		else if (i == right(p(i))) right(p(i)) = j;
		j = p(i);
	}
	p(i) = left(i) = right(i) = Null;
	return j;
}

// Return the sset formed by joining s1, i and s2; i is assumed to be
// a singleton and all keys in s1 are assumed to be less than kee(i)
// while all keys in s2 are assumed to be greater than kee(i).
sset ssets::join(sset s1, item i, sset s2) {
	assert(0 <= s1 && s1 <= n && 1 <= i && i <= n && 0 <= s2 && s2 <= n);
	left(i) = s1; right(i) = s2;
	if (s1 != Null) p(s1) = i;
	if (s2 != Null) p(s2) = i;
	return i;
}

// Split s on i, yielding two ssets, s1 containing items with keys smaller
// than kee(i), s2 with keys larger than kee(i) and leaving i in a singleton.
// Return the pair (s1,s2)
spair ssets::split(item i, sset s) {
	assert(1 <= i && i <= n && 1 <= s && s <= n);
	sset y = i; spair pair;
	pair.s1 = left(i); pair.s2 = right(i);
	for (sset x = p(y); x != Null; x = p(y)) {
		     if (y ==  left(x)) pair.s2 = join(pair.s2,x,right(x));
		else if (y == right(x)) pair.s1 = join(left(x),x,pair.s1);
		y = x;
	}
	left(i) = right(i) = p(i) = Null;
	p(pair.s1) = p(pair.s2) = Null;
	return pair;
}

ostream& operator<<(ostream& os, const ssets& T) {
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
}

// Print the sset s in key order, using parentheses to show tree structure
void ssets::print(ostream& os, sset s) const {
	assert(0 <= s && s <= n);
	if (s == Null) os << "-";
	else if (left(s) == Null && right(s) == Null) {
		misc::putNode(os,s,n);
		if (p(s) == Null) os << "*";
	} else {
		os << "("; print(os,left(s));
		os << " "; misc::putNode(os,s,n);
		if (p(s) == Null) os << "*";
		os << " ";
		print(os,right(s)); os << ")";
	}
}
