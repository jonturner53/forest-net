// usage: bssets_d [ seed ]
//
// Bssets_d is a test program for the nssets data structure.
// It creates an instance of ssets which can be
// modified by the following commands.
// The initial key values are randomized using the optional
// argument to initialize the random number generator.
//
//	key i		print the key of item i
//	setkey i k	set the key of item i to k
//	find i		return sset containing item i
//	access k s	return item in sset s with key k
//	insert i s	insert item i into sset s
//	remove i s	remove item i from sset s
//	print		print the collection of ssets showing bst structure
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "bssets.h"

main(int argc, char* argv[]) {
	int h,j,k,n,seed; string cmd;
	n = 26; bssets F(n);

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
			if (misc::getAlpha(cin,j)) 
				misc::putAlpha(cout,F.find(j));
		} else if (misc::prefix(cmd,"access")) {
			if (misc::getNum(cin,k) && misc::getAlpha(cin,j))
				misc::putAlpha(cout,F.access(k,j));
		} else if (misc::prefix(cmd,"insert")) {
			if (misc::getAlpha(cin,j) && misc::getAlpha(cin,h)) {
				F.print(cout,F.insert(j,h)); cout << endl;
			}
		} else if (misc::prefix(cmd,"remove")) {
			if (misc::getAlpha(cin,h) && misc::getAlpha(cin,j)) {
				F.print(cout,F.remove(h,j)); cout << endl; 
			}
		/* taken out until these are implemented
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
		*/
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
