// ppFifo class. Encapsulates data and routines used by the fifo
// variant of the preflow-push method. This version uses incremental
// updating of distance labels.

#ifndef PPFIFO_H
#define PPFIFO_H

#include "prePush.h"

class ppFifo : public prePush {
public: 
		ppFifo(flograph&, int&, bool);
protected:
	list	*unbal;		// list of unbalanced vertices
	void	newUnbal(vertex); // handle new unbalanced vertex
};

#endif
