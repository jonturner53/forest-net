// Usage:
//	scale2 reps n p maxcap span seed
//
// scale2 generates repeatedly generates random graphs
// and then computes a maximum flow using the scaling
// algorithm
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

void scale(flograph&);
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
		fatal("usage: scale2 reps n p maxcap span seed");

	srandom(seed);
	flograph G; 
	maxPaths = 0; avgPaths = avgAvg = avgMax = 0;
	for (int i = 1; i <= reps; i++) {
		G.rgraph(n,p,maxcap,0,span);
		scale(G);
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

int D; // scaling parameter

void scale(flograph& G) {
// Find maximum flow in G using the scaling method.
	int i;
	vertex u; edge e; flow f;
	list p(G.m);

	for (e = 1; e <= G.m; e++) D = max(D,G.res(G.tail(e),e));
	for (i = 1; i <= D; i <<= 1) {}   D = (i >> 1);

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

int findpath(flograph& G, list& path) {
// Find a path with unused residual capacity.
	vertex u,v; edge e;
	edge *pathedge = new edge[G.n+1];
	list queue(G.n);

	while (D > 0) {
		for (u = 2; u <= G.n; u++) pathedge[u] = -1;
		pathedge[1] = Null; queue.clear(); queue &= 1;
		while (pathedge[G.n] == -1 && queue(1) != Null) {
			u = queue(1); queue <<= 1;
			for (e = G.first(u); e != Null; e = G.next(u,e)) {
				v = G.mate(u,e);
				if (G.res(u,e) >= D && pathedge[v] == -1) {
					pathedge[v] = e; queue &= v;
				}
			}
		}
		if (pathedge[G.n] != -1) break; // found path
		D /= 2;
	}
	path.clear();
	if (pathedge[G.n] != -1) {
                u = G.n; e = pathedge[u];
                while (e != Null) {
                        path.push(e); u = G.mate(u,e); e = pathedge[u];
                }
	}
	delete[] pathedge;
	return path(1) != Null;
}
