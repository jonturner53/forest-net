// usage: lmp_d
//
// Lmp_d is a test program for the lmp data structure.
// The following operations can be performed.
//
//	insert a k h	insert IP address prefix a/k with next hop h
//	insert a k 	remove IP address prefix a/k
//	lookup a	lookup longest matching prefix for address a
//	print 		print the entire tree
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "lmp.h"

main(int argc, char* argv[]) {
	int k,h; string cmd;
	ipa_t adr;

	lmp T(100);
	while (cin >> cmd) {
		if (misc::prefix(cmd,"insert")) {
			if (misc::getIpAdr(cin,adr) &&
			    misc::getNum(cin,k) && misc::getNum(cin,h)) {
				T.insert(adr,k,h);
				T.print();
			}
		} else if (misc::prefix(cmd,"remove")) {
			if (misc::getIpAdr(cin,adr) && misc::getNum(cin,k)) {
				T.remove(adr,k);
				T.print();
			}
		} else if (misc::prefix(cmd,"lookup")) {
			if (misc::getIpAdr(cin,adr)) {
				cout << "nexthop=" << T.lookup(adr) << endl;
			}
		} else if (misc::prefix(cmd,"print")) {
			T.print();
		} else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
