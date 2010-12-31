// usage: rrobin
//
// rrobin reads a graph from stdin, computes its minimum spanning tree
// using Cheriton and Tarjan's round robin algorithm and prints it out.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/list.h"
#include "basic/dlist.h"
#include "basic/partition.h"
#include "heaps/llheaps.h"
#include "basic/wgraph.h"

void rrobin(wgraph&, wgraph&);

main() {
	wgraph G; G.get(stdin);
	wgraph T(G.n,G.n-1); rrobin(G,T);
	G.put(stdout); printf("\n"); T.put(stdout);
}

wgraph *gp; partition *pp;

// Return true if the endpoints of e are in same tree.
bit delf(item e) {
	return (*pp).find((*gp).left((e+1)/2)) == (*pp).find((*gp).right((e+1)/2));
}

void rrobin(wgraph& G, wgraph& T) {
// Find a minimum spanning tree of G using the round robin algorithm and
// return it in T. Actually finds a spanning forest, if no tree.
	edge e; vertex u,v,cu,cv; weight w;
	dlist q(G.n); list elist(2*G.m); lhNode *h = new lhNode[G.n+1];
	partition P(G.n); llheaps L(2*G.m,delf);
	gp = &G; pp = &P;
	for (e = 1; e <= G.m; e++) {
		L.setkey(2*e,G.w(e)); L.setkey(2*e-1,G.w(e));
	}
	for (u = 1; u <= G.n; u++) {
		elist.clear();
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			elist &= 2*e - (u == G.left(e));
		}
		if (elist(1) != Null) {
			h[u] = L.makeheap(elist); q &= u;
		}
	}
	while (q(2) != Null) {
		h[q(1)] = L.findmin(h[q(1)]);
		if (h[q(1)] == Null) { q -= q(1); continue; }
		e = (h[q(1)]+1)/2;
		u = G.left(e); v = G.right(e); w = G.w(e);
		cu = P.find(u); cv = P.find(v);
		T.join(u,v,w); q -= cu; q -= cv;
		h[P.link(cu,cv)] = L.lmeld(h[cu],h[cv]);
		q &= P.find(u);
	}
	delete [] h;
}
