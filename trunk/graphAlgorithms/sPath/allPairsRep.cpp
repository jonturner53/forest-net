// usage: 
//	allPairsRep method reps n p lo hi span
//
// allPairsRep repeated generates a random graph with the specified
// parameters and computes shortest paths with the specified method.
// Reps is the number of repetitions.
//
// If a graph has a negative length cycle, it prints an
// error message and halts.

#include "stdinc.h"
#include "wdigraph.h"

extern void dijkstraAll(wdigraph&, int**, vertex**);
extern void floyd(wdigraph&, int**, vertex**);

main(int argc, char *argv[]) {
	int i, reps, n, m, lo, hi, span;
	vertex u, v; wdigraph G; 
	int** dist; vertex** parent; vertex** mid;

	if (argc != 8 ||
	    sscanf(argv[2],"%d",&reps) != 1 ||
	    sscanf(argv[3],"%d",&n) != 1 ||
	    sscanf(argv[4],"%d",&m) != 1 ||
	    sscanf(argv[5],"%d",&lo) != 1  ||
	    sscanf(argv[6],"%d",&hi) != 1  ||
	    sscanf(argv[7],"%d",&span) != 1  )
		fatal("usage: allPairsRep method reps n p lo hi span");
	
	if (argc != 2) fatal("usage: allPairs method");

	if (strcmp(argv[1],"floyd") == 0) {
		dist = new int*[G.n()+1];
		mid = new vertex*[G.n()+1];
		for (u = 1; u <= G.n(); u++) {
			dist[u] = new int[G.n()+1];
			mid[u] = new vertex[G.n()+1];
		}
	} else if (strcmp(argv[1],"dijkstra") == 0) {
		dist = new int*[G.n()+1];
		parent = new vertex*[G.n()+1];
		for (u = 1; u <= G.n(); u++) {
			dist[u] = new int[G.n()+1];
			parent[u] = new vertex[G.n()+1];
		}
	} else {
		fatal("allPairs: undefined method");
	}

	for (i = 1; i <= reps; i++) {
		G.rgraph(n,m,span); G.randLen(lo,hi);
		if (strcmp(argv[1],"floyd") == 0) {
			floyd(G,dist,mid);
		} else if (strcmp(argv[1],"dijkstra") == 0) {
			dijkstraAll(G,dist,parent);
		}
	}
}
