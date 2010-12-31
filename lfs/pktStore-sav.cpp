#include "pktStore.h"

pktStore::pktStore(int N1, int M1) : N(N1), M(M1) {
// Constructor for pktStore, allocates space and initializes table.
	int b, p;

	n = m = 0;
	freePkts = new list(N); freeBufs = new list(M);
	buff = new buffer_t[M+1]; ref = new uint16_t[M+1];
	pd = new pktData[N+1];

	for (p = 1; p <= N; p++) { *freePkts &= p; pd[p].buf = 0; }
	for (b = 1; b <= M; b++) { *freeBufs &= b; ref[b] = 0; }
	pd[Null].buf = 0; ref[Null] = 0;
};
	
pktStore::~pktStore() {
	delete freePkts; delete freeBufs;
	delete [] buff; delete [] ref; delete [] pd;
}

int pktStore::alloc() {
// Allocate new packet and buffer and return index to packet.
// If no space for packet, return 0.
	int b, p;
	if (freePkts->empty() || freeBufs->empty()) return 0;
	n++; m++;
	p = (*freePkts)[1]; (*freePkts) <<= 1;
	b = (*freeBufs)[1]; (*freeBufs) <<= 1;
	pd[p].buf = b; ref[b] = 1;
	return p;
}

void pktStore::free(int p) {
// Free packet p and release buffer, if no other packets using it.
	int b = pd[p].buf; pd[p].buf = 0;
	(*freePkts).push(p); n--;
	if ((--ref[b]) == 0) { (*freeBufs).push(b); m--; }
}

int pktStore::clone(int p) {
// Allocate a new packet that references same buffer as p,
// and initialize its header fields to match p.
// Return the number of the new packet.
	int b, p1;
	if (freePkts->empty() || ref[pd[p].buf] >= MAXREFCNT) return 0;
	n++;
	p1 = (*freePkts)[1]; (*freePkts) <<= 1;
	ref[pd[p].buf]++; pd[p1] = pd[p];
	return p1;
}

