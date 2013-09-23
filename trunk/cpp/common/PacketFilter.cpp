/** @file PacketFilter.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "PacketFilter.h"

namespace forest {

/** Constructor for PacketFilter, allocates space and initializes private data.
 */
PacketFilter::PacketFilter() { on = false; }

/** Create a string representation of a filter.
 *  @param s is a reference to a string in which result is to be returned
 *  @return a reference to s
 */
string& PacketFilter::toString(string& s) const {
	stringstream ss;
	ss << (on ? '1' : '0' ) << " " << lnk << " "
	   << (in ? '1' : '0') << (out ? '1' : '0') << " ";
	ss << comt << " ";
	ss << Forest::fAdr2string(srcAdr,s) << " ";
	ss << Forest::fAdr2string(dstAdr,s) << " ";
	ss << Packet::pktTyp2string(type,s);
	ss << CtlPkt::cpType2string(cpType,s);
	s = ss.str();
	return s;
}

bool PacketFilter::fromString(string& s) {
	NetBuffer buf(s); string s1, s2;
	return	(buf.readBit(on) && buf.readInt(lnk) &&
		 buf.readBit(in) && buf.readBit(out) &&
		 buf.readInt(comt) &&
		 buf.readForestAddress(s1) &&
		 (srcAdr = Forest::forestAdr(s1.c_str())) != 0 &&
		 buf.readForestAddress(s2) &&
		 (dstAdr = Forest::forestAdr(s2.c_str())) != 0 &&
		 buf.readPktType(type) &&
		 buf.readCpType(cpType));
}
		

} // ends namespace

