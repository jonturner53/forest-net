// usage: dheap_d d
//
// Dheap_d is a test program for the dheap data structure.
// It creates a d-heap which can be modified by the 
// following commands.
//
//	findmin		return the item with smallest key
//	key i		return the key of item
//	member i	return true if item in heap
//	empty		return true if heap is empty
//	insert i k	insert item with specified key
//	remove i	remove item from heap
//	deletemin	delete and return smallest item
//	changekey i k	change the key of an item
//	print		print the heap
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "dheap.h"

main(int argc, char *argv[]) {
	int d,j,k; string cmd;

	if (argc > 1 && sscanf(argv[1],"%d",&d) == 1) {}
	else d = 2;

	cout << "d=" << d << endl;

	dheap H(26,d);

        while (cin >> cmd) {
                if (misc::prefix(cmd,"findmin")) {
			misc::putAlpha(cout,H.findmin()); cout << endl;
                } else if (misc::prefix(cmd,"key")) {
                        if (misc::getAlpha(cin,j)) {
				cout << H.key(j) << endl;
                        }
                } else if (misc::prefix(cmd,"member")) {
                        if (misc::getAlpha(cin,j)) {
				cout << (H.member(j) ? "true":"false") << endl;
			}
                } else if (misc::prefix(cmd,"empty")) {
			cout << (H.empty() ? "true" : "false") << endl;
                } else if (misc::prefix(cmd,"insert")) {
                        if (misc::getAlpha(cin,j) && misc::getNum(cin,k)) {
				H.insert(j,k); cout << H << endl;
			}
                } else if (misc::prefix(cmd,"remove")) {
                        if (misc::getAlpha(cin,j)) {
				H.remove(j); cout << H << endl;
			}
                } else if (misc::prefix(cmd,"deletemin")) {
			H.deletemin(); cout << H << endl;
                } else if (misc::prefix(cmd,"changekey")) {
                        if (misc::getAlpha(cin,j) && misc::getNum(cin,k)) {
				H.changekey(j,k); cout << H << endl;
			}
                } else if (misc::prefix(cmd,"print")) {
			cout << H << endl;
                } else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
