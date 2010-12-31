// usage:
//	check
//
// Check reads two graphs from stdin, and checks to see
// if the second is a minimum spanning tree of the first.
// It prints a message for each discrepancy that it finds.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/list.h"
#include "basic/clist.h"
#include "basic/wgraph.h"
#include "basic/partition.h"

void check(wgraph&, wgraph&);
void verify(wgraph&, wgraph&);
void rverify(wgraph&, wgraph&, vertex, vertex, vertex*, clist&, vertex*, int*);
int max_wt(vertex, vertex, vertex*, vertex*);
void nca(wgraph&, wgraph&, vertex*, clist&);
void nca_search(wgraph&, wgraph&, vertex, vertex, vertex*,
	clist&, partition&, vertex*, int*);

main()
{
	wgraph G; G.get(stdin);
	wgraph T; T.get(stdin);
	check(G,T);
}

void check(wgraph& G, wgraph& T) {
// Verify that T is a minimum spanning tree of G.
	vertex u,v; edge e, f;

	// check that T is a subgraph of G
	if (T.n != G.n || T.m != T.n-1) {
		printf("check: size error, aborting\n");
		exit(1);
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
				printf("check: edge %d in T is not in G\n",f);
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
		printf("check: T does not reach all vertices\n");
		return;
	}
	// check that there is no cheaper spanning tree
	verify(G,T);
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
		if (m > G.w(e)) printf("mst violation: edge %d in G\n",e);
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
		
