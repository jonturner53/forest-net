//#define NODEBUG

#include "digraph.h"

#define l(x) edges[x].l
#define r(x) edges[x].r
#define lnxt(x) edges[x].lnxt
#define rnxt(x) edges[x].rnxt

// Allocate and initialize dynamic storage for graph.
// Assumes mN and mM are already initialized.
void digraph::makeSpace() {
        li = new edge[mN+1]; li[Null] = Null;
	for (vertex u = 1; u <= n(); u++) li[u] = Null;
}
void digraph::mSpace() { graph::mSpace(); makeSpace(); }

// Construct a graph with space for N1 vertices and mM1 edges.
digraph::digraph(int mN1, int mM1) : graph(mN1,mM1) { makeSpace(); }

// Free dynamic storage space
void digraph::freeSpace() { delete [] li; }
void digraph::fSpace() { freeSpace(); graph::fSpace(); }
digraph::~digraph() { freeSpace(); }

// Copy contents of G to *this.
void digraph::copyFrom(const digraph& G) {
        assert(mN >= G.N && mM >= G.M);
        for (vertex u = 1; u <= N; u++) li[u] = G.li[u];
}
void digraph::cFrom(const digraph& G) {
	graph::cFrom(G); copyFrom(G);
}

// Construct a copy of a graph from a given graph
digraph::digraph(const digraph& G) : graph(G) {
	makeSpace(); copyFrom(G);
}

// Assign one digraph to another.
digraph& digraph::operator=(const digraph& G) {
	if (&G != this) { resize(G.n(),G.m()); cFrom(G); }
	return *this;
}

edge digraph::join(vertex u, vertex v) {
// Join u and v with edge. Return edge number.
// The edge is directed from u to v.
	assert(1 <= u && u <= N && 1 <= v && v <= N && M < mM);
	edge e = ++M;
	l(e) = u; r(e) = v;
	rnxt(e) = fe[v]; fe[v] = e;
	if (li[v] == Null) li[v] = e;
	if (li[u] == Null) {
		lnxt(e) = fe[u]; fe[u] = e;
	} else  {
		lnxt(e) = rnxt(li[u]); rnxt(li[u]) = e;
	}
	return e;
}

int digraph::ecmp(edge e1, edge e2, vertex u) const {
// Compare two edges incident to the same endpoint u.
// Return -1 if e1 is an incoming edge of u and e2 an outgoing
// Return +1 if e1 is an outgoing edge of u and e2 is an incoming
// Otherwise
// Return -1 if u's mate in e1 is less than u's mate in e2.
// Return +1 if u's mate in e1 is greater than than u's mate in e2.
// Return  0 if u's mate in e1 is equal to its mate in e2.

	if (u == head(e1) && u == tail(e2)) return -1;
	else if (u == tail(e1) && u == head(e2)) return 1;

	if (mate(u,e1) < mate(u,e2)) return -1;
	else if (mate(u,e1) > mate(u,e2)) return 1;
	else return 0;
}

void digraph::sortAdjLists() {
// Sort all the adjacency lists and initialize li values.
	vertex u; edge e;
	for (u = 1; u <= n(); u++) {
		sortAlist(u);
		// now recompute li values
		li[u] = Null;
		for (e = first(u); e != term(u); e = next(u,e)) {
			if (u == head(e)) li[u] = e;
		}
	}
}

bool digraph::getEdge(istream& is, edge& e) {
// Read one edge from is, store it as edge e and increment e.
// Return true on success, false on error.
        vertex u, v;
        if (misc::cflush(is,'(') == 0 || !misc::getNode(is,u,N) || 
	    misc::cflush(is,',') == 0 || !misc::getNode(is,v,N) ||
            misc::cflush(is,')') == 0)
		return false;
        if (u < 1 || u > N || v < 1 || v > N || e > M) return false;
        l(e) = u; r(e) = v; e++;
        return true;
}

