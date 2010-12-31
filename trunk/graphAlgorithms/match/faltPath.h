// AltPath(G,M) class encapsulates data and methods use for alternating
// path algorithm for max size matching. Use constructor to invoke algorithm

#ifndef FALTPATH_H
#define FALTPATH_H

#include "stdinc.h"
#include "graph.h"
#include "list.h"
#include "dlist.h"

class faltPath {
public:
	faltPath(graph&,dlist&,int&);
private:
	graph* G;		// graph we're finding matching for
	dlist* M;		// matching we're building

	enum stype {odd, even};
	stype* state;		// odd/even status of vertices
	int* visit;		// visit[u]=# of most recent search to visit u
	edge* mEdge;		// mEdge[u]=matching edge incident to u
	edge* pEdge;		// pEdge[u]=edge to parent of u in forest
	dlist* free;		// list of free vertices
	list* leaves;		// list of leaves in current forest

	int	sNum;		// index of current search
	
	void augment(edge);	// augment the matching
	edge findPath();	// find an augmenting path
	edge expand(vertex);	// expand tree at given vertex
};

#endif
