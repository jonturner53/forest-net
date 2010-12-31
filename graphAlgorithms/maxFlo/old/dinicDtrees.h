// Header file for dinicDtrees class. Encapsulates data and routines
// used to implement Dinic's algorithm with dynamic trees.
// To use, invoke dinicDtrees(G), where G is the flograph. This
// instantiates a variable of type dinicDtreesC. The class constructor
// computes the maximum flow, using Dinic's algorithm with dtrees.
//

class dinicDtreesC {
	flograph* G;		// graph we're finding flow on
	int*	nextEdge;	// ignore edges before nextEdge[u] in adj. list
	int*	upEdge;		// upEdge[u] is edge for dtrees link from u
	int*	level;		// level[u]=# of edges in path from source
	dtrees*	dt;		// dynamic trees data structure

	int	findpath();	// find augmenting path
	void	augment();	// add flow to augmenting path
	int	newphase();	// prepare for a new phase
public:	dinicDtreesC(flograph&);
};

inline void dinicDtrees(flograph& G) { dinicDtreesC x(G); }
