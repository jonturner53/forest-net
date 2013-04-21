/** @file Packet.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Packet.h"

namespace forest {


Packet::Packet() {
	version = 1; buffer = 0;
}

Packet::~Packet() {}

/** Unpack the packet from the buffer.
 *  @return true on success
 */
bool Packet::unpack() {
	if (buffer == 0) return false;
	buffer_t& b = *buffer;
	uint32_t x = ntohl(b[0]);
	version = (x >> 28) & 0xf;
	length = (x >> 16) & 0xfff;
	type = (ptyp_t) ((x >> 8) & 0xff);
	flags = (x & 0xff);
	comtree = ntohl(b[1]);
	srcAdr = ntohl(b[2]);
	dstAdr = ntohl(b[3]);
	return true;
}

/** Pack the packet into the buffer.
 *  @return true on success
 */
bool Packet::pack() {
	if (buffer == 0) return false;
	buffer_t& b = *buffer;
        uint32_t x = (version << 28)
                     | ((length & 0xfff) << 16) 
                     | ((type & 0xff) << 8)
                     | (flags & 0xff);
        b[0] = htonl(x);
        b[1] = htonl(comtree);
        b[2] = htonl(srcAdr);
        b[3] = htonl(dstAdr);
	return true;
}

/** Verify the header error check.
 *  @return true on success
 */
bool Packet::hdrErrCheck() const {
        return true;
}

/** Verify the payload error check.
 *  @return true on success
 */
bool Packet::payErrCheck() const {
        return true;
}

/** Update the header error check based on buffer contents.
 *  @return true on success
 */
void Packet::hdrErrUpdate() {
}

/** Update the payload error check based on buffer contents.
 *  @return true on success
 */
void Packet::payErrUpdate() {
}

/** Read an input packet and pack fields into buffer.
 *  @return true on success
 */
bool Packet::read(istream& in) {
	int flgs, comt; string ptypString;

	Util::skipBlank(in);
	if (!Util::readNum(in,length) ||
	    !Util::readWord(in,ptypString) ||
	    !Util::readNum(in,flgs) ||
	    !Util::readNum(in,comt) ||
	    !Forest::readForestAdr(in,srcAdr) ||
	    !Forest::readForestAdr(in,dstAdr))
		return false;
	flags = (flgs_t) flgs; comtree = (comt_t) comt;

             if (ptypString == "data")       type = CLIENT_DATA;
        else if (ptypString == "sub_unsub")  type = SUB_UNSUB;
        else if (ptypString == "connect")    type = CONNECT;
        else if (ptypString == "disconnect") type = DISCONNECT;
        else if (ptypString == "rteRep")     type = RTE_REPLY;
        else if (ptypString == "client_sig") type = CLIENT_SIG;
        else if (ptypString == "net_sig")    type = NET_SIG;
        else fatal("Packet::getPacket: invalid packet type");

	if (buffer == 0) return true;
	buffer_t& b = *buffer;
	pack(); int32_t x;
	for (int i = 0; i < min(8,(length-HDRLEN)/4); i++) {
		if (Util::readNum(in,x)) b[(HDRLEN/4)+i] = htonl(x);
		else b[(HDRLEN/4)+i] = 0;
	}
	hdrErrUpdate(); payErrUpdate();
	return true;
}

/** Create a string representing packet contents.
 *  @param b is a reference to a buffer containing the packet
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& Packet::toString(string& s) const {
	stringstream ss;
        ss << "len=" << setw(3) << length;
        ss << " typ=";

        if (type == CLIENT_DATA)     ss << "data      ";
        else if (type == SUB_UNSUB)  ss << "sub_unsub ";
        else if (type == CLIENT_SIG) ss << "client_sig";
        else if (type == CONNECT)    ss << "connect   ";
        else if (type == DISCONNECT) ss << "disconnect";
        else if (type == NET_SIG)    ss << "net_sig   ";
        else if (type == RTE_REPLY)  ss << "rteRep    ";
        else if (type == RTR_CTL)    ss << "rtr_ctl   ";
        else if (type == VOQSTATUS)  ss << "voq_status";
        else                         ss << "--------- ";
        ss << " flags=" << int(flags);
        ss << " comt=" << setw(3) << comtree;
        ss << " sadr=" << Forest::fAdr2string(srcAdr,s);
        ss << " dadr=" << Forest::fAdr2string(dstAdr,s);

	if (buffer == 0) {
		ss << endl; s = ss.str(); return s;
	}
	buffer_t& b = *buffer;
	int32_t x;
        for (int i = 0; i < min(8,(length-HDRLEN)/4); i++) {
		x = ntohl(b[(HDRLEN/4)+i]);
                ss << " " << x;
	}
        ss << endl;
	if (type == CLIENT_SIG || type == NET_SIG) {
		CtlPkt cp(payload(), length - Forest::OVERHEAD);
		cp.unpack();
		ss << cp.toString(s);
	}
	s = ss.str();
	return s;
}

} // ends namespace