void pktStore::unpack(int p) {
// Unpack header fields from buffer.
// For our purposes here, the packet payload is unimportant,
// so don't bother with port numbers and other such stuff.
	uint32_t *bp = header(p);
	uint32_t x = ntohl(bp[0]);
	setHleng(p,(x >> 24) & 0xf);
	setLeng(p,(x & 0xffff);
	x = ntohl(bp[2]);
	setProto(p,(x >> 15) & 0xff;
	setSrcAdr(p,ntohl(bp[3]));
	setDstAdr(p,ntohl(bp[4]));
	if (hleng(p) != 7 || optCode(p) != LFS_OPTION) return;
	// unpack LFS option fields
	x = ntohl(bp[5]);
	setOptCode(p,(uint8_t) (x >> 24) & 0xff);
	setOptLeng(p,(uint8_t) (x >> 16) & 0xff);
	setLfsOp(p,(uint8_t) (x >> 14) & 0x3);
	setLfsFlags(p,(uint8_t) (x >> 8) & 0x3f);
	// packed rates represet multiples of 10 Kb/s
	setLfsRrate(p,10*(((x >> 4) & 0xf) << (x & 0xf)));
	x = ntohl(bp[5]);
	setLfsTrace(p,x & 0xffffff);
	x >>= 24;
	setLfsArate(p,10*(((x >> 4) & 0xf) << (x & 0xf)));
}

void pktStore::pack(int p) {
// Pack header fields into buffer.
	uint32_t *bp = header(p);
	bp[0] = htonl(	(4 << 28) |
		     	((hlng(p) & 0xf) << 24) |
		     	((leng(p) & 0xff) << 16)
		);
	bp[1] = 0; // ignore fragmentation fields
	bp[2] = htonl((64 << 24) | (proto(p) & 0xff) << 16); 
	bp[3] = htonl(srcAdr(p));
	bp[4] = htonl(dstAdr(p));
	if (hleng(p) != 7 || optCode(p) != LFS_OPTION) return;
	// pack LFS option fields

	// first convert given rates to compact form, rounding up as needed
	int i, rrate, arate;
	rrate = (lfsRrate(p)+9)/10; // round up to next larger multiple of 10
	rrate += (15*rrate)/256;    // round up again to  avoid conversion loss
	for (i = 0; (rrate & 0xffffFFF0) != 0; i++) rrate >>= 1;
	if (i <= 15) rrate = (rrate << 4) | i;
	else rrate = 0xff;
	arate = (lfsArate(p)+9)/10; // repeat for allocated rate
	arate += (15*arate)/256;
	for (i = 0; (arate & 0xffffFFF0) != 0; i++) arate >>= 1;
	if (i <= 15) arate = (arate << 4) | i;
	else arate = 0xff;

	bp[5] = htonl( 	(optCode(p) << 24) |
	    		(optLeng(p) << 16) |
	    		((lfsOp(p) & 0x3) << 14) |
	    		((lfsFlags(p) & 0x3f) << 8) | rrate
		);
	bp[6] = htonl((arate << 24) | (lfsTrace(p) & 0xffffff));
}

bool pktStore::hdrErrCheck(int p) const {
        return true;
}

bool pktStore::payErrCheck(int p) const {
        return true;
}

void pktStore::hdrErrUpdate(int p) {
	uint32_t sum;
	uint16_t *bp = (uint16_t*) header(p);
	bp[5] = 0;
	hl = 2*hleng(p); sum = 0;
	for (int i = 0; i < hl; i++) sum += bp[i];
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	bp[5] = (uint16_t) (~sum);
}

void pktStore::payErrUpdate(int p) {
}

bool pktStore::getPacket(istream& is, int p) {
// Read an input packet from is and return it in p.
	int hleng, leng, optCode;
	misc::skipBlank(is);
	if (!misc::getNum(is,hleng) ||
	    !misc::getNum(is,leng) ||
	    !misc::getIpAdr(is,pd[p].src) ||
	    !misc::getIpAdr(is,pd[p].dst))
		return false;
	setHleng(p,hleng); setLeng(p,leng);
	if (hleng == 7 && misc::getNum(is,optCode) && optCode == LFS_OPTION) {
		setOptCode(p,LFS_OPTION);
		setOptLeng(p,8);
		int lfsOp, lfsFlags, lfsRrate, lfsArate, lfsTrace;
		if (!misc::getNum(is,lfsOp)    || !misc::getNum(is,lfsFlags) ||
		    !misc::getNum(is,lfsRrate) || !misc::getNum(is,lfsArate) ||
		    !misc::getNum(is,lfsTrace) ) 
			return false;
		setLfsOp(p,lfsOp); setLfsOp(p,lfsFlags);
		setLfsOp(p,lfsRrate); setLfsOp(p,lfsArate);
		setLfsOp(p,lfsTrace);
	}

	pack(p);

	uint32_t* bp = ps->payload(p);
	for (int i = 0; i < (leng-(4*hleng)/4; i++) {
		if (misc::getNum(is,x)) {
			bp[i] = htonl(x);
		} else {
			bp[i] = 0;
		}
	}
	ps->hdrErrUpdate(p); ps->payErrUpdate(p);
	return true;
}

void pktStore::print(ostream& os, int p) const {
// Print packet p for debugging. Prints header fields and raw
// version of first four words of buffer.
	int i;

	os << "hlen=" << setw(2) << hleng(p);
	os << " len=" << setw(4) << leng(p);
	os << " src="
	   << (srcAdr(p) >> 24) & 0xff << "."
	   << (srcAdr(p) >> 16) & 0xff << "."
	   << (srcAdr(p) >>  8) & 0xff << "."
	   << (srcAdr(p)      ) & 0xff;
	os << " dst="
	   << (dstAdr(p) >> 24) & 0xff << "."
	   << (dstAdr(p) >> 16) & 0xff << "."
	   << (dstAdr(p) >>  8) & 0xff << "."
	   << (dstAdr(p)      ) & 0xff;
	if (hleng(p) == 7 && optCode(p) == LFS_OPTION) {
		os << " lfsOp=" << lfsOp(p)
		   << " lfsFlags=" << lfsFlags(p)
		   << " lfsRrate=" << lfsRrate(p)
		   << " lfsArate=" << lfsArate(p)
		   << " lfsTrace=" << lfsTrace(p);
	}
	uint32_t *bp = payload(p);
	for (i = 0; i < min(8,(leng(p)-(4*hleng(p))/4); i++) {
		os << " " << ntohl(bp[i]);
	}
	os << endl;
}
