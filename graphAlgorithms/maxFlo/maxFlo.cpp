// usage: maxFlo method
//
// MaxFlo reads a flograph from stdin, computes a maximum flow
// using the method specified by the argument and then prints the
// flograph with the max flow.
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

main(int argc, char *argv[]) {
	int floVal;
	flograph G; cin >> G;

	if (argc != 2) fatal("usage: maxFlo method");

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
		fatal("maxFlo: undefined method");

	cout << G << "total flow of " << floVal << endl;
	exit(0);
}
