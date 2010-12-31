#include "stdinc.h"
#include "pathset1.h"

#define left(x) vec[x].lchild
#define right(x) vec[x].rchild
#define p(x) vec[x].parent
#define dcost(x) vec[x].deltacost
#define dmin(x) vec[x].deltamin
#define rank(x) vec[x].rank

// Initialize a pathset on items numbered {1,...,N}.
pathset::pathset(int N, int pprop1[]) {
	item i;
	n = N; vec = new pnode[n+1];
	pprop = pprop1;
	for (i = 0; i <= n; i++) {
		left(i) = right(i) = p(i) = Null;
		dcost(i) = dmin(i) = 0;
		rank(i) = 0;
	}
}

pathset::~pathset() { delete [] vec; }

// Left rotation.
void pathset::lrotate(item x) {
	item z = right(x);
	if (z == Null) return;

	if (p(x) == Null) pprop[z] = pprop[x];

	cost dmin_x, dmin_z;
	dmin_x = left(x) == Null && left(z) == Null ? dcost(x) :
		(left(x) == Null ? min(dcost(x),dmin(left(z))+dmin(z)) :
		 (left(z) == Null ? min(dcost(x),dmin(left(x))) :
		  min(dcost(x),min(dmin(left(x)),dmin(left(z))+dmin(z)))
		));
	dmin_z = dmin(x);
	dcost(x) = dcost(x) - dmin_x;
	dcost(z) = dcost(z) + dmin(z);
	dmin(left(x)) = dmin(left(x)) - dmin_x;
	dmin(left(z)) = dmin(left(z)) - (dmin_x - dmin(z));
	dmin(right(z)) = dmin(right(z)) + dmin(z);
	dmin(x) = dmin_x;
	dmin(z) = dmin_z;

	p(z) = p(x);
	     if (x == left(p(x)))  left(p(z)) = z;
	else if (x == right(p(x))) right(p(z)) = z;
	right(x) = left(z); p(right(x)) = x;
	left(z) = x; p(x) = z;
}

// Right rotation.
void pathset::rrotate(item x) {
	item z = left(x);
	if (z == Null) return;

	if (p(x) == Null) pprop[z] = pprop[x];

	cost dmin_x, dmin_z;
	dmin_x = right(x) == Null && right(z) == Null ? dcost(x) :
		(right(x) == Null ? min(dcost(x),dmin(right(z))+dmin(z)) :
		 (right(z) == Null ? min(dcost(x),dmin(right(x))) :
		  min(dcost(x),min(dmin(right(x)),dmin(right(z))+dmin(z)))
		));
	dmin_z = dmin(x);
	dcost(x) = dcost(x) - dmin_x;
	dcost(z) = dcost(z) + dmin(z);
	dmin(right(x)) = dmin(right(x)) - dmin_x;
	dmin(right(z)) = dmin(right(z)) - (dmin_x - dmin(z));
	dmin(left(z)) = dmin(left(z)) + dmin(z);
	dmin(x) = dmin_x;
	dmin(z) = dmin_z;

	p(z) = p(x);
	     if (x == left(p(x)))  left(p(z)) = z;
	else if (x == right(p(x))) right(p(z)) = z;
	left(x) = right(z); p(left(x)) = x;
	right(z) = x; p(x) = z;
}

// Return the set containing item i.
path pathset::findpath(item i) {
	while (p(i) != Null) i = p(i);
	return i;
}

// Return the last node in the path.
path pathset::findtail(path q) {
	if (q == Null) return Null;
	while (right(q) != Null) q = right(q);
	return q;
}

// Add x to the cost of every item in q.
void pathset::addpathcost(path q, cost x) {
	dmin(q) += x;
}

// Return the last item on the path that has minimum cost
// and the cost
cpair pathset::findpathcost(path q) {
	while (1) {
		if (right(q) != Null && dmin(right(q)) == 0)
			q = right(q);
		else if (dcost(q) > 0)
			q = left(q);
		else
			break;
	}
	cpair cp = { q, dmin(q) };
	return cp;
}


// Return the path formed by joining r, v and q;
// i is assumed to be a single item 
// BIG PROBLEM. What ensures that we get a balanced tree???
// Answer: nothing. To use bbst would need a version of join
// that produced a balanced tree, but not clear if we can
// do this by simple series of rotations. Might be possible,
// but not obvious how. This may be another reason to use
// self-adjusting bsts.

