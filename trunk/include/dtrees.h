// Header file for dynamic trees data structure.
// Maintains a set of trees on nodes numbered {1,...,n}.

#ifndef DTREES_H
#define DTREES_H

#include "stdinc.h"
#include "misc.h"
#include "pathset.h"

typedef int tree;
struct pipair { path p; node i;};

class dtrees {
public: 	dtrees(int=100);
		~dtrees();
	path	findroot(node);		// return root of tree containing node
	cpair	findcost(path);		// return last least cost node on path
					// to root - return it and its cost
	void	addcost(node,cost);	// add to cost of nodes on path to root
	void	link(tree,node);	// join tree to tree containing node
	void	cut(node);		// cut edge to node's parent
	cost	c(node);		// cost of node - mainly for debugging
	node	p(node);		// logical parent
	friend	ostream& operator<<(ostream&, const dtrees&); // print trees
protected:
	int	n;			// pathset defined on {1,...,n}
	node	*pvec;			// pvec[i] is logical parent of i
	node	*svec;			// svec[i] is link to next path
	pathset *ps;			// underlying path set data structure
	
	path	expose(node);
	pipair	splice(pipair);
	void 	printpath(ostream& os, node i) const; // print one path & succ
};

inline node dtrees::p(node i) { return pvec[i]; }

inline node dtrees::c(node i) { return ps->c(i); }

#endif
