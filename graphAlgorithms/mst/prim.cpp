#include "stdinc.h"
#include "dheap.h"
#include "wgraph.h"

void prim(wgraph& G, wgraph& T) {
// Find a minimum spanning tree of G using Prim's
// algorithm and return its in T.
	vertex u,v; edge e;
	edge *cheap = new edge[G.n()+1];
	dheap S(G.n(),max(3,G.m()/G.n()));

	for (e = G.first(1); e != G.term(1); e = G.next(1,e)) {
		u = G.mate(1,e); S.insert(u,G.w(e)); cheap[u] = e;
	}
	while (!S.empty()) {
		u = S.deletemin();
		e = T.join(G.left(cheap[u]),G.right(cheap[u]));
		T.changeWt(e,G.w(cheap[u]));
		for (e = G.first(u); e != G.term(u); e = G.next(u,e)) {
			v = G.mate(u,e);
			if (S.member(v) && G.w(e) < S.key(v)) {
				S.changekey(v,G.w(e)); cheap[v] = e;
			} else if (!S.member(v) && T.first(v) == T.term(v)) {
				S.insert(v,G.w(e)); cheap[v] = e;
			}
		}
	}
	delete [] cheap;
}
