// usage: sass_d [ seed ]
//
// Sbsts_d is a test program for the sass data structure.
// It creates an instance of sass which can be
// modified by the following commands. The key values
// are initialized randomly, using the optional seed.
//
//	key i		print the key of node i
//	setkey i k	set the key of node i to k
//	find i		return bst containing node i
//	access k s	return node in bst s with key k
//	insert i s	insert node i into bst s
//	remove i s	remove node i from bst s
//	join s1 i s2	join bst s1, s2 at node i
//	split i s	split bst s on node i and print bst pair
//	print		print the nodes in all trees
//	quit		exit the program
//

#include "stdinc.h"
#include "misc.h"
#include "sass.h"

main(int argc, char* argv[]) {
	int h,j,k,n,seed; string cmd;
	n = 26;
	sass F(n);

	seed = 1; if (argc > 1) sscanf(argv[1],"%d",&seed);
	srandom(seed);
	int vec[n+1]; misc::genPerm(n,vec);
	for (j = 1; j <= n; j++) F.setkey(j,vec[j]);

	while (cin >> cmd) {
		if (misc::prefix(cmd,"key")) {
			if (misc::getAlpha(cin,j)) cout << F.key(j);
		} else if (misc::prefix(cmd,"setkey")) {
			if (misc::getAlpha(cin,j) && misc::getNum(cin,k))
				F.setkey(j,k);
		} else if (misc::prefix(cmd,"find")) {
			if (misc::getAlpha(cin,j)) {
				j = F.find(j);
				misc::putAlpha(cout,j); cout << endl;
				F.print(cout,j); cout << endl;
			}
		} else if (misc::prefix(cmd,"access")) {
			if (misc::getNum(cin,k) && misc::getAlpha(cin,j)) {
				j = F.access(k,j);
				misc::putAlpha(cout,j); cout << endl;
				F.print(cout,j); cout << endl;
			}
		} else if (misc::prefix(cmd,"insert")) {
			if (misc::getAlpha(cin,j) && misc::getAlpha(cin,h)) {
				F.print(cout,F.insert(j,h)); cout << endl;
			}
		} else if (misc::prefix(cmd,"remove")) {
			if (misc::getAlpha(cin,h) && misc::getAlpha(cin,j)) {
				F.print(cout,F.remove(h,j)); cout << endl;
			}
		} else if (misc::prefix(cmd,"join")) {
			if (misc::getAlpha(cin,h) && misc::getAlpha(cin,j) &&
			    misc::getAlpha(cin,k)) {
				F.print(cout,F.join(h,j,k)); cout << endl;
			}
		} else if (misc::prefix(cmd,"split")) {
			if (misc::getAlpha(cin,h) && misc::getAlpha(cin,j)) {
				spair sp = F.split(h,j);
				F.print(cout,sp.s1); cout << "   ";
				F.print(cout,sp.s2); cout << endl;
			}
		} else if (misc::prefix(cmd,"print")) {
			cout << F;
		} else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
