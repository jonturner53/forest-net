#include "stdinc.h"
#include "basic/flograph.h"
#include "advanced/pathset.h"
#include "advanced/dtrees.h"
#include "basic/list.h"
#include "dinicDtrees.h"

dinicDtreesC::dinicDtreesC(flograph& G1) {
	vertex u;

	G = &G1;
	level = new int[G->n+1];
	nextEdge = new int[G->n+1];
	upEdge = new int[G->n+1];
	dt = new dtrees(G->n);

	for (u = 1; u <= G->n; u++) {
		dt->addcost(u,BIGINT);
		level[u] = nextEdge[u] = upEdge[u] = Null;
	}

	while (newphase()) {
        	while (findpath())
			augment();
        }
	delete [] nextEdge; delete [] upEdge; delete [] level; delete dt;
}

int dinicDtreesC::findpath() {
// Find an augmenting path.
        vertex u, v; edge e;

	while (nextEdge[1] != Null) {
		u = dt->findroot(1); e = nextEdge[u];
		while (1) {
			if (u == G->n) return 1;
			if (e == Null) { nextEdge[u] = Null; break; }
			v = G->mate(u,e);
			if (G->res(u,e) > 0 && level[v] == level[u] + 1
			    && nextEdge[v] != Null) {
				dt->addcost(u,G->res(u,e) - dt->c(u));
				dt->link(u,v); upEdge[u] = e;
				nextEdge[u] = e;
				u = dt->findroot(v); e = nextEdge[u];
			} else
				e = G->next(u,e);
		}
		for (e = G->first(u); e != Null; e = G->next(u,e)) {
			v = G->mate(u,e);
			if (u != dt->p(v) || e != upEdge[v]) continue;
			dt->cut(v); upEdge[v] = Null;
			G->addflow(v,e,(G->cap(v,e)-dt->c(v)) - G->f(v,e));
			dt->addcost(v,BIGINT - dt->c(v));
		}
	}
	return 0;
}

void dinicDtreesC::augment() {
// Add flow to the source-sink path defined by the path in the
// dynamic trees data structure
	vertex u; edge e; cpair p;

	p = dt->findcost(1);
	dt->addcost(1,-p.c);
	for (p = dt->findcost(1); p.c == 0; p = dt->findcost(1)) {
		u = p.s; e = upEdge[u];
		G->addflow(u,e,G->cap(u,e) - G->f(u,e));
		dt->cut(u); dt->addcost(u,BIGINT);
		upEdge[u] = Null;
	}
}

int dinicDtreesC::newphase() {
// Prepare for a new phase. Return the number of edges in the source/sink
// path, or 0 if none exists.
	vertex u, v; edge e;
	list q(G->n);
	for (u = 1; u <= G->n; u++) {
		nextEdge[u] = G->first(u);
		if (dt->p(u) != Null) { // cleanup from previous phase
			e = upEdge[u];
			G->addflow(u,e,(G->cap(u,e)-dt->c(u)) - G->f(u,e));
			dt->cut(u); dt->addcost(u,BIGINT - dt->c(u));
			upEdge[u] = Null;
		}
		level[u] = G->n;
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
