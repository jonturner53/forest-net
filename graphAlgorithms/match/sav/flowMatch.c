// flowMatch(G,M) - max matching on bipartite graphs
//
// This file contains two versions of flowMatch(G,M), the algorithm for
// computing a maximum matching in a bipartite graph using max flows.
// If the first argument is an unweighted graph, a maximum size
// matching is returned in M. If the first argument is a weighted
// graph, a maximum weight matching is returned.

#include "stdinc.h"
#include "wgraph.h"
#include "list.h"
#include "dlist.h"
#include "wflograph.h"
#include "dheap.h"
#include "dinic.h"
#include "lcap.h"

bool findCut(const graph&, list&);
void makFlograph(const graph&, const list&, flograph&);

void flowMatch(graph& G, dlist& M)  {
// Find a maximum size matching in the bipartite graph G and
// return it as a list of edges.
	vertex u,v; edge e;

	flograph *F = new flograph(G.n()+2,G.n()+G.m(),G.n()+1,G.n()+2);
	list *X = new list(G.n());

	if (!findCut(G,*X)) fatal("flowMatch: graph is not bipartite");
	makFlograph(G,*X,*F);

	flow floVal;
	dinic(*F,floVal);

	for (e = 1; e <= G.m(); e++) {
		if (F->f(G.left(e),e) != 0) M &= e;
	}
	delete F; delete X;
}

void flowMatch(wgraph& G, dlist& M) {
// Find a maximum weight matching in the bipartite graph G and
// return it as a list of edges.
	vertex u,v; edge e;

	wflograph *F = new wflograph(G.n()+2,G.n()+G.m(),G.n()+1,G.n()+2);
	list *X = new list(G.n());

	if (!findCut(G,*X)) fatal("floC: graph is not bipartite");
	makFlograph(G,*X,*F);
	for (e = 1; e <= G.m(); e++)  F->changeCost(e,-G.w(e));
	for (e = G.m()+1; e <= F->m(); e++) F->changeCost(e,0);

	flow floVal; cost floCost;
	lcap(*F,floVal,floCost,true);

	for (e = 1; e <= G.m(); e++) {
		if (F->f(G.left(e),e) != 0) M &= e;
	}
	delete F; delete X;
}

bool findCut(const graph& G, list& X) {
// Return true if G is bipartite, else false.
// If G is bipartite, return a set of vertices X that defines
// a cut that is crossed by all edges in G.
	vertex u, v, w; edge e;
	typedef enum stype { unreached, odd, even};
	stype state[G.n()+1];
	list q(G.n());

	for (u = 1; u <= G.n(); u++) state[u] = unreached;
	u = 1;
	while (u <= G.n()) {
		state[u] = even;
		q &= u; X &= u;
		while (q[1] != Null) {
			v = q[1]; q <<= 1;
			for (e = G.first(v); e != Null; e = G.next(v,e)) {
				w = G.mate(v,e);
				if (state[w] == state[v]) return false;
				if (state[w] == unreached) {
					state[w] = state[v]==even ? odd : even;
					if (state[w] == even) X &= w;
					q &= w;
				}
			}
		}
		// find next unreached vertex
		for (u++; u <= G.n() && state[u] != unreached; u++) {}
	}
	return true;
}

void makFlograph(const graph& G, const list& X, flograph& F) {
// Create flograph corresponding to G. Do this, so that corresponding
// edges have same edge numbers in F and G.
	vertex u, v; edge e, ee;

	for (e = 1; e <= G.m(); e++) {
		if (X.mbr(G.left(e))) u = G.left(e);
		else u = G.right(e);
		v = G.mate(u,e);
		edge ee = F.join(u,v);
		F.changeCap(ee,1);
	}
	for (u = 1; u <= G.n(); u++) {
		if (X.mbr(u)) e = F.join(F.src(),u);
		else e = F.join(u,F.snk());
		F.changeCap(e,1);
	}
}
