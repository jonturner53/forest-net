// Header file for data structure representing undirected graph with
// weighted edges.

#ifndef WGRAPH_H
#define WGRAPH_H

#include "stdinc.h"
#include "misc.h"
#include "graph.h"

typedef int weight;

class wgraph : public graph {
public:		wgraph(int=26,int=1000);
		wgraph(const wgraph&);
		virtual ~wgraph();
	weight	w(edge) const;		// return weight of e
	void	changeWt(edge,weight);	// change weight of e
	void	randWt(int,int);	// assign random edge weights
	wgraph&  operator=(const wgraph&); // assignment
        friend  int operator>>(istream&, wgraph&); // input graph
        void    virtual putEdge(ostream&,edge,vertex) const; // output edge
        friend  ostream& operator<<(ostream&, const wgraph&); // output graph
private:
	weight	*wt;			// edge weight

	bool	getEdge(vertex&,vertex&,weight&,istream&) const; // input edge 
	void	makeSpace();		// allocate space for wgraph
	void	virtual mSpace();
	void	freeSpace();		// free space for wgraph
	void	virtual fSpace();
	void	copyFrom(const wgraph&); // copy from given wgraph
	void	cFrom(const wgraph&); 
	bool	virtual getEdge(istream&,edge&); // input an edge
	void	virtual shuffle(int*, int*);	// permute vertices, edges
};

// Return weight of e;
inline edge wgraph::w(edge e) const {
        assert(1 <= e && e <= M);
        return wt[e];
}

inline void wgraph::changeWt(edge e, weight ww) {
	assert(1 <= e <= M);
	wt[e] = ww;
}

#endif
