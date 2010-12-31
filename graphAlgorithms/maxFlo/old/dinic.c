#include "stdinc.h"
#include "basic/flograph.h"
#include "basic/list.h"
#include "dinic.h"

dinicC::dinicC(flograph& G1) {
        vertex u; edge e; 

        G = &G1;
        level = new int[G->n+1];
        nextEdge = new int[G->n+1];
        list p(G->m);
	while (newphase()) {
        	while (findpath(1,p)) {
			augment(p);
		}
        }
        delete [] nextEdge; delete [] level;
        return;
}

int dinicC::newphase() {
// Prepare for new phase. Return the number of edges in the source/sink
// path, or 0 if none exists.
	vertex u,v; edge e;
	list q(G->n);

	for (u = 1; u <= G->n; u++) {
		level[u] = G->n; nextEdge[u] = G->first(u);
	}
	q &= 1; level[1] = 0;
	while (q(1) != Null) {
		u = q(1); q <<= 1;
		for (e = G->first(u); e != Null; e = G->next(u,e)) {
			v = G->mate(u,e);
			if (G->res(u,e) > 0 && level[v] == G->n) {
				level[v] = level[u] + 1; q &= v;
				if (v == G->n) return level[v];
			}
		}
	}
	return 0;
}

int dinicC::findpath(vertex u, list& p) {
// Find a shortest path with unused residual capacity.
// nextEdge[u] identifies the next edge that has a possibility
// of reaching the sink from u.
        vertex v; edge e;

	if (u == G->n) { p.clear(); return 1; }
	for (e = nextEdge[u]; e != Null; e = G->next(u,e)) {
		v = G->mate(u,e);
		if (G->res(u,e) == 0 || level[v] != level[u] + 1) continue;
		if (findpath(v,p)) {
			p.push(e); nextEdge[u] = e; return 1;
		}
	}
	nextEdge[u] = Null; return 0;
}

void dinicC::augment(list& p) {
	vertex u; edge e; flow f;

	u = 1; f = BIGINT;
	for (e = p(1); e != Null; e = p.suc(e)) {
		f = min(f,G->res(u,e)); u = G->mate(u,e);
	}
	u = 1;
	for (e = p(1); e != Null; e = p.suc(e)) {
		G->addflow(u,e,f); u = G->mate(u,e);
	}
}
