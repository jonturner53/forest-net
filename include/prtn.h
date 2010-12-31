// Data structure to maintain a partition on positive integers
// Called disjoint set data structure in chapter 2 of Tarjan.
// Or union-find in other contexts.

#ifndef PRTN_H
#define PRTN_H

#include "stdinc.h"
#include "misc.h"

typedef int item;

class prtn {
public:		prtn(int=26);
		~prtn();
	void	clear();		// re-inititialize
	item	find(item);		// return root (canonical element)
	item	link(item,item);	// combine two sets
	int	findcount();		// return nfind
	friend	ostream& operator<<(ostream&, prtn&); // output partition
private:
	int	n;			// partition is on {1,...,n}
	struct	pnode {
	item	p;			// p[i] is parent of item i
	int	rank;			// rank[i] is rank of item i
	} *node;
	int	nfind;			// count number of find calls
	item	findroot(int);		// return root without compressing
};

inline int prtn::findcount() { return nfind; }

#endif
