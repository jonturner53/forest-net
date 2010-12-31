#include "augPath.h"

augPath::augPath(flograph& G1, int& flow_value, pathMethod method1)
		 : G(&G1), method(method1) {
// Find maximum flow in G using the one of several variants of
// the augmenting path method
	pEdge = new edge[G->n()+1];

	if (method == scale) {
		// initialize scale factor to largest power of 2
		// that is <= (max edge capacity)/2
		edge e; int maxCap;
		for (e = 1; e <= G->m(); e++) 
			maxCap = max(maxCap,G->cap(G->tail(e),e));
		for (D = 1; D <= maxCap/2; D *= 2) {}   
	}

	flow_value = 0;
	while(findPath()) flow_value += augment(); 
	delete [] pEdge;
}

int augPath::augment() {
// Saturate the augmenting path p.
	vertex u, v; edge e; flow f;

	// determine residual capacity of path
	f = BIGINT;
	u = G->snk(); e = pEdge[u];
	while (u != G->src()) {
		v = G->mate(u,e);
		f = min(f,G->res(v,e));
		u = v; e = pEdge[u];
	}
	// add flow to saturate path
	u = G->snk(); e = pEdge[u];
	while (u != G->src()) {
		v = G->mate(u,e);
		G->addFlow(v,e,f);
		u = v; e = pEdge[u];
	}
	return f;
}

bool augPath::findPath() {
// Find the augmenting path and return
	switch (method) {
	case maxCap: 	return maxCapPath();
	case scale:  	return scalePath();
	case shortPath:	return shortestPath();
	default: fatal("augPath: unrecognized method");
	}
}

bool augPath::maxCapPath() {
// Find a path with unused residual capacity.
        vertex u, v; edge e;
        dheap S(G->n(),2+G->m()/G->n()); int bcap[G->n()+1];

        for (u = 1; u <= G->n(); u++) { pEdge[u] = Null; bcap[u] = 0; }
        bcap[G->src()] = BIGINT;
        S.insert(G->src(),-BIGINT); // store negative values, 
				    // so deletemin gives max cap
        while (!S.empty()) {
                u = S.deletemin();
                for (e = G->first(u); e != G->term(u); e = G->next(u,e)) {
                        v = G->mate(u,e);
			if (min(bcap[u], G->res(u,e)) > bcap[v]) {
                                bcap[v] = min(bcap[u],G->res(u,e));
                                pEdge[v] = e;
				if (S.member(v))
					S.changekey(v,-bcap[v]);
				else
                                	S.insert(v,-bcap[v]);
                        }
                }
        }
	return pEdge[G->snk()] != Null;
}

bool augPath::scalePath() {
// Find a path with sufficient unused residual capacity.
	vertex u,v; edge e;
	list queue(G->n());

	while (D > 0) {
		for (u = 1; u <= G->n(); u++) pEdge[u] = Null;
		queue &= G->src();
		while (!queue.empty()) {
			u = queue[1]; queue <<= 1;
			for (e = G->first(u); e != G->term(u); e=G->next(u,e)) {
				v = G->mate(u,e);
				if (G->res(u,e) >= D && pEdge[v] == Null 
				    && v != G->src()) {
					pEdge[v] = e; 
					if (v == G->snk()) return true;
					queue &= v;
				}
			}
		}
		D /= 2;
	}
	return false;
}

bool augPath::shortestPath() {
// Find a path with unused residual capacity.
	vertex u,v; edge e;
	list queue(G->n());

	for (u = 1; u <= G->n(); u++) pEdge[u] = Null;
	queue &= G->src();
	while (!queue.empty()) {
		u = queue[1]; queue <<= 1;
		for (e = G->first(u); e != G->term(u); e = G->next(u,e)) {
			v = G->mate(u,e);
			if (G->res(u,e) > 0 && pEdge[v] == Null && 
			    v != G->src()) {
				pEdge[v] = e;
				if (v == G->snk()) return true;
				queue &= v;
			}
		}
	}
	return false;
}
