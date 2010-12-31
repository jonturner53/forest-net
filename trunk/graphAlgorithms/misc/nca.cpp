#include "nca.h"

nca::nca(graph& T, vertex root1, vertex_pair pairs1[], int np1, int ncav1[])
	   : Tp(&T), root(root1), pairs(pairs1), np(np1), ncav(ncav1) {
// Compute the nearest common ancestor in T relative to root of
// all vertex pairs in the vector "pairs". Pair[i]=(pair[i][0],pair[i][1])
// is the i-th pair. Np is the number of pairs. On return ncav[i] is
// the nearest common ancestor of the vertices in pair[i].

	Gp = new graph(Tp->n(),np);
	for (int i = 0; i < np; i++) Gp->join(pairs[i].v1, pairs[i].v2);
	// note: edges are allocated sequentially from 1,
	// so edge i+1 corresponds to pair i

	P = new prtn(Tp->n());
	noa = new vertex[Tp->n()+1];
	state = new state_t[Tp->n()+1];
	for (vertex u = 1; u <= Tp->n(); u++) state[u] = unreached;

	compute_nca(root,root);

	delete Gp; delete P; delete [] noa; delete [] state;
}

void nca::compute_nca(vertex u, vertex pu) {
// Recursive search, where u is current vertex and pu is its parent
// (except on the top level call when pu=u).
	vertex v; edge e;

	state[u] = open;
	for (e = Tp->first(u); e != Tp->term(u); e = Tp->next(u,e)) {
		v = Tp->mate(u,e); if (v == pu) continue;
		compute_nca(v,u);
		P->link(P->find(u),P->find(v));
		noa[P->find(u)] = u;
	}
	for (e = Gp->first(u); e != Gp->term(u); e = Gp->next(u,e)) {
		v = Gp->mate(u,e);
		if (state[v] == closed)
			ncav[e-1] = noa[P->find(v)];
	}
	state[u] = closed;
}
