// usage: badCase k
//
// BadCase generates a flow graph that requires a long time
// to complete, using most max flow algorithms.
// The example has approximately 18k vertices and
// 18k+k^2 edges.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "flograph.h"

main(int argc, char* argv[]) {
	int i,j, k, n, m, c1, c2, c3, c4, bl, br;
	edge e;

	if (argc != 2 ||
	   (sscanf(argv[1],"%d",&k)) != 1)
		fatal("usage badCase k");

	// determine first vertex in each group
	c1 = 2;			// start of short chain from source
	c2 = c1 + 4*(k-1)+1;	// start of long chain from source
	bl = c2 + 4*(k-1)+3;	// start of first group of vertices in bipartite graph
	br = bl + k;		// start of second group of vertices in bipartite graph
	c3 = br + k;		// start of long chain to sink
	c4 = c3 + 4*(k-1)+3;	// start of short chain to sink

	n = c4 + 4*(k-1)+1;
	m = 16*(k-1) + k*k + 8*k + 4;	
	flograph G(n,m,1,n);

	// build short chain from source
	for (i = 0; c1+i < c2; i++) {
		if ((i%4) == 0) { 
			e = G.join(G.src(),c1+i); G.changeCap(e,k*k);
		}
		if (c1+i < c2-1) { 
			e = G.join(c1+i,c1+i+1); G.changeCap(e,k*k*k);
		}
	}
	// build long chain from source
	for (i = 0; c2+i < bl; i++) {
		if ((i%4) == 0) { 
			e = G.join(G.src(),c2+i); G.changeCap(e,k*k);
		}
		if (c2+i < bl-1) { 
			e = G.join(c2+i,c2+i+1); G.changeCap(e,k*k*k);
		}
	}
	// connect source chains to bipartite graph
	for (i = 0; i < k; i++) {
		e = G.join(c2-1,bl+i); G.changeCap(e,k*k); 
		e = G.join(bl-1,br+i); G.changeCap(e,k*k);
	}
	// build central bipartite graph
	for (i = 0; i < k; i++) {
		for (j = 0; j < k; j++) {
			e = G.join(bl+i, br+j); G.changeCap(e,1);
		}
	}
	// connect bipartite graph to sink chains
	for (i = 0; i < k; i++) {
		e = G.join(bl+i,c3); G.changeCap(e,k*k); 
		e = G.join(br+i,c4); G.changeCap(e,k*k);
	}
	// build long chain to sink
	for (i = 0; c3+i < c4; i++) {
		if ((i%4) == 2) {
			e = G.join(c3+i,G.snk()); G.changeCap(e,k*k);
		}
		if (c3+i < c4-1) { 
			e = G.join(c3+i,c3+i+1); G.changeCap(e,k*k*k);
		}
	}
	// build short chain to sink
	for (i = 0; c4+i < n; i++) {
		if ((i%4) == 0) { 
			e = G.join(c4+i,G.snk()); G.changeCap(e,k*k);
		}
		if (c4+i < n-1) { 
			e = G.join(c4+i,c4+i+1); G.changeCap(e,k*k*k);
		}
	}
	cout << G;
}
