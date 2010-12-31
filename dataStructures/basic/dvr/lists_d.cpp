// usage: lists_d
//
// Lists_d is a test program for the lists data structure.
//
//	insert v j	add value v at front of list j
//	remove j	remove first item from list j
//	remove v j	remove item with value v from list j
//	value j		return the value of item j
//	successor j	return the successor of j
//	clear j		remove all elements from list j
//	print 		print all the lists
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "lists.h"

main() {
	int v, j; string cmd; lists L(20);

        while (cin >> cmd) {
                if (misc::prefix(cmd,"insert")) {
                        if (misc::getNum(cin,v) && misc::getNum(cin,j)) {
				 j = L.insert(v,j); L.print(cout,j);
                        }
                } else if (misc::prefix(cmd,"remove")) {
                        if (misc::getNum(cin,v)) {
				if (misc::getNum(cin,j)) j = L.remove(v,j);
				else 			 j = L.remove(v);
				L.print(cout,j);
                        }
                } else if (misc::prefix(cmd,"value")) {
                        if (misc::getNum(cin,j)) {
				cout << L.value(j) << endl;
			}
                } else if (misc::prefix(cmd,"successor")) {
                        if (misc::getNum(cin,j)) {
				cout << L.suc(j) << endl;
			}
                } else if (misc::prefix(cmd,"clear")) {
                        if (misc::getNum(cin,j)) {
				L.clear(j); cout << L;
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

