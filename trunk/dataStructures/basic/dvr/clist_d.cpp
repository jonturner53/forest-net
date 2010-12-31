// usage: clist_d
//
// clist_d is a test program for the clist data structure.
//
//	remove i	remove i from its list
//	join i j	join the lists containing i and j
//	successor j	return the successor of j
//	predecessor j	return the predecessor of j
//	print		print the list
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "clist.h"

main() {
	int j,k; string cmd; clist L;

        while (cin >> cmd) {
                if (misc::prefix(cmd,"remove")) {
                        if (misc::getAlpha(cin,j)) {
				 L.remove(j); cout << L << endl;
                        }
                } else if (misc::prefix(cmd,"join")) {
                        if (misc::getAlpha(cin,j) && misc::getAlpha(cin,k)) {
				L.join(j,k); cout << L << endl;
                        }
                } else if (misc::prefix(cmd,"successor")) {
                        if (misc::getAlpha(cin,j)) {
				cout << misc::nam(L.suc(j)) << endl;
                        }
                } else if (misc::prefix(cmd,"predecessor")) {
                        if (misc::getAlpha(cin,j)) {
				cout << misc::nam(L.pred(j)) << endl;
                        }
                } else if (misc::prefix(cmd,"print")) {
			cout << L << endl;
                } else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
