#include "stdinc.h"
#include "cisets.h"

#define left(x) vec[x].lchild
#define right(x) vec[x].rchild
#define p(x) vec[x].parent
#define lo(x) vec[x].loval
#define hi(x) vec[x].hival

// The partition is defined on {1,...,N}.
cisets::cisets(int N) {
	n = N; vec = new spnode[n+1];
	for (item i = 1; i <= n; i++) {
		left(i) = i+1;
		right(i) = p(i) = Null;
	}
	free = 1; left(n) = Null;
	left(0) = right(0) = p(0) = Null;
	lo(0) = hi(0) = 0;
}

cisets::~cisets() { delete [] vec; }

// Splay at item i. Return root of resulting tree.
item cisets::splay(item x) {
	while (p(x) != Null) splaystep(x);
	return x;
}

// Perform a single splay step at x.
void cisets::splaystep(item x) {
	item y = p(x);
	if (y == Null) return;
	item z = p(y);
	     if (z == Null && x == left(y))  { rrotate(y); }
	else if (z == Null && x == right(y)) { lrotate(y); }
	else if (x == left(left(z))) 	    { rrotate(z); rrotate(y); }
	else if (x == right(right(z))) 	    { lrotate(z); lrotate(y); }
	else if (x == left(right(z))) 	    { rrotate(y); lrotate(z); }
	else if (x == right(left(z))) 	    { lrotate(y); rrotate(z); }
}

// Left rotation.
void cisets::lrotate(item y) {
	item x = right(y);
	if (x == Null) return;
	p(x) = p(y);
	     if (y == left(p(y)))  left(p(x)) = x;
	else if (y == right(p(y))) right(p(x)) = x;
	right(y) = left(x); p(right(y)) = y;
	left(x) = y; p(y) = x;
}

// Right rotation.
void cisets::rrotate(item y) {
	item x = left(y);
	if (x == Null) return;
	p(x) = p(y);
	     if (y == left(p(y)))  left(p(x)) = x;
	else if (y == right(p(y))) right(p(x)) = x;
	left(y) = right(x); p(left(y)) = y;
	right(x) = y; p(y) = x;
}

// Splay at node containing i or one of its neighbors if i not in set.
iset cisets::find(int i, iset s) {
	if (s == Null) return Null;
	while (1) {
		     if (i < lo(s) &&  left(s) != Null) s = left(s);
		else if (i > hi(s) && right(s) != Null) s = right(s);
		else break;
	}
	return splay(s);
}

// Splay at node containing smallest integer in set.
iset cisets::min( iset s) {
	if (s == Null) return Null;
	while (left(s) != Null) s = left(s);
	return splay(s);
}

// Splay at node containing largest integer in set.
iset cisets::max(iset s) {
	if (s == Null) return Null;
	while (right(s) != Null) s = right(s);
	return splay(s);
}

// Recover the nodes in s.
void cisets::recover(iset s) {
	if (s != Null) {
		recover(left(s)); recover(right(s));
		left(s) = free; free = s;
	}
}

// Return the largest interval containing i or an empty inteval.
interval cisets::search(int i, iset s) {
	interval p;
	s = find(i,s);
	if (s != Null && lo(s) <= i && i <= hi(s)) {
		p.l = lo(s); p.h = hi(s);
	} else {
		p.l = 0; p.h = -1;
	}
	return p;
}

// Insert interval [i,j] into set s.
iset cisets::insert(int i, int j, iset s) {
	s = remove(i,j,s);
	ispair sp = split(i,s);
	return join(sp.s1,i,j,sp.s2);
}

// Remove interval [i,j] from set s.
iset cisets::remove(int i, int j, iset s) {
	ispair sp = split(i,s);
	s = max(sp.s1);
	sp = split(j,sp.s2);
	right(s) = sp.s2; p(sp.s2) = s;
	recover(sp.s1);
	return s;
}

// Return the set formed by joining s1, the interval [i,j] and s2.
// It is assumed that i > max(s1) and j < min(s2).
iset cisets::join(iset s1, int i, int j, iset s2) {
	s1 = max(s1); s2 = min(s2);
	if (hi(s1) >= i || lo(s2) <= j) fatal("join: overlapping sets");
	if (hi(s1) == i-1 && lo(s2) > j+1) {
		hi(s1) = j; right(s1) = s2; p(s2) = s1; return s1;
	} else if (hi(s1) < i-1 && lo(s2) == j+1) {
		lo(s2) = i; left(s2) = s1; p(s1) = s2; return s2;
	} else if (hi(s1) == i-1 && lo(s2) == j-1) {
		hi(s1) = hi(s2); right(s1) = right(s2); p(right(s1)) = s1;
		left(s2) = free; free = s2; return s1;
	}
	if (free == Null) fatal("join: out of nodes");
	int x = free; free = left(free);
	lo(x) = i; hi(x) = j;
	left(x) = s1; right(x) = s2;
	p(s1) = p(s2) = x;
	return x;
}

// Split s on i, yielding two sets, s1 containing integers smaller than i
// and s2 with keys larger than i. Return the pair [s1,s2]
ispair cisets::split(int i, iset s) {
	ispair sp;
	s = find(i,s);
	if (hi(s) < i) {
		sp.s1 = s; sp.s2 = right(s);
		right(s) = p(sp.s2) = Null;
	} else if (lo(s) > i) {
		sp.s1 = left(s); sp.s2 = s;
		left(s) = p(sp.s1) = Null;
	} else if (lo(s) == i && i < hi(s)) {
		sp.s1 = left(s); sp.s2 = s;
		left(s) = p(sp.s1) = Null;
		lo(s) = i+1;
	} else if (lo(s) < i && i == hi(s)) {
		sp.s1 = s; sp.s2 = right(s);
		right(s) = p(sp.s2) = Null;
		hi(s) = i-1;
	} else if (lo(s) < i && i < hi(s)) {
		if (free == Null) fatal("split:: out of nodes");
		int x = free; free = left(x);
		sp.s1 = s; sp.s2 = x;
		lo(x) = i+1; hi(x) = hi(s); left(x) = Null;
		right(x) = right(s); p(right(x)) = x;
		hi(s) = i-1; right(s) = Null;
	} else if (lo(s) == i && i == hi(s)) {
		sp.s1 = left(s); sp.s2 = right(s);
		p(sp.s1) = p(sp.s2) = Null;
		right(s) = p(s) = Null;
		left(s) = free; free = s;
	}
	return sp;
}

// Print the contents of all the sets in the collection
void cisets::print() {
	for (iset i = 1; i <= n; i++) {
		if (p(i) == Null) { sprint(i); putchar('\n'); }
	}
}

// Print the contents of the set s.
void cisets::sprint(iset s) {
	if (s == Null) return;
	printf("(%d,%d-%d) ",s,lo(s),hi(s));
	sprint(left(s)); sprint(right(s));
}

// Print the set s in in-order; j is the depth of i
void cisets::tprint(iset s, int j) {
	const int MAXDEPTH = 20;
	static char tabstring[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	if (s == Null) return;
	tprint(right(s),j+1);
	printf("%s(%d,%d-%d)\n",tabstring+MAXDEPTH-j,s,lo(s),hi(s));
	tprint(left(s),j+1);
}
