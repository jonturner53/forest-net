// Header file for class that implements Edmond's algorithm for
// finding a maximum size matching in a general graph. To use,
// invoke the constructor.

#ifndef EDMONDS_H
#define EDMONDS_H

#include "stdinc.h"
#include "graph.h"
#include "list.h"
#include "dlist.h"
#include "rlist.h"
#include "prtn.h"

class edmonds {
public: edmonds(graph&,dlist&,int&);
private:
	graph* G;		// graph we're finding matching for
	dlist* M;		// matching we're building
	prtn *blossoms;		// partition of the vertices into blossoms
	rlist* augpath;		// reversible list used to construct path
	vertex* origin;		// origin[u] is the original vertex
				// corresponding to a blossom
	struct evpair {		// for an odd vertex u inside a blossom,
	edge e; vertex v;	// bridge[u].e is the edge that caused the
	} *bridge;		// formation of the innermost blossom containg u
				// bridge[u].v identifies the endpoint of the
				// edge that is u's descendant
	enum stype {unreached, odd, even};
	stype *state;		// state used in computation path search
	edge* mEdge;		// mEdge[u] is matching edge incident to u
	edge* pEdge;		// p[u] is parent of u in forest
	bool* mark;		// used in nearest common ancestor computation
	
	vertex nca(vertex,vertex); // return nearest-common ancestor
	edge path(vertex,vertex); // construct augmenting path
	void augment(edge);	// augment the matching
	edge findpath();	// find an altmenting path
};

#endif
