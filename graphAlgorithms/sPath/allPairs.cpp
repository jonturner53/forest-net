// usage: allPairs method
//
// Allpairs reads a graph from stdin, computes a solution to the all pairs
// shortest path problem using the specified method and prints the result.
//

#include "stdinc.h"
#include "wdigraph.h"

extern void dijkstraAll(wdigraph&, int**, vertex**);
extern void floyd(wdigraph&, int**, vertex**); 

main(int argc, char *argv[]) {
	vertex u, v;
	wdigraph D; cin >> D;
	
	if (argc != 2) fatal("usage: allPairs method");

	if (strcmp(argv[1],"floyd") == 0) {
		int** dist = new int*[D.n()+1];
		vertex** mid = new vertex*[D.n()+1];
		for (u = 1; u <= D.n(); u++) {
			dist[u] = new int[D.n()+1]; 
			mid[u] = new vertex[D.n()+1];
		}
	
		floyd(D,dist,mid);
	
		cout << "distances\n\n    ";
		for (v = 1; v <= D.n(); v++) {
			if (D.n() <= 26)
				cout << "  " << char(v+('a'-1)) << " ";
			else
				cout << "  " << setw(3) << v << " ";
		}
		printf("\n");
		for (u = 1; u <= D.n(); u++) {
			if (D.n() <= 26)
				cout << "  " << char(u+('a'-1)) << ": ";
			else
				cout << "  " << setw(2) << u << ": ";
			for (v = 1; v <= D.n(); v++) {
				cout << setw(3) << dist[u][v] << " ";
			}
			cout << endl;
		}
		cout << "\n\nmidpoint array\n\n    ";
		for (v = 1; v <= D.n(); v++)  {
			if (D.n() <= 26)
				cout << "  " << char(v+('a'-1)) << " ";
			else
				cout << " " << setw(3) << v;
		}
		cout << endl;
		for (u = 1; u <= D.n(); u++) {
			if (D.n() <= 26)
				cout << " " << char(u+('a'-1)) << ": ";
			else
				cout << setw(2) << u << ": ";
			for (v = 1; v <= D.n(); v++) {
				cout << setw(3) << mid[u][v] << " ";
			}
			cout << endl;
		}
	} else if (strcmp(argv[1],"dijkstra") == 0) {
		int** dist = new int*[D.n()+1];
		vertex** parent = new vertex*[D.n()+1];
		for (u = 1; u <= D.n(); u++) {
			dist[u] = new int[D.n()+1];
			parent[u] = new vertex[D.n()+1];
		}

		dijkstraAll(D,dist,parent);
	
		printf("distances\n\n"); printf("    ");
		for (v = 1; v <= D.n(); v++) {
			if (D.n() <= 26)
				cout << "  " << char(v+('a'-1)) << " ";
			else
				cout << " " << setw(3) << v;
		}
		printf("\n");
	        for (u = 1; u <= D.n(); u++) {
	                if (D.n() <= 26)
				cout << " " << char(u+('a'-1)) << ": ";
	                else
				cout << setw(2) << u << ": ";
	                for (v = 1; v <= D.n(); v++) {
				cout << setw(3) << dist[u][v] << " ";
	                }
			cout << endl;
	        }
	
		cout << "\n\nshortest path trees\n\n    ";
	        for (v = 1; v <= D.n(); v++)  {
	                if (D.n() <= 26)
				cout << "  " << char(v+('a'-1));
	                else
				cout << " " << setw(3) << v;
	        }
		cout << endl;
		for (u = 1; u <= D.n(); u++) {
	                if (D.n() <= 26)
				cout << " " << char(u+('a'-1)) << ": ";
	                else
				cout << setw(2) << u << ": ";
			for (v = 1; v <= D.n(); v++) {
				cout << setw(3) << parent[u][v] << " ";
			}
			cout << endl;
		}
	} else {
		fatal("allPairs: undefined method");
	}
}
