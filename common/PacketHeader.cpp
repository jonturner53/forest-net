/** @file PacketHeader.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "PacketHeader.h"

PacketHeader::PacketHeader() { setVersion(1); }

PacketHeader::~PacketHeader() {}

void PacketHeader::unpack(buffer_t& b) {
// Unpack PacketHeader fields from buffer.
	uint32_t x = ntohl(b[0]);
	setVersion((x >> 28) & 0xf);
	setLength((x >> 16) & 0xfff);
	int y = (x >> 8) & 0xff;
	setPtype((ptyp_t) y);
	setFlags(x & 0xff);
	setComtree(ntohl(b[1]));
	setSrcAdr(ntohl(b[2]));
	setDstAdr(ntohl(b[3]));
}

void PacketHeader::pack(buffer_t& b) {
// Pack PacketHeader fields into buffer.
        uint32_t x = (getVersion() << 28)
                     | ((getLength() & 0xfff) << 16) 
                     | ((getPtype() & 0xff) << 8)
                     | (getFlags() & 0xff);
        b[0] = htonl(x);
        b[1] = htonl(getComtree());
        b[2] = htonl(getSrcAdr());
        b[3] = htonl(getDstAdr());
}

bool PacketHeader::hdrErrCheck(buffer_t& b) const {
        return true;
}

bool PacketHeader::payErrCheck(buffer_t& b) const {
        return true;
}

void PacketHeader::hdrErrUpdate(buffer_t& b) {
}

void PacketHeader::payErrUpdate(buffer_t& b) {
}

bool PacketHeader::read(istream& in, buffer_t& b) {
// Read an input packet from in and initialize (*this) and buffer *b.
	int lng, ptyp, flgs, comt; fAdr_t src, dst; string ptypString;
	Misc::skipBlank(in);
	if (!Misc::readNum(in,lng) ||
	    !Misc::readWord(in,ptypString) ||
	    !Misc::readNum(in,flgs) ||
	    !Misc::readNum(in,comt) ||
	    !Forest::readForestAdr(in,src) ||
	    !Forest::readForestAdr(in,dst))
		return false;
	setLength(lng); setFlags(flgs); setComtree(comt);
	setSrcAdr(src); setDstAdr(dst);

             if (ptypString == "data")       setPtype(CLIENT_DATA);
        else if (ptypString == "sub_unsub")  setPtype(SUB_UNSUB);
        else if (ptypString == "connect")    setPtype(CONNECT);
        else if (ptypString == "disconnect") setPtype(DISCONNECT);
        else if (ptypString == "rteRep")     setPtype(RTE_REPLY);
        else if (ptypString == "client_sig") setPtype(CLIENT_SIG);
        else if (ptypString == "net_sig")    setPtype(NET_SIG);
        else fatal("PacketHeader::getPacket: invalid packet type");

	pack(b); int32_t x;
	for (int i = 0; i < min(8,(getLength()-Forest::HDR_LENG)/4); i++) {
		if (Misc::readNum(in,x)) b[(Forest::HDR_LENG/4)+i] = htonl(x);
		else b[(Forest::HDR_LENG/4)+i] = 0;
	}
	hdrErrUpdate(b); payErrUpdate(b);
	return true;
}

void PacketHeader::write(ostream& out, buffer_t& b) const {
// Prints PacketHeader fields and first 8 payload words of buffer.
        out << "len=" << setw(3) << getLength();
        out << " typ=";

        if (getPtype() == CLIENT_DATA)     out << "data      ";
        else if (getPtype() == SUB_UNSUB)  out << "sub_unsub ";
        else if (getPtype() == CLIENT_SIG) out << "client_sig";
        else if (getPtype() == CONNECT)    out << "connect   ";
        else if (getPtype() == DISCONNECT) out << "disconnect";
        else if (getPtype() == NET_SIG)    out << "net_sig   ";
        else if (getPtype() == RTE_REPLY)  out << "rteRep    ";
        else if (getPtype() == RTR_CTL)    out << "rtr_ctl   ";
        else if (getPtype() == VOQSTATUS)  out << "voq_status";
        else                            out << "--------- ";
        out << " flags=" << int(getFlags());
        out << " comt=" << setw(3) << getComtree();
        out << " sadr="; Forest::writeForestAdr(out, getSrcAdr());
        out << " dadr="; Forest::writeForestAdr(out, getDstAdr());

	int32_t x;
        for (int i = 0; i < min(8,(getLength()-Forest::HDR_LENG)/4); i++) {
		x = ntohl(b[(Forest::HDR_LENG/4)+i]);
                out << " " << x;
	}
        out << endl;
	if (getPtype() == CLIENT_SIG || getPtype() == NET_SIG) {
		CtlPkt cp;
		cp.unpack(&b[Forest::HDR_LENG/4],
			  getLength()-(Forest::HDR_LENG+4));
		cp.write(out);
	}
}
