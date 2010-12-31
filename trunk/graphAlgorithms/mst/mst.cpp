// usage: mst method
//
// mst reads a graph from stdin, computes its minimum spanning tree
// using the method specified by the argument and then prints the graph
// and the mst.
//

#include "stdinc.h"
#include "wgraph.h"

extern void kruskal(wgraph&, wgraph&);
extern void prim(wgraph&, wgraph&);
extern void primF(wgraph&, wgraph&);
extern void rrobin(wgraph&, wgraph&);

main(int argc, char *argv[]) {
	wgraph G; cin >> G;
	wgraph T(G.n(),G.n()-1);
	
	if (argc != 2) fatal("usage: mst method");

	if (strcmp(argv[1],"kruskal") == 0)
		kruskal(G,T);
	else if (strcmp(argv[1],"prim") == 0)
		prim(G,T);
	else if (strcmp(argv[1],"primF") == 0)
		primF(G,T);
	else if (strcmp(argv[1],"rrobin") == 0)
		rrobin(G,T);
	else
		fatal("mst: undefined method");

	cout << G << endl << T;
	exit(0);
}
