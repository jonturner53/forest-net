#include "stdinc.h"
#include "dheap.h"
#include "wdigraph.h"

void dijkstra(wdigraph& D, vertex u, vertex p[], int d[]) {
// Find a shortest path tree of D using Dijkstra's algorithm
// and return it in p as an array of parent pointers, with
// d giving the shortest path distances.
	vertex v,w; edge e;
	dheap S(D.n(),4);

	for (v = 1; v <= D.n(); v++) { p[v] = Null; d[v] = BIGINT; }
	d[u] = 0; S.insert(u,0);
	while (!S.empty()) {
		v = S.deletemin();
		for (e = D.firstOut(v); e != D.outTerm(v); e = D.next(v,e)) {
			w = D.head(e);
			if (d[w] > d[v] + D.len(e)) {
				d[w] = d[v] + D.len(e); p[w] = v;
				if (S.member(w)) S.changekey(w,d[w]);
				else S.insert(w,d[w]);
			}
		}
	}
	return;
}
