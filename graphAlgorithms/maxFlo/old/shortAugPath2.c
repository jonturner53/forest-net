// Usage:
//	shortAugPath2 reps n p maxcap span seed
//
// shortAugPath2 generates repeatedly generates random graphs
// and then computes a maximum flow using the shortest
// augmenting path algorithm
//
// Reps is the number of repeated computations,
// n is the number of vertices, p is the edge probability, 
// maxcap is the maximum edge capacity and span
// is the maximum difference beteen the endpoint indices of
// each edge.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/flograph.h"
#include "basic/list.h"

void shortAugPath(flograph&);
int findpath(flograph&, list&);

int maxPaths, maxPlen, nPath; 
double avgPaths, avgPlen, avgMax, avgAvg;

main(int argc, char* argv[])
{
	int reps, n, maxcap, span, seed;
	vertex u; edge e;
	double p;
	if (argc != 7 ||
	    sscanf(argv[1],"%d",&reps) != 1 ||
	    sscanf(argv[2],"%d",&n) != 1 ||
	    sscanf(argv[3],"%lf",&p) != 1 ||
	    sscanf(argv[4],"%d",&maxcap) != 1 ||
	    sscanf(argv[5],"%d",&span) != 1 ||
	    sscanf(argv[6],"%d",&seed) != 1)
		fatal("usage: shortAugPath2 reps n p maxcap span seed");

	srandom(seed);
	flograph G; 
	maxPaths = 0; avgPaths = avgAvg = avgMax = 0;
	for (int i = 1; i <= reps; i++) {
		G.rgraph(n,p,maxcap,0,span);
		shortAugPath(G);
		maxPaths = max(maxPaths, nPath);
		avgPaths += nPath;
		avgMax += maxPlen;
		avgAvg += avgPlen;
		for (e = 1; e <= G.m; e++) {
			u = G.tail(e);
			G.addflow(u,e,-G.f(u,e));
		}
	}
	avgPaths /= reps; avgMax /= reps; avgAvg /= reps;
	printf("%5d %6.4f %5d %8.0f %8d %8.2f %8.2f\n",
		n, p, span, avgPaths, maxPaths, avgAvg, avgMax);
}

void shortAugPath(flograph& G) {
// Find maximum flow in G using the shortest augmenting path method.
	int i;
	vertex u; edge e; flow f;
	list p(G.m);

	nPath = maxPlen = 0; avgPlen = 0;
	while (findpath(G,p)) {
		f = BIGINT;
		u = 1; i = 0;
		for (e = p(1); e != Null; e = p.suc(e)) {
			f = min(f,G.res(u,e)); u = G.mate(u,e);
			i++;
		}
		maxPlen = max(maxPlen,i);
		avgPlen += i;
		u = 1;
		for (e = p(1); e != Null; e = p.suc(e)) {
			G.addflow(u,e,f); u = G.mate(u,e);
		}
		nPath++;
	}
	avgPlen /= nPath;
	return;
}

int findpath(flograph& G, list& p) {
// Find a path with unused residual capacity.
	vertex u,v; edge e;
	vertex *parent = new vertex[G.n+1];
	edge *pathedge = new edge[G.n+1];
	int *dist = new int[G.n+1];
	int done = 0;
	list queue(G.n);

	for (u = 1; u <= G.n; u++) {
		parent[u] = Null; pathedge[u] = Null;
		dist[u] = BIGINT;
	}
	dist[1] = 0; queue &= 1;
	while (!done && queue(1) != Null) {
		u = queue(1); queue <<= 1;
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.head(e);
			if (G.res(u,e) > 0 && dist[v] > dist[u] +1) {
				parent[v] = u; pathedge[v] = e;
				dist[v] = dist[u]+1;
				queue &= v;
				if (v == G.n) { 
					done = 1; break;
				}
			}
		}
	}
	p.clear();
	for (u = G.n; parent[u] != Null; u = parent[u]) {
		p.push(pathedge[u]);
	}
	delete [] parent; delete [] pathedge; delete [] dist;
	return p(1) != Null;
}
