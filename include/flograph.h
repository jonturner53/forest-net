// Header file for data structure representing directed graph with
// edge capacities and flows.

#ifndef FLOGRAPH_H
#define FLOGRAPH_H

#include "stdinc.h"
#include "misc.h"
#include "digraph.h"

typedef int flow;

class flograph : public digraph {
public:		flograph(int=26,int=100,int=1,int=2);
		flograph(const flograph&);
		virtual ~flograph();
	vertex	src() const;		// return source vertex
	vertex	snk() const;		// return sink vertex
	void	clear();		// remove all edges from the graph.
	flow	cap(vertex,edge) const;	// return capacity from v on e
	flow	f(vertex,edge) const;	// return flow from v on e
	flow	res(vertex,edge) const;	// return residual capacity from v on e
	flow	join(vertex,vertex); 	// join two vertices with an edge
	flow	addFlow(vertex,edge,flow); // add flow from v on e
	void	setSS(vertex, vertex);	// set the source and sink vertices
	void	changeCap(edge,flow);	// change capacity of edge
	void	randCap(flow, flow);	// assign random edge capacities
	void	rgraph(int, int, int, int); // generate a random graph
	flograph& operator=(const flograph&); 		// assignment
	void	putEdge(ostream&,edge,vertex) const; 	// output edge
	friend	int operator>>(istream&, flograph&); 	// IO operators
	friend	ostream& operator<<(ostream&, const flograph&);
protected:
	struct floData {
	flow	cpy;				// edge capacity
	flow	flo;				// flow on edge
	} *flod;
	vertex	s, t;				// source and sink vertices
	// various helper methods
        void    makeSpace();    		// allocate dynamic storage
        void    virtual mSpace();
        void    freeSpace();    		// allocate dynamic storage
        void    virtual fSpace();
        void    copyFrom(const flograph&); 	// copy-in from given graph
        void    cFrom(const flograph&);
        bool    getEdge(istream&, edge&);	// input an edge
        bool    getGraph(istream&);		// input a flograph
        void    putGraph(ostream&) const;	// output a flograph
	void    virtual shuffle(int*, int*);    // permute vertices, edges
};

inline vertex flograph::src() const { return s; }
inline vertex flograph::snk() const { return t; }

// Return capacity from v on e.
inline flow flograph::cap(vertex v, edge e) const { 
	assert(1 <= v && v <= n() && 1 <= e && e <= m());
	return tail(e) == v ? flod[e].cpy : 0;
}

// Return flow from v on e.
inline flow flograph::f(vertex v, edge e) const {
	assert(1 <= v && v <= n() && 1 <= e && e <= m());
	return tail(e) == v ? flod[e].flo : -flod[e].flo;
}

// Return residual capacity from v on e.
inline flow flograph::res(vertex v, edge e) const {
	assert(1 <= v && v <= n() && 1 <= e && e <= m());
	return tail(e) == v ? flod[e].cpy - flod[e].flo : flod[e].flo;
}

inline void flograph::changeCap(edge e, flow capp) {
// Change the capacity of edge e to capp
        assert(1 <= e && e <= M);
        flod[e].cpy = capp;
}

inline void flograph::setSS(vertex s1, vertex t1) {
// Set source and sink vertices.
	s = s1; t = t1;
}

#endif
