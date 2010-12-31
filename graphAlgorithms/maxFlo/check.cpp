// usage: check
//
// wave reads a flograph from stdin, with a flow and checks
// that its a legal maximum flow.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "flograph.h"
#include "list.h"

main() {
	vertex u,v; edge e; int sum;
	flograph G; cin >> G;

	// verify that capacity constraints are respected
	for (e = 1; e <= G.m(); e++) {
		u = G.tail(e); v = G.head(e);
		if (G.f(u,e) < 0)
			cout << "Negative flow on edge " 
			     << e << "=(" << u << "," << v << ")\n";
		if (G.f(u,e) > G.cap(u,e))
			cout << "Flow exceeds capacity on edge "
			     << e << "=(" << u << "," << v << ")\n";
	}

	// verify that flow at each node is balanced
	for (u = 1; u <= G.n(); u++) { 
		if (u == G.src() || u == G.snk()) continue;
		sum = 0;
		for (e = G.first(u); e != G.term(u); e = G.next(u,e)) {
			if (u == G.head(e))
				sum += G.f(G.tail(e),e);
			else
				sum -= G.f(u,e);
		}
		if (sum != 0)
		cout << "Vertex " << u << "is not balanced\n";
	}

	// Verify that the flow is maximum by computing hop-count distance
	// over all edges that are not saturated. If sink ends up with a
	// distance smaller than n, then there is a path to sink with
	// non-zero residual capacity.
	int *d = new int[G.n()+1];
	for (u = 1; u <= G.n(); u++) d[u] = G.n();
	d[G.src()] = 0;
	list q(G.n()); q &= G.src();
	while (!q.empty()) {
		u = q[1]; q <<= 1;
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.mate(u,e);
			if (G.res(u,e) > 0 && d[v] > d[u] + 1) {
				d[v] = d[u] + 1;
				q &= v;
			}
		}
	}
	if (d[G.snk()] < G.n()) printf("Not a maximum flow\n");
}
