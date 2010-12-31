#include "altPath.h"

altPath::altPath(graph& G1, dlist& M1) : G(&G1), M(&M1) {
// Find a maximum size matching in the bipartite graph G and
// return it as a list of edges.
	pEdge = new edge[G->n()+1];
	
	edge e;
	while((e = findPath()) != Null) augment(e);

	delete [] pEdge;
}

void altPath::augment(edge e) {
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

edge altPath::findPath() {
// Search for an augmenting path in the bipartite graph G.
// Return the edge that joins two separate trees in the forest
// defined by pEdge. This edge, together with the paths
// to the tree roots is an augmenting path.
//
	vertex u,v,w,x,y; edge e, f;
	typedef enum stype { unreached, odd, even };
	stype state[G->n()+1];
	edge mEdge[G->n()+1];  // mEdge[u] = matching edge incident to u

	for (u = 1; u <= G->n(); u++) {
		state[u] = even; mEdge[u] = pEdge[u] = Null;
	}

	for (e = (*M)[1]; e != Null; e = M->suc(e)) {
		u = G->left(e); v = G->right(e);
		state[u] = state[v] = unreached;
		mEdge[u] = mEdge[v] = e;
	}
	list q(G->m());
	for (e = 1; e <= G->m(); e++) {
		if (state[G->left(e)] == even || state[G->right(e)] == even)
			q &= e;
	}

	while (q[1] != Null) {
		e = q[1]; q <<= 1;
		v = (state[G->left(e)] == even ? G->left(e) : G->right(e));
		w = G->mate(v,e);
		if (state[w] == unreached && mEdge[w] != Null) {
			x = G->mate(w,mEdge[w]);
			state[w] = odd; pEdge[w] = e;
			state[x] = even; pEdge[x] = mEdge[x];
			for (f = G->first(x); f != Null; f = G->next(x,f)) {
				if ((f != mEdge[x]) && !q.mbr(f)) q &= f;
			}
		} else if (state[w] == even) {
			for (x = w; pEdge[x] != Null; x = G->mate(x,pEdge[x])){}
			for (y = v; pEdge[y] != Null; y = G->mate(y,pEdge[y])){}
			if (x == y) fatal("findpath: graph not bipartite");
			return e;
		}
	}
	return Null;
}
