#include "stdinc.h"
#include "prtn.h"
#include "wgraph.h"
#include "list.h"

void sortEdges(edge *elist, const wgraph G) {
// Sort edges according to weight, using heap-sort.
        int i, p, c; edge e; weight w;

        for (i = G.m()/2; i >= 1; i--) {
                // do pushdown starting at i
                e = elist[i]; w = G.w(e); p = i;
                while (1) {
                        c = 2*p;
                        if (c > G.m()) break;
                        if (c+1 <= G.m() && G.w(elist[c+1]) >= G.w(elist[c]))
				c++;
                        if (G.w(elist[c]) <= w) break;
			elist[p] = elist[c]; p = c;
                }
		elist[p] = e;
        }
        // now edges are in heap-order with largest weight edge on top

        for (i = G.m()-1; i >= 1; i--) {
		e = elist[i+1]; elist[i+1] = elist[1];
                // now largest edges in positions G.m(), G.m()-1,..., i+1
                // edges in 1,...,i form a heap with largest weight edge on top
                // pushdown from 1
                w = G.w(e); p = 1;
                while (1) {
                        c = 2*p;
                        if (c > i) break;
                        if (c+1 <= i && G.w(elist[c+1]) >= G.w(elist[c]))
				c++;
                        if (G.w(elist[c]) <= w) break;
			elist[p] = elist[c]; p = c;
                }
		elist[p] = e;
        }
}

void kruskal(wgraph& G, wgraph& T) {
// Find a minimum spanning tree of G using Kruskal's algorithm and
// return it in T.
	edge e, e1; vertex u,v,cu,cv; weight w;
	class prtn P(G.n());
	edge *elist = new edge[G.m()+1];
	for (e = 1; e <= G.m(); e++) elist[e] = e;
	sortEdges(elist,G);
	for (e1 = 1; e1 <= G.m(); e1++) {
		e = elist[e1];
		u = G.left(e); v = G.right(e); w = G.w(e);
		cu = P.find(u); cv = P.find(v);
		if (cu != cv) {
			 P.link(cu,cv); e = T.join(u,v); T.changeWt(e,w);
		}
	}
}

void kruskal(wgraph& G, list& T) {
// Find a minimum spanning tree of G using Kruskal's algorithm and
// return it in T. This version returns a list of the edges using
// the edge numbers in G, rather than a separate wgraph data structure.
	edge e, e1; vertex u,v,cu,cv; weight w;
	class prtn P(G.n());
	edge *elist = new edge[G.m()+1];
	for (e = 1; e <= G.m(); e++) elist[e] = e;
	sortEdges(elist,G);
	for (e1 = 1; e1 <= G.m(); e1++) {
		e = elist[e1];
		u = G.left(e); v = G.right(e); w = G.w(e);
		cu = P.find(u); cv = P.find(v);
		if (cu != cv) {
			 P.link(cu,cv); T &= e;
		}
	}
}
