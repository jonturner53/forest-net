// Header file for wflograph data structure.
// Adds edge costs.

#ifndef WFLOGRAPH_H
#define WFLOGRAPH_H

#include "stdinc.h"
#include "misc.h"
#include "flograph.h"

typedef int cost;

class wflograph : public flograph {
public:		wflograph(int=26,int=100,int=1,int=2);
		wflograph(const wflograph&);
		~wflograph();
	flow	c(vertex,edge) const;		// edge cost
	void	changeCost(edge,cost);		// change cost of edge
	void	randCost(cost, cost);		// assign random edge costs
        wflograph& operator=(const wflograph&);         // assignment
        void    putEdge(ostream&,edge,vertex) const;    // output edge
        friend  int operator>>(istream&, wflograph&);   // IO operators
        friend  ostream& operator<<(ostream&, const wflograph&);
protected:
	flow	*cst;				// edge cost
        void    makeSpace();    		// allocate dynamic storage
        void    virtual mSpace();
        void    freeSpace();    		// release dynamic storage
        void    virtual fSpace();
        void    copyFrom(const wflograph&); 	// copy-in from given graph
        void    cFrom(const wflograph&);
        bool    getEdge(istream&,edge&);        // input an edge
	void	virtual shuffle(int*, int*);	// permute vertices, edges
};

inline flow wflograph::c(vertex v, edge e) const {
// Return cost from v on e.
	assert(1 <= v && v <= n() && 1 <= e && e <= m());
	return tail(e) == v ? cst[e] : -cst[e];
}

inline void wflograph::changeCost(edge e, cost cc) {
// Return first edge incident to x.
        assert(1 <= e && e <= M);
        cst[e] = cc;
}

#endif
