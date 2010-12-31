#include "stdinc.h"
#include "list.h"
#include "wdigraph.h"

void bfScan(wdigraph& D, vertex s, vertex p[], int d[]) {
// Compute shortest path tree using breadth-first scanning algorithm.
	int pass; vertex v,w,last; edge e;
	list q(D.n());

	for (v = 1; v <= D.n(); v++) { p[v] = Null; d[v] = BIGINT; }
	d[s] = 0; q &= s; pass = 0; last = s;

	while (!q.empty()) {
		v = q[1]; q <<= 1;
		for (e = D.firstOut(v); e != D.outTerm(v); e = D.next(v,e)) {
			w = D.head(e);
			if (d[v] + D.len(e) < d[w]) {
				d[w] = d[v] + D.len(e); p[w] = v;
				if (!q.mbr(w)) q &= w;
			}
		}
		if (v == last && !q.empty()) { pass++; last = q.tail(); }
		if (pass == D.n()) fatal("bfScan: graph has negative cycle");
	}
}
