// usage: bfs
//
// Bfs reads a graph from stdin, and lists its vertices in breadth-first order
// starting from vertex 1.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "list.h"
#include "graph.h"

void bfs(graph&, vertex);

main() { graph G; cin >> G; bfs(G,1); }

void bfs(graph& G, vertex s) {
	vertex u,v; edge e; list q(G.n());
	bool *mark = new bool[G.n()+1];
	for (u = 1; u <= G.n(); u++) mark[u] = false;
	q &= s; mark[s] = true;
	while (!q.empty()) {
		u = q[1]; q <<= 1; 
		cout << " "; misc::putNode(cout,u,G.n());
		for (e = G.first(u); e != G.term(u); e = G.next(u,e)) {
			v = G.mate(u,e);
			if (!mark[v]) { q &= v; mark[v] = 1; }
		}
	}
	cout << endl;
	delete [] mark;
}
