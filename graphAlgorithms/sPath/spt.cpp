// usage: spt method [src]
//
// Spt reads a graph from stdin, computes a shortest path tree (from src)
// using the method specified by the argument and then prints the graph
// and the spt. Src defaults to 1.
//

#include "stdinc.h"
#include "wdigraph.h"

extern void dijkstra(wdigraph&, vertex, vertex*, int*);
extern void bfScan(wdigraph&, vertex, vertex*, int*);

main(int argc, char *argv[]) {
	int s; wdigraph D; cin >> D;
	
	s = 1;
	if (argc < 2 || argc > 3 ||
	    argc == 3 && sscanf(argv[2],"%d",&s) != 1)
		fatal("usage: spt method [src]");

	int *p = new vertex[D.n()+1];
	int *d = new int[D.n()+1];

	if (strcmp(argv[1],"dijkstra") == 0)
		dijkstra(D,s,p,d);
	else if (strcmp(argv[1],"bfScan") == 0)
		bfScan(D,s,p,d);
	else
		fatal("spt: undefined method");

	wdigraph T(D.n(),D.n()-1);
	int sum = 0;
	for (vertex u = 1; u <= D.n(); u++) {
		if (p[u] != Null) {
			edge e = T.join(p[u],u); T.changeLen(e,d[u]);
			sum += (d[u] - d[p[u]]);
		}
	}
	T.sortAdjLists();
	cout << D << endl << T << endl;
	cout << "total cost=" << sum << endl;
	delete [] p; delete [] d;
	exit(0);
}
