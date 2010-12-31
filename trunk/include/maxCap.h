// AugPathMaxcap class. Encapsulates data and routines used by the
// maximum capacity augmenting path algorithm. Use constructor to
// invoke the algorithm.

#ifndef MAXCAP_H
#define MAXCAP_H

#include "augPath.h"

class maxCap : public augPath {
public: 
	maxCap(flograph&,int&);
private:
	bool	findPath();
};

#endif
