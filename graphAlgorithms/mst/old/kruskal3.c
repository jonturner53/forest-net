// usage:
//	kruskal3 doit reps n p maxkey maxelen
//
// kruskal3 generates a random graph and then repeatedly
// computes its minimum spanning tree using Kruskal's algorithm.
// If parameter doit is zero, the execution of Kruskal's algorithm
// is omitted, but everything else is done, allowing one to
// get an accurate measurement of the overhead required for
// all the other operations.  Reps is the number of repeated
// computations,  n is the number of vertices, p is the
// edge probability, maxkey is the maximum key and maxelen
// is the maximum difference beteen the endpoint indices of
// each edge.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/partition.h"
#include "basic/wgraph.h"

void kruskal(wgraph&, wgraph&);

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
		fatal("usage: kruskal3 reps n p maxkey maxelen");

	wgraph G; G.rgraph(n,p,maxkey,maxelen);
	wgraph *T;
	for (int i = 1; i <= reps; i++) {
		T = new wgraph(G.n,G.n-1);
		if (doit) kruskal(G,*T);
		delete T;
	}
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
