#include "pathset.h"

#define left(x) vec[x].left
#define right(x) vec[x].right
#define p(x) vec[x].p
#define dcost(x) vec[x].dcost
#define dmin(x) vec[x].dmin

// Initialize a pathset on nodes numbered {1,...,N}.
pathset::pathset(int N) : n(N) {
	node i;
	vec = new pnode[n+1];
	for (i = 0; i <= n; i++) {
		left(i) = right(i) = p(i) = Null;
		dcost(i) = dmin(i) = 0;
	}
}

pathset::~pathset() { delete [] vec; }

// Splay at node i. Return root of resulting tree.
node pathset::splay(node x) {
	while (p(x) != Null) splaystep(x);
	return x;
}

// Perform a single splay step at x.
void pathset::splaystep(node x) {
        node y = p(x);
        if (y == Null) return;
        node z = p(y);
        if (x == left(left(z)) || x == right(right(z)))
                rotate(y);
        else if (z != Null) // x is "inner grandchild"
                rotate(x);
        rotate(x);
}

void pathset::rotate(node x) {
// Perform a rotation at the parent of x,
// moving x up to take its parent's place.
        node y = p(x); if (y == Null) return;
        node a, b, c;
        if (x == left(y)) { a = left(x);  b = right(x); c = right(y); }
        else              { a = right(x); b = left(x);  c = left(y);  }

	// do the rotation
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

	// update dmin, dcost values
        dmin(a) += dmin(x); dmin(b) += dmin(x);

        dcost(x) = dcost(x) + dmin(x);
        cost dmx = dmin(x);
        dmin(x) = dmin(y);

        dmin(y) = dcost(y);
        if (b != Null) dmin(y) = min(dmin(y),dmin(b)+dmx);
        if (c != Null) dmin(y) = min(dmin(y),dmin(c));
        dcost(y) = dcost(y) - dmin(y);

        dmin(b) -= dmin(y); dmin(c) -= dmin(y);
}

// Return the canonical element of the set containing node i.
// Specifically, return the element that is the canonical element
// before findpath is called. After findpath is called, i will
// be the canonical element, due to the splay.
path pathset::findpath(node i) { 
	node x;
	for (x = i; p(x) != Null; x = p(x)) {}
	splay(i);
	return x;
}

// Return the node in s with key k, if there is one, else Null.
path pathset::findtail(path q) {
	if (q == Null) return Null;
	while (right(q) != Null) q = right(q);
	return splay(q);
}

// Add x to the cost of every node in q.
void pathset::addpathcost(path q, cost x) { dmin(q) += x; }

// Return the last node on the path that has minimum cost and the cost
cpair pathset::findpathcost(path q) {
	while (1) {
		if (right(q) != Null && dmin(right(q)) == 0)
			q = right(q);
		else if (dcost(q) > 0)
			q = left(q);
		else
			break;
	}
	q = splay(q);
	cpair cp = { q, dmin(q) };
	return cp;
}

// Return the root of the search tree containing i without
// doing a splay. This is used by the dtrees print routine
// so we can print without disturbing the structure.
path pathset::findtreeroot(node i) {
	while (p(i) != Null) i = p(i);
	return i;
}

// Return the path formed by joining r, v and q;
// i is assumed to be a single node 
path pathset::join(path r, node i, path q) {
	cost dmin_i = dmin(i);
	left(i) = r; right(i) = q;
	if (r == Null && q == Null) {
		; // do nothing
	} else if (r == Null) {
		dmin(i) = min(dmin(i),dmin(q));
		dmin(q) -= dmin(i);
		p(q) = i;
	} else if (q == Null) {
		dmin(i) = min(dmin(i),dmin(r));
		dmin(r) -= dmin(i);
		p(r) = i;
	} else {
		dmin(i) = min(dmin(r),min(dmin(i),dmin(q)));
		dmin(r) -= dmin(i); dmin(q) -= dmin(i);
		p(r) = p(q) = i;
	}
	dcost(i) = dmin_i - dmin(i);
	return i;
}

// Split the path containing i on i. Return the two
// paths that result from the split.
ppair pathset::split(node i) {
	ppair pair;

	splay(i);
	if (left(i) == Null) 
		pair.s1 = Null;
	else {
		pair.s1 = left(i); p(pair.s1) = Null; left(i) = Null;
		dmin(pair.s1) += dmin(i);
	} 
	if (right(i) == Null) 
		pair.s2 = Null;
	else {
		pair.s2 = right(i); p(pair.s2) = Null; right(i) = Null;
		dmin(pair.s2) += dmin(i);
	} 
	dmin(i) += dcost(i);
	dcost(i) = 0;

	return pair;
}

cost pathset::c(node i) {
// Return the cost of node i. Note, no splay done here, as we
// don't want this to restructure the tree.
	cost s;
	s = dcost(i);
	while (i != Null) { s += dmin(i); i = p(i); }
	return s;
}

// Print all the paths in the path set
ostream& operator<<(ostream& os, const pathset& pset) {
	for (node i = 1; i <= pset.n; i++) {
		if (pset.p(i) == Null) { pset.print(os,i,0); os << endl; }
	}
	cout << endl;
}

// Print the path q, in path order and with actual costs.
void pathset::print(ostream& os, path q, cost mc) const {
	if (q == Null) return;
	print(os,left(q),dmin(q)+mc);
	os << "("; misc::putNode(os,q,n); 
	if (p(q) == Null) os << "*";
	os << "," << dcost(q)+dmin(q)+mc << ") ";
	print(os,right(q),dmin(q)+mc);
}

// Print the path q as a tree in-order; j is the depth of i
void pathset::tprint(ostream& os, path q, int j) const {
	const int MAXDEPTH = 20;
	static char tabstring[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	if (q == Null) return;
	tprint(os,right(q),j+1);
	os << (tabstring+MAXDEPTH-j) << "("; misc::putNode(os,q,n);
	os << "," << dcost(q) << "," << dmin(q) << ")\n";
	tprint(os,left(q),j+1);
}
