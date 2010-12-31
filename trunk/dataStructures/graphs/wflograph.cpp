#include "wflograph.h"

#define l(x) edges[x].l
#define r(x) edges[x].r
#define lnxt(x) edges[x].lnxt
#define rnxt(x) edges[x].rnxt
#define flo(x) flod[x].flo
#define cpy(x) flod[x].cpy
#define cst(x) cst[x]

// Allocate and initialize dynamic storage for graph.
// Assumes mN and mM are already initialized.
void wflograph::makeSpace() {
        cst = new cost[mM+1];
}
void wflograph::mSpace() { flograph::mSpace(); wflograph::makeSpace(); }

// Construct a flograph with space for mN1 vertices and mM1 edges.
wflograph::wflograph(int mN1, int mM1, int s1, int t1)
    	: flograph(mN1,mM1,s1, t1) {
        assert(mN >= 1 && mM >= 0 && 1 <= s && s <= mN 
		&& 1 <= t && t <= mN && s != t);
        makeSpace();
}

// Free dynamic storage
void wflograph::freeSpace() { delete [] cst; }
void wflograph::fSpace() { freeSpace(); flograph::fSpace(); }
wflograph::~wflograph() { freeSpace(); }

// Copy contents of G to *this.
void wflograph::copyFrom(const wflograph& G) {
        assert(mN >= G.N && mM >= G.M);
        for (edge e = 1; e <= M; e++) {
                cst(e) = G.cst(e);
        }
}
void wflograph::cFrom(const wflograph& G) {
        flograph::cFrom(G); copyFrom(G);
}

wflograph::wflograph(const wflograph& G) : flograph(G) {
// Construct a wflograph from a given wflograph
        assert(mN >= 0 && mM >= 0);
        makeSpace(); copyFrom(G);
}

// Assignment operator
wflograph& wflograph::operator=(const wflograph& G) {
        if (&G != this) { resize(G.n(),G.m()); cFrom(G); }
        return *this;
}

bool wflograph::getEdge(istream& is, edge& e) {
// Read one edge from is, store it as edge e and increment e.
// Return true on success, false on error.
        vertex u, v; flow capp, ff; cost cc;
        if (misc::cflush(is,'(') == 0 || !misc::getNode(is,u,N) ||
            misc::cflush(is,',') == 0 || !misc::getNode(is,v,N) ||
            misc::cflush(is,',') == 0 || !(is >> capp) ||
            misc::cflush(is,',') == 0 || !(is >> ff) ||
            misc::cflush(is,',') == 0 || !(is >> cc) || 
	    misc::cflush(is,')') == 0) 
		return false;
        if (u < 1 || u > N || v < 1 || v > N || e > M) return false;
	l(e) = u; r(e) = v; cpy(e) = capp; flo(e) = ff; cst(e) = cc; e++;
        return true;
}

// Input operator
int operator>>(istream& is, wflograph& G) { return G.getGraph(is); }

void wflograph::putEdge(ostream& os, edge e, vertex u) const {
        if (e == Null) {
                os << "Null";
        } else {
                os << "("; misc::putNode(os,u,N); 
		os << ","; misc::putNode(os,mate(u,e),N);
                os << "," << setw(2) << cap(u,e) 
		   << "," << setw(2) << f(u,e) 
		   << "," << setw(2) << c(u,e) << ")";
        }
}

// Output operator
ostream& operator<<(ostream& os, const wflograph& G) {
        G.putGraph(os); return os;
}

void wflograph::shuffle(int vp[], int ep[]) {
// Shuffle the vertices and edges according to the given permutations.
        edge e; cost *cst1 = new cost[M+1];

        flograph::shuffle(vp,ep);
        for (e = 1; e <= M; e++) cst1[ep[e]] = cst(e);
        for (e = 1; e <= M; e++) cst(e) = cst1[e];
        delete [] cst1;
}

void wflograph::randCost(cost lo, cost hi) {
// Generate random edge costs.
        for (edge e = 1; e <= M; e++) changeCost(e,randint(lo,hi));
}
