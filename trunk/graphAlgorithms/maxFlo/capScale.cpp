#include "capScale.h"

capScale::capScale(flograph& G1, int& floVal) : augPath(G1,floVal) {
// Find maximum flow in G using the shortest augment path algorithm.
	// initialize scale factor to largest power of 2
	// that is <= (max edge capacity)/2
	edge e; int maxCap = 0;
	for (e = 1; e <= G->m(); e++) 
		maxCap = max(maxCap,G->cap(G->tail(e),e));
	for (D = 1; D <= maxCap/2; D *= 2) {}   
	floVal = 0;
	while(findPath()) floVal += augment(); 
}

bool capScale::findPath() {
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
