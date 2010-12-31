// Header file for data structure that implements a dual key search tree.

#ifndef DKST_H
#define DKST_H

#include "sass.h"

class dkst : public sass {
public: 	dkst(int);
		~dkst();
	static	const int MAX2 = BIGINT-1;  // maximum allowed key2 value
	void	setkey(item,keytyp,keytyp); // set key values
	keytyp	key1(item);		    // return key1 value
	keytyp	key2(item);		    // return key2 value
	item	first(sset) const;	    // return first item in set
	item	next(item) const;	    // return next item after given one
	item	access(keytyp,sset);	    // return "nearest" item to given key1
	keytyp	min2(sset);		    // return smallest key2 in sset
	void	change2(keytyp,sset); 	    // change key2 values for sset
	sset	insert(item,sset);	    // insert item into sset
	sset	remove(item,sset);	    // remove item from sset
	sset	join(sset,item,sset);	    // join two ssets at an item
	spair	split(item,sset);	    // split a set on given item
	friend	ostream& operator<<(ostream&, const dkst&);
private:
	keytyp	*dmin;			// delta min value for key 2
	keytyp	*dkey;			// delta key value for key 2
	void	virtual rotate(item);
};

// Set key values. Requires i to be an isolated vertex.
void inline dkst::setkey(item i,keytyp k1,keytyp k2) {
	assert(0 <= i && i <= n && k2 <= MAX2);
	assert(node[i].p == Null && node[i].left == Null && node[i].right == Null);
	node[i].kee = k1; dmin[i] = k2; dkey[i] = 0;
}

// Return key1 value for item
keytyp inline dkst::key1(item i) {
	assert(1 <= i && i <= n);
	return node[i].kee;
}

// Return smallest key2 value for sset
keytyp inline dkst::min2(sset s) {
	assert(1 <= s && s <= n);
	return dmin[s];
}

// Add diff to all key2 values in given sset
void inline dkst::change2(keytyp diff, sset s) {
	assert(1 <= s && s <= n);
	dmin[s] += diff;
}

#endif
