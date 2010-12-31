// Header file for data structure representing undirected graph.

#ifndef GRAPH_H
#define GRAPH_H

#include "stdinc.h"
#include "misc.h"

typedef int vertex;
typedef int edge;
struct vertex_pair { vertex v1, v2; };

class graph {
public:		graph(int=26,int=100);
		graph(const graph&);
		virtual ~graph();
	int	n() const;		// number of vertices 
	int	m() const;		// number of edges
	edge	first(vertex) const;	// return first edge incident to v
	edge	next(vertex,edge) const;// return next edge of v, following e
	edge	term(vertex) const;	// terminator for adjacency list
	vertex	left(edge) const;	// return left endpoint of e
	vertex	right(edge) const;	// return right endpoint of e
	vertex	mate(vertex,edge) const;// return other endpoint of e
	edge	join(vertex,vertex); 	// join two vertices with an edge
	graph&	operator=(const graph&); // assignment
	friend 	int operator>>(istream&, graph&); // input graph
	void	virtual putEdge(ostream&,edge,vertex) const; // output edge
	friend  ostream& operator<<(ostream&, const graph&); // output graph
	void	virtual sortAdjLists();	 // sort adjacency lists by mate
	void	rgraph(int, int, int);	// generate random graph
	void	rbigraph(int,int,int);	// generate random bipartite graph
	void	scramble();		// randomize vertex and edge numbers
protected:
	int	N, M;			// number of vertices and edges
	int	mN, mM;			// max number of vertices and edges
	struct gedge {
	vertex	l;			// l is left endpoint of edge
	vertex	r;			// r is right endpoint of edge
	edge	lnxt;			// lnxt is next edge incident to l
	edge	rnxt;			// rnxt is next edge incident to r
	} *edges;
	edge	*fe;			// fe[v] is first edge incident to v

	int	virtual ecmp(edge, edge, vertex) const;	// compare two edges
	void	sortAlist(vertex);	// sort one adjacency list
	void	virtual bldadj();	// build adjacency lists
	bool 	virtual getEdge(istream&, edge&); // input edge 
	void	makeSpace();			// allocate dynamic storage
	void	virtual mSpace();
	void	freeSpace();			// free dynamic storage
	void	virtual fSpace();
	void	copyFrom(const graph&);		// copy-in from given graph
	void	cFrom(const graph&);	
	void	resize(int,int);		// resize graph if needed
	bool	virtual getGraph(istream&); 	// IO helper functions
	void	virtual putGraph(ostream&) const;
	void	virtual shuffle(int*, int*);	// permute vertices, edges
};

// Return number of vertices.
inline int graph::n() const { return N; }

// Return number of edges.
inline int graph::m() const { return M; }

// Return first edge incident to x.
inline edge graph::first(vertex v) const { 
	assert(1 <= v && v <= N);
	return fe[v];
}

// Return terminator of adjacency list for v
inline edge graph::term(vertex v) const {
	assert(1 <= v && v <= N);
	return Null;
}

// Return next edge incident to v, following e.
inline edge graph::next(vertex v, edge e) const {
	assert(1 <= v && v <= N && 1 <= e && e <= M);
	return edges[e].l == v ? edges[e].lnxt : edges[e].rnxt;
}

// Return left endpoint of e.
inline vertex graph::left(edge e) const {
	assert(0 <= e && e <= M);
	return edges[e].l;
}

// Return right endpoint of e.
inline vertex graph::right(edge e) const { 
	assert(0 <= e && e <= M);
	return edges[e].r;
}

// Return other endpoint of e.
inline vertex graph::mate(vertex v, edge e) const {
	assert(1 <= v && v <= N && 1 <= e && e <= M);
	return edges[e].l == v ? edges[e].r : edges[e].l ;
}

#endif
