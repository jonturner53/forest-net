#include "stdinc.h"
#include "wgraph.h"
#include "clist.h"
#include "list.h"
#include "fheaps.h"

void primF(wgraph& G, wgraph& T) {
// Find a minimum spanning tree of G using Prim's
// algorithm and return its in T.
	vertex u,v; edge e;
	edge* cheap = new edge[G.n()+1];
	fheaps F(G.n()); fheap root;
	char *inHeap = new char[G.n()+1]; // track vertices in heap
	int numInHeap = 0;

	for (u = 1; u <= G.n(); u++) inHeap[u] = 0;

	e = G.first(1);
	if (e == G.term(1)) return;
	root = G.mate(1,e);
	do {
		u = G.mate(1,e); 
		root = F.insert(u,root,G.w(e)); 
		cheap[u] = e;
		inHeap[u] = 1; numInHeap++;
		e = G.next(1,e);
	} while (e != G.term(1)) ;

	while (numInHeap > 0) {
		u = root; root = F.deletemin(root);
		inHeap[u] = 0; numInHeap--;
		e = T.join(G.left(cheap[u]),G.right(cheap[u]));
		T.changeWt(e,G.w(cheap[u]));
		for (e = G.first(u); e != G.term(u); e = G.next(u,e)) {
			v = G.mate(u,e);
			if (inHeap[v] == 1 && G.w(e) < F.key(v)) {
				root = F.decreasekey(v,F.key(v)-G.w(e),root);
				cheap[v] = e;
			} else if (inHeap[v] == 0 && T.first(v) == G.term(v)) {
				root = F.insert(v,root,G.w(e));
				cheap[v] = e;
				inHeap[v] = 1; numInHeap++;
			}
		}
	}
	delete [] cheap; delete [] inHeap;
}
