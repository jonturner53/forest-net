#include "cycRed.h"

cycRed::cycRed(wflograph& G1, flow& floVal, cost& floCost) : G(&G1) {
// Find minimum cost, maximum flow in G using
// the cycle reduction algorithm. 
	vertex u;

	pEdge = new edge[G->n()+1];
	mark = new int[G->n()+1];

	dinicDtrees x(*G,floVal);

	while ((u = findCyc()) != Null) augment(u);

	floCost = 0;
	for (edge e = 1; e <= G->m(); e++) {
		vertex u = G->tail(e);
		floCost += G->f(u,e)*G->c(u,e);
	}
}

void cycRed::augment(vertex z) {
// Add flow to cycle defined by pEdge pointers that includes z.
	vertex u, v; edge e; flow f;

        // determine residual capacity of cycle
        f = BIGINT;
        u = z; e = pEdge[u];
        do {
                v = G->mate(u,e);
                f = min(f,G->res(v,e));
                u = v; e = pEdge[u];
        } while (u != z);

        // add flow to saturate cycle
        u = z; e = pEdge[u];
        do {
                v = G->mate(u,e);
                G->addFlow(v,e,f);
                u = v; e = pEdge[u];
        } while (u != z);
}

vertex cycRed::findCyc() {
// Find a negative cost cycle in the residual graph.
// If a cycle is found, return some vertex on the cycle,
// else Null. In the case there is a cycle, you can traverse
// the cycle by following pEdge pointers from the returned vertex.

	vertex u,v,last; edge e;
	int c[G->n()+1];
	list q(G->n());

	for (u = 1; u <= G->n(); u++) { 
		pEdge[u] = Null; c[u] = 0; q &= u; 
	}

	last = q.tail(); // each pass completes when last removed from q
	while (!q.empty()) {
		u = q[1]; q <<= 1;
		for (e = G->first(u); e != Null; e = G->next(u,e)) {
			if (G->res(u,e) == 0) continue;
			v = G->mate(u,e);
			if (c[v] > c[u] + G->c(u,e)) {
				pEdge[v] = e;
				c[v] = c[u] +  G->c(u,e);
				if (!q.mbr(v)) q &= v;
			}
		}

		if (u == last) {
			v = cycleCheck();
			if (v != Null) return v;
			last = q.tail();
		}
	}
	return Null;
}

vertex cycRed::cycleCheck() {
// If there is a cycle defined by the pEdge pointers, return
// some vertex on the cycle, else null.
	vertex u,v; edge e; int cm;

	for (u = 1; u <= G->n(); u++) { mark[u] = 0; }
	u = 1; cm = 1;
	while(u <= G->n()) {
		// follow parent pointers from u, marking new vertices
		// seen with the value of cm, so we can recognize a loop
		v = u; 
		while (mark[v] == 0) {
			mark[v] = cm;
			e = pEdge[v];
			if (e == Null) break;
			v = G->mate(v,e);
		}
		if (mark[v] == cm && e != Null) return v;
		
		// find next unmarked vertex 
		while (u <= G->n() && mark[u] != 0) u++;
		cm++;
	}
	return Null;
}
