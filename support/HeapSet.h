/** \file HeapSet.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef HEAPSET_H
#define HEAPSET_H

#include <list>
#include "stdinc.h"
#include "Misc.h"

typedef uint64_t keytyp;
typedef int item;

/** This class implements a collection of heaps.
 *  Items to be stored in the heap are identified by integers in 1..N,
 *  where N is a parameter specified in the constructor. Heaps are
 *  identified by integers in 1..M where M is another parameter to
 *  the constructor. Each item may be stored in at most one heap.
 *  Keys are 64 bit unsigned ints.
 *
 *  Heaps are implemented using a variant of the d-heap.
 */
class HeapSet {
public:		HeapSet(int,int);
		~HeapSet();

	// access methods
	item	findMin(int) const;
	keytyp	getKey(item) const;
	int	heapSize(int) const;

	// predicates 
	bool	empty(int);	

	// modifiers 
	bool	insert(item, keytyp, int);
	item 	deleteMin(int);
	void	changeKeyMin(keytyp, int);

	string& toString(int, string&) const;
private:
	static const int D = 8;		///< base of heap
	int	maxItem;		///< max number of items in all heaps
	int	maxHeap;		///< max number of heaps

	item	*heaps;			///< holds all items
	keytyp	*key;			///< key[i] is key of item i

	int	*root;			///< root[h] is position of heap h root
	int	*bot;			///< bot[h] is position of "bottom" node
	int	*hSize;			///< hSize[h] is # of items in heap h
	
	int	*child;			///< child[p] points to first child of p
	int	*parent;		///< parent[p/D] points to parent of p
	int	*pred;			///< pred[p/D] is predecessor of p
	
	int	free;			///< start of free node list

	item	nodeMinPos(int) const;	///< position of smallest item in node
	void	siftup(item,int);	///< move item up to restore heap order
	void	siftdown(item,int);	///< move down to restore heap order
};

inline int HeapSet::nodeMinPos(int p) const {
	if (p == -1 || heaps[p] == 0) return -1;
	int minPos = p;
	for (int q = p+1; q < p+D && heaps[q] != 0; q++)
		if (key[heaps[q]] < key[heaps[minPos]])
			minPos = q;
	return minPos;
}

// Return item at top of heap
inline int HeapSet::findMin(int h) const {
	if (hSize[h] == 0) return 0;
	int p = nodeMinPos(root[h]);
	return (p < 0 ? 0 : heaps[p]);
}

// Return key of i.
inline keytyp HeapSet::getKey(item i) const { return key[i]; }

// Return true if heap is empty, else false.
inline bool HeapSet::empty(int h) { return hSize[h] == 0; };

// Return the size of a heap.
inline int HeapSet::heapSize(int h) const { return hSize[h]; };

#endif
