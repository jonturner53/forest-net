#include "dinic.h"

dinic::dinic(flograph& G1, int& floVal) : augPath(G1,floVal) {
        level = new int[G->n()+1];
        nextEdge = new edge[G->n()+1];

	floVal = 0;
	while (newPhase()) {
        	while (findPath(G->src())) floVal += augment();
        }
}

dinic::~dinic() { delete [] level; delete [] nextEdge; }

bool dinic::newPhase() {
// Prepare for new phase. Return true if there is a source/sink path.
	vertex u,v; edge e;
	list q(G->n());

	for (u = 1; u <= G->n(); u++) {
		level[u] = G->n(); nextEdge[u] = G->first(u);
	}
	q &= G->src(); level[G->src()] = 0;
	while (!q.empty()) {
		u = q[1]; q <<= 1;
		for (e = G->first(u); e != G->term(u); e = G->next(u,e)) {
			v = G->mate(u,e);
			if (G->res(u,e) > 0 && level[v] == G->n()) {
				level[v] = level[u] + 1; 
				if (v == G->snk()) return true;
				q &= v;
			}
		}
	}
	return false;
}

bool dinic::findPath(vertex u) {
// Find a shortest path with unused residual capacity.
        vertex v; edge e;

	for (e = nextEdge[u]; e != G->term(u); e = G->next(u,e)) {
		v = G->mate(u,e);
		if (G->res(u,e) == 0 || level[v] != level[u] + 1) continue;
		if (v == G->snk() || findPath(v)) {
			pEdge[v] = e; nextEdge[u] = e; return true;
		}
	}
	nextEdge[u] = Null; return false;
}
