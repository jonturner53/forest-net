// usage: hashTbl_d
//
// HashTbl_d is a test program for the hashTbl data structure.
//
//	insert k1 k2 v	add (key,value) pair (k1:k2,v) to table
//	lookup k1 k2	lookup item with key k1:k2
//	remove k1 k2	remove item with key k1:k2
//	print		print the entire table contents
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "hashTbl.h"

main() {
	int k1, k2, v; uint64_t k; string cmd; hashTbl H(100);

        while (cin >> cmd) {
                if (misc::prefix(cmd,"insert")) {
                        if (misc::getNum(cin,k1) && misc::getNum(cin,k2) &&
			    misc::getNum(cin,v)) {
				k = k1; k <<= 32; k |= k2;
				if (v > 100) 
					cout << "values must be in 1..100\n";
				if (H.insert(k,v)) H.dump();
				else cout << "failed\n";
                        }
                } else if (misc::prefix(cmd,"lookup")) {
                        if (misc::getNum(cin,k1) && misc::getNum(cin,k2)) {
				k = k1; k <<= 32; k |= k2;
				cout << H.lookup(k) << endl;
                        }
                } else if (misc::prefix(cmd,"remove")) {
                        if (misc::getNum(cin,k1) && misc::getNum(cin,k2)) {
				k = k1; k <<= 32; k |= k2;
				H.remove(k); H.dump();
                        }
                } else if (misc::prefix(cmd,"print")) {
			H.dump();
                } else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}

