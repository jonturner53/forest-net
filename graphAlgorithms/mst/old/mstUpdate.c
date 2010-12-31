// usage: mstUpdate n p maxWt repCount check seed
//
// mstUpdate generates a random graph on n vertices with edge probability p.
// It then computes a minimum spanning tree of the graph then repeatedly
// modifies the minimimum spanning tree by randomly changing the weight
// of one of the non-tree edges.
//
// If check != 0, each computed mst is checked for correctness.
// Seed is used to initialize the random number generator.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/partition.h"
#include "basic/wgraph.h"
#include "basic/list.h"
#include "basic/clist.h"

void buildpp(wgraph&, list&, edge*);
int mstUpdate(wgraph&, edge*, edge, int);
void kruskal(wgraph&, list&);
void check(wgraph&, wgraph&);

main(int argc, char* argv[]) {
	int i, n, maxWt, repCount, checkIt, retVal, seed;
	int notZero, minCyc, maxCyc, avgCyc;
	double p;
	edge e, modEdge, *pe;
	vertex u, v;
	wgraph G, *T2;


	if (argc != 7 ||
	    sscanf(argv[1],"%d",&n) != 1 ||
	    sscanf(argv[2],"%lf",&p) != 1 ||
	    sscanf(argv[3],"%d",&maxWt) != 1 ||
	    sscanf(argv[4],"%d",&repCount) != 1 ||
	    sscanf(argv[5],"%d",&checkIt) != 1 ||
	    sscanf(argv[6],"%d",&seed) != 1)
		fatal("usage: mstUpdate n p maxWt repCount check seed");

	srandom(seed);
	G.rgraph(n,p,maxWt,n);
	list T(G.m);
	kruskal(G,T);

	pe = new edge[G.m+1];
	buildpp(G,T,pe);

	notZero = 0; minCyc = G.n; maxCyc = 0; avgCyc = 0;
	for (i = 1; i <= repCount; i++) {
		do {
			e = randint(1,G.m);
		} while (pe[G.left(e)] == e || pe[G.right(e)] == e);
		retVal = mstUpdate(G,pe,e,randint(1,maxWt));
		if (retVal > 0) {
			notZero++;
			minCyc = min(retVal,minCyc);
			avgCyc += retVal;
			maxCyc = max(retVal,maxCyc);
		}
		if (checkIt) {
			T2 = new wgraph(G.n,G.n-1);
			for (u = 1; u <= G.n; u++) {
				e = pe[u];
				if (e != Null) {
					v = G.mate(u,e);
					T2->join(u,v,G.w(e));
				}
			}
			check(G,*T2);
			delete T2;
		}
	}

	printf("%6d %2d %8.2f %4d\n",
		notZero,minCyc,double(avgCyc)/notZero,maxCyc);
}

void buildpp(wgraph&G, list& T, edge *pe) {
// Given a weighted graph data structure G and a list of edges
// representing a spanning tree of G, compute a vector of
// parent pointers that represents the same tree. The parent
// pointer vector pe actually stores edge numbers (from G).
// The edges stored allow us to get to the "other" endpoint
// and its weight.
// 
	vertex u, v; edge e;
	list q(G.n);

	pe[1] = Null; for (u = 2; u <= G.n; u++) pe[u] = G.m+1;

	q &= 1;
	while (!q.empty()) {
		u = q(1); q <<= 1;
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			v = G.mate(u,e);
			if (pe[v] == G.m+1 && T.mbr(e)) {
				pe[v] = e; 
				if (!q.mbr(v)) q &= v;
			}
		}
	}
}

