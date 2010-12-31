// usage: stepsCnt n seed
//
// StepsCnt performs n random change operations on a steps
// data structure followed by n random findmin operations.
//

#include "stdinc.h"
#include "misc.h"
#include "steps.h"

main(int argc, char* argv[]) {
	int lo, hi, dy, n, seed;

	if (argc != 3 ||
	    sscanf(argv[1],"%d",&n) != 1 ||
	    sscanf(argv[2],"%d",&seed) != 1)
		fatal("usage: stepsCnt n seed");

	steps S(n); srandom(seed);

	int i;
	for (i = 1; i <= n; i++) {
		lo = randint(0,10*n); hi = randint(lo,10*n); dy = randint(-n,n);
		S.change(lo,hi,dy);
	}
	for (i = 1; i <= n; i++) {
		lo = randint(0,10*n); hi = randint(lo,10*n);
		S.findmin(lo,hi);
	}
}
