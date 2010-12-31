// usage: kruskal
//
// kruskal reads a graph from stdin, computes its minimum spanning tree
// using Kruskal's algorithm and prints it out.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/partition.h"
#include "basic/wgraph.h"

void kruskal(wgraph&, wgraph&);

main() {
	wgraph G; G.get(stdin);
	wgraph T(G.n,G.n-1);
	kruskal(G,T);
	G.put(stdout); printf("\n");
	T.put(stdout);
	exit(0);
}

void kruskal(wgraph& G, wgraph& T) {
// Find a minimum spanning tree of G using Kruskal's algorithm and
// return it in T.
	edge e; vertex u,v,cu,cv;
	weight w; partition P(G.n);
	G.esort();
	for (e = 1; e <= G.m; e++) {
		u = G.left(e); v = G.right(e); w = G.w(e);
		cu = P.find(u); cv = P.find(v);
		if (cu != cv) { P.link(cu,cv); T.join(u,v,w); }
	}
}