int mstUpdate(wgraph& G, edge *pe, edge modEdge, int nuWt) {
// Given a weighted graph G and an MST represented by a vector
// of parent pointers p, modify p to reflect the effect of changing
// the weight of modEdge to nuWt.
//
	static int n = 0; static char *mark;
	int cycleLen;
	vertex u, v, w, nca, top;
	edge e, bigLeft, bigRight, prevEdge;

	if (G.n > n) { // allocate larger mark vector
		if (n > 0) delete [] mark;
		n = G.n; mark = new char[n+1];
		for (u = 1; u <= n; u++) mark[u] = 0;
	}

	if (G.w(modEdge) <= nuWt) { G.changeWt(modEdge,nuWt); return 0; }

	G.changeWt(modEdge,nuWt);

	// find nca by following parent pointers
	u = G.left(modEdge); v = G.right(modEdge); 
	while (1) {
		if (u != v && !mark[u] && !mark[v]) {
			if (pe[u] != Null) { mark[u] = 1; u = G.mate(u,pe[u]); }
			if (pe[v] != Null) { mark[v] = 1; v = G.mate(v,pe[v]); }
			continue;
		}
		if (u == v) {
			nca = top = u;
		} else if (mark[u]) {
			nca = u; top = v;
		} else { // mark[v] == 1
			nca = v; top = u;
		}
		break;
	}
	
	// find largest weight edges, calculate cycle length and reset marks
	bigLeft = modEdge; cycleLen = 1;
	for (u = G.left(modEdge); u != nca; u = G.mate(u,pe[u])) {
		if (G.w(pe[u]) > G.w(bigLeft)) bigLeft = pe[u];
		mark[u] = 0; cycleLen++;
	}
	bigRight = modEdge;
	for (v = G.right(modEdge); v != nca; v = G.mate(v,pe[v])) {
		if (G.w(pe[v]) > G.w(bigRight)) bigRight = pe[v];
		mark[v] = 0; cycleLen++;
	}
	while (v != top) { mark[v] = 0; v = G.mate(v,pe[v]); } mark[v] = 0;

	// if no edge on cycle has larger weight than modEdge, return early
	if (bigLeft == bigRight) return cycleLen;
	
	// Reverse parent pointers
	if (G.w(bigLeft) > G.w(bigRight)) {
		if (pe[G.left(bigLeft)] == bigLeft) w = G.left(bigLeft);
		else w = G.right(bigLeft);
		u = G.left(modEdge); prevEdge = modEdge;
		while (u != w) {
			v = G.mate(u,pe[u]);
			e = pe[u]; pe[u] = prevEdge; prevEdge = e;
			u = v;
		}
		pe[w] = prevEdge;
	} else {
		if (pe[G.left(bigRight)] == bigRight) w = G.left(bigRight);
		else w = G.right(bigRight);
		v = G.right(modEdge); prevEdge = modEdge;
		while (v != w) {
			u = G.mate(v,pe[v]);
			e = pe[v]; pe[v] = prevEdge; prevEdge = e;
			v = u;
		}
		pe[w] = prevEdge;
	}
	return cycleLen;
}


void kruskal(wgraph& G, list& T) {
// Find a minimum spanning tree of G using Kruskal's algorithm and
// return the edges of the tree in the list, T.
	edge e; vertex u,v,cu,cv;
	weight w; partition P(G.n);
	G.esort();
	for (e = 1; e <= G.m; e++) {
		u = G.left(e); v = G.right(e); w = G.w(e);
		cu = P.find(u); cv = P.find(v);
		if (cu != cv) { P.link(cu,cv); T &= e; }
	}
}


void verify(wgraph&, wgraph&);
void rverify(wgraph&, wgraph&, vertex, vertex, vertex*, clist&, vertex*, int*);
int max_wt(vertex, vertex, vertex*, vertex*);
void nca(wgraph&, wgraph&, vertex*, clist&);
void nca_search(wgraph&, wgraph&, vertex, vertex, vertex*,
	clist&, partition&, vertex*, int*);

void check(wgraph& G, wgraph& T) {
// Verify that T is a minimum spanning tree of G.
	vertex u,v; edge e, f;

	// check that T is a subgraph of G
	if (T.n != G.n || T.m != T.n-1) {
		fatal("check: size error, aborting");
	}
	vertex* edgeTo = new vertex[T.n+1];
	for (u = 1; u <= G.n; u++) edgeTo[u] = 0;
	for (u = 1; u <= G.n; u++) {
		for (e = G.first(u); e != Null; e = G.next(u,e))
			edgeTo[G.mate(u,e)] = e;
		for (f = T.first(u); f != Null; f = T.next (u,f)) {
			v = T.mate(u,f);
			e = edgeTo[v];
			if (e == 0 || T.w(f) != G.w(e))
				fatal("check: edge in T is not in G");
		}
		for (e = G.first(u); e != Null; e = G.next(u,e))
			edgeTo[G.mate(u,e)] = 0;
	}

	// check that T reaches all the vertices
	int* mark = new int[T.n+1]; int marked;
	for (u = 1; u <= T.n; u++) mark[u] = 0;
	mark[1] = 1; marked = 1;
	list q(G.n); q &= 1;
	while (q(1) != Null) {
		u = q(1); q <<= 1;
		for (e = T.first(u); e != Null; e = T.next(u,e)) {
			v = T.mate(u,e);
			if (mark[v] == 0) {
				q &= v; mark[v] = 1; marked++;
			}
		}
	}
	if (marked != T.n) {
		fatal("check: T does not reach all vertices\n");
	}
	// check that there is no cheaper spanning tree
	verify(G,T);
	delete [] mark; delete[] edgeTo;
}

