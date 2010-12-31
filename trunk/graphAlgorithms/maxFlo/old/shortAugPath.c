// usage: shortAugPath
//
// shortAugPath reads a flograph from stdin, computes a maximum flow between
// vertices 1 and n using the shortest augmenting path algorithm.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/flograph.h"
#include "basic/list.h"

void shortAugPath(flograph&);
int findpath(flograph&, list&);

main() {
	flograph G;
	G.get(stdin); shortAugPath(G); G.put(stdout);
}

void shortAugPath(flograph& G) {
// Find maximum flow in G using the shortest augmenting path method.
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

int findpath(flograph& G, list& p) {
// Find a path with unused residual capacity.
	vertex u,v; edge e;
	vertex *parent = new vertex[G.n+1];
	edge *pathedge = new edge[G.n+1];
	int *dist = new int[G.n+1];
	int done = 0;
	list queue(G.n);

	for (u = 1; u <= G.n; u++) {
		parent[u] = Null; pathedge[u] = Null;
		dist[u] = BIGINT;
	}
	dist[1] = 0; queue &= 1;
	while (queue(1) != Null) {
		u = queue(1); queue <<= 1;
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.mate(u,e);
			if (G.res(u,e) > 0 && dist[v] > dist[u] +1) {
				parent[v] = u; pathedge[v] = e;
				dist[v] = dist[u]+1;
				queue &= v;
				if (v == G.n) {
					done = 1; break;
				}
			}
		}
	}
	p.clear();
	for (u = G.n; parent[u] != Null; u = parent[u]) {
		p.push(pathedge[u]);
	}
	delete [] parent; delete [] pathedge; delete [] dist;
	return p(1) != Null;
}
