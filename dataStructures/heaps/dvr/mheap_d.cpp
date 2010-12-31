// usage: mheap_d minFlag
//
// Mheap_d is a test program for the mheap data structure.
// If the argument minFlag="true" then the mheap is configured
// as a min-heap, otherwise it is configured as a max-heap.
// It creates an mheap which can be modified by the 
// following commands.
//
//	findmin (max)	return the item with the top key
//	key i		return the key of item
//	member i	return true if item in heap
//	empty		return true if heap is empty
//	insert i k	insert item with specified key
//	remove i	remove item from heap
//	deletemin (max)	delete and return topmost item
//	changekey i k	change the key of an item
//	print		print the heap
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "mheap.h"

main(int argc, char* argv[]) {
	int j,k; string s, cmd; 
	
	if (argc != 2) fatal("usage: mheap_d minFlag");
	s = argv[1];
	mheap H(26,2,(s == "true"));

        while (cin >> cmd) {
                if (misc::prefix(cmd,"findmin")) {
			misc::putAlpha(cout,H.findmin()); cout << endl;
                } else if (misc::prefix(cmd,"findmax")) {
			misc::putAlpha(cout,H.findmax()); cout << endl;
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
                } else if (misc::prefix(cmd,"deletemax")) {
			H.deletemax(); cout << H << endl;
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
