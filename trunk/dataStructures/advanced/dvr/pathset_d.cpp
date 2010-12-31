// usage: pathset_d
//
// Pathset_d is a test program for the pathset data structure.
// It creates a pathset and initializes the
// node costs to distinct random values. These
// initial values are then printed out. The
// data structure can then be modified using any of
// the following commands.
//
//	fpath i		print the path containing i
//	ftail p		print the tail of the path p
//	fpcost p	print the last item on the path of min cost
//			and its cost
//	addpcost p x	add x to the cost of every item in p
//	join p1 i p2	join paths p1, p2 at item i
//	split i		split path containing i at i and print the
//			two paths resulting from the split
//	print		print the path set
//	pprint p	print the path p, showing actual costs
//	tprint p	print the path p in tree form
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "pathset.h"

main(int argc, char* argv[]) {
	int i,j,k,m,n; string cmd;

	n = 26; pathset P(n);
	int* vec = new int[n+1]; misc::genPerm(n,vec);
	for (i = 1; i <= n; i++) {
		P.addpathcost(i,vec[i]);
		cout << "("; misc::putAlpha(cout,i);
		cout << "," << setw(2) << vec[i] << ") ";
		if (i%10 == 0) cout << endl;
	}
	cout << endl;
	while (cin >> cmd) {
		cpair cp;
		if (misc::prefix(cmd,"fpath")) {
			if (misc::getAlpha(cin,j)) {
				P.pprint(cout,P.findpath(j)); cout << endl;
			}
		} else if (misc::prefix(cmd,"ftail")) {
			if (misc::getAlpha(cin,j)) {
				misc::putAlpha(cout,P.findtail(j));
				cout << endl;
			}
		} else if (misc::prefix(cmd,"fpcost")) {
			if (misc::getAlpha(cin,j)) {
				cp = P.findpathcost(j);
				misc::putAlpha(cout,cp.s);
				cout << "," << cp.c << endl;
			}
		} else if (misc::prefix(cmd,"addpcost")) {
			if (misc::getAlpha(cin,j) && misc::getNum(cin,k)) {
				P.addpathcost(j,k);
				P.pprint(cout,j); cout << endl;
			}
		} else if (misc::prefix(cmd,"join")) {
			if (misc::getAlpha(cin,j) && misc::getAlpha(cin,k) &&
			                            misc::getAlpha(cin,m)) {
				P.join(j,k,m);
				P.pprint(cout,k); cout << endl;
			}
		} else if (misc::prefix(cmd,"split")) {
			if (misc::getAlpha(cin,j)) {
				ppair pp = P.split(j);
				P.pprint(cout,pp.s1); cout << endl;
				P.pprint(cout,j);     cout << endl;
				P.pprint(cout,pp.s2); cout << endl;
			}
		} else if (misc::prefix(cmd,"print")) {
			cout << P;
		} else if (misc::prefix(cmd,"pprint")) {
			if (misc::getAlpha(cin,j)) {
				P.pprint(cout,j); cout << endl;
			}
		} else if (misc::prefix(cmd,"tprint")) {
			if (misc::getAlpha(cin,j)) {
				P.tprint(cout,j,0); cout << endl;
			}
		} else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
