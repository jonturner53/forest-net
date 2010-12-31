// usage: mcFlo method
//
// mcFlo reads a wflograph from stdin, computes a min cost maximum flow
// using the method specified by the argument and then prints the
// wflograph with the computed flow.
//

#include "stdinc.h"
#include "wflograph.h"
#include "cycRed.h"
#include "lcap.h"

main(int argc, char *argv[]) {
	flow floVal; cost floCost;
	wflograph G; cin >> G;
	
	if (argc != 2) fatal("usage: mcFlo method");

	if (strcmp(argv[1],"cycRed") == 0)
		cycRed(G,floVal,floCost);
	else if (strcmp(argv[1],"lcap") == 0)
		lcap(G,floVal,floCost,false);
	else if (strcmp(argv[1],"mostNeg") == 0)
		lcap(G,floVal,floCost,true);
	else
		fatal("mcFlo: undefined method");

	cout << G;
	cout << "flow value is " << floVal
	     << " and flow cost is " << floCost << endl;
}
