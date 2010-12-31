//#define NODEBUG

#include "wgraph.h"

#define l(x) edges[x].l
#define r(x) edges[x].r
#define lnxt(x) edges[x].lnxt
#define rnxt(x) edges[x].rnxt

// Allocate and initialize dynamic storage for graph.
// Assumes mN and mM are already initialized.
void wgraph::makeSpace() {
        wt = new weight[mM+1];
}
void wgraph::mSpace() { graph::mSpace(); makeSpace(); }

// Construct a graph with space for N1 vertices and mM1 edges.
wgraph::wgraph(int mN1, int mM1) : graph(mN1,mM1) { makeSpace(); }

// Free dynamic storage space
void wgraph::freeSpace() { delete [] wt; }
void wgraph::fSpace() { freeSpace(); graph::fSpace(); }
wgraph::~wgraph() { freeSpace(); }

// Copy contents of G to *this.
void wgraph::copyFrom(const wgraph& G) {
        assert(mN >= G.N && mM >= G.M);
	for (edge e = 1; e <= M; e++) wt[e] = G.wt[e];
}
void wgraph::cFrom(const wgraph& G) { graph::cFrom(G); copyFrom(G); }

// Construct a copy of a graph from a given graph
wgraph::wgraph(const wgraph& G) : graph(G) {
	makeSpace(); copyFrom(G);
}

// Assign one wgraph to another.
wgraph& wgraph::operator=(const wgraph& G) {
	if (&G != this) { resize(G.n(),G.m()); cFrom(G); }
	return *this;
}

bool wgraph::getEdge(istream& is, edge& e) {
// Read one edge from is and store it as edge e, and increment e.
// Return true on success, false on error.
	vertex u, v; weight ww;
        if (misc::cflush(is,'(') == 0 || !misc::getNode(is,u,N) ||
            misc::cflush(is,',') == 0 || !misc::getNode(is,v,N) ||
            misc::cflush(is,',') == 0 || !(is >> ww) || 
	    misc::cflush(is,')') == 0) 
		return false;
        if (u < 1 || u > N || v < 1 || v > N) return false;
        if (u < v) {
                if (e > M) return false;
                l(e) = u; r(e) = v; wt[e] = ww; e++;
        }
        return true;
}

int operator>>(istream& is, wgraph& G) { return G.getGraph(is); }

void wgraph::putEdge(ostream& os, edge e, vertex u) const {
	if (e == Null) {
		os << "Null";
	} else {
                os << "("; misc::putNode(os,u,N);
		os << ","; misc::putNode(os,mate(u,e),N);
		os << "," << setw(2) << w(e) << ")";
        }
}

// Output graph
ostream& operator<<(ostream& os, const wgraph& G) { G.putGraph(os); return os; }

void wgraph::shuffle(int vp[], int ep[]) {
// Shuffle the vertices and edges according to the given permutations.
        int e;
        vertex *wt1 = new weight[M+1];

	graph::shuffle(vp,ep);

        for (e = 1; e <= M; e++) { wt1[ep[e]] = wt[e]; }
        for (e = 1; e <= M; e++) { wt[e] = wt1[e]; }

        delete [] wt1;
}

// Assign edges of G a random weight in [lo,hi]
void wgraph::randWt(int lo, int hi) {
        for (edge e = 1; e <= M; e++) changeWt(e,randint(lo,hi));
}
