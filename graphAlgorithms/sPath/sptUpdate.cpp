// usage: sptUpdate n m maxLen repCount seed
//
// sptUpdate generates a random graph on n vertices with m edges.
// It then computes a shortest path tree of the graph then repeatedly
// modifies the shortest path tree by randomly changing the weight
// of one of the edges. The edge lengths are distributed in [1,maxLen].
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "list.h"
#include "dheap.h"
#include "wdigraph.h"

void dijkstra(wdigraph&, vertex, vertex*, int*);
int sptUpdate(wdigraph&, vertex*, int*, edge, int);
void check(wdigraph&, wdigraph&);

main(int argc, char* argv[]) {
	int i, n, m, maxLen, repCount, retVal, seed;
	int notZero, minTsiz, maxTsiz, avgTsiz;
	edge e; vertex u, v;
	wdigraph G, *T;

	if (argc != 6 ||
	    sscanf(argv[1],"%d",&n) != 1 ||
	    sscanf(argv[2],"%d",&m) != 1 ||
	    sscanf(argv[3],"%d",&maxLen) != 1 ||
	    sscanf(argv[4],"%d",&repCount) != 1 ||
	    sscanf(argv[5],"%d",&seed) != 1)
		fatal("usage: sptUpdate n m maxLen repCount seed");

	srandom(seed);
	G.rgraph(n,m,n-1); G.randLen(0,maxLen);

	vertex *p = new int[n+1]; int *d = new int[n+1];
	dijkstra(G,1,p,d);

	notZero = 0; minTsiz = G.n(); maxTsiz = 0; avgTsiz = 0;
	for (i = 1; i <= repCount; i++) {
		e = randint(1,G.m());
		retVal = sptUpdate(G,p,d,e,randint(1,maxLen));
		if (retVal > 0) {
			notZero++;
			minTsiz = min(retVal,minTsiz);
			avgTsiz += retVal;
			maxTsiz = max(retVal,maxTsiz);
		}
		/* for verification only
		T = new wdigraph(G.n,G.n-1);
		for (e = 1; e <= G.m; e++) {
			u = G.tail(e); v = G.head(e);
			if (p[v] == u) T->join(u,v,d[v]);
		}
		check(1,G,*T);
		delete T;
		*/
	}

	cout << setw(6) << notZero << " " << setw(2) << minTsiz
	     << " " << (notZero > 0 ? double(avgTsiz)/notZero : 0.0)
	     << " " << setw(4) << maxTsiz;
}

