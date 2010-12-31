// Dinic class. Encapsulates data and routines used by Dinic's
// algorithm for max flow. Use constructor to invoke algorithm.

#ifndef DINIC_H
#define DINIC_H

#include "augPath.h"

class dinic : public augPath {
public:
		dinic(flograph&,int&);
		~dinic();
private:
        int*    nextEdge;       // ignore edges before nextEdge[u] in adj. list
        int*    level;          // level[u]=# of edges in path from source

        bool    findPath(vertex); // find augmenting path
        bool    newPhase();     // prepare for a new phase
};

#endif
