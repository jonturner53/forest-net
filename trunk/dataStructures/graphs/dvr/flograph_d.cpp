// usage: flograph_d
//
// Flograph_d is a test program for the flograph data structure.
//
//	first v		print first edge incident to v
//	next v e	print next edge incident to v after e
//	tail e		print tail of e
//	head e		print head of e
//	cap v e		print capacity from v on e
//	flow v e	print flow from v on e
//	res v e		print residual capacity from v on e
//	addflow v e f	add f units of flow from v on e
//	join u v w	join two vertices with an edge of given capacity
//	clear		remove all edges
//	print		print the list
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "flograph.h"

main() {
	vertex u,v; edge e; flow c1,f1; string cmd; flograph G;

	while (cin >> cmd) {
		if (misc::prefix(cmd,"first")) {
			if (misc::getAlpha(cin,u)) {
                                e = G.first(u);
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
                } else if (misc::prefix(cmd,"capacity")) {
                        if (misc::getAlpha(cin,u) && misc::getNum(cin,e)) {
                                cout << G.cap(u,e) << endl;
                        }
                } else if (misc::prefix(cmd,"flow")) {
                        if (misc::getAlpha(cin,u) && misc::getNum(cin,e)) {
                                cout << G.f(u,e) << endl;
                        }
                } else if (misc::prefix(cmd,"residual")) {
                        if (misc::getAlpha(cin,u) && misc::getNum(cin,e)) {
                                cout << G.res(u,e) << endl;
                        }
                } else if (misc::prefix(cmd,"addflow")) {
                        if (misc::getAlpha(cin,u) && misc::getNum(cin,e) && 
			    misc::getNum(cin,f1)) {
                                cout << G.addFlow(u,e,f1) << endl;
                        }
                } else if (misc::prefix(cmd,"join")) {
                        if (misc::getAlpha(cin,u) && misc::getAlpha(cin,v) && 
			    misc::getNum(cin,c1)) {
                                e = G.join(u,v); G.changeCap(e,c1); cout << G;
                        }
                } else if (misc::prefix(cmd,"clear")) {
                        G.clear(); cout << G;
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
