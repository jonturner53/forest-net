#include "pktStore.h"

pktStore::pktStore(int N1, int M1) : N(N1), M(M1) {
// Constructor for pktStore, allocates space and initializes table.
	n = m = 0;
	phdr = new header[N+1]; pb = new int[N+1];
	buff = new buffer_t[M+1]; ref = new int[M+1];
	freePkts = new list(N); freeBufs = new list(M);

	int i;
	for (i = 1; i <= N; i++) { *freePkts &= i; pb[i] = 0; }
	for (i = 1; i <= M; i++) { *freeBufs &= i; ref[i] = 0; }
	pb[0] = ref[0] = 0;
};
	
pktStore::~pktStore() {
	delete [] phdr; delete [] pb; delete [] buff; delete [] ref;
	delete freePkts; delete freeBufs;
}

packet pktStore::alloc() {
// Allocate new packet and buffer and return it.
// If no space for packet, return 0.
	packet p; int b;
	if (freePkts->empty() || freeBufs->empty()) return 0;
	n++; m++;
	p = (*freePkts)[1]; (*freePkts) <<= 1;
	b = (*freeBufs)[1]; (*freeBufs) <<= 1;
	pb[p] = b; ref[b] = 1;
	return p;
}

void pktStore::free(packet p) {
// Free packet p and release buffer, if no other packets using it.
	assert(1 <= p && p <= N);
	int b = pb[p]; pb[p] = 0;
	(*freePkts).push(p); n--;
	if ((--ref[b]) == 0) { (*freeBufs).push(b); m--; }
}

packet pktStore::clone(packet p) {
// Allocate a new packet that references same buffer as p,
// and initialize its header fields to match p.
// Return a pointer to the new packet.
	int b = pb[p];
	if (freePkts->empty() || ref[b] >= MAXREFCNT) return 0;
	n++;
	packet p1 = (*freePkts)[1]; (*freePkts) <<= 1;
	ref[b]++; hdr(p1) = hdr(p); pb[p1] = pb[p];
	return p1;
}
