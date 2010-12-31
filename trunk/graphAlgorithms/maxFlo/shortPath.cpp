#include "shortPath.h"

shortPath::shortPath(flograph& G1,int& floVal) : augPath(G1,floVal) {
// Find maximum flow in G using the shortest augment path algorithm.
	floVal = 0;
	while(findPath()) floVal += augment(); 
}

bool shortPath::findPath() {
// Find a shortest path with unused residual capacity.
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