void verify(wgraph& G, wgraph& T) {
// Verify that there is no cheaper spanning tree than T.
	int i, m; vertex u, v; edge e;

	// Determine nearest common ancestor for each edge.
	vertex* first_edge = new edge[G.n+1];
	clist edge_sets(G.m);
	nca(G,T,first_edge,edge_sets);

	// Check paths from endpoints to nca, and compress.
	vertex* a = new vertex[T.n+1];  // a[u] is an ancestor of u
	int* mw = new int[T.n+1];	// mw[u] is max edge wt, between u, a[u]
	rverify(G,T,1,1,first_edge,edge_sets,a,mw);

	delete [] first_edge; delete [] a; delete [] mw;
}

void rverify(wgraph& G, wgraph& T, vertex u, vertex pu,
	    vertex first_edge[], clist& edge_sets, vertex a[], int mw[]) {
// Recursively verify the subtree rooted at u with parent pu.
	vertex v; edge e; int m;
	for (e = T.first(u); e != Null; e = T.next(u,e)) {
		v = T.mate(u,e);
		if (v == pu) continue;
		a[v] = u; mw[v] = T.w(e);
		rverify(G,T,v,u,first_edge,edge_sets,a,mw);
	}
	e = first_edge[u];
	if (e == Null) return;
	while (1) {
		m = max( max_wt(G.left(e),u,a,mw),
			 max_wt(G.right(e),u,a,mw) );
		if (m > G.w(e)) fatal("mst violation");
		e = edge_sets.suc(e);
		if (e == first_edge[u]) break;
	}
}

int max_wt(vertex u, vertex v, vertex a[], int mw[]) {
// Return the maximum weight of edges on path from u to v,
// defined by the ancestor pointers in a. Compress a & mw as a
// side effect.
	if (u == v) return 0;
	int m = max(mw[u],max_wt(a[u],v,a,mw));
	a[u] = v; mw[u] = m;
	return m;
}
		
void nca(wgraph& G, wgraph& T, vertex *first_edge, clist& edge_sets) {
// Compute the nearest common ancestors in T of edges in G.
// On return edge_sets contains a collection of lists, with two edges
// appearing on the same list if they have the same nearest common ancestor.
// On return, first_edge[u] is an edge for which u is the nearest common
// ancestor, or null if there is no such edge.
	partition npap(G.n);
	vertex *npa = new vertex[G.n+1];
	int *mark = new int[G.m+1];

	for (edge e = 1; e <= G.m; e++) mark[e] = 0;
	for (vertex u = 1; u <= G.n; u++) {
		first_edge[u] = Null; npa[u] = u;
	}
	nca_search(G,T,1,1,first_edge,edge_sets,npap,npa,mark);
	delete [] npa; delete [] mark;
}

void nca_search(wgraph& G, wgraph& T, vertex u, vertex pu, vertex first_edge[],
	clist& edge_sets, partition& npap, vertex npa[], int mark[]) {
	vertex v, w; edge e;

	for (e = T.first(u); e != Null; e = T.next(u,e)) {
		v = T.mate(u,e);
		if (v == pu) continue;
		nca_search(G,T,v,u,first_edge,edge_sets,npap,npa,mark);
		npap.link(npap.find(u),npap.find(v));
		npa[npap.find(u)] = u;
	}
	for (e = G.first(u); e != Null; e = G.next(u,e)) {
		v = G.mate(u,e);
		if (! mark[e]) mark[e] = 1;
		else {
			w = npa[npap.find(v)];
			edge_sets.join(e,first_edge[w]);
			first_edge[w] = e;
		}
	}
}
		
