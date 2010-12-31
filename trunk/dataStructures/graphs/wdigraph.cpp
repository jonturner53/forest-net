//#define NODEBUG

#include "wdigraph.h"

#define l(x) edges[x].l
#define r(x) edges[x].r
#define lnxt(x) edges[x].lnxt
#define rnxt(x) edges[x].rnxt

// Allocate and initialize dynamic storage for graph.
// Assumes mN and mM are already initialized.
void wdigraph::makeSpace() { lng = new length[mM+1]; }
void wdigraph::mSpace() { digraph::mSpace(); makeSpace(); }

// Construct a graph with space for N1 vertices and mM1 edges.
wdigraph::wdigraph(int mN1, int mM1) : digraph(mN1,mM1) {
        makeSpace();
}

// Free dynamic storage space
void wdigraph::freeSpace() { delete [] lng; }
void wdigraph::fSpace() { freeSpace(); digraph::fSpace(); }
wdigraph::~wdigraph() { freeSpace(); }

// Copy contents of G to *this.
void wdigraph::copyFrom(const wdigraph& G) {
        assert(mN >= G.N && mM >= G.M);
        for (edge e = 1; e <= M; e++) lng[e] = G.lng[e];
}
void wdigraph::cFrom(const wdigraph& G) {
	digraph::cFrom(G); copyFrom(G);
}

// Construct a copy of a graph from a given graph
wdigraph::wdigraph(const wdigraph& G) : digraph(G) {
        makeSpace(); copyFrom(G);
}

wdigraph& wdigraph::operator=(const wdigraph& G) {
        if (&G != this) { resize(G.n(),G.m()); cFrom(G); }
        return *this;
}

bool wdigraph::getEdge(istream& is, edge& e) {
// Read one edge from is, store it as edge e and increment e.
// Return true on success, false on error.
        vertex u, v; length ll; char c;
        if (misc::cflush(is,'(') == 0 || !misc::getNode(is,u,N) ||
            misc::cflush(is,',') == 0 || !misc::getNode(is,v,N) ||
            misc::cflush(is,',') == 0 || !(is >> ll) || 
	    misc::cflush(is,')') == 0)
		return false;
        if (u < 1 || u > N || v < 1 || v > N || e > M) return false;
        l(e) = u; r(e) = v; lng[e] = ll; e++;
        return true;
}

void wdigraph::putEdge(ostream& os, edge e, vertex u) const {
        if (e == Null) {
                os << "Null";
	} else {
		os << "("; misc::putNode(os,u,N);
		os << ","; misc::putNode(os,mate(u,e),N);
		os << "," << setw(2) << len(e) << ")";
        }
}

// IO operators
int operator>>(istream& is, wdigraph& G) { return G.getGraph(is); }
ostream& operator<<(ostream& os, const wdigraph& G)
{ G.putGraph(os); return os; }

void wdigraph::shuffle(int vp[], int ep[]) {
// Shuffle the vertices and edges according to the given permutations.
        int e;
        length *lng1 = new length[M+1];

        digraph::shuffle(vp,ep);

        for (e = 1; e <= M; e++) { lng1[ep[e]] = lng[e]; }
        for (e = 1; e <= M; e++) { lng[e] = lng1[e]; }

        delete [] lng1;
}

// Assign edges of G a random weight in [lo,hi]
void wdigraph::randLen(int lo, int hi) {
        for (edge e = 1; e <= M; e++) changeLen(e,randint(lo,hi));
}
