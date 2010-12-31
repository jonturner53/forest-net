// Usage:
//      bwMinCostFlow2 reps n p maxwt seed
//
// bwMinCostFlow2 generates repeatedly generates random bipartite graphs
// and then computes a maximum size matching using the augmenting
// path algorithm
//
// Reps is the number of repeated computations,
// 2n is the number of vertices, p is the edge probability,
// and maxwt is the maximum edge weight.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/wgraph.h"
#include "basic/list.h"

void bwMinCostFlow(wgraph&, list&);

main(int argc, char* argv[])
{
        int reps, n, size, seed, maxwt;
        vertex u; edge e;
        double p;
        if (argc != 6 ||
            sscanf(argv[1],"%d",&reps) != 1 ||
            sscanf(argv[2],"%d",&n) != 1 ||
            sscanf(argv[3],"%lf",&p) != 1 ||
            sscanf(argv[4],"%d",&maxwt) != 1 ||
            sscanf(argv[5],"%d",&seed) != 1)
                fatal("usage: bwMinCostFlow2 reps n p maxwt seed");

        srandom(seed);
        wgraph G;
        list M(max(1000,int(1.1*n*n*p)));
        for (int i = 1; i <= reps; i++) {
                G.rbigraph(n,p,maxwt);
                bwMinCostFlow(G,M);
                M.clear();
        }
}

#include "basic/flograph.h"

bit getCut(wgraph&, list&);
void lcap(flograph&);

void bwMinCostFlow(wgraph& G, list& M) {
// Find a maximum weight matching in the bipartite graph G and
// return it as a vector of edges, with a Null edge used to
// indicate the end of the vector. The list X, is a set of
// vertices that defines a bipartition.
	vertex u,v; edge e;
	list X(G.n);

	if (!getCut(G,X)) fatal("bwMinCostFlow: graph is not bipartite");

	// Create flograph corresponding to G.
	// Do this, so that corresponding edges have same
	// edge numbers in F and G.
	flograph F(2+G.n,G.n+G.m);
	for (e = 1; e <= G.m; e++) {
		if (X.mbr(G.left(e))) u = G.left(e);
		else u = G.right(e);
		v = G.mate(u,e);
		F.join(u+1,v+1,1,-G.w(e)); 	// shift vertex numbers 
					// to make room for source
	}
	for (u = 1; u <= G.n; u++) {
		if (X.mbr(u)) F.join(1,u+1,1,0);
		else F.join(u+1,F.n,1,0);
	}

	lcap(F);

	for (e = 1; e <= G.m; e++) {
		if (F.f(1+G.left(u),e) != 0) M &= e;
	}
}

bit getCut(wgraph& G, list& X) {
// Return true if G is bipartite, else false.
// If G is bipartite, return a set of vertices X that defines
// a cut that is crossed by all edges in G.
	vertex u, v, w; edge e;
	list queue(G.n);
	typedef enum stype {unreached, odd, even};
	stype *state = new stype[G.n+1];

	for (u = 1; u <= G.n; u++) state[u] = unreached;
	u = 1;
	while (u <= G.n) {
		state[u] = even;
		queue &= u; X &= u;
		while (queue(1) != Null) {
			v = queue(1); queue <<= 1;
			for (e = G.first(v); e != Null; e = G.next(v,e)) {
				w = G.mate(v,e);
				if (state[w] == state[v]) {
					return false;
				}
				if (state[w] == unreached) {
					state[w] = state[v]==even ? odd : even;
					if (state[w] == even) X &= w;
					queue &= w;
				}
			}
		}
		for (u++; u <= G.n && state[u] != unreached; u++)
			;
	}
	delete[] state;
	return true;
}

void initLabels(flograph&, int*);
void findpath(flograph&, int*, list&);

void lcap(flograph& G) {
// Find minimum cost (but not max value) flow in G using
// the least-cost augmenting path algorithm.  Halt when the next
// augmenting path has non-negative cost.

	vertex u; edge e; flow f;
	list p(G.m);
	int c, *lab = new int[G.n+1];
	for (u = 1; u <= G.n; u++) lab[u] = 0;

	// Assign initial values for the labels to make effective
	// edge costs non-negative
	initLabels(G,lab);

	findpath(G,lab,p);
	while (p(1) != Null) {
		f = BIGINT; c = 0;
		u = 1;
		for (e = p(1); e != Null; e = p.suc(e)) {
			f = min(f,G.res(u,e)); c += G.cost(u,e);
			u = G.mate(u,e);
		}
		if (c >= 0) break;
		u = 1;
		for (e = p(1); e != Null; e = p.suc(e)) {
			G.addflow(u,e,f); u = G.mate(u,e);
		}
		findpath(G,lab,p);
	}
	delete [] lab;
	return;
}

void initLabels(flograph& G, int *lab) {
// Compute values for labels that give non-negative transformed costs.
// The labels are the least cost path distances from an imaginary
// vertex with a length 0 edge to every vertex in the graph.
// Uses the breadth-first scanning algorithm to compute shortest
// paths.
        int pass;
	vertex v, w,last; edge e;
	vertex *p = new vertex[G.n+1];
        list q(G.n);

        for (v = 1; v <= G.n; v++) {
		p[v] = Null; lab[v] = 0; q &= v;
	}

        pass = 0; last = G.n;

        while (q(1) != Null) {
                v = q(1); q <<= 1;
                for (e = G.first(v); e != Null; e = G.next(v,e)) {
                        w = G.head(e);
			if (w == v) continue;
                        if (lab[w] > lab[v] + G.cost(v,e)) {
                                lab[w] = lab[v] + G.cost(v,e); p[w] = v;
                                if (!q.mbr(w)) q &= w;
                        }
                }
                if (v == last && q(1) != Null) { pass++; last = q.tail(); }
                if (pass == G.n) fatal("initLabels: negative cost cycle");
        }
}

#include "heaps/dheap.h"

void findpath(flograph& G, int* lab, list& p) {
// Find a least cost augmenting path.
	vertex u,v; edge e;
	edge *pathedge = new edge[G.n+1];
	int *c = new int[G.n+1];
	dheap S(G.n,2);

	for (u = 1; u <= G.n; u++) { pathedge[u] = Null; c[u] = BIGINT; }
	c[1] = 0; S.insert(1,0);
	while (!S.empty()) {
		u = S.deletemin();
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			if (G.res(u,e) == 0) continue;
			v = G.mate(u,e);
			if (c[v] > c[u] + G.cost(u,e) + (lab[u] - lab[v])) {
				pathedge[v] = e;
				c[v] = c[u] +  G.cost(u,e) + (lab[u] - lab[v]);
				if (!S.member(v)) S.insert(v,c[v]);
				else S.changekey(v,c[v]);
			}
		}
	}
	p.clear();
	for (u = G.n; pathedge[u] != Null; u = G.mate(u,pathedge[u])) {
		p.push(pathedge[u]);
	}
	for (u = 1; u <= G.n; u++) {
		lab[u] += c[u];
	}
	delete [] pathedge; delete [] c;
	return;
}
