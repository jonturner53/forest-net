// Header file for data structure that maintains a staircase function.
// Main operations include testing for the minimum value in a
// specified range and increasing (or decreasing) the value
// across all points in a specified range.
// x values are required to be non-negative.

#ifndef STEPS_H
#define STEPS_H

#include "dkst.h"
#include "list.h"

class steps {
public: 	steps(int);
		~steps();
	static	const int MAXY = BIGINT-1;  // maximum allowed key2 value
	int	value(int); 		    // return y value for given x
	int	findmin(int,int); 	    // return min value in given range
	void	change(int,int,int);	    // change values in range by delta
	friend	ostream& operator<<(ostream&, const steps&);
protected:
	int	n;			// max number of steps function may have
	dkst	*points;		// data structure for "change points"
	list	*free;			// list of unused items in points
};

#endif
