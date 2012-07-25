/** @file Heap.java
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

package forest.common;

/** This class implements a heap data structure.
 *  In this heap, keys are 64 bit unsigned integers.
 */
public class Heap {
	private static final int D = 8;	// base of heap
	private int N;			// max number of items in heap
	private int n;			// number of items in heap
	private int [] h;		// {h[1],...,h[n]} is set of items
	private int [] pos;		// pos[i] gives position of i in h
	private long [] kee;		// kee[i] is key of item i

	// Initialize a heap to store item in {1,...,N}.
	public Heap(int N) {
		this.N = N; n = 0;
		h = new int[N+1]; pos = new int[N+1]; kee = new long[N+1];
		for (int i = 1; i <= N; i++) pos[i] = 0;
		h[0] = pos[0] = 0; kee[0] = 0;
	}

	// Return item at top of heap
	public int findmin() { return n == 0 ? 0 : h[1]; }
	
	// Delete and return item at top of heap
	public int deletemin() {
		if (n == 0) return 0;
		int i = h[1]; remove(h[1]);
		return i;
	}
	
	// Return key of i.
	public long key(int i) { return kee[i]; }
	
	// Return true if i in heap, else false.
	public boolean member(int i) { return pos[i] != 0; }
	
	// Return true if heap is empty, else false.
	public boolean empty() { return n == 0; };
	
	private int p(int x) { return (((x)+(D-2))/D); }
	private int left(int x) { return (D*((x)-1)+2); }
	private int right(int x) { return (D*(x)+1); }
	
	// Add i to heap.
	public void insert(int i, long k) {
		kee[i] = k; n++; siftup(i,n);
	}
	
	// Remove item i from heap.
	public void remove(int i) {
		int j = h[n--];
		if (i != j) {
			if (kee[j] <= kee[i]) siftup(j,pos[i]);
			else siftdown(j,pos[i]);
		}
		pos[i] = 0;
	}
	
	// Shift i up from position x to restore heap order.
	private void siftup(int i, int x) {
		int px = p(x);
		while (x > 1 && kee[i] < kee[h[px]]) {
			h[x] = h[px]; pos[h[x]] = x;
			x = px; px = p(x);
		}
		h[x] = i; pos[i] = x;
	}
	
	// Shift i down from position x to restore heap order.
	private void siftdown(int i, int x) {
		int cx = minchild(x);
		while (cx != 0 && kee[h[cx]] < kee[i]) {
			h[x] = h[cx]; pos[h[x]] = x;
			x = cx; cx = minchild(x);
		}
		h[x] = i; pos[i] = x;
	}
	
	// Return the position of the child of the item at position x
	// having the smallest key.
	private int minchild(int x) {
		int y; int minc = left(x);
		if (minc > n) return 0;
		for (y = minc + 1; y <= right(x) && y <= n; y++) {
			if (kee[h[y]] < kee[h[minc]]) minc = y;
		}
		return minc;
	}
	
	// Change the key of i and restore heap order.
	public void changekey(int i, long k) {
		long ki = kee[i]; kee[i] = k;
		if (k == ki) return;
		if (k < ki) siftup(i,pos[i]);
		else siftdown(i,pos[i]);
	}
	
	public String toString() {
		String s = "";
		for (int i = 1; i <= n; i++) {
			s += "(" + h[i] + "," + kee[h[i]] + ") ";
		}
		return s;
	}
}
