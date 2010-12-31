// usage:
//	prim2 doit reps n p maxkey maxelen
//
// prim2 generates a random graph and then repeatedly
// computes its minimum spanning tree using Prim's algorithm.
// If parameter doit is zero, the execution of Prim's algorithm
// is omitted, but everything else is done, allowing one to
// get an accurate measurement of the overhead required for
// all the other operations. Reps is the number of repeated computations, 
// n is the number of vertices, p is the edge probability,
// maxkey is the maximum key and maxelen is the maximum
// difference beteen the endpoint indices of each edge.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "heaps/dheap.h"
#include "basic/wgraph.h"

void prim(wgraph&, wgraph&);

main(int argc, char* argv[])
{
	int doit, reps, d, n, maxkey, maxelen;
	double p;
	if (argc != 7 ||
	    sscanf(argv[1],"%d",&doit) != 1 ||
	    sscanf(argv[2],"%d",&reps) != 1 ||
	    sscanf(argv[3],"%d",&n) != 1 ||
	    sscanf(argv[4],"%lf",&p) != 1 ||
	    sscanf(argv[5],"%d",&maxkey) != 1 ||
	    sscanf(argv[6],"%d",&maxelen) != 1)
		fatal("usage: prim2 reps n p maxkey maxelen");

	wgraph G; G.rgraph(n,p,maxkey,maxelen);
	wgraph *T;
	for (int i = 1; i <= reps; i++) {
		T = new wgraph(G.n,G.n-1);
		if (doit) prim(G,*T);
		delete T;
	}
}

void prim(wgraph& G, wgraph& T) {
// Find a minimum spanning tree of G using Prim's
// algorithm and return its in T.
	vertex u,v; edge e;
	edge* cheap = new edge[G.n+1];
	dheap S(G.n,2+G.m/G.n);

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