path pathset::join(path r, item i, path q) {
	cost dmin_i = dmin(i);
	left(i) = r; right(i) = q;
	if (r == Null && q == Null) {
		;
	} else if (r == Null) {
		dmin(i) = min(dmin(i),dmin(q));
		dmin(q) -= dmin(i);
		p(q) = i;
	} else if (q == Null) {
		dmin(i) = min(dmin(i),dmin(r));
		dmin(r) -= dmin(i);
		p(r) = i;
	} else {
		dmin(i) = min(dmin(r),min(dmin(i),dmin(q)));
		dmin(r) -= dmin(i); dmin(q) -= dmin(i);
		p(r) = p(q) = i;
	}
	dcost(i) = dmin_i - dmin(i);
	return i;
}

// Split the path containing i on i. Return the two
// paths that result from the split.
ppair pathset::split(item i) {
	item x, y, s1, s2;
	cost mc;
	ppair pair;
	
	if (i == Null) return Null;

	// Need to check this carefully.

	for (x = i; p(x) != Null; x = p(x)) ;
	mc = dmin(x); s1 = s2 = Null;
	while (x != i) {
		dmin(x) = mc;
		if (key(i) < key(x)) {
			pair = sever(x);
			s2 = join(s2,x,pair.s2)
			x = pair.s1;
		} else {
			pair = sever(x);
			s1 = join(pair.s1,x,s1)
			x = pair.s2;
		}
		mc += dmin(x);
	}
	s1 = left(i); s2 = right(i);
	if (s1 != Null) dmin(s1) += dmin(i);
	if (s2 != Null) dmin(s2) += dmin(i);
	x = p(i); y = i; s1 = left(y); s2 = right(y);


	


	while (x != Null) {
// still need to update costs. Hmm.
		if (y == left(x) {
			pair.s2 = join(pair.s2,x,right(x)); 
			y = x; x = p(x);
		} else {
			pair.s1 = join(left(x),x,pair.s1); y = x; x = p(x);
		}
	}

	if (pair.s1 != Null) 
		p(pair.s1) = Null; dmin(pair.s1) += dmin(i);
	} 
	if (pair.s2 != Null) 
		p(pair.s2) = Null; dmin(pair.s2) += dmin(i);
	} 
	dmin(i) += dcost(i); dcost(i) = 0;
	p(i) = left(i) = right(i) = Null;

	return pair;
}

cost pathset::c(item i) {
	cost s;
	s = dcost(i);
	while (i != Null) { s += dmin(i); i = p(i); }
	return s;
}

// Print all the paths in the path set
void pathset::print() {
	for (item i = 1; i <= n; i++) {
		if (p(i) == Null) {
			pprint(i,0);
			putchar('\n');
		}
	}
	putchar('\n');
}

// Print the path q, in path order and with actual costs.
void pathset::pprint(path q, cost mc) {
	if (q == Null) return;
	pprint(left(q),dmin(q)+mc);
	if (p(q) == Null) {
		if (n <= 26)
			printf("(%c*,%d) ",'a'+(q-1),dcost(q)+dmin(q)+mc);
		else
			printf("(%d*,%d) ",q,dcost(q)+dmin(q)+mc);
	} else {
		if (n <= 26)
			printf("(%c,%d) ",'a'+(q-1),dcost(q)+dmin(q)+mc);
		else
			printf("(%d,%d) ",q,dcost(q)+dmin(q)+mc);
	}
	pprint(right(q),dmin(q)+mc);
}

// Print the path q as a tree in-order; j is the depth of i
void pathset::tprint(path q, int j) {
	const int MAXDEPTH = 20;
	static char tabstring[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	if (q == Null) return;
	tprint(right(q),j+1);
	if (n <= 26)
		printf("%s(%c,%d,%d)\n",tabstring+MAXDEPTH-j,
			'a'+(q-1),dcost(q),dmin(q));
	else
		printf("%s(%d,%d,%d)\n",tabstring+MAXDEPTH-j,
			q,dcost(q),dmin(q));
	tprint(left(q),j+1);
}
