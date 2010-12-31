// Usage:
//      bsAugPath2 reps n p seed
//
// bsAugPath2 generates repeatedly generates random bipartite graphs
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
#include "basic/dlist.h"

bit is_bipartite(wgraph&);
void bsAugPath(wgraph&, dlist&);
bit findpath(wgraph&, dlist&, list&);

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
                fatal("usage: bsAugPath2 reps n p seed");

        srandom(seed);
        wgraph G;
	dlist M(max(1000,int(1.1*n*n*p)));
        for (int i = 1; i <= reps; i++) {
                G.rbigraph(n,p,1);
                bsAugPath(G,M);
		M.clear();
        }
}

void bsAugPath(wgraph& G, dlist& M) {
// Find a maximum size matching in the bipartite graph G and
// return it as a vector of edges, with a Null edge used to
// indicate the end of the vector.
	list pathedge(G.m); 	// edges in augmenting path

	while(findpath(G,M,pathedge)) {
		M &= pathedge(1); pathedge <<= 1;
		while (pathedge(1) != Null) {
			M -= pathedge(1); pathedge <<= 1;
			M &= pathedge(1); pathedge <<= 1;
		}
	}
}

bit findpath(wgraph& G, dlist& M, list& pathedge) {
// Search for an augmenting path in the bipartite graph G. M is the 
// current matching. Return the path as a list of edges in pathedge.
// Return true if a path is found, else false.
	vertex u,v,w,x,y; edge e, f;
	typedef enum stype {unreached, odd, even};
	stype *state = new stype[G.n+1];
	edge *pe = new edge[G.n+1]; 	// pe[u] = edge to parent of u
	edge *medge = new edge[G.n+1];	// medge[u] = matching edge at u

	for (u = 1; u <= G.n; u++) {
		pe[u] = Null; state[u] = even; medge[u] = Null;
	}
	for (e = M(1); e != Null; e = M.suc(e)) {
		u = G.left(e); v = G.right(e);
		state[u] = state[v] = unreached;
		medge[u] = medge[v] = e;
	}
	list queue(G.m);
	for (e = 1; e <= G.m; e++) {
		if (state[G.left(e)] == even || state[G.right(e)] == even)
			queue &= e;
	}
	pathedge.clear();
	while (queue(1) != Null) {
		e = queue(1); queue <<= 1;
		v = state[G.left(e)] == even ? G.left(e) : G.right(e);
		w = G.mate(v,e);
		if (state[w] == unreached && medge[w] != Null) {
			x = G.mate(w,medge[w]);
			state[w] = odd; pe[w] = e;
			state[x] = even; pe[x] = medge[x];
			for (f = G.first(x); f != Null; f = G.next(x,f)) {
				if ((f != medge[x]) && !queue.mbr(f))
					queue &= f;
			}
		} else if (state[w] == even) {
			for (x = v; pe[x] != Null; x = G.mate(x,pe[x])) 
				pathedge.push(pe[x]);
			pathedge &= e;
			for (y = w; pe[y] != Null; y = G.mate(y,pe[y]))
				pathedge &= pe[y];
			if (x == y) fatal("findpath: graph not bipartite");
			queue.clear();
		}
	}
	delete [] state;
	delete [] pe;
	delete [] medge;
	return pathedge(1) == Null ? false : true;
}
