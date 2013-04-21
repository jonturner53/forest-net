/** @file HeapSet.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "HeapSet.h"

namespace forest {


/*
Implementation notes

Heaps are constructed from logical nodes of size D.
Each node contains up to D items, and each item in a
node has a child pointer that identifes the node
containing its children in the tree that forms the heap.
Each node also has a parent pointer that points to the
position of the parent item in the parent node

Each node also a predecessor pointer that points to
the preceding node in its heap in the breadth-first
ordering of the nodes. This pointer is used when
adding and removing nodes from a heap.

The heaps array is organized into sub-arrays of size D.
Each subarry contains the items in one node.
The child array is organized similarly; that is, if an
item is stored at position p in the heaps array, its
child pointer is stored at position p in the child array.

All "pointers" are actually integers that refer to positions
in the heaps array. Most of these pointers refer to the first
position in a node, and so are divisible by D. The one exception
is the parent pointers. These may refer to any position within
a node.

Each heap has a root pointer that identifies the node at
the root of its tree. It also has a bot pointer that identifies
the last node in its tree (in breadth-first order).
The number of nodes in each heap is stored in the hSize array.
The hSize values are relied upon to deal with boundary
cases like empty heaps. The valued of the root and bot pointers
of an empty heap are undefined.
*/

/** Constructor for HeapSet.
 *  @param maxItem1 is the maximum item number
 *  @param maxHeap1 is the maximum heap number
 */
HeapSet::HeapSet(int maxItem1, int maxHeap1)
		 : maxItem(maxItem1), maxHeap(maxHeap1) {
	int numNodes = (maxItem/D) + maxHeap;

	heaps = new item[numNodes*D]; 	// each D-word block is a "node"
	child = new int[numNodes*D];    // each item in heaps has child node

	parent = new int[numNodes];   	// note, one per node
	pred = new int[numNodes];     	// ditto

	key = new uint64_t[maxItem+1];

	root = new int[maxHeap+1];	// values are indices in heaps array
	bot = new int[maxHeap+1];	// ditto
	hSize = new int[maxHeap+1];

	for (int h = 1; h <= maxHeap; h++) hSize[h] = 0;
	for (int p = 0; p < numNodes*D; p++) heaps[p] = 0;

	// build free list using "parent pointer" of each "node"
	for (int i = 0; i < numNodes-1; i++)
		parent[i] = (i+1)*D;
	parent[numNodes-1] = -1; // use -1 to mark end of list
	free = 0;
}

HeapSet::~HeapSet() {
	delete [] heaps; delete [] child; delete [] parent;
	delete [] pred; delete [] key; delete [] root;
	delete [] bot; delete [] hSize;
}

/** Add an item to a heap.
 *  @param i is the number of the item to be added
 *  @param k is the key for the item being inserted
 *  @param h is the number of the heap in which i is to be inserted
 *  @return true on success, false on failure
 */
bool HeapSet::insert(item i, uint64_t k, int h) {
	key[i] = k;
	if (i == 0) return false;
	int n = hSize[h]; int r = (n-1)%D;
	if (n != 0 && r != D-1) {
		// no new node required
		int p = bot[h] + r + 1;
		child[p] = -1;
		(hSize[h])++;
		siftup(i,p);
		return true;
	}
	// allocate new node
	if (free < 0) return false;
	int p = free; free = parent[free/D];
	heaps[p] = i; child[p] = -1;
	(hSize[h])++;
	//for (int j = 1; j < D; j++) heaps[p+j] = 0;
	if (n == 0) {
		root[h] = bot[h] = p;
		pred[p/D] = parent[p/D] = -1;
		return true;
	}
	pred[p/D] = bot[h]; bot[h] = p;

	// now, find the parent node, and set child and parent pointers
	int q = pred[p/D] + (D-1);
	while (parent[q/D] >= 0  && q%D == D-1)
		q = parent[q/D];
	q = ((q%D != D-1) ? q+1 : q - (D-1));
	while (child[q] != -1) q = child[q];
	child[q] = p; parent[p/D] = q;

	siftup(i,p);
	return true;
}

// Delete and return item with smallest key.
int HeapSet::deleteMin(int h) {
        int n = hSize[h];
        if (n == 0) return 0;
        int i, p;
        if (n == 1) {
                p = root[h]; i = heaps[p]; heaps[p] = 0;
                parent[p/D] = free; free = p;
                hSize[h] = 0;
                return i;
        }

        p = nodeMinPos(root[h]); i = heaps[p];
        if (n <= D) { // single node with at least 2 items
                heaps[p] = heaps[root[h]+(--n)];
		heaps[root[h]+n] = 0;
		hSize[h] = n;
                return i;
        }

        // so, must be at least two nodes
        int q = bot[h]; int r = (n-1)%D;
        int j = heaps[q+r]; heaps[q+r] = 0;
        (hSize[h])--;
        if (r == 0) { // return last node to free list
		if (parent[q/D] >= 0) child[parent[q/D]] = -1;
		bot[h] = pred[q/D];
                parent[q/D] = free; free = q;
        }

        // sift the last item down from the top
        siftdown(j, p);
        return i;
}

// Shift i up from position p to restore heap order.
void HeapSet::siftup(item i, int p) {
	int pp = parent[p/D];
	while (pp >= 0 && key[heaps[pp]] > key[i]) {
		heaps[p] = heaps[pp]; p = pp; pp = parent[pp/D];
	}
	heaps[p] = i;
}

// Shift i down from position p to restore heap order.
void HeapSet::siftdown(item i, int p) {
	int cp = nodeMinPos(child[p]);
	while (cp >= 0 && key[heaps[cp]] < key[i]) {
		heaps[p] = heaps[cp]; p = cp;
		cp = nodeMinPos(child[cp]);
	}
	heaps[p] = i;
}

// Change the key of the min item in a heap.
void HeapSet::changeKeyMin(uint64_t k, int h) {
	int p = nodeMinPos(root[h]);
	item i = heaps[p]; key[i] = k;
	siftdown(i,p);
}

string& HeapSet::toString(int h, string& s) const {
	s = "";
	if (hSize[h] == 0) {
		s = "[]"; return s;
	}

	list<int> nodeList;
	for (int p = bot[h]; p != -1; p = pred[p/D])
		nodeList.push_front(p);
	int cnt = 0; int numPerRow = 1;
	list<int>::iterator lp;
	for (lp = nodeList.begin(); lp != nodeList.end(); lp++) {
		int p = *lp;
		int q = p; string s1;
		s += "[";
		while (q < p+D && heaps[q] != 0) {
			if (q > p) s += " ";
			item i = heaps[q++];
			s += Util::num2string(i,s1) + ":";
			s += Util::num2string(key[i],s1);
		}
		s += "] ";
		if (++cnt == numPerRow) {
			s += "\n"; cnt = 0; numPerRow *= D;
		}
	}
	if (cnt != 0) s += "\n";
	return s;
}

} // ends namespace

