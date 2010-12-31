// usage: lheaps_d 
//
// Lheaps_d is a test program for the lheaps data structure.
// It creates an instance of lheaps which can be modified by
// the following commands.
//
//	key i		print the key of item i
//	setkey i k	set the key of item i to k
//	insert i h	insert item i into heap h
//	deletemin h	remove and print the smallest item in h
//	meld h1 h2	meld sets h1, h2
//	print		print partition
//	tprint h	print single set as tree
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "lheaps.h"

main(int argc, char* argv[]) {
	int h,j,k; string cmd; lheaps L(26);

	while (cin >> cmd) {
		if (misc::prefix(cmd,"key")) {
			if (misc::getAlpha(cin,j)) cout << L.key(j);
		} else if (misc::prefix(cmd,"setkey")) {
			if (misc::getAlpha(cin,j) && misc::getNum(cin,k)) {
				L.setkey(j,k);
			}
		} else if (misc::prefix(cmd,"insert")) {
			if (misc::getAlpha(cin,j) && misc::getAlpha(cin,h)) {
				L.tprint(cout,L.insert(j,h),0);
			}
		} else if (misc::prefix(cmd,"deletemin")) {
			if (misc::getAlpha(cin,h)) {
				cout << misc::nam(L.deletemin(h)) << endl;
			}
		} else if (misc::prefix(cmd,"meld")) {
			if (misc::getAlpha(cin,h) && misc::getAlpha(cin,j)) {
				L.tprint(cout,L.meld(h,j),0);
			}
		} else if (misc::prefix(cmd,"print")) {
			cout << L;
		} else if (misc::prefix(cmd,"tprint")) {
			if (misc::getAlpha(cin,h)) L.tprint(cout,h,0);
		} else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