int sptUpdate(wdigraph& G, vertex p[], int d[], edge e, int nuLen) {
// Given a graph G and an SPT T, update the SPT and distance vector to
// reflect the change in the weight of edge e to nuLen.
// Return 0 if no change is needed. Otherwise, return the number
// of vertices in the subtree affected by the update.
	vertex u, v, x, y; edge f;
	int oldLen, diff, tSiz;
	static int n=0; static dheap *S; static list *L;

	// Allocate new heap and list if necessary.
	// For repeated calls on same graph, this is only done once.
	if (G.n() > n) { 
		if (n > 0) { delete S; delete L; }
		n = G.n(); S = new dheap(G.n(),2); L = new list(G.n());
	}

	u = G.tail(e); v = G.head(e);
	oldLen = G.len(e); G.changeLen(e,nuLen);

	// case 1 - non-tree edge gets more expensive
	if (p[v] != u && nuLen >= oldLen) return 0;

	// case 2 - non-tree edge gets less expensive, but not enough
	// to change anything
	if (p[v] != u && d[u] + nuLen >= d[v]) return 0;

	// In the above two cases, nv=0 and the running time is O(1)
	// as required.

	// case 3 - edge gets less expensive and things change
	if (nuLen < oldLen) {
		// start Dijkstra's algorithm from v
		p[v] = u; d[v] = d[u] + nuLen;
		S->insert(v,d[v]);
		tSiz = 0;
		while (!S->empty()) {
			x = S->deletemin();
			for (f = G.firstOut(x); 
			     f != G.outTerm(x); f = G.next(x,f)) {
				y = G.head(f);
				if (d[y] > d[x] + G.len(f)) {
					d[y] = d[x] + G.len(f); p[y] = x;
					if (S->member(y)) S->changekey(y,d[y]);
					else S->insert(y,d[y]);
				}
			}
			tSiz++;
		}
		return tSiz;
	}
	// In case 3, the vertices affected by the update are those that
	// get inserted into the heap. Hence, the number of executions of
	// the outer loop is O(nv). The inner loop iterates over edges
	// incident to the vertices for which the distance changes, hence
	// the number of calls to changekey is O(mv). The d-heap parameter
	// is 2, which gives us an overall running time of O((mv log nv).

	// case 4 - tree edge gets more expensive

	// Put vertices in subtree into list.
	L->clear(); (*L) &= v; x = v; tSiz = 0;
	do {
		tSiz++;
		for (f = G.firstOut(x); f != G.outTerm(x); f = G.next(x,f)) {
			y = G.head(f);
			if (p[y] == x) {
				if (L->mbr(y)) {
					printf("u=%d v=%d x=%d y=%d\n",u,v,x,y);
					cout <<  "u=" << u << " v=" << v
					     << " x=" << x << " y=" << y 
					     << endl << L;
				}
				(*L) &= y;
			}
		}
		x = L->suc(x);
	} while (x != Null);

	// The time for the above loop is O(nv+mv)

	// Insert vertices in the subtree with incoming edges into heap.
	for (x = (*L)[1]; x != Null; x = L->suc(x)) {
		// find best incoming edge for vertex x
		p[x] = Null; d[x] = BIGINT;
		for (f = G.firstIn(x); f != G.inTerm(x); f = G.next(x,f)) {
			y = G.tail(f);
			if (!L->mbr(y) && d[y] + G.len(f) < d[x]) {
				p[x] = y; d[x] = d[y] + G.len(f);
			} 
		}
		if (p[x] != Null) S->insert(x,d[x]);
	}

	// The time for the above loop is  O(mv + nvlog nv)

	// Run Dijkstra's algorithm on the vertices in the subtree.
	while (!S->empty()) {
		x = S->deletemin();
		for (f = G.firstOut(x); f != G.outTerm(x); f = G.next(x,f)) {
			y = G.head(f);
			if (d[y] > d[x] + G.len(f)) {
				d[y] = d[x] + G.len(f); p[y] = x;
				if (S->member(y)) S->changekey(y,d[y]);
				else S->insert(y,d[y]);
			}
		}
	}
	// Same as earlier case. The outer loop is executed nv times,
	// the inner loop mv times. So the overall running time is
	// O(mv log nv).
}

void check(int s, wdigraph& D, wdigraph& T) {
// Verify that T is a shortest path tree of D.
	vertex u,v; edge e, f;

	// check size of T matches D
	if (T.n() != D.n() || T.m() != T.n()-1)
		fatal("spt_check: size error, aborting");

	// check that T is a subgraph of D
	for (v = 1; v <= T.n(); v++) {
		if (v == s) continue;
		f = T.firstIn(v);
		if (f == Null) 
			cout << "check: non-root vertex " << v
			     << " has no incoming edge\n";
		u = T.tail(f);
		for (e = D.firstIn(v); ; e = D.next(v,e)) {
			if (e == D.inTerm(v)) {
				cout << "check: edge (" << u << "," << v
				     << ") in T is not in D\n";
			}
			if (D.tail(e) == u) break;
		}
	}

	// check that T reaches all the vertices
	bool* mark = new bool[T.n()+1]; int marked;
	for (u = 1; u <= T.n(); u++) mark[u] = false;
	mark[s] = true; marked = 1;
	list q(D.n()); q &= s;
	while (!q.empty()) {
		u = q[1]; q <<= 1;
		for (e = T.firstOut(u); e != T.outTerm(u); e = T.next(u,e)) {
			v = T.head(e);
			if (!mark[v]) {
				q &= v; mark[v] = true; marked++;
			}
		}
	}
	if (marked != T.n()) {
		cout << "check: T does not reach all vertices\n";
		return;
	}

	// check that tree is minimum
	int du, dv;
	for (u = 1; u <= D.n(); u++) {
		du = T.firstIn(u) == Null ? 0 : T.len(T.firstIn(u));
		for (e = D.firstOut(u); e != Null; e = D.next(u,e)) {
			v = D.head(e);
			dv = T.firstIn(v) == Null ? 0 : T.len(T.firstIn(v));
			if (dv > du + D.len(e))
				cout << "check:(" << u << "," << v
				     << ") violates spt condition\n";
			if (T.firstIn(v) != Null && 
			    T.tail(T.firstIn(v)) == u && 
			    dv != du + D.len(e))
				cout << "check: tree edge (" << u << "," << v
				     << ") violates spt condition\n";
		}
	}
}
