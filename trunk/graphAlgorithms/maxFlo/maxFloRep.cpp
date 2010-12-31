// usage:
//	maxFloRep method reps n m mss ec1 ec2 span
//
// MaxFloRep repeatedly generates a random graph and computes
// a maximum flow using the specified method.
// Reps is the number of repetitions.
// n is the number of vertices, p is the edge probability,
// mss is the number of source and sink edges
// ec1 is the mean edge capacity for the source/sink edges,
// ec2 is the mean edge capacity of the other edges.
// span is the maximum difference beteen the endpoint indices of
// each edge.
//

#include "stdinc.h"
#include "flograph.h"
#include "maxCap.h"
#include "capScale.h"
#include "shortPath.h"
#include "dinic.h"
#include "dinicDtrees.h"
#include "prePush.h"
#include "ppFifo.h"

main(int argc, char* argv[]) {
	int i, reps, n, m, mss, ec1, ec2, span, floVal;
	if (argc != 9 ||
	    sscanf(argv[2],"%d",&reps) != 1 ||
	    sscanf(argv[3],"%d",&n) != 1 ||
	    sscanf(argv[4],"%d",&m) != 1 ||
	    sscanf(argv[5],"%d",&mss) != 1 ||
	    sscanf(argv[6],"%d",&ec1) != 1 ||
	    sscanf(argv[7],"%d",&ec2) != 1 ||
	    sscanf(argv[8],"%d",&span) != 1)
		fatal("usage: maxFloRep method reps n m mss ec1 ec2 span");

	flograph G(n,m,1,2); 
	for (i = 1; i <= reps; i++) {
		G.rgraph(n,m,mss,span); G.randCap(ec1,ec2);

		if (strcmp(argv[1],"maxCap") == 0)
			maxCap(G,floVal);
		else if (strcmp(argv[1],"capScale") == 0)
			capScale(G,floVal);
		else if (strcmp(argv[1],"shortPath") == 0)
			shortPath(G,floVal);
		else if (strcmp(argv[1],"dinic") == 0)
			dinic(G,floVal);
		else if (strcmp(argv[1],"dinicDtrees") == 0)
			dinicDtrees(G,floVal);
		else if (strcmp(argv[1],"ppFifo") == 0)
			ppFifo(G,floVal,false);
		else if (strcmp(argv[1],"ppFifoBatch") == 0)
			ppFifo(G,floVal,true);
		else
			fatal("maxFloRep: undefined method");
	}
}
