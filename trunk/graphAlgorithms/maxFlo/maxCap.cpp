#include "maxCap.h"

maxCap::maxCap(flograph& G1,int& floVal) : augPath(G1,floVal) {
// Find maximum flow in G using the shortest augment path algorithm.
	floVal = 0;
	while(findPath()) floVal += augment(); 
}

bool maxCap::findPath() {
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
				if (v == G->snk()) return true;
				if (S.member(v))
					S.changekey(v,-bcap[v]);
				else
                                	S.insert(v,-bcap[v]);
                        }
                }
        }
	return false;
}
