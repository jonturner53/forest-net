#include "augPath.h"

augPath::augPath(flograph& G1, int& flow_value) : G(&G1) {
// Find maximum flow in G. Base class constructor initializes
// dynamic data common to all algorithms. Constructors
// for derived class actually implement algorithm.
	pEdge = new edge[G->n()+1];
}

augPath::~augPath() { delete [] pEdge; }

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
	fatal("augPathC::findPath(): this should never be called");
}
