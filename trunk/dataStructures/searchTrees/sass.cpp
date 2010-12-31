#include "sass.h"

#define left(x) node[x].left
#define right(x) node[x].right
#define p(x) node[x].p
#define kee(x) node[x].kee

sass::sass(int N) : ssets(N) { }

sass::~sass() { }

// Splay at item i. Return root of resulting tree.
item sass::splay(item x) {
	while (p(x) != Null) splaystep(x);
	return x;
}

// Perform a single splay step at x.
void sass::splaystep(item x) {
	item y = p(x);
	if (y == Null) return;
	item z = p(y);
        if (x == left(left(z)) || x == right(right(z)))
		rotate(y);
	else if (z != Null) // x is "inner grandchild"
		rotate(x);
	rotate(x);
}

// Return the sset containing item i.
sset sass::find(item i) { return splay(i); }

// Return the item in s with key k, if there is one, else Null.
item sass::access(keytyp k, sset s) {
	assert (0 <= s && s <= n);
	while (true) {
		     if (k < kee(s) && left(s) != Null) s = left(s);
		else if (k > kee(s) && right(s) != Null) s = right(s);
		else break;
	}
	splay(s);
	return (k == kee(s) ? s : Null);
}

// Insert item i into sset s; i is assumed to belong to a singleton
// sset before the operation.
item sass::insert(item i, sset s) { ssets::insert(i,s); return splay(i); }

// Remove item i from sset s. This leaves i in a singleton sset.
item sass::remove(item i, sset s) {
	item j = ssets::remove(i); return splay(j);
}

// Split s at item i, yielding two ssets, s1 containing items with keys smaller
// than key(i), s2 with keys larger than key(i) and leaving i in a singleton.
// Return the pair [s1,s2]
spair sass::split(item i, sset s) {
	assert(1 <= i && i <= n && 1 <= s && s <= n);
	spair pair;
	splay(i);
	pair.s1 = left(i); pair.s2 = right(i);
	left(i) = right(i) = p(i) = Null;
	p(pair.s1) = p(pair.s2) = Null;
	return pair;
}
