// usage: check
//
// wave reads a flograph from stdin, with a flow and checks
// that its a legal maximum flow.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/flograph.h"
#include "basic/list.h"

main() {
	vertex u,v; edge e; int s;
	flograph G;

	G.get(stdin);

	for (e = 1; e <= G.m; e++) {
		u = G.tail(e); v = G.head(e);
		if (G.f(u,e) < 0)
			printf("Negative flow on edge %d=(%d,%d)\n",e,u,v);
		if (G.f(u,e) > G.cap(u,e))
			printf("Flow exceeds capacity on edge %d=(%d,%d)\n",e,u,v);
	}

	for (u = 2; u < G.n; u++) { 
		s = 0;
		for (e = G.first(u); e != Null; e = G.next(u,e))
			if (u == G.head(e))
				s += G.f(G.tail(e),e);
			else
				s -= G.f(u,e);
			if (s != 0)
			printf("Vertex %d is not balanced\n",u);
	}

	int *d = new int[G.n+1];
	for (u = 1; u <= G.n; u++) d[u] = G.n;
	d[1] = 0;
	list q(G.n); q &= 1;
	while (q(1) != Null) {
		u = q(1); q <<= 1;
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.mate(u,e);
			if (G.res(u,e) > 0 && d[v] > d[u] + 1) {
				d[v] = d[u] + 1;
				q &= v;
			}
		}
	}
	if (d[G.n] < G.n) printf("Not a maximum flow\n");
}
