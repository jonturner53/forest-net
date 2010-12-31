#include "lcap.h"

lcap::lcap(wflograph& G1, flow& floVal, cost& floCost, bool mostNeg) : G(&G1) {
// Find minimum cost, flow in G using the least-cost augmenting path
// algorithm.  If the mostNeg flag is true, the algorithm finds a
// flow with the largest negative cost. Otherwise, it finds a min
// cost, max flow.  Returns flow value in floVal and cost in floCost.

	lab = new int[G->n()+1];
	pEdge = new edge[G->n()+1];

	initLabels();
	floVal = floCost = 0;
	while (findpath()) {
		flow nuFlo; cost pathCost;
		pathRcapCost(nuFlo,pathCost);
		if (mostNeg && pathCost >= 0) break;
		augment(nuFlo);
		floVal += nuFlo; floCost += nuFlo*pathCost;
	}
	delete [] lab; delete [] pEdge;
}


void lcap::initLabels() {
// Compute values for labels that give non-negative transformed costs.
// The labels are the least cost path distances from an imaginary
// vertex with a length 0 edge to every vertex in the graph.
// Uses the breadth-first scanning algorithm to compute shortest
// paths.
        int pass;
	vertex u, v,last; edge e;
        list q(G->n());

        for (u = 1; u <= G->n(); u++) {
		pEdge[u] = Null; lab[u] = 0; q &= u;
	}
        pass = 0; last = q.tail();
        while (q[1] != Null) {
                u = q[1]; q <<= 1;
                for (e = G->first(u); e != Null; e = G->next(u,e)) {
                        v = G->head(e);
			if (v == u) continue;
                        if (lab[v] > lab[u] + G->c(u,e)) {
                                lab[v] = lab[u] + G->c(u,e); pEdge[v] = e;
                                if (!q.mbr(v)) q &= v;
                        }
                }
                if (u == last && q[1] != Null) { pass++; last = q.tail(); }
                if (pass == G->n()) fatal("initLabels: negative cost cycle");
        }
}

bool lcap::findpath() {
// Find a least cost augmenting path.
	vertex u,v; edge e;
	int c[G->n()+1]; dheap S(G->n(),4);

	for (u = 1; u <= G->n(); u++) { pEdge[u] = Null; c[u] = BIGINT; }
	c[G->src()] = 0; S.insert(G->src(),0);
	while (!S.empty()) {
		u = S.deletemin();
		for (e = G->first(u); e != Null; e = G->next(u,e)) {
			if (G->res(u,e) == 0) continue;
			v = G->mate(u,e);
			if (c[v] > c[u] + G->c(u,e) + (lab[u] - lab[v])) {
				pEdge[v] = e;
				c[v] = c[u] +  G->c(u,e) + (lab[u] - lab[v]);
				if (!S.member(v)) S.insert(v,c[v]);
				else S.changekey(v,c[v]);
			}
		}
	}
	// update labels for next round
	for (u = 1; u <= G->n(); u++) lab[u] += c[u];
	return (pEdge[G->snk()] != Null ? true : false);
}

void lcap::pathRcapCost(flow& rcap, cost& pc) {
// Return the residual capacity and cost of the path defined by pEdge
// in rcap and pc.
        vertex u, v; edge e;

	rcap = BIGINT; pc = 0;
        u = G->snk(); e = pEdge[u];
        while (u != G->src()) {
                v = G->mate(u,e);
                rcap = min(rcap,G->res(v,e)); pc += G->c(v,e);
                u = v; e = pEdge[u];
        }
}

void lcap::augment(flow& f) {
// Add f units of flow to the path defined by pEdge.
        vertex u, v; edge e;

        u = G->snk(); e = pEdge[u];
        while (u != G->src()) {
                v = G->mate(u,e);
                G->addFlow(v,e,f);
                u = v; e = pEdge[u];
        }
}
