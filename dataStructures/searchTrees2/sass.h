// Header file for data structure representing a collection of
// sorted sets (implemented as self-adjusting binary search trees).

#ifndef SASS_H
#define SASS_H

#include "ssets.h"

class sass : public ssets {
public: 	sass(int=100);
		~sass();
	sset	find(item);		// return sset containing item
	item	access(keytyp,sset);	// return item in sset with given key
	item	insert(item,sset);	// insert item into sset
	item	remove(item,sset);	// remove item from sset
	spair	split(item,sset);	// split sset on item & return sset pair
protected:
	item	splay(item);
	void	splaystep(item);
};

#endif