// Input graph. Allocate space only if necessary.
bool digraph::getGraph(istream & is) {
        int nuN, nuM; vertex u,v;
        is >> nuN >> nuM;
        if (nuN > mN || nuM > mM) {
                fSpace(); mN = nuN; mM = nuM; mSpace();
        }
        N = nuN; M = nuM;
	edge e = 1;
	while (e <= M) {
		if (!getEdge(is,e)) return false;
        }
        bldadj();
        return true;
}
int operator>>(istream& is, digraph& G) { return G.getGraph(is); }

// Output digraph - only print outgoing edges for each vertex
void digraph::putGraph(ostream& os) const {
	int i; vertex u; edge e;
	os << n() << " " << m() << endl;
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
ostream& operator<<(ostream& os, const digraph& G) {G.putGraph(os);return os;}

void digraph::shuffle(int vp[], int ep[]) {
// Shuffle the vertices and edges according to the given permutations.
        int e;
        edge *li1 = new edge[N+1];

	graph::shuffle(vp,ep);

        for (e = 1; e <= N; e++) { li1[vp[e]] = ep[li[e]]; }
        for (e = 1; e <= N; e++) { li[e] = li1[e]; }

        delete [] li1;
}

void digraph::rgraph(int n, int m, int span) {
// Generate a random digraph and return it in G. The number
// of vertices is n, The number of generated edges is m.
// Span is the maximum distance separating edges and is
// interpreted circularly.
	int i, j, k, mm; vertex u, v;

	n = max(0,n); m = max(0,m);
	resize(n,m);
	N = n; M = 0;
	for (u = 1; u <= N; u++) {fe[u] = li[u] = Null; }

	if (span < n/2) {
		mm = 2*n*span; m = min(m,mm);
		for (i = 0; m > 0; m--, i = j) {
			// i is index of most recent edge generated
			// j is index of new edge
			k = randTruncGeo(double(m)/(mm-i),mm-((m+i)-1));
			j = i + k; 
			u = (j-1)/(2*span) + 1; 
			if (j - 2*(u-1)*span > span) {
				v = u + (j - (2*u-1)*span);
				if (v > n) v -= n;
			} else {
				v = u - (((2*u-1)*span+1) - j);
				if (v < 1) v += n;
			}
			join(u,v);
		}
	} else {
		mm = n*(n-1); m = min(m,mm);
		for (i = 0; m > 0; m--, i = j) {
			// i is index of most recent edge generated
			// j is index of new edge
			k = randTruncGeo(double(m)/(mm-i),mm-((m+i)-1));
			j = i + k;
			u = (j-1)/(n-1) + 1;
			v = j - (u-1)*(n-1);
			if (v >= u) v++;
			join(u,v);
		}
	}
	sortAdjLists();
}

void digraph::rdag(int n, int m, int span) {
// Generate a random directed acyclic graph on n vertices with m edges.
// probability p. This is done by creating edges only from
// lower numbered vertices to higher numbered vertices.
// Edges are also restricted so that they join vertices whose indices are
// at most span apart from each other.
// Generate random edge costs uniformly in the interval [1,maxcost].
	int i, j, k, mm, x; vertex u, v;

	n = max(0,n); 
	span = max(0,span); span = min(n-1,span);
	x = int(0.5*span*(span-1));
	mm = (n-span)*span + x;
	m = max(0,m); m = min(m,mm);
	resize(n,m);
	N = n; M = 0;
	for (i = 0; m > 0; m--, i = j) {
		// i is index of most recent edge generated
		// j is index of new edge
		k = randTruncGeo(double(m)/(mm-i),mm-((m+i)-1));
		j = i + k;
		if (j <= x) {
			v = 1+int(0.5*(1+sqrt(double(1.0+8*(j-1)))));
			u = j - int(0.5*(v-1)*(v-2));
		} else {
			v = 1 + int(0.5*(span+1)*span + (j-1))/span;
			u = j + (v-1) - (x + span*(v-span));
		}
		join(u,v);
	}
	sortAdjLists();
}
