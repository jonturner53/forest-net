// usage:
//	mstRep method reps n m maxkey span
//
// mstRep repeatedly generates a random graph and computes
// its minimum spanning tree using the specified method.
// Reps is the number of repetitions.
// n is the number of vertices, m is the number of edges,
// maxkey is the maximum key and span is the maximum
// difference beteen the endpoint indices of each edge.
//

#include "stdinc.h"
#include <sys/times.h>
#include <unistd.h>
#include "wgraph.h"

extern void kruskal(wgraph&, wgraph&);
extern void prim(wgraph&, wgraph&);
extern void primF(wgraph&, wgraph&);
extern void rrobin(wgraph&, wgraph&);
extern void rgraph(int, int, int, bool, graph&);
extern void randWt(int, int, wgraph&);

main(int argc, char* argv[])
{
	int i, reps, n, m, maxkey, span;
	clock_t tick1, tick2;
	struct tms t1, t2;
	if (argc != 7 ||
	    sscanf(argv[2],"%d",&reps) != 1 ||
	    sscanf(argv[3],"%d",&n) != 1 ||
	    sscanf(argv[4],"%d",&m) != 1 ||
	    sscanf(argv[5],"%d",&maxkey) != 1 ||
	    sscanf(argv[6],"%d",&span) != 1)
		fatal("usage: mstRep method reps n p maxkey span");

	srand(1);
	wgraph G(n,m), *T;
	for (i = 1; i <= reps; i++) {
		G.rgraph(n,m,span); G.randWt(0,maxkey);
		T = new wgraph(n,n-1);

		tick1 = times(&t1);
		if (strcmp(argv[1],"kruskal") == 0)
			kruskal(G,*T);
		else if (strcmp(argv[1],"prim") == 0)
			prim(G,*T);
		else if (strcmp(argv[1],"primF") == 0)
			primF(G,*T);
		else if (strcmp(argv[1],"rrobin") == 0)
			rrobin(G,*T);
		else
			fatal("mstRep: undefined method");
		tick2 = times(&t2);
		cout << tick2 - tick1 << endl;

		delete T;
	}
}
