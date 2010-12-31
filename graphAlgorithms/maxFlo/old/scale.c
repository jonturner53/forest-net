// usage: scale
//
// Scale reads a flograph from stdin, computes a maximum flow between
// vertices 1 and n using the augmenting path algorithm with capacity
// scaling.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/flograph.h"
#include "basic/list.h"

void scale(flograph&);
int findpath(flograph&, list&);

main() {
	flograph G;
	G.get(stdin); scale(G); G.put(stdout);
}

int D; // scaling parameter

void scale(flograph& G) {
// Find maximum flow in G using the shortest augmenting path method.
	int i; vertex u; edge e; flow f;
	list path(G.m);

	for (e = 1; e <= G.m; e++) D = max(D,G.res(G.tail(e),e));
	for (i = 1; i <= D; i <<= 1) {}   D = (i >> 1);

	while (findpath(G,path)) {
		u = 1; f = BIGINT;
		for (e = path(1); e != Null; e = path.suc(e)) {
			f = min(f,G.res(u,e)); u = G.mate(u,e);
		}
		u = 1;
		for (e = path(1); e != Null; e = path.suc(e)) {
			G.addflow(u,e,f); u = G.mate(u,e);
		}
	}
	return;
}

int findpath(flograph& G, list& path) {
// Find a path with unused residual capacity.
	vertex u,v; edge e;
	int i; 
	edge *pathedge = new edge[G.n+1];
	list queue(G.n);

	while (D > 0) {
		for (u = 2; u <= G.n; u++) pathedge[u] = -1;
		pathedge[1] = Null; queue.clear(); queue &= 1;
		while (pathedge[G.n] == -1 && queue(1) != Null) {
			u = queue(1); queue <<= 1;
			for (e = G.first(u); e != Null; e = G.next(u,e)) {
				v = G.mate(u,e);
				if (G.res(u,e) >= D && pathedge[v] == -1) {
					pathedge[v] = e; queue &= v;
				}
			}
		}
		if (pathedge[G.n] != -1) break; // found path
		D /= 2;
	}
	path.clear();
	if (pathedge[G.n] != -1) {
                u = G.n; e = pathedge[u];
                while (e != Null) {
                        path.push(e); u = G.mate(u,e); e = pathedge[u];
                }
	}
	delete[] pathedge;
	return path(1) != Null;
}
