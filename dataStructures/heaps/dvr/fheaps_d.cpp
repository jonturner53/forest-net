// usage: fheaps_d
//
// Fheaps_d is a test program for the fheaps data structure.
// It creates an instance of fheaps which can be 
// modified by the following commands.
//
//	findmin h	return the item with minimum key in h
//	key j		print the key of item j
//	insert j k h	insert item j into heap h
//	deletemin h	remove and print the smallest item in h
//	meld h1 h2	meld heaps h1, h2
//	decKey j k h 	decrease key of j in h by k
//	remove j h	remove j from h
//	print		print all the heaps
//	tprint h	print single heap as tree
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "fheaps.h"

main() {
	int h,j,k; string cmd; fheaps F(26);

        while (cin >> cmd) {
                if (misc::prefix(cmd,"findmin")) {
			if (misc::getAlpha(cin,h)) {
				misc::putAlpha(cout,F.findmin(h)); cout << endl;
			}
                } else if (misc::prefix(cmd,"key")) {
                        if (misc::getAlpha(cin,j)) {
				cout << F.key(j) << endl;
                        }
                } else if (misc::prefix(cmd,"insert")) {
                        if (misc::getAlpha(cin,j) && misc::getNum(cin,k) && 
			    misc::getAlpha(cin,h)) {
				F.tprint(cout,F.insert(j,h,k),0);
			}
                } else if (misc::prefix(cmd,"deletemin")) {
                        if (misc::getAlpha(cin,h)) {
				j = F.deletemin(h);
				misc::putAlpha(cout,j);
				cout << endl << F << endl;
			}
                } else if (misc::prefix(cmd,"meld")) {
                        if (misc::getAlpha(cin,j) && misc::getAlpha(cin,h)) {
				F.tprint(cout,F.meld(j,h),0);
			}
                } else if (misc::prefix(cmd,"decKey")) {
                        if (misc::getAlpha(cin,j) && misc::getNum(cin,k) && 
			    misc::getAlpha(cin,h)) {
				F.tprint(cout,F.decreasekey(j,k,h),0);
			}
                } else if (misc::prefix(cmd,"remove")) {
                        if (misc::getAlpha(cin,j) && misc::getAlpha(cin,h)) {
				F.tprint(cout,F.remove(j,h),0);
			}
                } else if (misc::prefix(cmd,"print")) {
			cout << F << endl;
                } else if (misc::prefix(cmd,"tprint")) {
                        if (misc::getAlpha(cin,j)) F.tprint(cout,j,0);
                } else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
