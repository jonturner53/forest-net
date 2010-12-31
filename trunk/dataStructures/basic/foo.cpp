#include "stdinc.h"
#include "list.h"
main() {
	int i;

	for (i = 1; i <= 5; i++) {
		list L(30);
		L &= i;
		cout << L << endl;
	}
}
