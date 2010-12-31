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
	uint32_t *bp = (uint32_t *) buffer(p);
	uint32_t x = ntohl(bp[0]);
	pd[p].lng = ((x >> 16) & 0xfff);
	pd[p].typ = ptyp_t((x >> 8) & 0xff);
	pd[p].vnet = ntohl(bp[1]);
	pd[p].sadr = ntohl(bp[2]);
	pd[p].dadr = ntohl(bp[3]);
}

void pktStore::pack(int p) {
// Pack header fields into buffer.
	uint32_t *bp = (uint32_t *) &buff[pd[p].buf];
	uint32_t x = (WUNET_VERSION << 28)
		     | ((pd[p].lng & 0xfff) << 16) 
		     | ptyp_t((pd[p].typ & 0xff) << 8);
	bp[0] = htonl(x);
	bp[1] = htonl(pd[p].vnet);
	bp[2] = htonl(pd[p].sadr);
	bp[3] = htonl(pd[p].dadr);
}

void pktStore::print(ostream& os, int p) const {
// Print packet p for debugging. Prints header fields and raw
// version of first four words of buffer.
	int i;

/*
	os << " length:  " << pd[p].lng  << endl;
	os << " pktTyp: ";
	if (pd[p].typ == DATA) os << "data";
	else if (pd[p].typ == SUBSCRIBE) os << "subscribe";
	else os << "unsubscribe";
	os << endl;
	os << "   vnet:  " << pd[p].vnet << endl;
	os << "src adr:  " << pd[p].sadr << endl;
	os << "dst adr:  " << pd[p].dadr << endl;
	os << "payload:  ";
	os.setf(ios::hex);
	for (i = 4; i < min(10,(pd[p].lng+3)/4); i++) {
		os << setw(4) << ntohl(buff[pd[p].buf][i]) << " ";
	}
	os.unsetf(ios::hex);
	os << endl;
*/
	os << "len=" << setw(3) << pd[p].lng;
	os << " typ=";
	if (pd[p].typ == DATA) os << "data ";
	else if (pd[p].typ == SUBSCRIBE) os << "sub  ";
	else if (pd[p].typ == UNSUBSCRIBE) os << "unsub";
	else if (pd[p].typ == VOQSTATUS) os << "vstat";
	else os << "-----";
	os << " vnet=" << setw(2) << pd[p].vnet;
	os << " sadr=" << setw(3) << pd[p].sadr;
	os << " dadr=" << setw(10) << pd[p].dadr;
	for (i = 4; i < min(10,(pd[p].lng+3)/4); i++) {
		os << " " << ntohl(buff[pd[p].buf][i]);
	}
}
