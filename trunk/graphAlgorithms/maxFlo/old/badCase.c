// usage: badCase k
//
// BadCase generates a flow graph that requires a long time
// to complete, using most max flow algorithms.
// The example has approximately 18k vertices and
// 18k+k^2 edges.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/flograph.h"

main(int argc, char* argv[]) {
	int i,j, k, n, m, c1, c2, c3, c4, bl, br;

	if (argc == 3 ||
	   (sscanf(argv[1],"%d",&k)) != 1)
		fatal("usage badCase k");

	// determine first vertex in each group
	c1 = 2;			// top left chain
	c2 = c1 + 4*(k-1)+1;	// top right chain
	c3 = c2 + 4*(k-1)+3;	// bottom left chain
	c4 = c3 + 4*(k-1)+3;	// top right chain
	bl = c4 + 4*(k-1)+1;	// left group of vertices in bipartite graph
	br = bl + k;		// right group of vertices in bipartite graph

	n = br+k;
	m = 16*(k-1) + k*k + 8*k + 4;	
	flograph G(n,m);

	// build top-left chain
	for (i = 0; c1+i < c2; i++) {
		if ((i%4) == 0) G.join(1,c1+i,k*k,0);
		if (c1+i < c2-1) G.join(c1+i+1,c1+i,k*k*k,0);
	}
	// build top-right chain
	for (i = 0; c2+i < c3; i++) {
		if ((i%4) == 2) G.join(1,c2+i,k*k,0);
		if (c2+i < c3-1) G.join(c2+i+1,c2+i,k*k*k,0);
	}
	// build bottom-left chain
	for (i = 0; c3+i < c4; i++) {
		if ((i%4) == 2) G.join(c3+i,n,k*k,0);
		if (c3+i < c4-1) G.join(c3+i,c3+i+1,k*k*k,0);
	}
	// build bottom-right chain
	for (i = 0; c4+i < bl; i++) {
		if ((i%4) == 0) G.join(c4+i,n,k*k,0);
		if (c4+i < bl-1) G.join(c4+i,c4+i+1,k*k*k,0);
	}
	// build central bipartite graph
	for (i = 0; i < k; i++)
		for (j = 0; j < k; j++)
			G.join(bl+i, br+j, 1, 0);
	// connect bipartite graph to chains
	for (i = 0; i < k; i++) {
		G.join(c1,bl+i,k*k,0); G.join(c2,br+i,k*k,0);
		G.join(bl+i,c3,k*k,0); G.join(br+i,c4,k*k,0);
	}
	G.put(stdout);
}
