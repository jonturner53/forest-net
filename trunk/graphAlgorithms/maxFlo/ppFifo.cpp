#include "ppFifo.h"

ppFifo::ppFifo(flograph& G1, int& floVal, bool batch) : prePush(G1,floVal) {
// Find maximum flow in G using the fifo varaint of the 
// preflow-push algorithm. If the batch flag is set, then
// batch updating is done, instead of incremental updating.
	unbal = new list(G->n());
	// initialization
	vertex s = G->src();
        for (edge e = G->firstOut(s); e != G->outTerm(s); e = G->next(s,e)) {
                vertex v = G->head(e);
                if (v != G->snk()) (*unbal) &= v; 
        }

	vertex u;
	if (!batch) { // incremental relabeling
     		while (!unbal->empty()) {
	                u = (*unbal)[1]; (*unbal) <<= 1;
			if (!balance(u)) {
				d[u] = 1 + minlabel(u);
				nextedge[u] = G->first(u);
				(*unbal) &= u;
			}
		}
	} else { // batch relabeling
	     	while (!unbal->empty()) {
	     		while (!unbal->empty()) {
		                u = (*unbal)[1]; (*unbal) <<= 1; balance(u);
			}
	                initdist();
	                for (u = 1; u <= G->n(); u++) {
				if (u == G->src() || u == G->snk()) continue;
	                        nextedge[u] = G->first(u);
	                        if (excess[u] > 0) (*unbal) &= u;
	                }
	        }
	}

	floVal = flowValue();
	delete unbal;
}

void ppFifo::newUnbal(vertex u) { if (!unbal->mbr(u)) (*unbal) &= u; }

