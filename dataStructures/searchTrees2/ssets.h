// Header file for data structure representing a collection of sorted
// sets (implemented as binary search trees).

#ifndef SSETS_H
#define SSETS_H

#include "stdinc.h"
#include "misc.h"

typedef int sset;		// tree in collection
typedef int item;		// item in set
typedef int keytyp;
struct spair { sset s1,s2; };	// pair of sset, returned by split

class ssets {
public:
		ssets(int);
		~ssets();
	keytyp	key(item) const;	// return the key
	void	setkey(item,keytyp);	// set the key of item
	sset	find(item) const;	// return sset containing node
	item	access(keytyp,sset) const;// return item in sset with given key
	item	insert(item,sset);	// insert item into sset
	item	remove(item,sset);	// remove item from sset
	sset	join(sset,item,sset);	// join two ssets at item
	spair	split(item,sset);	// split sset on item & return sset pair
	void	print(ostream&, sset) const;	// print single sset
	friend	ostream& operator<<(ostream&, const ssets&); // print all ssets
protected:
	int	n;			// items are integers in {1,...,n}
	struct snode {
	item	left, right, p;		// left child, right child, parent
	keytyp 	kee;			// key values
	} *node;
	void virtual swap(item,item);	// swap two ssets
	void virtual rotate(item);	// rotate item up to replace its parent
	item 	sibling(item, item);	// return "other child" of an item
	item	remove(item);		// private helper for public remove
};

// Return the key of i.
inline keytyp ssets::key(item i) const { return node[i].kee; }

// Set the key of item i. Valid only for singletons
inline void ssets::setkey(item i, keytyp k) {
	assert(node[i].left == Null && node[i].right == Null && node[i].p == Null);
	node[i].kee = k;
}

item inline ssets:: sibling(item x, item px) {
// Return child of px that is not x. Returns Null when px = Null.
	return (x == node[px].left ? node[px].right : node[px].left);
}

#endif
