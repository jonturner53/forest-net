// cycRed class encapsulate data and routines used by cycle reduction
// algorithm for min cost flow. Users invoke algorithm with constructor.

#ifndef CYCRED_H
#define CYCRED_H

#include "stdinc.h"
#include "wflograph.h"
#include "list.h"
#include "pathset.h"
#include "dtrees.h"
#include "dinicDtrees.h"

class cycRed {
public: 	
		cycRed(wflograph&,flow&,cost&);
private:
	wflograph* G;		// graph we're finding flow on
	edge*	pEdge;		// pEdge[u] is edge to parent of u in spt
	int*	mark;		// used by cycleCheck

	void augment(vertex);	// add flow to a cycle
	vertex findCyc();	// find negative cost cycle
	vertex cycleCheck();	// check for cycle in pEdge pointers
};

#endif
