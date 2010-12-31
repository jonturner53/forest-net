#include "prePush.h"

prePush::prePush(flograph& G1, int& floVal) : G(&G1) {
// Find maximum flow in G using the preflow-push method.
// The base clase constructor initializes data used by all
// variants.

	excess = new int[G->n()+1];
	nextedge = new edge[G->n()+1];

	// initialization
	for (vertex u = 1; u <= G->n(); u++) {
		nextedge[u] = G->first(u); excess[u] = 0;
	}
	vertex s = G->src();
        for (edge e = G->firstOut(s); e != G->outTerm(s); e = G->next(s,e)) {
                vertex v = G->head(e);
                G->addFlow(s,e,G->cap(s,e));
                if (v != G->snk()) excess[v] = G->cap(s,e);
        }
	d = new int[G->n()+1];
	initdist();
	// constructor of derived class takes over from here
}

prePush::~prePush() { delete [] d; delete [] excess; delete [] nextedge; }

void prePush::newUnbal(vertex u) {
	fatal("prePush::newUnbal: execution should never reach here");
}

bool prePush::balance(vertex u) {
// Attempt to balance vertex u, by pushing flow through admissible edges.
	if (excess[u] <= 0) return true;
	while (true) {
		edge e = nextedge[u];
		if (e == Null) return false; 
		vertex v = G->mate(u,e);
		if (G->res(u,e) > 0 && d[u] == d[v]+1 && nextedge[v] != Null) {
			flow x = min(excess[u],G->res(u,e));
			G->addFlow(u,e,x);
			excess[u] -= x; excess[v] += x;
			if (v != G->src() && v != G->snk()) newUnbal(v);
			if (excess[u] <= 0) return true;
		}
		nextedge[u] = G->next(u,e);
	}
}

void prePush::initdist() {
// Compute exact distance labels and return in d.
// For vertices that can't reach t, compute labels to s.
	vertex u,v; edge e;
	list queue(G->n());

	for (u = 1; u < G->n(); u++) d[u] = 2*G->n();

	// compute distance labels for vertices that have path to sink
	d[G->snk()] = 0;
	queue &= G->snk();
	while (!queue.empty()) {
		u = queue[1]; queue <<= 1;
		for (e = G->first(u); e != G->term(u); e = G->next(u,e)) {
			v = G->mate(u,e);
			if (G->res(v,e) > 0 && d[v] > d[u] + 1) {
				d[v] = d[u] + 1;
				queue &= v;
			}
		}
	}

	if (d[G->src()] < G->n()) 
		fatal("initdist: path present from source to sink");

	// compute distance labels for remaining vertices
	d[G->src()] = G->n();
	queue &= G->src();
	while (!queue.empty()) {
		u = queue[1]; queue <<= 1;
		for (e = G->first(u); e != G->term(u); e = G->next(u,e)) {
			v = G->mate(u,e);
			if (G->res(v,e) > 0 && d[v] > d[u] + 1) {
				d[v] = d[u] + 1;
				queue &= v;
			}
		}
	}
}

int prePush::minlabel(vertex u) {
// Find smallest label on a vertex adjacent to v through an edge with
// positive residual capacity.
	int small; edge e;

	small = 2*G->n();
	for (e = G->first(u); e != G->term(u); e = G->next(u,e))
		if (G->res(u,e) > 0)
			small = min(small,d[G->mate(u,e)]);
	return small;
}

int prePush::flowValue() {
// Return the value of the flow leaving the source.
	int fv = 0; vertex s = G->src();
        for (edge e = G->first(s); e != G->term(s); e = G->next(s,e))
		fv += G->f(s,e);
	return fv;
}
