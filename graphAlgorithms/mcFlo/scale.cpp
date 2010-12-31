// WORK IN PROGRESS - convert to implement the scaling algorithm
// Coding started, but not yet complete.

// usage: scale
//
// Scale reads a flograph from stdin, and computes a minimum cost
// maximum flow between vertices 1 and n using the scaling algorithm
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "flograph.h"
#include "list.h"
#include "dheap.h"
#include "dinic.h"

void scale(flograph&);
void initLabels(flograph&, int*);
void findpath(flograph&, int*, list&);

main() {
	flograph G;
	G.get(stdin); lcap(G); G.put(stdout);
}

class scale {
        flograph* G;            // graph we're finding flow on
        int*    lab;            // lab[u]=distance label for transformed costs
        int*    excess;         // excess[u]=excess flow entering u
	list*	S;		// Source vertices
	list*	T;		// Sink vertices
	int	Delta;		// Current scaling factor
	vertex	cs;		// current source vertex

        int     findpath(vertex,list&); // find augmenting path
        void    augment(list&); // add flow to augmenting path
        int     newphase();     // prepare for a new phase
        int     initLabels();   // assign initial values to labels
public: scale(flograph*);	// main program
};

scale::scale(flograph& G1) {
void scale(flograph& G) {
// Find minimum cost maximum flow in G using the scaling algorithm.
// Assumes that the original graph has no negative cost cycles.
	int flo;

        vertex u; edge e; 
	flow maxcap;
	list p(G.m);

        G = &G1;
	lab = new int[G.n+1]; excess = new int[G.n+1];
	S = new list(G.n); T = new list(G.n);

	for (u = 1; u <= G.n; u++) lab[u] = excess[u] = 0;

	// Initialize scaling factor
	maxcap = 0;
	for (e = 1; e <= G.m; e++) 
		maxcap = max(maxcap,G.cap(G.tail(e),e));
	for (Delta = 1; 2*Delta <= maxcap; Delta <<= 1) {}

	// Determine a max flow so that we can initialize excess
	// values at s and t
	dinic(G,flo);
	for (e = G->first(1); e != Null; e = G->next(1,e)) 
		excess[1] += G->f(1,e);
	excess[G->n] = -excess[1];
	for (e = G->first(1); e != Null; e = G->next(1,e)) {
		u = G->tail(e); G->addflow(u,e,-(G->f(u,e)));
	}

	initLabels();

	while (newphase()) {
		while (findpath(u,p))  {
			augment(p);
		}
		Delta /= 2;
	}
	delete [] lab; delete [] excess;
	delete S; delete T;
}

void initLabels() {
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

void newPhase() {
// Do start of phase processing.

	// identify unbalanced vertices
	S.clear(); T.clear();
	for (u = 2; u < G->n; u++) {
		if (excess[u] > 0) S &= u;
		else if (excess[u] < 0) T &= u;
	}
	// Put source and sink at end of lists.
	S &= 1; T &= G->n;


	// If any edge violates labeling condition, add Delta units of
	// flow to it. This eliminates it from the residual graph for
	// the current scaling factor.

	vertex u,v; edge e;

	for (e = 1; e <= G.m; e++) {
		u = G.tail(e); v = G.head(e);
		if (G.res(u,e) >= Delta) {
			if (G.cost(u,e) + lab[u] - lab[v] < 0) {
				G.addflow(u,e,Delta);
				excess[u] -= Delta; excess[v] += Delta;
			}
		}
		if (G.res(v,e) >= Delta) {
			if (G.cost(v,e) + lab[v] - lab[u] < 0) {
				G.addflow(v,e,Delta);
				excess[v] -= Delta; excess[u] += Delta;
			}
		}
	}
}


void findpath(flograph& G, int Delta, int* lab, list& p) {
// Find a least cost augmenting path and update the labels.
	vertex u,v; edge e;
	edge *pathedge = new edge[G.n+1];
	int *c = new int[G.n+1];
	dheap S(G.n,2);

	for (u = 1; u <= G.n; u++) { pathedge[u] = Null; c[u] = BIGINT; }
	c[1] = 0; S.insert(1,0);
	while (!S.empty()) {
		u = S.deletemin();
		for (e = G.first(u); e != Null; e = G.next(u,e)) {
			if (G.res(u,e) < Delta) continue;
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

void augment(flograph& G,p) {
// Augment the flow G along the path p.
	vertex u; edge e;
	
	f = BIGINT;
	u = 1;
	for (e = p(1); e != Null; e = p.suc(e)) {
		f = min(f,G.res(u,e)); u = G.mate(u,e);
	}
	u = 1;
	for (e = p(1); e != Null; e = p.suc(e)) {
		G.addflow(u,e,f); u = G.mate(u,e);
	}
	return;
}

