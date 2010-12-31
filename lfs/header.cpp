#include "header.h"

void header::unpack(buffer_t& b) {
// Unpack header fields from buffer.
// For our purposes here, the header payload is unimportant,
// so don't bother with port numbers and other such stuff.
	uint32_t x = ntohl(b[0]);

	// unpack selected IP header fields
	hleng() = (x >> 24) & 0xf;
	leng() = x & 0xffff;
	x = ntohl(b[2]);
	proto() = (x >> 15) & 0xff;
	srcAdr() = ntohl(b[3]);
	dstAdr() = ntohl(b[4]);
	if (hleng() != 7) return;

	// unpack LFS option fields
	x = ntohl(b[5]);
	optCode() = (x >> 24) & 0xff;
	if (optCode() != LFS_OPTION) return;
	optLeng() = (x >> 16) & 0xff;
	if (optLeng() != 8) return;
	lfsOp() = (x >> 14) & 0x3;
	lfsFlags() = (x >> 8) & 0x3f;

	// packed rates represent multiples of 1 Kb/s
	// normalized floating point with 4 bit mantissa
	// with implicit fifth bit; 4 bit exponent
	// gives range from 16 Kb/s to just over 1 Gb/s.
	x |= 0x100; // add in implicit fifth bit of mantissa
	lfsRrate() = ((x >> 4) & 0x1f) << (x & 0xf);
	x = ntohl(b[6]);
	lfsTrace() = x & 0xffffff;
	x >>= 24; // get arate field
	x |= 0x100;
	lfsArate() = ((x >> 4) & 0x1f) << (x & 0xf);

	// ignore transport level fields
}

int header::rateCalc(uint32_t x) {
// Compute packed rate for x. Multiples of 1 Kb/s
// using normalized floating point with 4 bit mantissa
// with implicit fifth bit; 4 bit exponent
// gives range from 16 Kb/s to just over 1 Gb/s.
        uint32_t i, mask, mantissa;
        x = max(x,16); x = min(x,0x1f << 0xf);
        // find high order bit
        i = 19, mask = (1 << 19);
        while ((x & mask) == 0) {
                i--; mask >>= 1;
        }
        mantissa = x >> (i-4);
        if (x == (mantissa << (i-4))) {
                x = (mantissa << 4) | (i-4);
        } else if (mantissa < 0x1f) {
                x = ((mantissa + 1) << 4) | (i-4);
        } else {
                x = (1 << 8) | (i-3);
        }
        x &= 0xff; // trim off high bit of mantissa which is always 1

        x |= 0x100; // put back high bit of mantissa
        return ((x >> 4) & 0x1f) << (x & 0xf);
}

void header::pack(buffer_t& b) {
// Pack header fields into buffer.
	b[0] = htonl(	(4 << 28) |
		     	((hleng() & 0xf) << 24) |
		     	(leng() & 0xffff)
		);
	b[1] = 0; // ignore fragmentation fields
	b[2] = htonl((64 << 24) | (proto() & 0xff) << 16); 
	b[3] = htonl(srcAdr());
	b[4] = htonl(dstAdr());
	if (hleng() != 7 || optCode() != LFS_OPTION) return;

	// pack LFS option fields
	uint32_t rrate = rateCalc(lfsRrate());
	uint32_t arate = rateCalc(lfsArate());

	b[5] = htonl( 	(LFS_OPTION << 24) | (8 << 16) |
	    		((lfsOp() & 0x3) << 14) |
	    		((lfsFlags() & 0x3f) << 8) | rrate
		);
	b[6] = htonl((arate << 24) | (lfsTrace() & 0xffffff));
}

bool header::hdrErrCheck(buffer_t& b) const {
        return true;
}

bool header::payErrCheck(buffer_t& b) const {
        return true;
}

void header::hdrErrUpdate(buffer_t& b) {
	uint16_t* bp = reinterpret_cast<uint16_t*>(&b[0]);
	uint32_t sum;
	bp[5] = 0;
	int hl = 2*hleng(); sum = 0;
	for (int i = 0; i < hl; i++) sum += bp[i];
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	bp[5] = static_cast<uint16_t>(~sum);
}

void header::payErrUpdate(buffer_t& b) {
}

bool header::getPacket(istream& is, buffer_t& b) {
// Read an input packet from is and initialize (*this) and buffer *b.
	int hlng, lng, optCod; ipa_t src, dst;
	misc::skipBlank(is);
	if (!misc::getNum(is,hlng) ||
	    !misc::getNum(is,lng) ||
	    !misc::getIpAdr(is,src) ||
	    !misc::getIpAdr(is,dst))
		return false;
	hleng() = hlng; leng() = lng; srcAdr() = src; dstAdr() = dst;
	if (hlng == 7) {
		optCode() = LFS_OPTION;
		optLeng() = 8;
		int op, flags, rrate, arate, trace;
		if (!misc::getNum(is,op)    || !misc::getNum(is,flags) ||
		    !misc::getNum(is,rrate) || !misc::getNum(is,arate) ||
		    !misc::getNum(is,trace) ) 
			return false;
		lfsOp() = op; lfsFlags() = flags; lfsRrate() = rrate;
		lfsArate() = arate; lfsTrace() = trace;
	}

	proto() = 17;
	pack(b); uint32_t x;
	for (int i = 0; i < (lng-(4*hlng))/4; i++) {
		if (misc::getNum(is,x)) b[hlng+i] = htonl(x);
		else b[hlng+i] = 0;
	}
	hdrErrUpdate(b); payErrUpdate(b);
	return true;
}

void header::print(ostream& os, buffer_t& b) {
// Prints header fields and first 8 payload words of buffer.
	os << "hlen=" << setw(2) << hleng();
	os << " len=" << setw(4) << leng();
	os << " src="
	   << ((srcAdr() >> 24) & 0xff) << "."
	   << ((srcAdr() >> 16) & 0xff) << "."
	   << ((srcAdr() >>  8) & 0xff) << "."
	   << ((srcAdr()      ) & 0xff);
	os << " dst="
	   << ((dstAdr() >> 24) & 0xff) << "."
	   << ((dstAdr() >> 16) & 0xff) << "."
	   << ((dstAdr() >>  8) & 0xff) << "."
	   << ((dstAdr()      ) & 0xff);
	if (hleng() == 7 && optCode() == LFS_OPTION) {
		os << " lfsOp=" << lfsOp()
		   << " lfsFlags=" << lfsFlags()
		   << " lfsRrate=" << lfsRrate()
		   << " lfsArate=" << lfsArate()
		   << " lfsTrace=" << lfsTrace();
	}
	for (int i = 0; i < min(8,(leng()-4*hleng())/4); i++)
		os << " " << ntohl(b[hleng()+i]);
	os << endl;
}
