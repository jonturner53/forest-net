// usage:
//	sptRep method reps n m span lo hi
//
// sptRep repeatedly generates a random graph and computes
// its shortest path tree using the specified method.
// Reps is the number of repetitions.
// n is the number of vertices, m is the number of edges
// [lo,hi] is the range of edge lengths and span is the maximum
// difference beteen the endpoint indices of each edge.
//
//

#include "stdinc.h"
#include "wdigraph.h"

extern void dijkstra(wdigraph&, vertex, vertex*, int*);
extern void bfScan(wdigraph&, vertex, vertex*, int*);

main(int argc, char *argv[]) {
	int i, reps, n, m, lo, hi, span;

	if (argc != 8 ||
	    sscanf(argv[2],"%d",&reps) != 1 ||
	    sscanf(argv[3],"%d",&n) != 1 ||
	    sscanf(argv[4],"%d",&m) != 1 ||
	    sscanf(argv[5],"%d",&span) != 1 ||
	    sscanf(argv[6],"%d",&lo) != 1 ||
	    sscanf(argv[7],"%d",&hi) != 1)
		fatal("usage: mstRep method reps n m span lo hi");

	vertex *p = new vertex[n+1]; vertex *d = new vertex[n+1];
	wdigraph G; wdigraph *T;
	for (i = 1; i <= reps; i++) {
		G.rgraph(n,m,span); 
		G.randLen(lo,hi);
		T = new wdigraph(G.n(),G.n()-1);
		if (strcmp(argv[1],"dijkstra") == 0)
			dijkstra(G,1,p,d);
		else if (strcmp(argv[1],"bfScan") == 0)
			bfScan(G,1,p,d);
		else
			fatal("sptRep: undefined method");
		delete T;
	}
}
