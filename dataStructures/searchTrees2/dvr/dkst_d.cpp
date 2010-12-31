// usage: dkst_d [ seed ]
//
// Dkst_d is a test program for the dkst data structure.
// It creates an instance of dkst which can be
// modified by the following commands. The key values
// are initialized randomly, using the optional seed.
//
//	key i		print the keys of node i
//	setkey i k1 k2	set the keys of node i to k1, k2
//	find i		return sset containing node i
//	first s		return first item in s
//	next i		return next item after i
//	min2 s		return minimum key2 value for sset
//	change2 diff s	change key2 values for a sset
//	access k s	return node in sset s with key k
//	insert i s	insert node i into sset s
//	remove i s	remove node i from sset s
//	join s1 i s2	join sset s1, s2 at node i
//	split i s	split sset s on node i and print sset pair
//	print		print the nodes in all trees
//	quit		exit the program
//

#include "stdinc.h"
#include "misc.h"
#include "dkst.h"

main(int argc, char* argv[]) {
	sset h,j,k;
	keytyp k1,k2,lo,hi;
	int n,seed;
	string cmd;

	n = 26; dkst F(n);
	seed = 1; if (argc > 1) sscanf(argv[1],"%d",&seed);
	srandom(seed);
	int vec[n+1]; misc::genPerm(n,vec);
	for (j = 1; j <= n; j++) F.setkey(j,vec[j],vec[vec[j]]);

	while (cin >> cmd) {
		if (misc::prefix(cmd,"key")) {
			if (misc::getAlpha(cin,j)) cout << F.key(j);
		} else if (misc::prefix(cmd,"setkey")) {
			if (misc::getAlpha(cin,j) &&
			    misc::getNum(cin,k1) && misc::getNum(cin,k2))
				F.setkey(j,k1,k2);
		} else if (misc::prefix(cmd,"find")) {
			if (misc::getAlpha(cin,j)) {
				j = F.find(j);
				misc::putAlpha(cout,j); cout << endl;
				F.print(cout,j); cout << endl;
			}
		} else if (misc::prefix(cmd,"first")) {
			if (misc::getAlpha(cin,j)) {
				j = F.first(j);
				misc::putAlpha(cout,j); cout << endl;
			}
		} else if (misc::prefix(cmd,"next")) {
			if (misc::getAlpha(cin,j)) {
				j = F.next(j);
				misc::putAlpha(cout,j); cout << endl;
			}
		} else if (misc::prefix(cmd,"access")) {
			if (misc::getNum(cin,k1) && misc::getAlpha(cin,j)) {
				j = F.access(k1,j);
				misc::putAlpha(cout,j); cout << endl;
				cout << F;
			}
		} else if (misc::prefix(cmd,"min2")) {
			if (misc::getAlpha(cin,j)) {
				cout << F.min2(j) << endl;
			}
		} else if (misc::prefix(cmd,"change2")) {
			if (misc::getNum(cin,k1) && misc::getAlpha(cin,j)) {
				F.change2(k1,j);
				cout << F;
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
