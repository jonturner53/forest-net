// in progress

// edmonds(G,M) - return max size matching in general graph G
//
// Edmonds(G,M) computes a max size matching in graph G and
// returns it as a list of edges, M. Edmonds(G,M) uses Tarjan's
// version of Edmonds's algorithm and is implemented by invoking
// the constructor for the edmondsC class. The edmondsC class
// encapsulate data and methods for finding the matching.

#include "stdinc.h"
#include "graph.h"
#include "list.h"
#include "dlist.h"

class edmondsC {
public: edmondsC(graph&,dlist&);
private:
	graph* G;		// graph we're finding matching for
	dlist* M;		// matching we're building
	prtn *P;		// partition of the vertices into blossoms
	rlist* path;		// reversible list to re-trace path
	vertex* origin;		// origin[u] is the original vertex
				// corresponding to a blossom
	edge* bridge;		// bridge[u] is the edge that joins the
				// two branches of the blossom
	edge* pEdge;		// pEdge[u] is edge to parent of u in forest
	
	void augment(edge);	// augment the matching
	edge findpath();	// find an altmenting path
};

void edmonds(graph& G, dlist& M) { edmondsC x(G,M); }

edmondsC::edmondsC(graph& G1, dlist& M1) G(&G1), M(&M1) {
// Find a maximum size matching in the graph G and
// return it as a list of edges.
	edge e;
	pEdge = new edge[G->n+1];
	prtn *P = new prtn(G->n());
	rlist *path = new rlist(G->n());  // still need to implement this
// combine into single vector?
	vertex *origin = new vertex[G->n()+1];
	edge *bridge = new edge[G->n()+1];
	edge *pEdge = new edge[G->n()+1];
	
	while((e = findpath()) != Null) augment(e);

	delete [] pEdge;
}

// rework
void edmondsC::augment(edge e) {
// Modify the matching by augmenting along the path defined by
// the edge e and the pEdge pointers.
	vertex u, v; edge ee;

	u = G->left(e);
	while (pEdge[u] != Null) {
		ee = pEdge[u]; (*M) -= ee; u = G->mate(u,ee); 
		ee = pEdge[u]; (*M) &= ee; u = G->mate(u,ee); 
	}
	u = G->right(e);
	while (pEdge[u] != Null) {
		ee = pEdge[u]; (*M) -= ee; u = G->mate(u,ee); 
		ee = pEdge[u]; (*M) &= ee; u = G->mate(u,ee); 
	}
	(*M) &= e;
}

// rework
edge edmondsC::findpath() {
// Search for an augmenting path in the bipartite graph G.
// Return the edge that joins two separate trees in the forest
// defined by pEdge. This edge, together with the paths
// to the tree roots is an augmenting path.
//
	vertex u,v,w,x,y; edge e, f;
	typedef enum stype {unreached, odd, even};
	stype state[G->n+1];
	edge mEdge[G->n+1];  // mEdge[u] = matching edge incident to u

	for (u = 1; u <= G->n; u++) {
		state[u] = even; mEdge[u] = pEdge[u] = Null;
	}

	for (e = (*M)(1); e != Null; e = M->suc(e)) {
		u = G->left(e); v = G->right(e);
		state[u] = state[v] = unreached;
		mEdge[u] = mEdge[v] = e;
	}
	list q(G->m);
	for (e = 1; e <= G->m; e++) {
		if (state[G->left(e)] == even || state[G->right(e)] == even)
			q &= e;
	}

	while (q(1) != Null) {
		e = q(1); q <<= 1;
		v = state[G->left(e)] == even ? G->left(e) : G->right(e);
		w = G->mate(v,e);
		if (state[w] == unreached && mEdge[w] != Null) {
			x = G->mate(w,mEdge[w]);
			state[w] = odd; pEdge[w] = e;
			state[x] = even; pEdge[x] = mEdge[x];
			for (f = G->first(x); f != Null; f = G->next(x,f)) {
				if ((f != mEdge[x]) && !q.mbr(f)) q &= f;
			}
		} else if (state[w] == even) {
			for (x = w; pEdge[x] != Null; x = G->mate(x,pEdge[x])) 
				;
			for (y = v; pEdge[y] != Null; y = G->mate(y,pEdge[y]))
				;
			if (x == y) fatal("findpath: graph not bipartite");
			return e;
		}
	}
	return Null;
}
