#include "steps.h"

steps::steps(int n1) : n(n1) { 
	// allocate enough space to support n distinct steps in function
	points = new dkst(2*n+1); free = new list(2*n+1);

	// item #1 is always in the search tree that defines the function
	// by initializing its keys to (0,0), we effectively initialize
	// the function to be zero for all non-negative values of x
	points->setkey(0,0,1);

	// now, place all other items in the free list
	for (int i = 2; i <= 2*n+1; i++) (*free) &= i;
} 

steps::~steps() { delete points; delete free; }

// Return the y value at a specified x coordinate
item steps::value(int x)  {
	assert (x >= 0);
	item v = points->access(x,points->find(1));
	return points->key2(v);
}

// Return the smallest y value that the function takes on in
// the range [lo,hi]
int steps::findmin(int lo, int hi) {
	assert(0 <= lo && lo <= hi);
	// your code here
}

// Add diff to all values in the range lo,hi.
void steps::change(int lo, int hi, int diff) {
	assert(0 <= lo && lo <= hi);
	// when adding new points to the function, first select unused item
	// from free list and insert that point into points
	// after removing an item from points, put it back on free
	// note - never remove item #1 from the search tree
}

ostream& operator<<(ostream& os, const steps& S) {
// Print the list of points in the function
	for (item i = 1; i != Null; i = S.points->next(i)) {
		os << "(" << S.points->key1(i)
		   << "," << S.points->key2(i) << ") ";
	}
	os << endl;
}
