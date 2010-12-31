// usage: match {size|weight} {bipartite|general} method
//
// Match reads a graph from stdin, computes a matching
// using the method specified by the argument and then prints the
// resulting matching.
//
// Methods currently implemented include
// 
//  size  bipartite 	altPath faltPath flowMatch
// weight bipartite 	flowMatch
//

#include "stdinc.h"
#include "dlist.h"
#include "wgraph.h"
#include "altPath.h"
#include "faltPath.h"
#include "edmonds.h"

extern void flowMatch(graph&,dlist&,int&);
extern void flowMatch(wgraph&,dlist&,int&,int&);

main(int argc, char *argv[]) {
	edge e; int i, mSize, mWeight;
	bool size, bipartite;
	graph G; wgraph wG;
	
	if (argc != 4)
		fatal("usage: match {size|weight} {bipartite|general} method");

	if (strcmp(argv[1],"size") == 0)  size = true;
	else if (strcmp(argv[1],"weight") == 0)  size = false;
	else fatal("usage: match {size|weight} {bipartite|general} method");

	if (strcmp(argv[2],"bipartite") == 0)  bipartite = true;
	else if (strcmp(argv[2],"general") == 0)  bipartite = false;
	else fatal("usage: match {size|weight} {bipartite|general} method");

	if ((size && (cin >> G)) || (!size && (cin >> wG))) {}
	else fatal("match: error reading graph from stdin");

	int n = (size ? G.n() : wG.n());
	int m = (size ? G.m() : wG.m());

	dlist M(m);

	if (size && bipartite) {
		if (strcmp(argv[3],"altPath") == 0) {
			altPath(G,M,mSize);
		} else if (strcmp(argv[3],"faltPath") == 0) {
			faltPath(G,M,mSize);
		} else if (strcmp(argv[3],"flowMatch") == 0) {
			flowMatch(G,M,mSize);
		} else {
			fatal("match: invalid method");
		}
	} else if (!size && bipartite) {
		if (strcmp(argv[3],"flowMatch") == 0) {
			flowMatch(wG,M,mSize,mWeight);
		} else {
			fatal("match: invalid method");
		}
	} else if (size) {
		if (strcmp(argv[3],"edmonds") == 0) {
			edmonds(G,M,mSize);
		} else {
			fatal("match: invalid method");
		}
	} else { // no algorithms for other cases (yet)
		fatal("match: invalid method");
	}
	cout << mSize << " edges in matching";
	if (!size) cout << " with total weight " << mWeight;
	cout << endl;

	if (G.n() > 100) exit(0); // don't print out really big matchings
	i = 0;
	for (e = M[1]; e != Null; e = M.suc(e)) {
		if (size) {
			cout << "("; misc::putNode(cout,G.left(e),n);
			cout << ","; misc::putNode(cout,G.right(e),n);
		} else {
			cout << "("; misc::putNode(cout,wG.left(e),n);
			cout << ","; misc::putNode(cout,wG.right(e),n);
			cout << "," << wG.w(e);
		}
		cout << ") ";
		if ((++i % 5) == 0) cout << endl;
	}
	cout << endl;
}
