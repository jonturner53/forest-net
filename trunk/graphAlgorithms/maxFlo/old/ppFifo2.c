// Usage:
//	ppFifo2 reps n p maxcap span seed
//
// ppFifo2 generates repeatedly generates random graphs
// and then computes a maximum flow using the FIFO variant
// or the preflow-push algorithm
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

void ppFifo(flograph&);

int relabcnt, satPush, nonSatPush, nextEdgeSteps;
double avgRelabcnt, avgSatPush, avgNonSatPush, avgNextEdgeSteps;
int maxRelabcnt, maxSatPush, maxNonSatPush, maxNextEdgeSteps;

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
		fatal("usage: ppFifo2 reps n p maxcap span seed");

	srandom(seed);
	flograph G; 

	avgRelabcnt = avgSatPush = avgNonSatPush = avgNextEdgeSteps = 0;
	maxRelabcnt= maxSatPush= maxNonSatPush= maxNextEdgeSteps= 0;

	for (int i = 1; i <= reps; i++) {
		G.rgraph(n,p,maxcap,0,span);
		ppFifo(G);

		avgRelabcnt += relabcnt;
		avgSatPush += satPush;
		avgNonSatPush += nonSatPush;
		avgNextEdgeSteps += nextEdgeSteps;

		maxRelabcnt = max(maxRelabcnt,relabcnt); 
		maxSatPush = max(maxSatPush,satPush);
		maxNonSatPush = max(maxNonSatPush,nonSatPush);
		maxNextEdgeSteps = max(maxNextEdgeSteps,nextEdgeSteps);

		for (e = 1; e <= G.m; e++) {
			u = G.tail(e);
			G.addflow(u,e,-G.f(u,e));
		}
	}
	avgRelabcnt /= reps;
	avgSatPush /= reps;
	avgNonSatPush /= reps;
	avgNextEdgeSteps /= reps;
	printf("%5d %6.4f %5d %8.0f %8d %8.2f %8d %8.2f %8d %8.2f %8d\n",
		n, p, span, avgRelabcnt, maxRelabcnt,
		avgSatPush, maxSatPush, avgNonSatPush, maxNonSatPush,
		avgNextEdgeSteps, maxNextEdgeSteps);
}

void initdist(flograph&, int[]);
edge getnextedge(flograph&, vertex, edge, int[]);
int minlabel(flograph&, vertex, int []);

void ppFifo(flograph& G) {
// Find maximum flow in G using the fifo varaint of the preflow-push algorithm.
        int x;
	vertex u, v; edge e;
        int *d = new int[G.n+1];
        int *excess = new int[G.n+1];
        int *nextedge = new int[G.n+1];
        list queue(G.n);

        for (u = 1; u <= G.n; u++) {
                nextedge[u] = G.first(u);
                excess[u] = 0;
        }
        for (e = G.first(1); e != Null; e = G.next(1,e)) {
                v = G.mate(1,e);
                G.addflow(1,e,G.cap(1,e));
                if (v != G.n) {
                        queue &= v;
                        excess[v] = G.cap(1,e);
                }
        }
        initdist(G,d);
        relabcnt = satPush = nonSatPush = nextEdgeSteps = 0;
        while (queue(1) != Null) {
                u = queue(1); queue <<= 1;
                e = nextedge[u];
                while (excess[u] > 0) {
                        if (e == Null) {
                                d[u] = 1 + minlabel(G,u,d);
                                nextedge[u] = G.first(u);
                                queue &= u;
                                relabcnt++;
                                if (relabcnt % G.n == 0) {
                                        initdist(G,d);
                                        for (v = 1; v <= G.n; v++)
                                                nextedge[v] = G.first(v);
                                }
                                break;
                        } else if (G.res(u,e)==0 || d[u] != d[G.mate(u,e)]+1) {
                                e = nextedge[u] = getnextedge(G,u,e,d);
                        } else {
                                v = G.mate(u,e);
                                x = min(excess[u],G.res(u,e));
                                G.addflow(u,e,x);
                                excess[u] -= x; excess[v] += x;
                                if (v != 1 && v != G.n && !queue.mbr(v))
                                        queue &= v;
				if (G.res(u,e) > 0) nonSatPush++;
				else satPush++;
                        }
                }
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

edge getnextedge(flograph& G, vertex u, edge e, int d[]) {
// Look for next admissible edge from u or Null if none.
	while(e != Null) {
		nextEdgeSteps++;
		if (G.res(u,e) > 0 && d[u] == d[G.mate(u,e)] + 1)
			return e;
		e = G.next(u,e);
	}
	return e;
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
