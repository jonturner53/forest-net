#include "prtn.h"

#define p(x) node[x].p
#define rank(x) node[x].rank

prtn::prtn(int N) : n(N) {
// Initialize partition so that every element is in separate set.
	node = new pnode[n+1];
	clear();
}

prtn::~prtn() { delete [] node; }

void prtn::clear() {
// Inititialize data structure.
	for (item i = 1; i <= n; i++) { p(i) = i; rank(i) = 0; }
	nfind = 0;
	p(Null) = Null; rank(Null) = 0;
}

item prtn::find(item x) {
// Find and return the canonical element of the set containing x.
	assert(1 <= x && x <= n);
	nfind++;
	if (x != p(x)) p(x) = find(p(x));
	return p(x);
}

item prtn::link(item x, item y) {
// Combine the sets whose canonical elements are x and y.
// Return the canonical element of the new set.
	assert(1 <= x && x <= n && 1 <= y && y <= n && x != y);
	if (rank(x) > rank(y)) {
		item t = x; x = y; y = t;
	} else if (rank(x) == rank(y))
		rank(y)++;
	return p(x) = y;
}

int prtn::findroot(int x) {
// Return the canonical element of the set containing x.
// Do not restructure tree in the process.
	if (x == p(x)) return(x);
	else return findroot(p(x));
}

ostream& operator<<(ostream& os, prtn& P) {
// Print the prtn. Show only the sets with more than one element.
	int i,j;
	int *root = new int[P.n+1]; bool *mark = new bool[P.n+1];
	for (i = 1; i <= P.n; i++) mark[i] = false;
	for (i = 1; i <= P.n; i++) {
		root[i] = P.findroot(i);
		if (root[i] != i) mark[root[i]] = true;
	}
	for (i = 1; i <= P.n; i++) {
		if (mark[i]) {
			misc::putNode(os,i,P.n);
			os << ":";
			for (j = 1; j <= P.n; j++) {
				if (j != i && root[j] == i) {
					os << " "; misc::putNode(os,j,P.n);
				}
			}
			os << endl;
		}
	}
	delete [] root; delete [] mark; 
	return os;
}
