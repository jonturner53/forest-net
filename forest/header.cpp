#include "header.h"

void header::unpack(buffer_t& b) {
// Unpack header fields from buffer.
	uint32_t x = ntohl(b[0]);
	version() = (x >> 28) & 0xf;
	leng() = (x >> 16) & 0xfff;
	int y = (x >> 8) & 0xff;
	ptype() = (y == 1 ? USERDATA :
		  (y == 11 ? CONNECT :
		  (y == 12 ? DISCONNECT :
		  (y == 13 ? SUB_UNSUB :
		  (y == 101 ? RTE_REPLY : UNDEF_PKT)))));
	flags() = x & 0xff;
	comtree() = ntohl(b[1]);
	srcAdr() = ntohl(b[2]);
	dstAdr() = ntohl(b[3]);
}

void header::pack(buffer_t& b) {
// Pack header fields into buffer.
        uint32_t x = (FOREST_VERSION << 28)
                     | ((leng() & 0xfff) << 16) 
                     | ((ptype() & 0xff) << 8)
                     | (flags() & 0xff);
        b[0] = htonl(x);
        b[1] = htonl(comtree());
        b[2] = htonl(srcAdr());
        b[3] = htonl(dstAdr());
}

bool header::hdrErrCheck(buffer_t& b) const {
        return true;
}

bool header::payErrCheck(buffer_t& b) const {
        return true;
}

void header::hdrErrUpdate(buffer_t& b) {
}

void header::payErrUpdate(buffer_t& b) {
}

bool header::getPacket(istream& is, buffer_t& b) {
// Read an input packet from is and initialize (*this) and buffer *b.
	int lng, ptyp, flgs, comt; fAdr_t src, dst; string ptypString;
	misc::skipBlank(is);
	if (!misc::getNum(is,lng) ||
	    !misc::getWord(is,ptypString) ||
	    !misc::getNum(is,flgs) ||
	    !misc::getNum(is,comt) ||
	    !forest::getForestAdr(is,src) ||
	    !forest::getForestAdr(is,dst))
		return false;
	leng() = lng; flags() = flgs; comtree() = comt;
	srcAdr() = src; dstAdr() = dst;

             if (ptypString == "data")       ptype() = USERDATA;
        else if (ptypString == "sub_unsub")  ptype() = SUB_UNSUB;
        else if (ptypString == "connect")    ptype() = CONNECT;
        else if (ptypString == "disconnect") ptype() = DISCONNECT;
        else if (ptypString == "rteRep")     ptype() = RTE_REPLY;
        else fatal("header::getPacket: invalid packet type");

	pack(b); int32_t x;
	for (int i = 0; i < min(8,(leng()-HDR_LENG)/4); i++) {
		if (misc::getNum(is,x)) b[(HDR_LENG/4)+i] = htonl(x);
		else b[(HDR_LENG/4)+i] = 0;
	}
	hdrErrUpdate(b); payErrUpdate(b);
	return true;
}

void header::print(ostream& os, buffer_t& b) {
// Prints header fields and first 8 payload words of buffer.
        os << "len=" << setw(3) << leng();
        os << " typ=";
        if (ptype() == USERDATA)        os << "data      ";
        else if (ptype() == SUB_UNSUB)  os << "sub_unsub ";
        else if (ptype() == CONNECT)    os << "connect   ";
        else if (ptype() == DISCONNECT) os << "disconnect";
        else if (ptype() == RTE_REPLY)  os << "rteRep    ";
        else                            os << "--------- ";
        os << "flags=" << int(flags());
        os << " comt=" << setw(3) << comtree();
        os << " sadr="; forest::putForestAdr(os, srcAdr());
        os << " dadr="; forest::putForestAdr(os, dstAdr());

	int32_t x;
        for (int i = 0; i < min(8,(leng()-HDR_LENG)/4); i++) {
		x = ntohl(b[(HDR_LENG/4)+i]);
                os << " " << x;
	}
        os << endl;
}
