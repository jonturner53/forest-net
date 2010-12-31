// faltPath(G,M) - return max size matching in bipartite graph
//                 using "fast" version of alternating path method
//
// FaltPath(G,M) returns a max size matching in bipartite graph G and
// returns it as a list of edges, M. FaltPath is implemented by invoking
// the constructor for the fAltPathC class (see below). FaltPathC
// encapsulates data and methods for finding the matching.
//
// The faltPath variant attempts to speed up the common case,
// by eliminating most of the per search overhead.

#include "stdinc.h"
#include "graph.h"
#include "list.h"
#include "dlist.h"

class faltPathC {
public: faltPathC(graph&,dlist&);
private:
	graph* G;		// graph we're finding matching for
	dlist* M;		// matching we're building

	enum stype {odd, even};
	stype* state;		// odd/even status of vertices
	int* visit;		// visit[u]=# of most recent search to visit u
	edge* mEdge;		// mEdge[u]=matching edge incident to u
	edge* pEdge;		// pEdge[u]=edge to parent of u in forest
	dlist* free;		// list of free vertices
	list* leaves;		// list of leaves in current forest

	int	sNum;		// index of current search
	
	void augment(edge);	// augment the matching
	edge findpath();	// find an augmenting path
	edge expand(vertex);	// expand tree at given vertex
};

void faltPath(graph& G, dlist& M) { faltPathC x(G,M); }

faltPathC::faltPathC(graph& G1, dlist& M1) : G(&G1), M(&M1) {
// Find a maximum size matching in the bipartite graph G and
// return it as a list of edges.
	int sNum; vertex u; edge e;

	state = new stype[G->n()+1]; visit = new int[G->n()+1];
	mEdge = new edge[G->n()+1]; pEdge = new edge[G->n()+1];
	free = new dlist(G->n()); leaves = new list(G->n());

// would be smart to build an initial maximal matching by
// making one pass through the edges

	for (u = 1; u <= G->n(); u++) {
		visit[u] = 0; mEdge[u] = Null; (*free) &= u;
	}
	
	sNum = 0;
	while((e = findpath()) != Null) 
		augment(e);
	delete [] state; delete [] visit; delete [] mEdge;
	delete [] pEdge; delete free; delete leaves;
}

void faltPathC::augment(edge e) {
// Modify the matching by augmenting along the path defined by
// the edge e and the pEdge pointers.
	vertex u, v; edge ee;

	u = G->left(e);
	while (pEdge[u] != Null) {
		ee = pEdge[u]; (*M) -= ee; v = G->mate(u,ee);
		ee = pEdge[v]; (*M) &= ee; u = G->mate(v,ee); 
		mEdge[u] = mEdge[v] = ee;
	}
	(*free) -= u;
	u = G->right(e);
	while (pEdge[u] != Null) {
		ee = pEdge[u]; (*M) -= ee; v = G->mate(u,ee);
		ee = pEdge[v]; (*M) &= ee; u = G->mate(v,ee); 
		mEdge[u] = mEdge[v] = ee;
	}
	(*free) -= u;
	(*M) &= e;
	mEdge[G->left(e)] = mEdge[G->right(e)] = e;
}

edge faltPathC::findpath() {
// Search for an augmenting path in the bipartite graph G.
// Return the edge that joins two separate trees in the forest
// defined by pEdge. This edge, together with the paths
// to the tree roots is an augmenting path. SNum is the
// index of the current search.
	vertex u; edge e;

	sNum++;

// alternative would be to initialize leaves to contain
// everything in free. If we had an assignment operator
// from a dlist to a list, this could be written as a
// simple assignment here.

	leaves->clear();
	for (u = (*free)[1]; u != Null; u = free->suc(u)) {
		visit[u] = sNum; state[u] = even; pEdge[u] = Null;
		e = expand(u);
		if (e != Null) return e;
	}
	while (!leaves->empty()) {
		u = (*leaves)[1]; (*leaves) <<= 1;
		e = expand(u);
		if (e != Null) return e;
	}
	return Null;
}

edge faltPathC::expand(vertex v) {
// Expand the forest at vertex v. If we find an edge connecting
// to another tree, return it.
	vertex w, x, y; edge e;

	for (e = G->first(v); e != Null; e = G->next(v,e)) {
		if (e == mEdge[v]) continue;
		w = G->mate(v,e);
		if (visit[w] < sNum && mEdge[w] != Null) {
			x = G->mate(w,mEdge[w]);
			visit[w] = visit[x] = sNum;
			state[w] = odd; pEdge[w] = e;
			state[x] = even; pEdge[x] = mEdge[x];
			(*leaves) &= x;
		} else if (visit[w] < sNum || state[w] == even) {
			for (x = w; pEdge[x] != Null; x = G->mate(x,pEdge[x])){}
			for (y = v; pEdge[y] != Null; y = G->mate(y,pEdge[y])){}
			if (x == y) fatal("findpath: graph not bipartite");
			return e;
		} 
	}
	return Null;
}

