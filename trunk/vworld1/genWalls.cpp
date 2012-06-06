#include "stdinc.h"

/** usage:
 * 	genWalls n pLeft pTop pBoth seed
 *
 * GenWalls produces a random "walls file" for use by Avatar.
 * The virtual world has dimensions nxn. PLeft is the probabilty
 * that a given square has a left wall only, pTop is the probability
 * that it has a top wall only, and pBoth is the probability that
 * it has both. Seed is used to intialize the random number genrator.
 */
int main(int argc, char* argv[]) {
	int n, seed; double pLeft, pTop, pBoth;
	if (argc != 6 ||
	    sscanf(argv[1],"%d",&n) != 1 ||
	    sscanf(argv[2],"%lf",&pLeft) != 1 ||
	    sscanf(argv[3],"%lf",&pTop) != 1 ||
	    sscanf(argv[4],"%lf",&pBoth) != 1 ||
	    sscanf(argv[5],"%d",&seed) != 1)
		fatal("usage: genWalls n pLeft pTop pBoth");

	srand(seed);
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			double p = randfrac();
			char c;
			if (p < pLeft) 			c = '|';
			else if (p < pLeft+pTop) 	c = '-';
			else if (p < pLeft+pTop+pBoth) 	c = '+';
			else				c = ' ';
			cout << c;
		}
		cout << endl;
	}
}
