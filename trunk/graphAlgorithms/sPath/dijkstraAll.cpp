#include "stdinc.h"
#include "list.h"
#include "dheap.h"
#include "wdigraph.h"

extern void dijkstra(wdigraph&, vertex, vertex*, int*);
extern void bfScan(wdigraph&, vertex, vertex*, int*);

void dijkstraAll(wdigraph& D, int* dist[], vertex* parent[]) {
// Compute a solution to the all pairs shortest path problem
// using Dijkstra's algorithm, with transformed edge lengths.
// Return dist[u][v]=distance from u to v and 
// parent[u][v]=parent of v in the shortest path tree rooted at u.

	vertex u,v; edge e;

	// Compute distances in augmented graph.
	vertex* p1 = new vertex[D.n()+1];
	int* d1 = new int[D.n()+1];
	bfScan(D,1,p1,d1);

	// Modify edge costs.
	for (e = 1; e <= D.m(); e++) {
		u = D.tail(e); v = D.head(e);
		D.changeLen(e,D.len(e)+(d1[u]-d1[v]));
	}

	// Compute shortest paths & put inverse-transformed distances in dist.
	vertex* p2 = new vertex[D.n()+1];
	int* d2 = new int[D.n()+1];
	for (u = 1; u <= D.n(); u++) {
		dijkstra(D,u,p2,d2);
		for (v = 1; v <= D.n(); v++) {
			dist[u][v] = d2[v]-(d1[u]-d1[v]);
			parent[u][v] = p2[v];
		}
	}

	// Restore original edge costs.
	for (e = 1; e <= D.m(); e++) {
		u = D.tail(e); v = D.head(e);
		D.changeLen(e,D.len(e)-(d1[u]-d1[v]));
	}

	delete [] d1; delete[] p1; delete[] d2; delete [] p2;
}
