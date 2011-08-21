/** @file Heap.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Heap.h"

#define p(x) (((x)+(D-2))/D)
#define left(x) (D*((x)-1)+2)
#define right(x) (D*(x)+1)

Heap::Heap(int N1) : N(N1) {
// Initialize a heap to store items in {1,...,N}.
	n = 0;
	h = new item[N+1]; pos = new int[N+1]; kee = new keytyp[N+1];
	for (int i = 1; i <= N; i++) pos[i] = 0;
	h[0] = pos[0] = 0; kee[0] = 0;
}

Heap::~Heap() { delete [] h; delete [] pos; delete [] kee; }

// Add i to heap.
void Heap::insert(item i, keytyp k) {
	kee[i] = k; n++; siftup(i,n);
}

// Remove item i from heap. Name remove is used since delete is C++ keyword.
void Heap::remove(item i) {
	int j = h[n--];
	if (i != j) {
		if (kee[j] < kee[i]) siftup(j,pos[i]);
		else if (kee[j] > kee[i]) siftdown(j,pos[i]);
	}
	pos[i] = 0;
}

void Heap::siftup(item i, int x) {
// Shift i up from position x to restore heap order.
	int px = p(x);
	while (x > 1 && kee[i] < kee[h[px]]) {
		h[x] = h[px]; pos[h[x]] = x;
		x = px; px = p(x);
	}
	h[x] = i; pos[i] = x;
}

void Heap::siftdown(item i, int x) {
// Shift i down from position x to restore heap order.
	int cx = minchild(x);
	while (cx != 0 && kee[h[cx]] < kee[i]) {
		h[x] = h[cx]; pos[h[x]] = x;
		x = cx; cx = minchild(x);
	}
	h[x] = i; pos[i] = x;
}

// Return the position of the child of the item at position x
// having the smallest key.
int Heap::minchild(int x) {
	int y; int minc = left(x);
	if (minc > n) return 0;
	for (y = minc + 1; y <= right(x) && y <= n; y++) {
		if (kee[h[y]] < kee[h[minc]]) minc = y;
	}
	return minc;
}

void Heap::changekey(item i, keytyp k) {
// Change the key of i and restore heap order.
	keytyp ki = kee[i]; kee[i] = k;
	if (k == ki) return;
	if (k < ki) siftup(i,pos[i]);
	else siftdown(i,pos[i]);
}
