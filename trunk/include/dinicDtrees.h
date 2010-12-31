// DinicDtrees class. Encapsulates data and routines
// used to implement Dinic's algorithm with dynamic trees.
// Use constructor to invoke algorithm.

#ifndef DINICDTREES_H
#define DINICDTREES_H

#include "stdinc.h"
#include "flograph.h"
#include "pathset.h"
#include "dtrees.h"
#include "list.h"

class dinicDtrees {
public:	
		dinicDtrees(flograph&,int&);
private:
	flograph* G;		// graph we're finding flow on
	int*	nextEdge;	// pointer into adjacency list
	int*	upEdge;		// upEdge[u] is edge for dtrees link from u
	int*	level;		// level[u]=# of edges in path from source
	dtrees*	dt;		// dynamic trees data structure

	bool	findPath();	// find augmenting path
	int	augment();	// add flow to augmenting path
	bool	newPhase();	// prepare for a new phase
};

#endif
