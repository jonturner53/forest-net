// AugPath class. Encapsulates data and routines used by the augmenting
// path algorithms for max flow. Users invoke algorithm by calling the
// constructor with the appropriate method specified.
// The constructor computes a maximum flow and updates the
// flow fields of the flograph's edges.

#ifndef AUGPATH_H
#define AUGPATH_H

#include "stdinc.h"
#include "flograph.h"
#include "list.h"
#include "dheap.h"

// methods for selecting augmenting paths
enum pathMethod { maxCap, scale, shortPath };

class augPath {
public: 
	augPath(flograph&,int&,pathMethod=maxCap);
private:
        flograph* G;            // graph we're finding flow on
	pathMethod method;	// method for this computation	

	edge *pEdge;		// pEdge[u] is edge to parent of u in spt
	flow	D;		// scale factor for scaling method

        int   	augment();	// add flow to augmenting path
	bool	findPath();	// find augmenting path
	bool	maxCapPath(); 	// find max capacity path
	bool	scalePath();  	// find "wide enough" path
	bool	shortestPath(); // find shortest path
};

#endif
