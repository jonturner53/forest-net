// usage: ppFifo
//
// ppFifo reads a flograph from stdin, computes a maximum flow between
// vertices 1 and n using the fifo variant of the preflow-push algorithm.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/flograph.h"
#include "basic/list.h"

void ppFifo(flograph&);
void initdist(flograph&, int[]);
int minlabel(flograph&, vertex, int []);

main() {
        flograph G;
        G.get(stdin); ppFifo(G); G.put(stdout);
}

void ppFifo(flograph& G) {
// Find maximum flow in G using the fifo varaint of the preflow-push algorithm.
	int x; vertex u, v; edge e;
	int *d = new int[G.n+1];
	int *excess = new int[G.n+1];
	int *nextedge = new int[G.n+1];
	list queue(G.n);

	for (u = 1; u <= G.n; u++) {
		nextedge[u] = G.first(u);
		excess[u] = 0;
	}
        for (e = G.first(1); e != Null; e = G.next(1,e)) {
                v = G.head(e);
		if (v == 1) continue;
                G.addflow(1,e,G.cap(1,e));
                if (v != G.n) {
                        queue &= v; excess[v] = G.cap(1,e);
                }
        }
	initdist(G,d);
        while (queue(1) != Null) {
                u = queue(1); queue <<= 1;
                e = nextedge[u];
                while (excess[u] > 0) {
                        if (e == Null) {
                                d[u] = 1 + minlabel(G,u,d);
                                queue &= u;
                                break;
                        } 
                        v = G.mate(u,e);
			if (G.res(u,e) > 0 && d[u] == d[v]+1) {
                                x = min(excess[u],G.res(u,e));
                                G.addflow(u,e,x);
                                excess[u] -= x; excess[v] += x;
                                if (v != 1 && v != G.n && !queue.mbr(v))
                                        queue &= v;
                        } else {
                                e = G.next(u,e);
                        }
                }
		nextedge[u] = (e != Null ? e : G.first(u));
        }
}

void initdist(flograph& G, int d[]) {
// Compute exact distance labels and return in d.
// For vertices that can't reach t, compute labels to s.
	vertex u,v; edge e;
	list queue(G.n);

	for (u = 1; u < G.n; u++) d[u] = 2*G.n;
	d[G.n] = 0;
	queue &= G.n;
	while (queue(1) != Null) {
		u = queue(1); queue <<= 1;
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.mate(u,e);
			if (G.res(v,e) > 0 && d[v] > d[u] + 1) {
				d[v] = d[u] + 1;
				queue &= v;
			}
		}
	}

	if (d[1] < G.n) 
		fatal("initdist: path present from source to sink");

	d[1] = G.n;
	queue &= 1;
	while (queue(1) != Null) {
		u = queue(1); queue <<= 1;
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.mate(u,e);
			if (G.res(v,e) > 0 && d[v] > d[u] + 1) {
				d[v] = d[u] + 1;
				queue &= v;
			}
		}
	}
}

int minlabel(flograph& G, vertex u, int d[]) {
// Find smallest label on a vertex adjacent to v through an edge with
// positive residual capacity.
	int small; edge e;

	small = 2*G.n;
	for (e = G.first(u); e != Null; e = G.next(u,e))
		if (G.res(u,e) > 0)
			small = min(small,d[G.mate(u,e)]);
	return small;
}
