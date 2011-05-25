/** \file PacketStore.cpp */

#include "PacketStore.h"

PacketStore::PacketStore(int N1, int M1) : N(N1), M(M1) {
// Constructor for PacketStore, allocates space and initializes table.
	n = m = 0;
	phdr = new PacketHeader[N+1]; pb = new int[N+1];
	buff = new buffer_t[M+1]; ref = new int[M+1];
	freePkts = new UiList(N); freeBufs = new UiList(M);

	int i;
	for (i = 1; i <= N; i++) { freePkts->addLast(i); pb[i] = 0; }
	for (i = 1; i <= M; i++) { freeBufs->addLast(i); ref[i] = 0; }
	pb[0] = ref[0] = 0;
};
	
PacketStore::~PacketStore() {
	delete [] phdr; delete [] pb; delete [] buff; delete [] ref;
	delete freePkts; delete freeBufs;
}

packet PacketStore::alloc() {
// Allocate new packet and buffer and return it.
// If no space for packet, return 0.
	packet p; int b;
	if (freePkts->empty() || freeBufs->empty()) return 0;
	n++; m++;
	p = freePkts->get(1); freePkts->removeFirst();
	b = freeBufs->get(1); freeBufs->removeFirst();
	pb[p] = b; ref[b] = 1;
	return p;
}

void PacketStore::free(packet p) {
// Free packet p and release buffer, if no other packets using it.
	assert(1 <= p && p <= N);
	int b = pb[p]; pb[p] = 0;
	freePkts->addFirst(p); n--;
	if ((--ref[b]) == 0) {
	freeBufs->addFirst(b); m--; }
}

packet PacketStore::clone(packet p) {
// Allocate a new packet that references the same buffer as p,
// and initialize its header fields to match p.
// Return a pointer to the new packet.
	int b = pb[p];
	if (freePkts->empty()) return 0;
	n++;
	packet p1 = freePkts->get(1); freePkts->removeFirst();
	ref[b]++; setHeader(p1,getHeader(p)); pb[p1] = pb[p];
	return p1;
}
