// AugPath class. Encapsulates data and routines used by the augmenting
// path algorithms for max flow. This serves as a base class for
// specific variants of the agmenting path algorithm.

#ifndef AUGPATH_H
#define AUGPATH_H

#include "stdinc.h"
#include "flograph.h"
#include "list.h"
#include "dheap.h"

class augPath {
public: 
	augPath(flograph&,int&);
	~augPath();
protected:
        flograph* G;            // graph we're finding flow on
	edge *pEdge;		// pEdge[u] is edge to parent of u in spt

        int   	augment();	// add flow to augmenting path
	bool	findPath(); 	// find augmenting path
};

#endif
