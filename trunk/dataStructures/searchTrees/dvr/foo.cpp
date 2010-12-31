// usage: foo seed
//
// Test remove method for dkst with lots of random insertions and removals
//

#include "stdinc.h"
#include "misc.h"
#include "dkst.h"

main(int argc, char* argv[]) {
	sset h,i,j,r;
	int n,seed;
	string cmd;

	n = 1000; dkst F(n);
	seed = 1; if (argc > 1) sscanf(argv[1],"%d",&seed);
	srandom(seed);
	int vec[n+1]; misc::genPerm(n,vec);
	for (j = 1; j <= n; j++) F.setkey(j,vec[j],vec[vec[j]]);

	for (j = 2; j <= n; j++) F.insert(j,F.root(1));
	// cout << F;

	for (i = 1; i <= 10*n; i++) {
		r = F.root(1);
		j = randint(1,n);
		F.remove(j,r);
		for (h = 1; h <= n; h++) {
			if (F.key2(h) != vec[vec[h]]) {
				cout << "bad key2 value for " << h << endl;
				cout << F;
				exit(1);
			}
		}
		r = F.root(j == 1 ? 2 : 1);
		F.insert(j,r);
		// cout << F;
	}
}
