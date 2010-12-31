// usage: mstUpdate2 n m maxWt repCount seed
//
// mstUpdate2 generates a random graph on n vertices with edge probability p.
// It then computes a minimum spanning tree of the graph then repeatedly
// modifies the minimimum spanning tree by randomly changing the weight
// of one of the non-tree edges.
//
// This version just uses Kruskal for the updates; to provide a
// comparison to the version that updates incrementally.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "prtn.h"
#include "wgraph.h"
#include "list.h"

extern void kruskal(wgraph&, list&);

void buildpp(wgraph&, list&, edge*);
int mstUpdate(wgraph&, edge*, edge, int);

main(int argc, char* argv[]) {
	int i, n, m, maxWt, repCount, retVal, seed;
	int notZero, minCyc, maxCyc, avgCyc;
	edge e, modEdge;
	vertex u, v;
	wgraph G;


	if (argc != 6 ||
	    sscanf(argv[1],"%d",&n) != 1 ||
	    sscanf(argv[2],"%d",&m) != 1 ||
	    sscanf(argv[3],"%d",&maxWt) != 1 ||
	    sscanf(argv[4],"%d",&repCount) != 1 ||
	    sscanf(argv[5],"%d",&seed) != 1)
		fatal("usage: mstUpdate2 n m maxWt repCount seed");

	srandom(seed);

	G.rgraph(n,m,n); G.randWt(0,maxWt);
	list T(G.m());

	for (i = 1; i <= repCount; i++) {
		e = randint(1,G.m());
		G.changeWt(e,randint(1,maxWt));
		kruskal(G,T);
		T.clear();
	}
}
