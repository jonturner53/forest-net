// Header file for dinic class. Encapsulates data and routines
// used to implement Dinic's algorithm for the max flow problem.
// To use, invoke dinic(G), where G is the flograph. This
// instantiates a variable of type dinicC. The class constructor
// computes the maximum flow, using Dinic's algorithm.
//

class dinicC {
        flograph* G;            // graph we're finding flow on
        int*    nextEdge;       // ignore edges before nextEdge[u] in adj. list
        int*    level;          // level[u]=# of edges in path from source

        int     findpath(vertex,list&); // find augmenting path
        void    augment(list&); // add flow to augmenting path
        int     newphase();     // prepare for a new phase
public: dinicC(flograph&);
};

inline void dinic(flograph &G) { dinicC x(G); }
