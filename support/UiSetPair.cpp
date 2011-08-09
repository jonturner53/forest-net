/** @file UiSetPair.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "UiSetPair.h"

/** Create a list pair that can hold items in 1..n1 */
UiSetPair::UiSetPair(int n1) : n(n1) {
	assert(n >= 0); 
	nxt = new int[n+1]; prv = new int[n+1];
	inHead = inTail = 0;
	// put everything in the outlist at the start
	for (int i = 1; i < n; i++) {
		nxt[i] = -(i+1); prv[i+1] = -i;
	}
	nxt[n] = prv[1] = 0;
	outHead = 1; outTail = n;

	nxt[0] = prv[0] = 0;
}

UiSetPair::~UiSetPair() { delete [] nxt; delete [] prv; }

/** Move an item from one list to the other.
 *  Inserts item at end of the other list
 *  @param i is the value to be swapped
 */
void UiSetPair::swap(int i) {
	if (i < 1 || i > n) return;
	if (isIn(i)) {
		if (nxt[i] == 0) inTail = prv[i];
		else prv[nxt[i]] = prv[i];

		if (prv[i] == 0) inHead = nxt[i];
		else nxt[prv[i]] = nxt[i];

		nxt[i] = 0;
		if (outTail == 0) {
			outHead = i; prv[i] = 0;
		} else {
			nxt[outTail] = -i; prv[i] = -outTail;
		}
		outTail = i;
	} else {
		if (nxt[i] == 0) outTail = -prv[i];
		else prv[-nxt[i]] = prv[i];

		if (prv[i] == 0) outHead = -nxt[i];
		else nxt[-prv[i]] = nxt[i];

		nxt[i] = 0;
		if (inTail == 0) {
			inHead = i; prv[i] = 0;
		} else {
			nxt[inTail] = i; prv[i] = inTail;
		}
		inTail = i;
	}
}

/** Create a string representation of a given string.
 *
 *  @param s is string used to return value
 */
string& UiSetPair::toString(string& s) const {
	s = "[ ";
	for (int i = firstIn(); i != 0; i = nextIn(i)) {
		string s1;
		s = s + Misc::num2string(i,s1) + " ";
	}
	s += "] [ ";
	for (int i = firstOut(); i != 0; i = nextOut(i)) {
		string s1;
		s = s + Misc::num2string(i,s1) + " ";
	}
	s += "]";
	return s;
}
