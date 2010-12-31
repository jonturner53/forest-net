// usage: maxAugPath
//
// maxAugPath reads a flograph from stdin, computes a maximum flow between
// vertices 1 and n using the max capacity augmenting path algorithm.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/flograph.h"
#include "basic/list.h"
#include "heaps/dheap.h"

void maxAugPath(flograph&);
int findpath(flograph&, list&);

main() {
	flograph G;
	G.get(stdin); maxAugPath(G); G.put(stdout);
}

void maxAugPath(flograph& G) {
// Find maximum flow in G using the max capacity augmenting path method.
	vertex u; edge e; flow f;
	list p(G.m);

	while (findpath(G,p)) {
		f = BIGINT;
		u = 1;
		for (e = p(1); e != Null; e = p.suc(e)) {
			f = min(f,G.res(u,e)); u = G.mate(u,e);
		}
		u = 1;
		for (e = p(1); e != Null; e = p.suc(e)) {
			G.addflow(u,e,f); u = G.mate(u,e);
		}
	}
	return;
}

int findpath(flograph& G, list& path) {
// Find a path with unused residual capacity.
        vertex u, v; edge e;
        dheap S(G.n,4);
        edge *pathedge = new edge[G.n+1];
        int *bcap = new int[G.n+1];

        for (u = 1; u <= G.n; u++) { pathedge[u] = Null; bcap[u] = 0; }
        bcap[1] = BIGINT;
        S.insert(1,-BIGINT); // store negative bcap, so deletemin gives max cap
        while (!S.empty()) {
                u = S.deletemin();
                for (e = G.first(u); e != Null; e = G.next(u,e)) {
                        v = G.mate(u,e);
			if (min(bcap[u], G.res(u,e)) > bcap[v]) {
                                bcap[v] = min(bcap[u],G.res(u,e));
                                pathedge[v] = e;
				if (S.member(v))
					S.changekey(v,-bcap[v]);
				else
                                	S.insert(v,-bcap[v]);
                        }
                }
        }
        path.clear();
	if (bcap[G.n] != 0) {
		u = G.n; e = pathedge[u];
		while (e != Null) {
			path.push(e); u = G.mate(u,e); e = pathedge[u];
		}
	}
        delete [] pathedge; delete [] bcap;
        return path(1) != Null;
}
