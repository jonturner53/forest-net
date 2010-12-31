// usage: graph_d
//
// Graph_d is a test program for the wgraph data structure.
//
//	first v		print first edge incident to v
//	next v e	print next edge of v, following e
//	left e		print left endpoint of e
//	right e		print right endpoint of e
//	mate v e 	print other endpoint of e
//	join u v	join two vertices with an edge of given weight
//	print		print the graph
//	quit		exit the program
//
// Note, this version is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "misc.h"
#include "graph.h"

main() {
	vertex u,v; edge e; string cmd; graph G;

	while (cin >> cmd) {
		if (misc::prefix(cmd,"first")) {
			if (misc::getAlpha(cin,u)) {
				e = G.first(u);
				cout << "e" << e << "="; G.putEdge(cout,e,u);
				cout << endl;
			}
		} else if (misc::prefix(cmd,"next")) {
			if (misc::getAlpha(cin,u) && misc::getNum(cin,e)) {
				e = G.next(u,e);
				cout << "e" << e << "="; G.putEdge(cout,e,u);
				cout << endl;
			}
		} else if (misc::prefix(cmd,"left")) {
			if (misc::getNum(cin,e)) {
				misc::putAlpha(cout,G.left(e)); cout << endl;
			}
		} else if (misc::prefix(cmd,"right")) {
			if (misc::getNum(cin,e)) {
				misc::putAlpha(cout,G.right(e)); cout << endl;
			}
		} else if (misc::prefix(cmd,"mate")) {
			if (misc::getAlpha(cin,u) && misc::getNum(cin,e)) {
				misc::putAlpha(cout,G.mate(u,e)); cout << endl;
			}
		} else if (misc::prefix(cmd,"join")) {
			if (misc::getAlpha(cin,u) && misc::getAlpha(cin,v)) {
				G.join(u,v); cout << G;
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
