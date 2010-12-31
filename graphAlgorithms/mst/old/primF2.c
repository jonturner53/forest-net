// usage:
//	prim3 reps n p maxkey maxelen
//
// prim3 generates a random graph and then repeatedly
// computes its minimum spanning tree using Prim's algorithm.
// This version uses a Fibonacci heap, instead of a d-heap.
// Reps is the number of repeated computations, d is the
// d-heap parameter, n is the number of vertices, p is the
// edge probability, maxkey is the maximum key and maxelen
// is the maximum difference beteen the endpoint indices of
// each edge.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/wgraph.h"
#include "basic/clist.h"
#include "basic/list.h"
#include "heaps/fheaps.h"

void prim(wgraph&, wgraph&);

main(int argc, char* argv[]) {
	int doit, reps, d, n, maxkey, maxelen;
	double p;
	if (argc != 6 ||
	    sscanf(argv[1],"%d",&reps) != 1 ||
	    sscanf(argv[2],"%d",&n) != 1 ||
	    sscanf(argv[3],"%lf",&p) != 1 ||
	    sscanf(argv[4],"%d",&maxkey) != 1 ||
	    sscanf(argv[5],"%d",&maxelen) != 1)
		fatal("usage: prim3 reps n p maxkey maxelen");

	wgraph G; G.rgraph(n,p,maxkey,maxelen);
	wgraph *T;
	for (int i = 1; i <= reps; i++) {
		T = new wgraph(G.n,G.n-1);
		prim(G,*T);
		delete T;
	}
}

void prim(wgraph& G, wgraph& T) {
// Find a minimum spanning tree of G using Prim's
// algorithm and return its in T.
	vertex u,v; edge e;
	edge* cheap = new edge[G.n+1];
	fheaps F(G.n); fheap root;
	char *inHeap = new char[G.n+1]; // track vertices in heap
	int numInHeap = 0;

	for (u = 1; u <= G.n; u++) inHeap[u] = 0;

	e = G.first(1);
	if (e == Null) return;
	root = G.mate(1,e);
	do {
		u = G.mate(1,e); 
		root = F.insert(u,root,G.w(e)); 
		cheap[u] = e;
		inHeap[u] = 1; numInHeap++;
		e = G.next(1,e);
	} while (e != Null) ;

	while (numInHeap > 0) {
		u = root; root = F.deletemin(root);
		inHeap[u] = 0; numInHeap--;
		T.join(G.left(cheap[u]),G.right(cheap[u]),G.w(cheap[u]));
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.mate(u,e);
			if (inHeap[v] == 1 && G.w(e) < F.key(v)) {
				root = F.decreasekey(v,F.key(v)-G.w(e),root);
				cheap[v] = e;
			} else if (inHeap[v] == 0 && T.first(v) == Null) {
				root = F.insert(v,root,G.w(e));
				cheap[v] = e;
				inHeap[v] = 1; numInHeap++;
			}
		}
	}
}
