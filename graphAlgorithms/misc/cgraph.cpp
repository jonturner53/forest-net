// usage: cgraph type
//
// Copy a graph of specified type from stdin to stdout.
// Why you ask? To test input and output operators, of course.
// We also do an assignment in between input and output,
// in order to test the assignment operator.
//
// The allowed values of type are graph, wgraph,
// digraph, wdigraph, flograph, wflograph.

#include "stdinc.h"
#include "wgraph.h"
#include "wdigraph.h"
#include "wflograph.h"

main(int argc, char *argv[]) { 

	if (argc != 2) fatal("usage: cgraph type");

	if (strcmp(argv[1],"graph") == 0) {
		graph G; cin >> G; graph G1(1,1); G1 = G; cout << G1;
	} else if (strcmp(argv[1],"wgraph") == 0) {
		wgraph wG; cin >> wG; wgraph wG1(1,1); 
		wG1 = wG; cout << wG1;
	} else if (strcmp(argv[1],"digraph") == 0) {
		digraph D; cin >> D; digraph D1(1,1); D1 = D; cout << D1;
	} else if (strcmp(argv[1],"wdigraph") == 0) {
		wdigraph wD; cin >> wD; wdigraph wD1(1,1); wD1 = wD; cout << wD1;
	} else if (strcmp(argv[1],"flograph") == 0) {
		flograph F; cin >> F; flograph F1(2,1); F1 = F; cout << F1;
	} else if (strcmp(argv[1],"wflograph") == 0) {
		wflograph wF; cin >> wF;
		wflograph wF1(1,1); wF1 = wF; cout << wF1;
	} else {
		fatal("usage: cgraph type");
	}
}
