// usage: toposort
//
// Toposort reads a graph from stdin, and creates an equivalent
// graph whose vertices are in topologically sorted order.
// This graph, and the mapping used to produce, are written
// to stdout.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "list.h"
#include "digraph.h"

void toposort(digraph&, vertex*, vertex*);

main() {
	int i;
	vertex u,v; edge e;
	digraph G; cin >> G;
	int *pos = new int[G.n()+1];
	vertex *vert = new vertex[G.n()+1];
	toposort(G,pos,vert);
	digraph H(G.n(),G.m());
	cout << "# ";
	for (i = 1; i <= G.n(); i++) {
		u = vert[i];
		misc::putNode(cout,u,G.n()); cout << "->";
		misc::putNode(cout,pos[u],G.n()); cout << " ";
		if ((i%10) == 0) cout << "\n# ";
		for (e=G.firstOut(u); e != G.outTerm(u); e=G.next(u,e)) {
			v = G.head(e);
			H.join(pos[u],pos[v]); 
		}
	}
	H.sortAdjLists();
	cout << endl << H;
}

void toposort(digraph& G, int *pos, int *vert) {
// Compute a topological ordering of G. On return, pos[u]
// is the position of vertex u in the ordering and vert[i]
// is the vertex in the i-th position in the ordering.
	int i; vertex u,v; edge e;
	list q(G.n());
	int *nin = new int[G.n()+1];

	// Let nin[u]=in-degree of u and put nodes u with nin[u]=0 on q
	for (u = 1; u <= G.n(); u++) {
		nin[u] = 0;
		for (e=G.firstIn(u); e != G.inTerm(u); e=G.next(u,e)) {
			nin[u]++;
		}
		if (nin[u] == 0) q &= u;
	}
	i = 0;
	while (!q.empty()) { // q contains nodes u with nin[u] == 0
		u = q[1]; q <<= 1; pos[u] = ++i; vert[i] = u;
		for (e=G.firstOut(u); e != G.outTerm(u); e=G.next(u,e)) {
			v = G.head(e);
			if ((--(nin[v])) == 0) q &= v;
		}
	}
	if (i < G.n()) fatal("toposort: graph has cycle");
}
