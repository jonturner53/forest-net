// Header file for data structure representing a collection of sorted
// sets (implemented as balanced binary search trees).

#ifndef BSSETS_H
#define BSSETS_H

#include "ssets.h"

class bssets : public ssets {
public:
		bssets(int);
		~bssets();
	item	insert(item,sset);	// insert item into sset
	item	remove(item,sset);	// remove item from sset
	sset	join(sset,item,sset);	// join two ssset at item
	spair	split(item,sset);	// split sset on item & return sset pair
protected:
	int	*rvec;			// vector of ranks
	void	virtual swap(item,item);// swap position of two items
	friend	ostream& operator<<(ostream&, const bssets&);
};

#endif
