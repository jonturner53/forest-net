// Usage:
//      bsDinic2 reps n p seed
//
// bsDinic2 generates repeatedly generates random bipartite graphs
// and then computes a maximum size matching using the augmenting
// path algorithm
//
// Reps is the number of repeated computations,
// 2n is the number of vertices, p is the edge probability.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/wgraph.h"
#include "basic/list.h"
#include "basic/flograph.h"
#include "max_flow/dinic.h"

bit getCut(wgraph&, list&);
void bsDinic(wgraph&, list&);

main(int argc, char* argv[])
{
        int reps, n, size, seed;
        vertex u; edge e;
        double p;
        if (argc != 5 ||
            sscanf(argv[1],"%d",&reps) != 1 ||
            sscanf(argv[2],"%d",&n) != 1 ||
            sscanf(argv[3],"%lf",&p) != 1 ||
            sscanf(argv[4],"%d",&seed) != 1)
                fatal("usage: bsDinic2 reps n p seed");

        srandom(seed);
        wgraph G;
        list M(max(1000,int(1.1*n*n*p)));
        for (int i = 1; i <= reps; i++) {
                G.rbigraph(n,p,1);
                bsDinic(G,M);
		M.clear();
        }
}

bit getCut(wgraph& G, list& X) {
// Return true if G is bipartite, else false.
// If G is bipartite, return a set of vertices X that defines
// a cut that is crossed by all edges in G.
	vertex u, v, w; edge e;
	list queue(G.n);
	typedef enum stype {unreached, odd, even};
	stype *state = new stype[G.n+1];

	for (u = 1; u <= G.n; u++) state[u] = unreached;
	u = 1;
	while (u <= G.n) {
		state[u] = even;
		queue &= u; X &= u;
		while (queue(1) != Null) {
			v = queue(1); queue <<= 1;
			for (e = G.first(v); e != Null; e = G.next(v,e)) {
				w = G.mate(v,e);
				if (state[w] == state[v]) {
					return false;
				}
				if (state[w] == unreached) {
					state[w] = state[v]==even ? odd : even;
					if (state[w] == even) X &= w;
					queue &= w;
				}
			}
		}
		for (u++; u <= G.n && state[u] != unreached; u++)
			;
	}
	delete[] state;
	return true;
}


void bsDinic(wgraph& G, list& M) {
// Find a maximum size matching in the bipartite graph G and
// return it as a vector of edges, with a Null edge used to
// indicate the end of the vector. The list X, is a set of
// vertices that defines a bipartition.
	vertex u,v; edge e;
	list X(G.n);

	if (!getCut(G,X)) fatal("bsDinic: graph is not bipartite");

	// Create flograph corresponding to G.
	// Do this, so that corresponding edges have same
	// edge numbers in F and G.
	flograph F(2+G.n,G.n+G.m);
	for (e = 1; e <= G.m; e++) {
		if (X.mbr(G.left(e))) u = G.left(e);
		else u = G.right(e);
		v = G.mate(u,e);
		F.join(u+1,v+1,1,0); 	// shift vertex numbers 
					// to make room for source
	}
	for (u = 1; u <= G.n; u++) {
		if (X.mbr(u)) F.join(1,u+1,1,0);
		else F.join(u+1,F.n,1,0);
	}

	dinic(F);

	for (e = 1; e <= G.m; e++) {
		if (F.f(1+G.left(u),e) != 0) M &= e;
	}
}

