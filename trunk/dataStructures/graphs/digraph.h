// Header file for data structure representing directed graph.
// Same basic structure as graph, but incoming edges appear before
// outgoing edges in adjacency lists and we add a pointer to the
// first outgoing edge.

#ifndef DIGRAPH_H
#define DIGRAPH_H

#include "stdinc.h"
#include "misc.h"
#include "graph.h"

class digraph : public graph {
public:		digraph(int=26,int=100);
		digraph(const digraph&);
		~digraph();
	edge	firstIn(vertex) const;	// return first edge into v
	edge	firstOut(vertex) const;	// return first edge out of v
	edge	inTerm(vertex) const;	// terminator for u's input list
	edge	outTerm(vertex) const;	// terminator for u's output list
	vertex	tail(edge) const;	// return tail of e
	vertex	head(edge) const;	// return head of e
	edge	join(vertex,vertex);	// add edge from 1st to 2nd vertex
	void	sortAdjLists();		// sort adjacency lists
	void	rgraph(int,int,int);	// generate random digraph
	void	rdag(int,int,int);	// generate random acyclic digraph
	digraph&  operator=(const digraph&); // assignment
	friend	int operator>>(istream&,digraph&); // input digraph
	friend	ostream& operator<<(ostream&,const digraph&); // output digraph
protected:
	edge	*li;			// li[v] is last incoming edge of v
					// or Null if there is none
	int	virtual ecmp(edge, edge, vertex) const; // compare two edges
        void    makeSpace();  	          	// allocate dynamic storage
        void    virtual mSpace();            
        void    freeSpace();       	 	// free dynamic storage
        void    virtual fSpace();            
        void    copyFrom(const digraph&); 	// copy-in from given graph
        void    cFrom(const digraph&); 
	bool	virtual getEdge(istream&, edge&); // input an edge
        bool    virtual getGraph(istream&);      // IO helper functions
        void    virtual putGraph(ostream&) const;
	void	virtual shuffle(int*, int*);	// permute vertices, edges
};

// Return first edge incident to x.
inline edge digraph::firstIn(vertex v) const { 
	assert(1 <= v && v <= N);
	return fe[v];
}

// Return first edge incident to x.
inline edge digraph::firstOut(vertex v) const { 
	assert(1 <= v && v <= N);
	return (li[v] == Null ? fe[v] : edges[li[v]].rnxt);
}

// Return the terminator of v's input adjacency list
inline edge digraph::inTerm(vertex v) const {
	assert(1 <= v && v <= N);
	return (li[v] == Null ? fe[v] : edges[li[v]].rnxt);
}

// Return the terminator of v's output adjacency list
inline edge digraph::outTerm(vertex v) const {
	assert(1 <= v && v <= N);
	return Null;
}

// Return left endpoint of e.
inline vertex digraph::tail(edge e) const {
	assert(0 <= e && e <= M);
	return left(e);
}

// Return right endpoint of e.
inline vertex digraph::head(edge e) const { 
	assert(0 <= e && e <= M);
	return right(e);
}

#endif
