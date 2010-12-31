// usage: list_d
//
// List_d is a test program for the list data structure.
//
//	append j	add j to end of list
//	retrieve i	print i-th item on the list
//	remove i	remove the first i items
//	successor j	return the successor of j
//	member j	return true if j is member
//	print		print the list
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "list.h"

main() {
	int j; string cmd; list L;

        while (cin >> cmd) {
                if (misc::prefix(cmd,"append")) {
                        if (misc::getAlpha(cin,j)) {
				 L &= j; cout << L << endl;
                        }
                } else if (misc::prefix(cmd,"retrieve")) {
                        if (misc::getNum(cin,j)) {
				misc::putAlpha(L[j]); cout << endl;
                        }
                } else if (misc::prefix(cmd,"remove")) {
                        if (misc::getNum(cin,j)) {
				L <<= j; cout << L << endl;
			}
                } else if (misc::prefix(cmd,"successor")) {
                        if (misc::getAlpha(cin,j)) {
				misc::putAlpha(L.suc(j)); cout << endl;
			}
                } else if (misc::prefix(cmd,"member")) {
                        if (misc::getAlpha(cin,j)) {
				cout << (L.mbr(j) ? "true" : "false") << endl;
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

