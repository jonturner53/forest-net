// Header file for data structure that implements a dual key search tree.

#ifndef DKST_H
#define DKST_H

#include "sass.h"

class dkst : public sass {
public: 	dkst(int);
		~dkst();
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
	keytyp	minHelper(item);
	void	changeHelper(keytyp, item);
};

// Set key values. Requires i to be an isolated vertex.
void inline dkst::setkey(item i,keytyp k1,keytyp k2) {
	assert(0 <= i && i <= n);
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
	return minHelper(s);
}

keytyp inline dkst::minHelper(item i){
	
	if (node[i].left == Null && node[i].right == Null) return key2(i);	
	
	keytyp k1 = 0x7fffffff;
	keytyp k2 = 0x7fffffff;

	if (node[i].left != Null) k1 = minHelper(node[i].left);
	if (node[i].right != Null) k2 = minHelper(node[i].right);
	
	if (k1 > k2)
		return k2;
	return k1;	

}

// Add diff to all key2 values in given sset
void inline dkst::change2(keytyp diff, sset s) {
	assert(1 <= s && s <= n);
	changeHelper(diff, find(s));
}

void inline dkst::changeHelper(keytyp diff, item i){

	setkey(i, key1(i), key2(i) + diff);	
	if (node[i].left != Null) change2(diff, node[i].left);
	if (node[i].right != Null) change2(diff, node[i].right);

}

#endif
