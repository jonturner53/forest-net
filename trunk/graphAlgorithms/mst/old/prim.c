// usage:
//	prim
//
// prim reads a graph from stdin, computes its minimum
// spanning trees using Prim's algorithm and prints it out.
// them out.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "heaps/dheap.h"
#include "basic/wgraph.h"

void prim(wgraph&, wgraph&);

main()
{
	wgraph G; G.get(stdin);
	wgraph T(G.n,G.n-1); prim(G,T);
	G.put(stdout); printf("\n"); T.put(stdout);
}

void prim(wgraph& G, wgraph& T) {
// Find a minimum spanning tree of G using Prim's
// algorithm and return its in T.
	vertex u,v; edge e;
	edge* cheap = new edge[G.n+1];
	dheap S(G.n,max(3,G.m/G.n));

	for (e = G.first(1); e != Null; e = G.next(1,e)) {
		u = G.mate(1,e); S.insert(u,G.w(e)); cheap[u] = e;
	}
	while (!S.empty()) {
		u = S.deletemin();
		T.join(G.left(cheap[u]),G.right(cheap[u]),G.w(cheap[u]));
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.mate(u,e);
			if (S.member(v) && G.w(e) < S.key(v)) {
				S.changekey(v,G.w(e)); cheap[v] = e;
			} else if (!S.member(v) && T.first(v) == Null) {
				S.insert(v,G.w(e)); cheap[v] = e;
			}
		}
	}
}
