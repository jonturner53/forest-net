/** @file Graph.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "stdinc.h"
#include "Graph.h"

/** Construct a Graph with space for a specified number of vertices and edges.
 *  @param N1 is number of vertices in the graph
 *  @param maxEdge1 is the maximum number of edges
 */
Graph::Graph(int N1, int maxEdge1) : N(N1), maxEdge(maxEdge1) {
	assert(N > 0 && maxEdge > 0);
	M = 0;
	fe = new edge[N+1];
	evec = new EdgeInfo[maxEdge+1];
	edges = new UiSetPair(maxEdge);
	adjLists = new Clist(2*maxEdge+1);
	for (vertex u = 1; u <= N; u++) fe[u] = 0;
}

/** Free space used by Graph */
Graph::~Graph() {
	delete [] fe; delete [] evec; delete edges; delete adjLists;
}

/** Join two vertices with an edge.
 *  @param u is the left endpoint for the new edge
 *  @param v is the right endpoint for the new edge
 *  @param lng is the length of the new edge
 *  @return the edge number for the new edge or 0
 *  on failure
 */
edge Graph::join(vertex u, vertex v, int lng) {
	assert(1 <= u && u <= N && 1 <= v && v <= N);

	edge e = edges->firstOut();
	if (e == 0) return 0;
	edges->swap(e);

	// initialize edge information
	evec[e].l = u; evec[e].r = v; evec[e].len = lng;

	// add edge to the adjacency lists
	// in the adjLists data structure, each edge appears twice,
	// as 2*e and 2*e+1
	if (fe[u] != 0) adjLists->join(2*e,fe[u]);
	if (fe[v] != 0) adjLists->join(2*e+1,fe[v]);
	if (fe[u] == 0) fe[u] = 2*e;
	if (fe[v] == 0) fe[v] = 2*e+1;

	M++;

	return e;
}

/** Remove an edge from the graph.
 *  @param e is the edge to be removed.
 *  @return true on success, false on failure
 */
bool Graph::remove(edge e) {
	assert(1 <= e && e <= maxEdge);
	if (edges->isOut(e)) return false;
	edges->swap(e);

	vertex u = evec[e].l;
	if (fe[u] == 2*e)
		fe[u] = (adjLists->suc(2*e) == 2*e ? 0 : adjLists->suc(2*e));
	u = evec[e].r;
	if (fe[u] == 2*e+1)
		fe[u] = (adjLists->suc(2*e+1) == 2*e+1 ?
				0 : adjLists->suc(2*e+1));

	adjLists->remove(2*e); adjLists->remove(2*e+1);

	evec[e].l = 0;

	M--;
	return true;
}

/** Construct a string representation of the Graph object.
 *  For small graphs (at most 26 vertices), vertices are
 *  represented in the string as lower case letters.
 *  For larger graphs, vertices are represented by integers.
 *  @param s is a string object provided by the caller which
 *  is modified to provide a representation of the Graph.
 *  @return a reference to the string
 */
string& Graph::toString(string& s) const {
	s.clear();
	for (vertex u = 1; u <= n(); u++) {
		if (first(u) == 0) continue;
		for (edge e = first(u); e != 0; e = next(u,e)) {
			string s1, s2, s3;
			s += "(";
			if (n() <= 26)
				s = s + ((char) ('a'+(u-1))) + ","
				      + ((char) ('a'+(mate(u,e)-1))) + ","
				      + Misc::num2string(length(e),s1);
			else 
				s = s + Misc::num2string(u,s1) + ","
				      + Misc::num2string(mate(u,e),s2) + ","
				      + Misc::num2string(length(e),s3);
			s += ")"; if (next(u,e) != 0) s += " ";
		}
		s += "\n";
	}
	return s;
}
