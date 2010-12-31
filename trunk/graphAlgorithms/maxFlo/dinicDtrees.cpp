#include "dinicDtrees.h"

dinicDtrees::dinicDtrees(flograph& G1, int& floVal) : G(&G1) {
	level  = new int[G->n()+1]; nextEdge = new int[G->n()+1];
	upEdge = new int[G->n()+1]; dt = new dtrees(G->n());

	for (vertex u = 1; u <= G->n(); u++) {
		dt->addcost(u,BIGINT);
		level[u] = nextEdge[u] = upEdge[u] = Null;
	}

	floVal = 0;
	while (newPhase()) {
        	while (findPath()) floVal += augment();
        }
	delete [] nextEdge; delete [] upEdge; delete [] level; delete dt;
}

bool dinicDtrees::findPath() {
// Find an augmenting path.
        vertex u, v; edge e;

	while (nextEdge[G->src()] != Null) {
		u = dt->findroot(G->src()); e = nextEdge[u];
		while (1) { // look for forward path
			if (u == G->snk()) return true;
			if (e == Null) { nextEdge[u] = Null; break; }
			v = G->mate(u,e);
			if (G->res(u,e) > 0 && level[v] == level[u] + 1
			    && nextEdge[v] != Null) {
				dt->addcost(u,G->res(u,e) - dt->c(u));
				dt->link(u,v); upEdge[u] = e;
				nextEdge[u] = e;
				u = dt->findroot(G->src()); e = nextEdge[u];
			} else {
				e = G->next(u,e);
			}
		}
		// prune dead-end
		for (e = G->first(u); e != G->term(u); e = G->next(u,e)) {
			v = G->mate(u,e);
			if (u != dt->p(v) || e != upEdge[v]) continue;
			dt->cut(v); upEdge[v] = Null;
			G->addFlow(v,e,(G->cap(v,e)-dt->c(v)) - G->f(v,e));
			dt->addcost(v,BIGINT - dt->c(v));
		}
	}
	return false;
}

int dinicDtrees::augment() {
// Add flow to the source-sink path defined by the path in the
// dynamic trees data structure
	vertex u; edge e; cpair p;

	p = dt->findcost(G->src());
	int flo = p.c;
	dt->addcost(G->src(),-flo);
	for (p = dt->findcost(G->src()); p.c == 0; p = dt->findcost(G->src())) {
		u = p.s; e = upEdge[u];
		G->addFlow(u,e,G->cap(u,e) - G->f(u,e));
		dt->cut(u);
		dt->addcost(u,BIGINT);
		upEdge[u] = Null;
	}
	return flo;
}

bool dinicDtrees::newPhase() {
// Prepare for a new phase. Return the number of edges in the source/sink
// path, or 0 if none exists.
	vertex u, v; edge e;
	list q(G->n());
	for (u = 1; u <= G->n(); u++) {
		nextEdge[u] = G->first(u);
		if (dt->p(u) != Null) { // cleanup from previous phase
			e = upEdge[u];
			G->addFlow(u,e,(G->cap(u,e)-dt->c(u)) - G->f(u,e));
			dt->cut(u); dt->addcost(u,BIGINT - dt->c(u));
			upEdge[u] = Null;
		}
		level[u] = G->n();
	}
	q &= G->src(); level[G->src()] = 0;
	while (!q.empty()) {
		u = q[1]; q <<= 1;
		for (e = G->first(u); e != G->term(u); e = G->next(u,e)) {
			v = G->mate(u,e);
			if (G->res(u,e) > 0 && level[v] == G->n()) {
				level[v] = level[u] + 1; q &= v;
				if (v == G->snk()) return true;
			}
		}
	}
	return false;
}
