//#define NODEBUG

#include "stdinc.h"
#include "graph.h"

#define l(x) edges[x].l
#define r(x) edges[x].r
#define lnxt(x) edges[x].lnxt
#define rnxt(x) edges[x].rnxt

// Allocate and initialize dynamic storage for graph.
// Assumes mN and mM are already initialized.
void graph::makeSpace() {
	fe = new edge[mN+1]; edges = new gedge[mM+1];
	for (vertex u = 0; u <= mN; u++) fe[u] = Null;
	l(Null) = r(Null) = lnxt(Null) = rnxt(Null) = Null;
}
void graph::mSpace() { makeSpace(); }

// Construct a graph with space for mN vertices and mM1 edges.
// Number of vertices initialized to mN, number of edges to 0.
graph::graph(int mN1, int mM1) : mN(mN1), mM(mM1) {
	assert(mN >= 0 && mM >= 0);
	makeSpace();
	N = mN; M = 0;
}

// Free dynamic storage.
void graph::freeSpace() { delete [] fe; delete [] edges; }
void graph::fSpace() { graph::freeSpace(); }

graph::~graph() { freeSpace(); }

// Copy contents of G to *this.
void graph::copyFrom(const graph& G) {
	assert(mN >= G.N && mM >= G.M);
	N = G.N; M = G.M;
	for (vertex u = 1; u <= n(); u++) fe[u] = G.fe[u];
	for (edge e = 1; e <= m(); e++) {
		l(e) = G.l(e); r(e) = G.r(e);
		lnxt(e) = G.lnxt(e); rnxt(e) = G.rnxt(e);
	}
}
void graph::cFrom(const graph& G) {
	copyFrom(G);
}

// Construct a graph from a given graph G,
// allocating the same amount of space as in G.
graph::graph(const graph& G) : mN(G.mN), mM(G.mM) {
	assert(mN >= 0 && mM >= 0);
	makeSpace(); copyFrom(G);
}

void graph::resize(int n, int m) {
	if (n > mN || m > mM) {
		fSpace(); mN = n; mM = m; mSpace();
	}
}

graph& graph::operator=(const graph& G) {
	if (&G != this) { resize(G.n(),G.m()); cFrom(G); }
	return *this;
}

edge graph::join(vertex u, vertex v) {
// Join u and v with edge of given weight. Return edge number.
	assert(1 <= u && u <= N && 1 <= v && v <= N && M < mM);
	edge e = ++M;
	l(e) = u; r(e) = v;
	lnxt(e) = fe[u]; fe[u] = e;
	rnxt(e) = fe[v]; fe[v] = e;
	return e;
}

int graph::ecmp(edge e1, edge e2, vertex u) const {
// Compare two edges incident to the same endpoint u.
// Return -1 if u's mate in e1 is less than u's mate in e2.
// Return +1 if u's mate in e1 is greater than than u's mate in e2.
// Return  0 if u's mate in e1 is equal to its mate in e2.
	if (mate(u,e1) < mate(u,e2)) return -1;
	else if (mate(u,e1) > mate(u,e2)) return 1;
	else return 0;
}

void graph::sortAlist(vertex u) {
// Sort edges in u's adjacency list based on comparison order
// provided by ecmp().
	edge e; int j, k, p, c;

	if (first(u) == term(u)) return; // empty list

	int  *elist = new int[2*N+1]; // need up to 2*N edges for digraphs

	// make a list of edges incident to u
	k = 1;
	for (e = first(u); e != term(u); e = next(u,e)) {
		if (k > 2*N) fatal("graph::sortAlist: adjacency list to long");
		elist[k++] = e;
	}
	k--;

	// put edge list in heap-order using mate(u) as key
	for (j = k/2; j >= 1; j--) {
		// do pushdown starting at position j
		e = elist[j]; p = j;
		while (1) {
			c = 2*p;
			if (c > k) break;
			if (c+1 <= k && ecmp(elist[c+1],elist[c],u) > 0) c++;
			if (ecmp(elist[c],e,u) <= 0) break;
			elist[p] = elist[c]; p = c;
		}		
		elist[p] = e;
	}

	// repeatedly extract the edge with largest mate(u) from heap and
	// restore heap order
	for (j = k-1; j >= 1; j--) {
		e = elist[j+1]; elist[j+1] = elist[1];
		// now largest edges are in positions j+1,...,k
		// elist[1,...,j] forms a heap with edge having
		// largest mate(u) on top
		// pushdown from 1 in this restricted heap
		p = 1;
		while (1) {
			c = 2*p;
			if (c > j) break;
			if (c+1 <= j && ecmp(elist[c+1],elist[c],u) > 0) c++;
			if (ecmp(elist[c],e,u) <= 0) break;
			elist[p] = elist[c]; p = c;
		}		
		elist[p] = e;
	}
	// now elist is sorted by mate(u)

	// now rebuild links forming adjacency list for u
	if (l(elist[k]) == u) lnxt(elist[k]) = Null;
	else rnxt(elist[k]) = Null;
	for (j = k-1; j >= 1; j--) {
		if (l(elist[j]) == u) lnxt(elist[j]) = elist[j+1];
		else rnxt(elist[j]) = elist[j+1];
	}
	fe[u] = elist[1];

	delete [] elist;
}

void graph::sortAdjLists() {
// Sort all the adjacency lists.
	for (vertex u = 1; u <= N; u++) sortAlist(u);
}

void graph::bldadj() {
// Build adjacency lists and sort them.
	vertex u; edge e;
	for (u = 1; u <= n(); u++) fe[u] = Null;
	for (e = m(); e >= 1; e--) {
		lnxt(e) = fe[l(e)]; fe[l(e)] = e;
		rnxt(e) = fe[r(e)]; fe[r(e)] = e;
	}
	sortAdjLists();
}

