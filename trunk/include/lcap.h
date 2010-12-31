// lcapc class encapsulates data and routines used by least cost aug path
// algorithm for min cost flow. Use constructor to invoke algorithm.

#ifndef LCAP_H
#define LCAP_H

#include "stdinc.h"
#include "wflograph.h"
#include "list.h"
#include "dheap.h"

class lcap {
public:
	lcap(wflograph&, flow&, cost&, bool);
protected:
	wflograph* G;		// graph we're finding flow on
	int*	lab;		// lab[u] is label used to transform costs
	edge*	pEdge;		// pEdge[u] is edge to parent of u in spt

	void pathRcapCost(flow&,cost&); // return path res cap and cost
	void augment(flow&); 		// add flow to augmenting path
	void initLabels();	// initialize labels for transformed costs
	bool findpath();	// find a least cost augmenting path
};

#endif
