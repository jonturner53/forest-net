// Nearest-common ancestor computation
//
// User invokes constructor for the nca class (nca).
// The class encapuslates data used by the recursive
// search procedure.

#ifndef NCA_H
#define NCA_H

#include "stdinc.h"
#include "misc.h"
#include "prtn.h"
#include "graph.h"

// Class encapsulates private data and hides the search routine.
class nca {
public:
		nca(graph&, vertex, vertex_pair[], int, vertex[]);
private:
	graph 	*Tp;		// pointer to tree
	vertex	root;		// tree root
	vertex_pair *pairs;	// vector of vertex pairs
	int	np;		// number of vertex pairs
	vertex	*ncav;		// pointer to nca vector

	graph	*Gp;		// graph used to represent pairs internally
	prtn	*P;		// groups closed vertices with their noa
	vertex	*noa;		// if u is a canonical element, noa[u] is noa

	enum state_t { unreached, open, closed };
	state_t	*state;		// states of vertices in search

	void	compute_nca(int, int); // recursive search routine
};

#endif
