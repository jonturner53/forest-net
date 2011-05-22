/** \file Utest.h 
 *  Class to facilitate unit testing of other classes.
 */

#ifndef UTEST_H
#define UTEST_H

#include "stdinc.h"

class Utest {
public:	static bool assertTrue(bool, const char*);
	static bool assertEqual(int, int, const char*);
	static bool assertEqual(const string&, const string&, const char*);
};

inline bool Utest::assertTrue(bool condition, const char* s) {
	if (!condition) {
		cout << s << endl; exit(1);
	}
}

inline bool Utest::assertEqual(int x, int y, const char* s) {
	if (x != y) {
		cout << s << endl; exit(1);
	}
}

inline bool Utest::assertEqual(const string& p, const string& q, const char* s) {
	if (p.compare(q) != 0) {
		cout << s << endl; exit(1);
	}
}

#endif
