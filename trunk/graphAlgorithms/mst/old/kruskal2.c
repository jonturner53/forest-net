// usage: kruskal2
//
// kruskal2 generates a number of random graphs, prints out for each one
// the number of vertices, the number of edges, the cost of a minimum spanning
// tree and the number of calls to the find subroutine in the partition
// data structure.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/partition.h"
#include "basic/wgraph.h"

int kruskal(wgraph&);

int nfind;

main() {
	int i,n,treecost; wgraph G;

	for (n = 10; n <= 100; n += 10) {
		for (i = 1; i <= 50; i++) {
			G.rgraph(n,0.5,n,n);
			treecost = kruskal(G);
			printf("%6d %6d %6d %6d\n",G.n,G.m,treecost,nfind);
		}
	}
}

int kruskal(wgraph& G) {
// Find a minimum spanning tree of G using Kruskal's algorithm and
// return its cost.
	edge e; vertex u,v,cu,cv; int cost = 0;
	weight w; partition P(G.n);
	G.esort();
	for (e = 1; e <= G.m; e++) {
		u = G.left(e); v = G.right(e); w = G.w(e);
		cu = P.find(u); cv = P.find(v);
		if (cu != cv) {
			P.link(cu,cv); cost += w;
		}
	}
	nfind = P.findcount();
	return cost;
}