bool graph::getEdge(istream& is, edge& e) {
// Read one edge from is, store it as edge e and increment e.
// Since for undirected graphs, edges appear on both adjacency lists,
// ignore an edge if its second vertex is larger than the first.
// Return true on success, false on error.
	vertex u, v;
	if (misc::cflush(is,'(') == 0 || !misc::getNode(is,u,N) ||
	    misc::cflush(is,',') == 0 || !misc::getNode(is,v,N) || 
	    misc::cflush(is,')') == 0) {
		return false;
	}
	if (u < 1 || u > N || v < 1 || v > N) return false;
	if (u < v) { 
		if (e > M) return false;
		l(e) = u; r(e) = v; e++; 
	}
	return true;
}

// Input graph. Allocate space only if necessary.
bool graph::getGraph(istream& is) {
	int nuN, nuM; vertex u,v;
	is >> nuN >> nuM;
	resize(nuN,nuM);
	N = nuN; M = nuM;
	int i; edge e = 1;
	for (i = 1; i <= 2*M; i++) {
		if (!getEdge(is,e)) return false;
	}
	if (e-1 != M) return false;
	bldadj();
	return true;
}
int operator>>(istream& is, graph& G) { return G.getGraph(is); }

// Output edge e with vertex u shown first. 
void graph::putEdge(ostream& os, edge e, vertex u) const {
	if (e == Null) {
		os << "Null";
	} else {
		os << "("; misc::putNode(os,u,N); 
		os << ","; misc::putNode(os,mate(u,e),N);
		os << ")";
	}
}

// Output graph
void graph::putGraph(ostream& os) const {
	int i; vertex u; edge e;
	os << N << " " << M << endl;
	for (u = 1; u <= n(); u++) {
		i = 0;
		for (e = first(u); e != term(u); e = next(u,e)) {
			putEdge(os,e,u); os << " ";
                        if ((++i)%5 == 0) os << endl;
                }
                if (i>0 && i%5 != 0) os << endl;
	}
	os << endl;
}
ostream& operator<<(ostream& os, const graph& G) { G.putGraph(os); return os; }

void graph::shuffle(int vp[], int ep[]) {
// Shuffle the vertices and edges according to the given permutations.
// More precisely, remap all vertices u to vp[u] and all edges e to ep[e].
	vertex u; edge e;
	edge *fe1 = new edge[N+1]; gedge *edges1 = new gedge[M+1];

	// re-map edge data
	for (e = 1; e <= M; e++) {
		l(e) = vp[l(e)]; r(e) = vp[r(e)];
		lnxt(e) = ep[lnxt(e)]; rnxt(e) = ep[rnxt(e)];
		edges1[ep[e]] = edges[e];
	}
	for (e = 1; e <= M; e++) edges[e] = edges1[e];

	// re-map edges and vertices both in fe[]
	for (u = 1; u <= N; u++) { fe1[vp[u]] = ep[fe[u]]; }
	for (u = 1; u <= N; u++) { fe[u] = fe1[u]; }

	delete [] fe1; delete [] edges1;
}

void graph::scramble() {
// Scamble the vertices and edges in the graph.
	int *vp = new int[N+1]; int *ep = new int[M+1];
	misc::genPerm(N,vp); misc::genPerm(M,ep);
	shuffle(vp,ep);
        sortAdjLists();
}

void graph::rgraph(int n, int m, int span) {
// Generate a random graph. The number of vertices is n.
// The number of generated edges is m. Span is the maximum distance
// separating edges and is interpreted circularly.
        int i, j, k, mm; vertex u, v;

        n = max(0,n); m = max(0,m);
	resize(n,m);
	N = n; M = 0;
	for (u = 1; u <= N; u++) { fe[u] = Null; }

        if (span < n/2) {
                // mm = # of candidate edges, m = # still to generate
                mm = n*span; m = min(m,mm);
                for (i = 0; m > 0; m--, i = j) {
                        // i is index of most recent edge generated
                        // j is index of new edge
                        k = randTruncGeo(double(m)/(mm-i),mm-((m+i)-1));
                        j = i + k;
                        u = (j-1)/span + 1;
                        v = u + (j - (u-1)*span);
                        if (v > n) v -= n;
                        join(u,v);
                }
        } else {
                mm = n*(n-1)/2; m = min(m,mm);
                for (i = 0; m > 0; m--, i = j) {
                        // i is index of most recent edge generated
                        // j is index of new edge
                        k = randTruncGeo(double(m)/(mm-i),mm-((m+i)-1));
                        j = i + k;
                        v = int(1 + (1+sqrt(1+8*(j-1)))/2);
                        u = v - (((v*(v-1)/2) - j) + 1);
                        join(u,v);
                }
        }
        sortAdjLists();
}

void graph::rbigraph(int n, int m, int span) {
// Generate a random bipartite graph on n vertices with m edges.
// Span limits the range of neighbors to which a given vertex connects
        int i,j,k,mm,n1,n2; vertex u,v;

	span = max(1,span); span = min(n,span);
        n = max(1,n); m = max(0,m); m = min(m,n*span);
        resize(n,m);

	n1 = n/2; n2 = n - n1;
        N = n; M = 0;
        mm = n1*span; // mm = # candidate edges, m = # still to be generated
        for (i = 0; m > 0; m--, i = j) {
                // i is index of most recent edge generated
                // j is index of new edge
                k = randTruncGeo(double(m)/(mm-i),mm-((m+i)-1));
		j = i + k;
                u = 1 + (j-1)/span;
		v = (u+n1) - (span/2) + (j%span);
		if (v <= n1) v += n2;
		if (v > n1+n2) v -= n2;
                join(u,v);
        }
        sortAdjLists();
}
