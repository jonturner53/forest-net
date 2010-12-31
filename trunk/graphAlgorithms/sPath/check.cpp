// usage:
//	check [src]
//
// check reads two graphs from stdin, and checks to see
// if the second is a shortest path tree of the first.
// It prints a message for each discrepancy that it finds.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "list.h"
#include "wdigraph.h"

void check(int, wdigraph&, wdigraph&);

main(int argc, char *argv[])
{
	int s = 1;
	if (argc == 2 && sscanf(argv[1],"%d",&s) != 1)
		fatal("usage: check [src]");
	wdigraph D; cin >> D; wdigraph T; cin >> T; check(s,D,T);
}

// generalize to use a specified source vertex.

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
