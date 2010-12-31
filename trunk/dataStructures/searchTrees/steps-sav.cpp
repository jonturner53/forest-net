#include "steps.h"

steps::steps(int n1) : n(n1) { 
	points = new dkst(2*n+1); free = new list(2*n+1);
	points->setkey(0,0,1); // initialize function to zero for all x>=0
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

	item loPnt = points->access(lo,points->find(1));
	spair loPair = points->split(loPnt,points->find(loPnt));
	if (loPair.s2 == Null) {
		points->join(loPair.s1,loPnt,loPair.s2);
		return points->key2(loPnt);
	}
	item hiPnt = points->access(hi,loPair.s2);
	if (hiPnt == Null) {
		points->join(loPair.s1,loPnt,loPair.s2);
		return points->key2(loPnt);
	}
	spair hiPair = points->split(hiPnt,points->find(hiPnt));
	int v1 = points->key2(loPnt);
	int v2 = v1;
	if (hiPair.s1 != Null) v2 = points->min2(hiPair.s1);
	int v3 = points->key2(hiPnt);
	v1 = min(v1,v2); v1 = min(v1,v3);
	points->join(hiPair.s1,hiPnt,hiPair.s2);
	points->join(loPair.s1,loPnt,points->find(hiPnt));
	return v1;
}

// Add diff to all values in the range lo,hi.
void steps::change(int lo, int hi, int diff) {
	assert(0 <= lo && lo <= hi);
	if (diff == 0) return;

	item p, loPnt, hiPnt;
	p = points->access(lo,points->find(1));
	if (points->key1(p) == lo) loPnt = p;
	else {
		if (free->empty()) fatal("steps:: too many steps in function");
		loPnt = (*free)[1]; (*free) <<= 1;
		points->setkey(loPnt,lo,points->key2(p));
		points->insert(loPnt,points->find(1));
	}
	p = points->access(hi+1,points->find(1));
	if (points->key1(p) == hi+1) hiPnt = p;
	else {
		if (free->empty()) fatal("steps:: too many steps in function");
		hiPnt = (*free)[1]; (*free) <<= 1;
		points->setkey(hiPnt,hi+1,points->key2(p));
		points->insert(hiPnt,points->find(1));
	}

	// divide set into parts so we can change the values for each part
	spair loPair = points->split(loPnt,points->find(loPnt));
	spair hiPair = points->split(hiPnt,loPair.s2);
	points->change2(diff,loPnt);
	if (hiPair.s1 != Null) points->change2(diff,hiPair.s1);
	points->join(hiPair.s1,hiPnt,hiPair.s2);
	points->join(loPair.s1,loPnt,points->find(hiPnt));

	// Now, remove redundant points	
	p = points->access(hi,points->find(1));
	if (points->key2(p) == points->key2(hiPnt)) {
		points->remove(hiPnt,points->find(1));
		(*free) &= hiPnt;
	}
	if (loPnt == 1) return; // never remove first point
	p = points->access(lo-1,points->find(1));
	if (points->key2(p) == points->key2(loPnt)) {
		points->remove(loPnt,points->find(1));
		(*free) &= loPnt;
	}
}

ostream& operator<<(ostream& os, const steps& S) {
// Print the list of points in the function
	for (item i = 1; i != Null; i = S.points->next(i)) {
		os << "(" << S.points->key1(i)
		   << "," << S.points->key2(i) << ") ";
	}
	os << endl;
}
