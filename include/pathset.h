// Header file for path set data structure used to implement
// dynamic trees. Maintains a set of paths on nodes numbered
// {1,...,n}.

#ifndef PATHSET_H
#define PATHSET_H

#include "stdinc.h"
#include "misc.h"

typedef int path;		// path
typedef int node;		// node in path
typedef int cost;
struct cpair { node s; cost c;};// pair returned by findpathcost
struct ppair { path s1, s2;};	// pair returned by split

class pathset {
public: 	pathset(int);
		~pathset();
	path	findpath(node);		// return path containing node
	node	findtail(path);		// return tail of path
	cpair	findpathcost(path);	// return last min cost vertex and cost
	node	findtreeroot(node);	// return the root of the search tree
	void	addpathcost(path,cost);	// add to cost of every node in path
	path	join(path,node,path);	// join node with two paths
	ppair	split(node);		// split path on node & return set pair
	cost	c(node);		// cost of node - mainly for debugging
	friend	ostream& operator<<(ostream&, const pathset&); // print paths
	void	tprint(ostream&,path,int) const; // print single path as tree
	void	pprint(ostream&,path) const; // print a path as a path
protected:
	int	n;			// pathset defined on {1,...,n}
	struct pnode {
	node	left, right, p;		// left child, right child, parent
	cost	dcost, dmin;		// delta cost and delta min
	} *vec;
	node	splay(node);
	void	splaystep(node);
	void	lrotate(node);
	void	rrotate(node);
	void	print(ostream&,path,cost) const; // print single path
};

inline void pathset::pprint(ostream& os, path p) const { print(os,p,0); }

#endif
