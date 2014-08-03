/** @file Forest.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Forest.h"

namespace forest {

/** Read a forest address.
 *  @param in is an open input stream
 *  @param fa is a reference to a forest address in which result is returned;
 *  a negative value on the input stream in interpreted as a multicast address;
 *  otherwise, we expect a unicast address in dotted decimal format;
 *  we require that either the zip code part is >0 or both parts are
 *  equal to zero; we allow 0.0 for null addresses and x.0 for unicast
 *  routes to foreign zip codes. The address is returned in host byte order.
 */
bool Forest::readForestAdr(istream& in, fAdr_t& fa) {
	int b1, b2;
	
	if (!Util::readInt(in,b1)) return false;
	if (b1 < 0) { fa = b1; return true; }
	if (!Util::verify(in,'.') || !Util::readInt(in,b2))
		return false;
	if (b1 == 0 && b2 != 0) return false;
	if (b1 > 0xffff || b2 > 0xffff) return false;
	fa = Forest::forestAdr(b1,b2);
	return true;
}

string Forest::nodeType2string(ntyp_t nt) {
	string s;
	if (nt == CLIENT) s = "client";
	else if (nt == SERVER) s = "server";
	else if (nt == ROUTER) s = "router";
	else if (nt == CONTROLLER) s = "controller";
	else s = "unknown node type";
	return s;
}

Forest::ntyp_t Forest::getNodeType(string& s) {
	if (s.compare("client") == 0) return CLIENT;
	if (s.compare("server") == 0) return SERVER;
	if (s.compare("router") == 0) return ROUTER;
	if (s.compare("controller") == 0) return CONTROLLER;
	return UNDEF_NODE;
}


} // ends namespace

