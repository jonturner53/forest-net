// usage: check
//
// Check reads a wflograph from stdin, with a flow and checks
// that its a legal maximum flow and has a minimum cost.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "wflograph.h"
#include "list.h"

main() {
	vertex u,v,w; edge e; int s;
	wflograph G;

	cin >> G;

	// Check that capacity constraints are respected.
	for (e = 1; e <= G.m(); e++) {
		u = G.tail(e); v = G.head(e);
		if (G.f(u,e) < 0)
			printf("Negative flow on edge %d=(%d,%d)\n",e,u,v);
		if (G.f(u,e) > G.cap(u,e))
			printf("Flow exceeds capacity on edge %d=(%d,%d)\n",
				e,u,v);
	}

	// Make sure each vertex is balanced.
	for (u = 2; u < G.n(); u++) { 
		s = 0;
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			if (u == G.head(e))
				s += G.f(G.tail(e),e);
			else
				s -= G.f(u,e);
		}
		if (s != 0)
			printf("Vertex %d is not balanced\n",u);
	}

	// Check that there is no augmenting path.
	int *d = new int[G.n()+1];
	for (u = 1; u <= G.n(); u++) d[u] = G.n();
	d[1] = 0;
	list q(G.n()); q &= 1;
	while (q[1] != Null) {
		u = q[1]; q <<= 1;
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.mate(u,e);
			if (G.res(u,e) > 0 && d[v] > d[u] + 1) {
				d[v] = d[u] + 1;
				q &= v;
			}
		}
	}
	if (d[G.n()] < G.n()) printf("Not a maximum flow\n");

	// Check that there are no negative cost cycles in residual graph.
	int** cst = new int*[G.n()+1];
	for (u = 1; u <= G.n(); u++) {
		cst[u] = new int[G.n()+1];
	}
	for (u = 1; u <= G.n(); u++) {
		for (v = 1; v <= G.n(); v++) {
			if (u == v) cst[u][v] = 0;
			else cst[u][v] = BIGINT;
		}
	}
	for (u = 1; u <= G.n(); u++) {
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.mate(u,e);
			if (G.res(u,e) > 0)
				cst[u][v] = min(cst[u][v],G.c(u,e));
		}
	}
	for (v = 1; v <= G.n(); v++) {
		if (cst[v][v] < 0) {
			printf("Vertex %2d on a negative cost cycle\n",v);
			exit(0);
		}
		for (u = 1; u <= G.n(); u++) {
			for (w = 1; w <= G.n(); w++) {
				if (cst[u][v] != BIGINT && cst[v][w] != BIGINT
				    && cst[u][w] > cst[u][v] + cst[v][w]) {
					cst[u][w] = cst[u][v] + cst[v][w];
				}
			}
		}
	}
}
