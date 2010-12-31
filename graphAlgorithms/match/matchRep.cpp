// usage:
//	matchRep {size|weight} {bipartite|general} method reps n m maxwt seed
//
// matchRep repeatedly generates a random graph and computes
// a matching using the specified method. When bipartite
// graphs are specified, it generates bipartite graphs.
//
// Reps is the number of repetitions.
// n is the number of vertices, p is the edge probability,
// maxwt is the maximum edge weight parameter.

#include "stdinc.h"
#include "dlist.h"
#include "wgraph.h"
#include "altPath.h"
#include "faltPath.h"
#include "edmonds.h"

extern void flowMatch(graph&,dlist&,int&);
extern void flowMatch(wgraph&,dlist&,int&,int&);

main(int argc, char* argv[])
{
	edge e; int i, reps, n, m, mSize, mWeight, maxwt, seed;
	bool size, bipartite;
	graph G; wgraph wG;
	dlist *M;
	
	if (argc != 9 ||
	    sscanf(argv[4],"%d",&reps) != 1 ||
	    sscanf(argv[5],"%d",&n) != 1 ||
	    sscanf(argv[6],"%d",&m) != 1 ||
	    sscanf(argv[7],"%d",&maxwt) != 1 ||
	    sscanf(argv[8],"%d",&seed) != 1)
		fatal("usage: match {size|weight} {bipartite|general} method reps n p maxwt seed");

	if (strcmp(argv[1],"size") == 0)  size = true;
	else if (strcmp(argv[1],"weight") == 0)  size = false;
	else fatal("usage: match {size|weight} {bipartite|general} method");

	if (strcmp(argv[2],"bipartite") == 0)  bipartite = true;
	else if (strcmp(argv[1],"general") == 0)  bipartite = false;
	else fatal("usage: match {size|weight} {bipartite|general} method");

	srandom(seed);
	for (i = 1; i <= reps; i++) {
		M = new dlist(m);
		if (size && bipartite) { 
			G.rbigraph(n,m,n);
			if (strcmp(argv[3],"altPath") == 0) {
				altPath(G,*M,mSize);
			} else if (strcmp(argv[3],"faltPath") == 0) {
				faltPath(G,*M,mSize);
			} else if (strcmp(argv[3],"flowMatch") == 0) {
				flowMatch(G,*M,mSize);
			} else {
				fatal("match: invalid method");
			}
		} else if (!size && bipartite) {
			wG.rbigraph(n,m,n); wG.randWt(0,maxwt);
			if (strcmp(argv[3],"flowMatch") == 0) {
				flowMatch(wG,*M,mSize,mSize);
			} else {
				fatal("match: invalid method");
			}
		} else if (size & !bipartite) {
			G.rgraph(n,m,n);
			if (strcmp(argv[3],"edmonds") == 0) {
				edmonds(G,*M,mSize);
			} else {
				fatal("match: invalid method");
			}
		} else { // no algorithms for other cases (yet)
			fatal("match: invalid method");
		}
		delete M;
	}
}
