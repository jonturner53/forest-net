// usage: badCase n
//
// BadCase generates a weighted digraph that causes Dijkstra's
// shortest path algorithm to perform poorly, when
// started from vertex 1.
//
// The parameter n is the number of vertices.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "wdigraph.h"

main(int argc, char* argv[]) {
	vertex u,v; edge e; int n;

	if (argc != 2 || sscanf(argv[1],"%d",&n) != 1)
		fatal("usage badCase n");

	wdigraph D(n,n*n/2);

	for (u = 1; u <= n-1; u++) {
		e = D.join(u,u+1); D.changeLen(e,1);
		for (v = u+2; v <= n; v++) {
			e = D.join(u,v); D.changeLen(e,2*(n-u));
		}
	}
	D.sortAdjLists();
	cout << D;
}
