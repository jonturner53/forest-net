// usage: prtn
//
// Prtn_d is a test program for the partition data structure.
// It creates an initial partition on {1,...,26} which can be
// modified by the following commands.
//
//	find x		print the canonical element of set containing x
//	link x y	link sets whose canonical elements are x and y
//	print		print the list
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "prtn.h"

main() {
	int j, k; string cmd; prtn P;

	while (cin >> cmd) {
                if (misc::prefix(cmd,"find")) {
                        if (misc::getAlpha(cin,j)) {
                                 misc::putAlpha(cout,P.find(j)); cout << endl;
                        }
                } else if (misc::prefix(cmd,"link")) {
                        if (misc::getAlpha(cin,j) && misc::getAlpha(cin,k)) {
				P.link(j,k); cout << P;
                        }
                } else if (misc::prefix(cmd,"print")) {
			cout << P;
                } else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}

