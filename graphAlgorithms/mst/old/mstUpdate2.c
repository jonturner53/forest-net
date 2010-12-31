// usage: mstUpdate2 n p maxWt repCount seed
//
// mstUpdate2 generates a random graph on n vertices with edge probability p.
// It then computes a minimum spanning tree of the graph then repeatedly
// modifies the minimimum spanning tree by randomly changing the weight
// of one of the non-tree edges.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/partition.h"
#include "basic/wgraph.h"
#include "basic/list.h"

void buildpp(wgraph&, list&, edge*);
int mstUpdate(wgraph&, edge*, edge, int);
void kruskal(wgraph&, list&);

main(int argc, char* argv[]) {
	int i, n, maxWt, repCount, retVal, seed;
	int notZero, minCyc, maxCyc, avgCyc;
	double p;
	edge e, modEdge;
	vertex u, v;
	wgraph G;


	if (argc != 6 ||
	    sscanf(argv[1],"%d",&n) != 1 ||
	    sscanf(argv[2],"%lf",&p) != 1 ||
	    sscanf(argv[3],"%d",&maxWt) != 1 ||
	    sscanf(argv[4],"%d",&repCount) != 1 ||
	    sscanf(argv[5],"%d",&seed) != 1)
		fatal("usage: mstUpdate2 n p maxWt repCount seed");

	G.rgraph(n,p,maxWt,n);
	list T(G.m);

	srandom(seed);

	for (i = 1; i <= repCount; i++) {
		e = randint(1,G.m);
		G.changeWt(e,randint(1,maxWt));
		kruskal(G,T);
		T.clear();
	}
}

void kruskal(wgraph& G, list& T) {
// Find a minimum spanning tree of G using Kruskal's algorithm and
// return the edges of the tree in the list, T.
	edge e; vertex u,v,cu,cv;
	weight w; partition P(G.n);
	G.esort();
	for (e = 1; e <= G.m; e++) {
		u = G.left(e); v = G.right(e); w = G.w(e);
		cu = P.find(u); cv = P.find(v);
		if (cu != cv) { P.link(cu,cv); T &= e; }
	}
}
