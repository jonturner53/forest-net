#include "flograph.h"

#define l(x) edges[x].l
#define r(x) edges[x].r
#define lnxt(x) edges[x].lnxt
#define rnxt(x) edges[x].rnxt
#define flo(x) flod[x].flo
#define cpy(x) flod[x].cpy

// Allocate and initialize dynamic storage for graph.
// Assumes mN and mM are already initialized.
void flograph::makeSpace() {
        flod = new floData[mM+1];
}
void flograph::mSpace() { digraph::mSpace(); makeSpace(); }

// Construct a flograph with space for mN1 vertices and mM1 edges.
flograph::flograph(int mN1, int mM1, int s1, int t1) 
	: digraph(mN1, mM1), s(s1), t(t1) {
        assert(mN >= 1 && mM >= 0 && 1 <= s && s <= mN 
		&& 1 <= t && t <= mN && s != t);
	makeSpace();
}

// Free dynamic storage
void flograph::freeSpace() { delete [] flod; }
void flograph::fSpace() { freeSpace(); digraph::fSpace(); }
flograph::~flograph() { freeSpace(); }

// Copy contents of G to *this.
void flograph::copyFrom(const flograph& G) {
        assert(mN >= G.N && mM >= G.M);
        for (edge e = 1; e <= M; e++) {
		flo(e) = G.flo(e); cpy(e) = G.cpy(e);
	}
	s = G.s; t = G.t;
}
void flograph::cFrom(const flograph& G) {
	digraph::cFrom(G); copyFrom(G);
}

flograph::flograph(const flograph& G) : digraph(G) {
// Construct a flograph from a given flograph
	assert(mN >= 0 && mM >= 0);
	makeSpace(); copyFrom(G);
}

// Assignment operator
flograph& flograph::operator=(const flograph& G) {
        if (&G != this) { resize(G.n(),G.m()); cFrom(G); }
        return *this;
}

edge flograph::join(vertex u, vertex v) {
// Join u and v with edge of given weight. Return edge number.
	assert(1 <= u && u <= N && 1 <= v && v <= N && M < mM);
	edge e = digraph::join(u,v);
	flo(e) = 0;
	return e;
}

// Add to the flow from v on e. Return the new flow
flow flograph::addFlow(vertex v, edge e, flow ff) {
	if (tail(e) == v) {
		if (ff + flo(e) > cpy(e) || ff + flo(e) < 0) {
			fatal("addflow: requested flow outside allowed range");
		}
		flo(e) += ff;
		return flo(e);
	} else {
		if (flo(e) - ff < 0 || flo(e) - ff > cpy(e)) {
			fatal("addflow: requested flow outside allowed range");
		}
		flo(e) -= ff;
		return -flo(e);
	}
}

// Remove all edges from the graph
void flograph::clear() {
	for (vertex u = 1; u <= n(); u++) fe[u] = li[u] = Null;
	for (edge e = 1; e <= m(); e++) flo(e) = cpy(e) = 0;
	M = 0;
}

bool flograph::getEdge(istream& is, edge& e) {
// Read one edge from is, store it as edge e and increment e.
// Return true on success, false on error.
        vertex u, v; flow capp, ff; char c;
        if (misc::cflush(is,'(') == 0 || !misc::getNode(is,u,N) ||
            misc::cflush(is,',') == 0 || !misc::getNode(is,v,N) ||
            misc::cflush(is,',') == 0 || !(is >> capp) ||
            misc::cflush(is,',') == 0 || !(is >> ff) || 
	    misc::cflush(is,')') == 0)
		return false;
        if (u < 1 || u > N || v < 1 || v > N || e > M) return false;
	l(e) = u; r(e) = v; cpy(e) = capp; flo(e) = ff; e++;
        return true;
}

// Input flograph. Allocate space only if necessary.
bool flograph::getGraph(istream & is) {
        int nuN, nuM; vertex u,v;
        is >> nuN >> nuM;
        if (nuN > mN || nuM > mM) {
                fSpace(); mN = nuN; mM = nuM; mSpace();
        }
        N = nuN; M = nuM;

	vertex src, snk;
	if (!misc::getNode(is,src,N) || !misc::getNode(is,snk,N))
		return false;
	setSS(src,snk);

	edge e = 1;
	while (e <= M) {
		if (!getEdge(is,e)) return false;
        }
        bldadj();
        return true;
}
int operator>>(istream& is, flograph& G) { return G.getGraph(is); }

// Output an edge
void flograph::putEdge(ostream& os, edge e, vertex u) const {
        if (e == Null) {
                os << "Null";
	} else {
		os << "("; misc::putNode(os,u,N); 
		os << ","; misc::putNode(os,mate(u,e),N);
		os << "," << setw(2) << cap(u,e) 
		   << "," << setw(2) << f(u,e) << ")";
        }
}

// Output flograph - only print outgoing edges for each vertex
void flograph::putGraph(ostream& os) const {
	int i; vertex u; edge e;
	os << n() << " " << m() << " ";
	misc::putNode(os,src(),n()); os << " ";
	misc::putNode(os,snk(),n()); os << endl;
	for (u = 1; u <= n(); u++) {
		i = 0;
		for (e = firstOut(u); e != outTerm(u); e = next(u,e)) {
			putEdge(os,e,tail(e)); os << " ";
                        if ((++i)%5 == 0) os << endl;
                }
                if (i>0 && i%5 != 0) os << endl;
	}
	os << endl;
}

// Output operator
ostream& operator<<(ostream& os, const flograph& G) { G.putGraph(os); return os; }

void flograph::shuffle(int vp[], int ep[]) {
// Shuffle the vertices and edges according to the given permutations.
        edge e;
	floData *flod1 = new floData[M+1];

        digraph::shuffle(vp,ep);
        for (e = 1; e <= M; e++) flod1[ep[e]] = flod[e];
        for (e = 1; e <= M; e++) flod[e] = flod1[e];
	s = vp[s]; t = vp[t];

        delete [] flod1;
}

void flograph::rgraph(int n, int m, int mss, int span) {
// Generate random flow graph on n vertices with m edges.
// There is a "core" subgraph with n-2 nodes and m-2*mss edges generated 
// using digraph::rgraph. The source and sink are then added to this.
// The source has out-degree mss and the sink has in-degree mss.
// If mss>(n-2), it is first reduced to (n-2).
	int i;

	n = max(n,3);
	mss = max(1,mss); mss = min(mss,n-2); 
	m = max(2*mss,m);
	resize(n,m);
	digraph::rgraph(n-2,m-2*mss,span);
	N = n; s = n-1; t = n;
	fe[s] = li[s] = fe[t] = li[t] = Null;

	vertex *neighbors = new vertex[n-1];
	misc::genPerm(n-2,neighbors);
	for (i = 1; i <= mss; i++) join(s,neighbors[i]);

	misc::genPerm(n-2,neighbors);
	for (i = 1; i <= mss; i++) join(neighbors[i],t);
	sortAdjLists();
}

void flograph::randCap(flow ec1, flow ec2) {
// Generate random edge capacities.
	for (edge e = 1; e <= M; e++) {
		if (tail(e) == s || head(e) == t)
			changeCap(e,randint(1,2*ec1));
		else
			changeCap(e,randint(1,2*ec2));
	}
}
