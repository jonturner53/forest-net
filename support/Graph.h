/** @file Graph.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef GRAPH_H
#define GRAPH_H

#include "stdinc.h"
#include "Misc.h"
#include "Clist.h"
#include "UiSetPair.h"

typedef int vertex;
typedef int edge;

/** Data structure for undirected graph with edge weights.
 *
 *  Graph size (number of vertices and max number of edges) must
 *  be specified when a Graph object is instantiated.
 *  Edges can be added and removed from the graph.
 *  Methods are provided to facilitate graph traversal,
 *  either by iterating through all edges of the graph
 *  or all edges incident to a specific vertex.
 */
class Graph {
public:		Graph(int,int);
		~Graph();

	// number of vertices and edges
	int	n() const;		
	int	m() const;	

	// methods for iterating through overall edge list, adjacency lists
	edge	first() const;
	edge	next(edge) const;
	edge	first(vertex) const;
	edge	next(vertex,edge) const;

	// methods for accessing edge endpoints and length
	vertex	left(edge) const;	
	vertex	right(edge) const;	
	vertex	mate(vertex,edge) const;

	// methods for accessing/changing length
	int	length(edge) const;
	void	setLength(edge,int);

	// methods for adding/removing edges
	edge	join(vertex,vertex,int); 	
	bool	remove(edge);	

	// create a string representation
	string&	toString(string&) const;

protected:
	int	N, M;			///< number of vertices and edges
	int	maxEdge;		///< max number of edges

	edge	*fe;			///< fe[v] is first edge incident to v

	struct EdgeInfo {
	vertex	l;			///< l is left endpoint of edge
	vertex	r;			///< r is right endpoint of edge
	int	len;			///< length of the edge
	};
	EdgeInfo *evec;			///< array of edge structures

	UiSetPair *edges;		///< sets of in-use and free edges

	Clist	*adjLists;		///< collection of edge adjacency lists
					///< stores edge e as both 2e and 2e+1
};

/** Get the number of vertices.
 *  @return the number of vertices in the graph.
 */
inline int Graph::n() const { return N; }

/** Get the number of edges.
 *  @return the number of edges in the graph.
 */
inline int Graph::m() const { return M; }

/** Get the first edge in the overall list of edges.
 *  @return the first edge in the list
 */
inline edge Graph::first() const { return edges->firstIn(); }

/** Get the next edge in the overall list of edges.
 *  @param e is the edge whose successor is requested
 *  @return the next edge in the list, or 0 if e is not in the list
 *  or it has no successor
 */
inline edge Graph::next(edge e) const { return edges->nextIn(e); }

/** Get the first edge incident to a vertex.
 *  @param v is the the vertex of interest
 *  @return the first edge incident to v
 */
inline edge Graph::first(vertex v) const { 
	assert(1 <= v && v <= N);
	return fe[v]/2;
}

/** Get the next edge in the adjacency list for a specific vertex.
 *  @param v is the edge whose adjacency list we're accessing
 *  @param e is the edge whose successor is requested
 *  @return the next edge in the adjacency list for v
 *  or 0 if e is not incident to v or is the last edge on the list
 */
inline edge Graph::next(vertex v, edge e) const {
	assert(1 <= v && v <= N && 1 <= e && e <= maxEdge);
	if (v != evec[e].l && v != evec[e].r) return 0;
	int ee = (v == evec[e].l ? 2*e : 2*e+1);
	int ff = adjLists->suc(ee);
	return (fe[v] == ff ? 0 : ff/2);
}

/** Get the left endpoint of an edge.
 *  @param e is the edge of interest
 *  @return the left endpoint of e, or 0 if e is not a valid edge.
 */
inline vertex Graph::left(edge e) const {
	assert(0 <= e && e <= maxEdge);
	return evec[e].l;
}

/** Get the right endpoint of an edge.
 *  @param e is the edge of interest
 *  @return the right endpoint of e, or 0 if e is not a valid edge.
 */
inline vertex Graph::right(edge e) const {
	assert(0 <= e && e <= maxEdge);
	return (evec[e].l == 0 ? 0 : evec[e].r);
}

/** Get the other endpoint of an edge.
 *  @param v is a vertex
 *  @param e is an edge incident to v
 *  @return the other vertex incident to e, or 0 if e is not a valid edge
 *  or it is not incident to v.
 */
// Return other endpoint of e.
inline vertex Graph::mate(vertex v, edge e) const {
	assert(1 <= v && v <= N && 1 <= e && e <= maxEdge);
	return (evec[e].l == 0 ?
		0 : (v == evec[e].l ?
		     evec[e].r : (v == evec[e].r ? evec[e].l : 0)));
}

/** Get the length of an edge.
 *  @param e is the edge of interest
 *  @return the length of e, or 0 if e is not a valid edge.
 */
inline int Graph::length(edge e) const {
	assert(0 <= e && e <= maxEdge);
	return (evec[e].l == 0 ? 0 : evec[e].len);
}

/** Set the length of an edge.
 *  @param e is the edge of interest
 *  @param len is the desired length
 */
inline void Graph::setLength(edge e, int lng) {
	assert(0 <= e && e <= maxEdge);
	evec[e].len = lng;
}

#endif
