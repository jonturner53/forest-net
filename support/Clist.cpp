#include "Clist.h"

Clist::Clist(int N1) : N(N1) {
	assert(N >= 0);
	node = new lnode[N+1];
	for (item i = 0; i <= N; i++) { node[i].next = node[i].prev = i; }
}

Clist::~Clist() { delete [] node; }

/** Remove an item from its list.
 *  This method turns the item into a singleton list.
 *  @param i is a list item
 */
void Clist::remove(item i) {
	assert(0 <= i && i <= N);
	node[node[i].prev].next = node[i].next;
	node[node[i].next].prev = node[i].prev;
	node[i].next = node[i].prev = i;
}

/** Join two lists together.
 *  @param i is a list item
 *  @param j is a list item on some other list
 *  Note: the method will corrupt the data structure if
 *  i and j already belong to the same list; it's the caller's
 *  responsiblity to ensure this doesn't happen
 */
void Clist::join(item i, item j) {
	assert(0 <= i && i <= N && 0 <= j && j <= N);
	if (i == 0 || j == 0) return;
	node[node[i].next].prev = node[j].prev;
	node[node[j].prev].next = node[i].next;
	node[i].next = j; node[j].prev = i;
}

/** Produce a string representation of the object.
 *  @param s is a string in which the result will be returned
 *  @return a reference to s
 */
string& Clist::toString(string& s) const {
	s.clear();
	item i, j; string s1;
	int *mark = new int[N+1];
	for (i = 1; i <= N; i++) mark[i] = 0;
	for (i = 1; i <= N; i++) {
		if (mark[i]) continue; 
		if (i > 1) s += ", ";
		mark[i] = 1;
		s += "(";
		if (N <= 26) s += ((char) ('a'+(i-1)));
		else s += Misc::num2string(i,s1);
		for (j = node[i].next; j != i; j = node[j].next) {
			mark[j] = 1;
			s += " ";
			if (N <= 26) s = s + ((char) ('a'+(j-1)));
			else s += Misc::num2string(j,s1);
		}
		s += ")";
	}
	delete [] mark;
	return s;
}
