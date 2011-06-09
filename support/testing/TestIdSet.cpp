/** \file TestUiList.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "IdSet.h"
#include "Utest.h"

void basicTests() {
	int n1 = 10; IdSet set1(n1);

	Utest::assertEqual(set1.firstId(), 0, "initial set not empty");
	cout << "writing empty idset: "; set1.write(cout); cout << endl;

	Utest::assertEqual(set1.getId(1234),1,"wrong id for first item");
	cout << "writing one id set: "; set1.write(cout); cout << endl;
	string s1; set1.add2string(s1);
	Utest::assertEqual(s1, "{ (1234,1) }",
		"mismatch on adding first item");

	Utest::assertEqual(set1.getId(2345),2,"wrong id for second item");
	Utest::assertEqual(set1.getId(3456),3,"wrong id for third item");
	s1.clear(); set1.add2string(s1);
	Utest::assertEqual(s1, "{ (1234,1) (2345,2) (3456,3) }",
		"mismatch after adding third item");
	cout << "writing 3 id set"; set1.write(cout); cout << endl;

	set1.releaseId(2345);
	s1.clear(); set1.add2string(s1);
	Utest::assertEqual(s1, "{ (1234,1) (3456,3) }",
		"mismatch after releasing second id");

	set1.getId(4567);
	s1.clear(); set1.add2string(s1);
	Utest::assertEqual(s1, "{ (1234,1) (3456,3) (4567,2) }",
		"mismatch after reusing released id");

	set1.clear();
	s1.clear(); set1.add2string(s1);
	Utest::assertEqual(s1, "{ }", "mismatch after clearing set");
}

/**
 *  Unit test for UiList data structure.
 */
main() {
	cout << "running basic tests\n";
	basicTests();
	cout << "basic tests passed\n";

	// add more systematic tests for each individual method
}
