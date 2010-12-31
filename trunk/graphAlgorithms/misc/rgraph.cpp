// usage: rgraph type n m span scram [..] seed
//
// Create a random graph on n vertices and m edges
// and print it. Type specifies what type of graph
// to generate (see below). Span is the max separation
// between vertices in the same edge. Scramble=1 if
// vertex numbers should be randomized, otherwise 0.
// Certain cases require additional arguments that are detailed
// below, along with the string that specifies the
// graph type.
// 
//     "graph"  n m span scram seed
//   "bigraph"  n m span scram seed
//    "wgraph" 	n m span scram lo hi seed
//  "wbigraph"  n m span scram lo hi seed
//   "digraph"  n m span scram seed
//       "dag"  n m span scram seed
//  "wdigraph" 	n m span scram lo hi seed
//      "wdag" 	n m span scram lo hi seed
//  "flograph"  n m span scram mss ecap1 ecap2 seed
// "wflograph" 	n m span scram mss ecap1 ecap2 lo hi seed
//
// For weighted graphs, [lo,hi] is the range of edge
// weights. These are distributed uniformly in the range.
// For flographs, mss is the number of edges from the source
// and to the sink, ecap1 is the average edge capacity of
// source/sink edges and ecap2 is the average edge capacity
// of all other edges.

#include "stdinc.h"
#include "wgraph.h"
#include "wdigraph.h"
#include "wflograph.h"

extern void rgraph(int, int, int, bool, graph&);
extern void randWt(int, int, wgraph&);
extern void rdigraph(int, int, int, bool, digraph&);
extern void randLeng(int, int, wdigraph&);
extern void rflograph(int, int, int, bool, flograph&);

main(int argc, char* argv[]) {
	int i, n, m, mss, span, scram, lo, hi, ecap1, ecap2, seed;
	char *gtyp = argv[1];

	if (argc < 7 || argc > 12 ||
	    sscanf(argv[2],"%d",&n) != 1 ||
	    sscanf(argv[3],"%d",&m) != 1 ||
	    sscanf(argv[4],"%d",&span) != 1 ||
	    sscanf(argv[5],"%d",&scram) != 1 ||
	    sscanf(argv[argc-3],"%d",&lo) != 1 ||
	    sscanf(argv[argc-2],"%d",&hi) != 1 ||
	    sscanf(argv[argc-1],"%d",&seed) != 1)
		fatal("usage: rgraph type n m span scram [..] seed");
	if (argc >= 10 && (
	    sscanf(argv[6],"%d",&mss) != 1 ||
	    sscanf(argv[7],"%d",&ecap1) != 1 ||
	    sscanf(argv[8],"%d",&ecap2) != 1))
		fatal("usage: rgraph type n m span scram [..] seed");

	srandom(seed);
	if (strcmp(gtyp,"graph") == 0 && argc == 7) {
		graph G(n,m);
		G.rgraph(n,m,span);
		if (scram) G.scramble();
		cout << G;
	} else if (strcmp(gtyp,"bigraph") == 0 && argc == 7) {
		graph bG(n,m);
		bG.rbigraph(n,m,span);
		if (scram) bG.scramble();
		cout << bG;
	} else if (strcmp(gtyp,"wgraph") == 0 && argc == 9) {
		wgraph wG(n,m);
		wG.rgraph(n,m,span);
		wG.randWt(lo,hi);
		if (scram) wG.scramble();
		cout << wG;
	} else if (strcmp(gtyp,"wbigraph") == 0 && argc == 9) {
		wgraph wbG(n,m);
		wbG.rbigraph(n,m,span);
		wbG.randWt(lo,hi);
		if (scram) wbG.scramble();
		cout << wbG;
	} else if (strcmp(gtyp,"digraph") == 0 && argc == 7) {
		digraph D(n,m);
		D.rgraph(n,m,span);
		if (scram) D.scramble();
		cout << D;
	} else if (strcmp(gtyp,"dag") == 0 && argc == 7) {
		digraph aD(n,m);
		aD.rdag(n,m,span);
		if (scram) aD.scramble();
		cout << aD;
	} else if (strcmp(gtyp,"wdigraph") == 0 && argc == 9) {
		wdigraph wD(n,m);
		wD.rgraph(n,m,span);
		wD.randLen(lo,hi);
		if (scram) wD.scramble();
		cout << wD;
	} else if (strcmp(gtyp,"wdag") == 0 && argc == 9) {
		wdigraph waD(n,m);
		waD.rdag(n,m,span);
		waD.randLen(lo,hi);
		if (scram) waD.scramble();
		cout << waD;
	} else if (strcmp(gtyp,"flograph") == 0 && argc == 10) {
		flograph F(n,m,1,2);
		F.rgraph(n,m,mss,span);
		F.randCap(ecap1,ecap2);
		if (scram) F.scramble();
		cout << F;
	} else if (strcmp(gtyp,"wflograph") == 0 && argc == 12) {
		wflograph wF(n,m,1,2);
		wF.rgraph(n,m,mss,span);
		wF.randCap(ecap1,ecap2);
		wF.randCost(lo,hi);
		if (scram) wF.scramble();
		cout << wF;
	} else 
		fatal("usage: rgraph type n m span scram [..] seed");
}
