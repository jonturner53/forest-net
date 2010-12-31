// usage: wdigraph_d
//
// Wdigraph_d is a test program for the wdigraph data structure.
//
//	firstIn v	print first edge into v
//	firstOut v	print first edge into v
//	next v e	print next edge of v, following e
//	tail e		print tail of e
//	head e		print head of e
//	mate v e 	print other endpoint of e
//	length e	print the length of e
//	join u v ll	join two vertices with an edge of given length
//	print		print the graph
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "wdigraph.h"

main() {
	vertex u,v; edge e; length leng; string cmd; wdigraph G;

	while (cin >> cmd) {
		if (misc::prefix(cmd,"firstIn")) {
			if (misc::getAlpha(cin,u)) {
				e = G.firstIn(u);
				cout << "e" << e << "=";
				G.putEdge(cout,e,G.tail(e));
				cout << endl;
			}
		} else if (misc::prefix(cmd,"firstOut")) {
			if (misc::getAlpha(cin,u)) {
				e = G.firstOut(u);
				cout << "e" << e << "=";
				G.putEdge(cout,e,G.tail(e));
				cout << endl;
			}
		} else if (misc::prefix(cmd,"next")) {
			if (misc::getAlpha(cin,u) && misc::getNum(cin,e)) {
				e = G.next(u,e);
				cout << "e" << e << "=";
				G.putEdge(cout,e,G.tail(e));
				cout << endl;
			}
		} else if (misc::prefix(cmd,"tail")) {
			if (misc::getNum(cin,e)) {
				misc::putAlpha(cout,G.tail(e)); cout << endl;
			}
		} else if (misc::prefix(cmd,"head")) {
			if (misc::getNum(cin,e)) {
				misc::putAlpha(cout,G.head(e)); cout << endl;
			}
		} else if (misc::prefix(cmd,"mate")) {
			if (misc::getAlpha(cin,u) && misc::getNum(cin,e)) {
				misc::putAlpha(cout,G.mate(u,e)); cout << endl;
			}
                } else if (misc::prefix(cmd,"length")) {
                        if (misc::getNum(cin,e)) {
                                cout << G.len(e) << endl;
                        }
		} else if (misc::prefix(cmd,"join")) {
			if (misc::getAlpha(cin,u) && misc::getAlpha(cin,v) && 
			    misc::getNum(cin,leng)) {
				e = G.join(u,v); G.changeLen(e,leng); cout << G;
			}
		} else if (misc::prefix(cmd,"print")) {
			cout << G;
		} else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
