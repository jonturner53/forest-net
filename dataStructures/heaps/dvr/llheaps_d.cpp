// usage: lheaps_d
//
// Lheaps_d is a test program for the lheaps data structure.
// It creates an instance of lheaps which can be modified by
// the following commands.
//
//	key i		print the key of item i
//	setkey j k	set the key of item j to k
//	insert j h	insert item j into heap h
//	delete j	mark item j as deleted
//	findmin h	print somallest item in h
//	meld h1 h2	meld heaps h1, h2
//	lmeld h1 h2	lazy meld of heaps h1, h2
//	makeheap i1...	combine single item heaps into single heap
//	print		print partition
//	tprint h	print single set as tree
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "llheaps.h"

bool *dvec;
bool delfunc(item i) { return dvec[i]; }

main(int argc, char* argv[]) {
	int h,j,k,n; string cmd;

        n = 13; // leave room for dummy nodes
        llheaps L(n,delfunc); list Q(n); dvec = new bool[n+1];
        for (j = 1; j <= n; j++) {
                dvec[j] = false; L.setkey(j,randint(0,99));
        }
        cout << L;

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
		} else if (misc::prefix(cmd,"delete")) {
			if (misc::getAlpha(cin,j)) {
				dvec[j] = true; cout << L;
			}
		} else if (misc::prefix(cmd,"meld")) {
			if (misc::getAlpha(cin,h) && misc::getAlpha(cin,j)) {
				L.tprint(cout,L.meld(h,j),0);
			}
		} else if (misc::prefix(cmd,"lmeld")) {
			if (misc::getAlpha(cin,h) && misc::getAlpha(cin,j)) {
				L.tprint(cout,L.lmeld(h,j),0);
			}
		} else if (misc::prefix(cmd,"findmin")) {
			if (misc::getAlpha(cin,h)) {
				j = L.findmin(h);
				cout << "(" << misc::nam(j) << "," 
				     << L.key(j) << ")\n";
				L.tprint(cout,j,0);
			}
		} else if (misc::prefix(cmd,"makeheap")) {
			Q.clear();
			while (misc::getAlpha(cin,j)) Q &= j;
			L.tprint(cout,L.makeheap(Q),0);
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
