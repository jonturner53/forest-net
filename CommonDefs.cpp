/** @file CommonDefs.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "CommonDefs.h"

bool Forest::readForestAdr(istream& in, fAdr_t& fa) {
// If next thing on the current line in a forest address,
// return it in fa and return true. Otherwise, return false.
// A negative value on the input stream in interpreted as
// a multicast address. Otherwise, we expect a unicast
// address in dotted decimal format. We require that
// either the zip code part is >0 or both parts are
// equal to zero. We allow 0.0 for null addresses and
// x.0 for unicast routes to foreign zip codes.
// The address is returned in host byte order.
	int b1, b2;
	
	if (!Misc::readNum(in,b1)) return false;
	if (b1 < 0) { fa = b1; return true; }
	if (!Misc::verify(in,'.') || !Misc::readNum(in,b2))
		return false;
	if (b1 == 0 && b2 != 0) return false;
	if (b1 > 0xffff || b2 > 0xffff) return false;
	fa = Forest::forestAdr(b1,b2);
	return true;
}

void Forest::writeForestAdr(ostream& out, fAdr_t fa) {
// Print the given forest address.
	if (fa < 0) out << fa;
	else out << zipCode(fa) << "." << localAdr(fa);
}