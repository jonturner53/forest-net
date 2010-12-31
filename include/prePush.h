// prePush class. Encapsulates data and routines used by the preflow-push
// algorithms for max flow. Users invoke the algorithm using the constructor
// with optional arguments to select different variants.

#ifndef PREPUSHC_H
#define PREPUSHC_H

#include "stdinc.h"
#include "flograph.h"
#include "list.h"

class prePush {
public: 
		prePush(flograph&, int&);
		~prePush();
protected:
        flograph* G;            // graph we're finding flow on
	int 	*d;		// vector of distance labels
	int 	*excess; 	// excess flow entering vertex
	edge 	*nextedge;	// pointer into adjacency list

	bool 	balance(vertex);// attempt to balance u
        void   	initdist(); 	// initialize distance labels
        int    	minlabel(vertex);// find smallest neighboring label
	void	virtual newUnbal(vertex); // handle new unbalanced vertex
	int	flowValue();	// return the value of the current flow
};

#endif
