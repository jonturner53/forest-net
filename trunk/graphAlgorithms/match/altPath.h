// AltPath(G,M) class encapsulates data and methods use for alternating
// path algorithm for max size matching. Use constructor to invoke algorithm

#ifndef ALTPATH_H
#define ALTPATH_H

#include "stdinc.h"
#include "graph.h"
#include "list.h"
#include "dlist.h"

class altPath {
public:
	altPath(graph&,dlist&,int&);
private:
	graph* G;		// graph we're finding matching for
	dlist* M;		// matching we're building
	edge* pEdge;		// pEdge[u] is edge to parent of u in forest
	
	void augment(edge);	// augment the matching
	edge findPath();	// find an altmenting path
};

#endif
