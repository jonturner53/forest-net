// usage: dtrees_d
//
// Dtrees_d is a test program for the dynamic trees data structure.
// It creates a set of single node trees and
// initializes the node costs to distinct random values in [1,n].
// These initial values are then printed out. The
// data structure can then be modified using any of
// the following commands.
//
//	froot i		return the root of the tree containing i
//	fcost i		print the last min cost item on the path
// 			from i to its tree root, and the cost
//	addcost i x	add x to the cost of every item in on path
//			from it to its tree root
//	link t i	combine the tree t with the one containing i at i
//	cut i		cut the subtree at i from the rest of its tree
//	print		print the underlying path set and successors
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "pathset.h"
#include "dtrees.h"

main(int argc, char* argv[]) {
	int i,j,k,n; string cmd;

	n = 26; dtrees T(n);
	int* vec = new int[n+1]; misc::genPerm(n,vec);
	for (i = 1; i <= n; i++) {
		T.addcost(i,vec[i]);
		cout << "("; misc::putAlpha(cout,i);
		cout << "," << setw(2) << vec[i] << ") ";
		if (i%10 == 0) cout << endl;
	}
	cout << endl;
	while (cin >> cmd) {
		cpair cp;
		if (misc::prefix(cmd,"froot")) {
			if (misc::getAlpha(cin,j)) {
				misc::putAlpha(cout,T.findroot(j));
				cout << endl;
			}
		} else if (misc::prefix(cmd,"fcost")) {
			if (misc::getAlpha(cin,j)) {
				cp = T.findcost(j);
				misc::putAlpha(cout,cp.s);
				cout << "," << cp.c << endl;
			}
		} else if (misc::prefix(cmd,"addcost")) {
			if (misc::getAlpha(cin,j) && misc::getNum(cin,k)) {
				T.addcost(j,k); cout << T << endl;
			}
		} else if (misc::prefix(cmd,"link")) {
			if (misc::getAlpha(cin,j) && misc::getAlpha(cin,k)) {
				T.link(j,k); cout << T << endl;
			}
		} else if (misc::prefix(cmd,"cut")) {
			if (misc::getAlpha(cin,j)) {
				T.cut(j); cout << T << endl;
			}
		} else if (misc::prefix(cmd,"print")) {
			cout << T;
		} else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
