// usage: rlist_d
//
// rlist_d is a test program for the rlist data structure.
//
//	pop t		pop first item from list with last item t
//	join t1 t2	join the lists with last items t1, t2
//	reverse t	return the list whose last item is t
//	print t		print the list with last item t
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "rlist.h"

main() {
	int t, t1, t2; string cmd; rlist L;

        while (cin >> cmd) {
                if (misc::prefix(cmd,"pop")) {
                        if (misc::getAlpha(cin,t)) {
				 L.print(cout,L.pop(t)); cout << endl;
                        }
                } else if (misc::prefix(cmd,"join")) {
                        if (misc::getAlpha(cin,t1) && misc::getAlpha(cin,t2)) {
				L.print(cout,L.join(t1,t2)); cout << endl;
                        }
                } else if (misc::prefix(cmd,"reverse")) {
                        if (misc::getAlpha(cin,t)) {
				L.print(cout,L.reverse(t)); cout << endl;
                        }
                } else if (misc::prefix(cmd,"print")) {
                        if (misc::getAlpha(cin,t)) {
				L.print(cout,t); cout << endl;
                        }
                } else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
