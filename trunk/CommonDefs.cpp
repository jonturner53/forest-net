/** @file CommonDefs.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "CommonDefs.h"

bool Forest::readForestAdr(istream& is, fAdr_t& fa) {
// If next thing on the current line is a forest address,
// return it in fa and return true. Otherwise, return false.
// A negative value on the input stream is interpreted as
// a multicast address. Otherwise, we expect a unicast
// address in dotted decimal format, with both parts >0
// or with both parts =0 (this is to allow null addresses).
// The address is returned in host byte order.
	int b1, b2;
	
	if (!Misc::readNum(is,b1)) return false;
	if (b1 < 0) { fa = b1; return true; }
	if (!Misc::verify(is,'.') || !Misc::readNum(is,b2))
		return false;
	if ((b1 == 0 && b2 != 0) || (b1 != 0 && b2 == 0)) return false;
	if (b1 > 0xffff || b2 > 0xffff) return false;
	fa = (b1 << 16) | b2;
	return true;
}

void Forest::writeForestAdr(ostream& os, fAdr_t fa) {
// Print the given forest address.
	if (fa < 0) os << fa;
	else os << zipCode(fa) << "." << localAdr(fa);
}
