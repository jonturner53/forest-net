#include "dtrees.h"

#define parent(x) pvec[x]
#define succ(x) svec[x]

// Initialize a dtrees on nodes numbered {1,...,N}.
dtrees::dtrees(int N) {
	node i;
	n = N;
	svec = new node[n+1]; pvec = new node[n+1];
	ps = new pathset(n);
	for (i = 0; i <= n; i++) parent(i) = succ(i) = Null;
}

dtrees::~dtrees() { delete [] svec; delete [] pvec; delete ps; }

// Expose the path from i to the root of its tree.
path dtrees::expose(node i) {
	pipair pip;
	pip.p = Null; pip.i = i;
	while (pip.i != Null) pip = splice(pip);
	succ(pip.p) = Null;
	return pip.p;
}

// Combine path segments.
pipair dtrees::splice(pipair pip) {
	ppair pp; node w;
	w = succ(ps->findpath(pip.i));
	pp = ps->split(pip.i);
	if (pp.s1 != Null) succ(pp.s1) = pip.i;
	pip.p = ps->join(pip.p,pip.i,pp.s2); pip.i = w;
	return pip;
}

// Return the root of the tree containing node i.
path dtrees::findroot(node i) {
	node x;
	x = ps->findtail(expose(i));
	succ(x) = Null; // relies on fact that x is canonical element on return
	return x;
}

// Find the last min cost node on the path to the root.
// Return it and its cost.
cpair dtrees::findcost(node i) {
	cpair cp = ps->findpathcost(expose(i));
	succ(cp.s) = Null;
	return cp;
}

// Add x to the cost of every node on path from i to its tree root
void dtrees::addcost(node i, cost x) {
	ps->addpathcost(expose(i),x);
}

// Link the tree t to the tree containing i at i.
void dtrees::link(tree t, node i) {
	parent(t) = i;
	succ(ps->join(Null,expose(t),expose(i))) = Null;
}

// Cut the subtree rooted at i from the rest of its tree.
void dtrees::cut(node i) {
	ppair pp;
	parent(i) = Null;
	expose(i); pp = ps->split(i);
	succ(i) = Null;
	if (pp.s2 != Null) succ(pp.s2) = Null;
	return;
}

void dtrees::printpath(ostream& os, node i) const {
// Print a single path and its successor information
	ps->pprint(os,i);
	os << " succ("; misc::putNode(os,i,n);
	os << ")="; misc::putNode(os,succ(i),n); os << endl;
}

// Print the collection of trees as paths with successor
// information.
ostream& operator<<(ostream& os, const dtrees& dt) {
	for (node i = 1; i <= dt.n; i++) {
		node j = dt.ps->findtreeroot(i);
		if (i == j) dt.printpath(os,i);
	}
}
