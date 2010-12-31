// usage: steps_d
//
// Steps_d is a test program for the steps data structure.
// It creates an instance of steps which can be
// modified by the following commands.
// The optional argument is an upper bound on the number
// of steps in the function. It defaults to 26.
//
//	value x		value of the function at x
//	findmin lo hi	return min value in range [lo,hi]
//	change lo hi dy add dy to function value in [lo,hi]
//	print		print the entire function
//	quit		exit the program
//

#include "stdinc.h"
#include "misc.h"
#include "steps.h"

main(int argc, char* argv[]) {
	int lo, hi, x, dy; string cmd;
	steps S(26);

	while (cin >> cmd) {
		if (misc::prefix(cmd,"value")) {
			if (misc::getNum(cin,x)) cout << S.value(x) << endl;
		} else if (misc::prefix(cmd,"findmin")) {
			if (misc::getNum(cin,lo) && misc::getNum(cin,hi)) 
				cout << S.findmin(lo,hi);
		} else if (misc::prefix(cmd,"change")) {
			if (misc::getNum(cin,lo) && misc::getNum(cin,hi) &&
			    misc::getNum(cin,dy)) {
				S.change(lo,hi,dy);
				cout << S;
			}
		} else if (misc::prefix(cmd,"print")) {
			cout << S;
		} else if (misc::prefix(cmd,"quit")) {
			break;
		} else {
			warning("illegal command");
		}
		cin.ignore(1000,'\n');
	}
}
