// usage: listset_d
//
// Listset_d is a test program for the listset data structure.
//
//	enq i j		add item i to end of list j
//	push i j	add item i to front of list j
//	deq j		remove and return first item on list j
//	empty j		return true if list j is empty
//	member i	return true if i is member of some list
//	successor i	return the successor of item i
//	head j 		return first item on list j
//	tail j 		return last item on list j
//	print		print all non-empty lists
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "listset.h"

main() {
	int i,j; string cmd; listset L;

        while (cin >> cmd) {
                if (misc::prefix(cmd,"enq")) {
                        if (misc::getAlpha(cin,i) && misc::getNum(cin,j)) {
				 L.enq(i,j); L.print(cout,j);
                        }
                } else if (misc::prefix(cmd,"push")) {
                        if (misc::getAlpha(cin,i) && misc::getNum(cin,j)) {
				 L.push(i,j); L.print(cout,j);
                        }
                } else if (misc::prefix(cmd,"deq")) {
                        if (misc::getNum(cin,j)) {
				L.deq(j); L.print(cout,j);
			}
                } else if (misc::prefix(cmd,"member")) {
                        if (misc::getAlpha(cin,i)) {
				cout << (L.mbr(i) ? "true" : "false") << endl;
			}
                } else if (misc::prefix(cmd,"empty")) {
                        if (misc::getNum(cin,j)) {
				cout << (L.empty(j) ? "true" : "false") << endl;
			}
                } else if (misc::prefix(cmd,"successor")) {
                        if (misc::getAlpha(cin,i)) {
				misc::putAlpha(cout,L.suc(i)); cout << endl;
			}
                } else if (misc::prefix(cmd,"head")) {
                        if (misc::getNum(cin,j)) {
				misc::putAlpha(cout,L.head(j)); cout << endl;
			}
                } else if (misc::prefix(cmd,"tail")) {
                        if (misc::getNum(cin,j)) {
				misc::putAlpha(cout,L.tail(j)); cout << endl;
			}
                } else if (misc::prefix(cmd,"print")) {
			cout << L;
                } else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}

