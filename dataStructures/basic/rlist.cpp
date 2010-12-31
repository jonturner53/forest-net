#include "rlist.h"

#define succ(x) node[x].next
#define pred(x) node[x].prev

rlist::rlist(int N1) : N(N1) {
// Construct rlist with space for N1 items
	assert(N >= 0);
	node = new lnode[N+1];
	for (item i = 1; i <= N; i++) { succ(i) = pred(i) = i; }
	succ(Null) = pred(Null) = Null;
}

// Free dynamic storage
rlist::~rlist() { delete [] node; }

item rlist::pop(item t) {
// Remove first item from the list whose last item is t
// Return the last item of the resulting list.
// No effect for lists with just a single item.
	assert(0 <= t && t <= N);
	item h = succ(t);
	if (h == t) return t;
	if (pred(h) == t) succ(t) = succ(h);
	else              succ(t) = pred(h);
	if (pred(succ(t)) == h) pred(succ(t)) = t;
	else                    succ(succ(t)) = t;
	succ(h) = pred(h) = h;
	return t;
}

item rlist::join(item t1, item t2) {
// Combine the lists whose last items are t1 and t2,
// by appending the second list to the first.
// Return the last item on the resulting list.
// The original lists are destroyed.
	assert(0 <= t1 && t1 <= N && 0 <= t2 && t2 <= N);
	if (t1 == Null) return t2;
	else if (t2 == Null || t2 == t1) return t1;

	item h1 = succ(t1); item h2 = succ(t2);
	succ(t1) = h2; succ(t2) = h1;
	if (t1 == pred(h1)) pred(h1) = t2;
	else                succ(h1) = t2;
	if (t2 == pred(h2)) pred(h2) = t1;
	else                succ(h2) = t1;

	return t2;
}

item rlist::reverse(item t) {
// Reverse the list whose last item is t.
// Return the last item on the reversed list.
	assert(0 <= t && t <= N);
	item h = succ(t);
	if (t == Null || h == t) return t;
	if (t == pred(h)) pred(h) = succ(h);
	succ(h) = t;
	return h;
}

void rlist::print(ostream& os, item t) {
// Print the list whose last item is t.
	item h = succ(t);
	if (t == Null) os << "-";
	else if (h == t) misc::putNode(os,h,N);
	else {
		item x = h; item y = t;
		do {
			misc::putNode(os,x,N); os << " ";
			if (y == pred(x)) {
				y = x; x = succ(x);
			} else {
				y = x; x = pred(x);
			}
		} while (x != h);
	}
}
