/** @file Utest.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef UTEST_H
#define UTEST_H

#include "stdinc.h"

/**
 *  Class to facilitate unit testing of other classes.
 */
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
