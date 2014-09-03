/** @file PacketStore.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "PacketStore.h"

namespace forest {

/** Constructor allocates space and initializes free lists.
 *  @param px is log2(number of packets to allocate space for)
 *  @param bx is log2(number of buffers to allocate space for)
 */
PacketStore::PacketStore(int px, int bx) : N(1 << px), M(1 << bx) {
	n = m = 0;
	pkt = new Packet[N+1];
	buff = new buffer_t[M+1];
	ref = new atomic<int>[M+1];
	freePkts = new Lfq<int>(N); freeBufs = new Lfq<int>(M);

	int i;
	for (i = 1; i <= N; i++) { freePkts->enq(i); pkt[i].buffer = 0; }
	for (i = 1; i <= M; i++) { freeBufs->enq(i); ref[i].store(0); }
	pkt[0].buffer = 0; ref[0].store(0);
};
	
PacketStore::~PacketStore() {
	delete [] pkt; delete [] buff; delete [] ref;
	delete freePkts; delete freeBufs;
}

/** Allocate a new packet and buffer.
 *  @return the packet number or 0, if no more packets available
 */
pktx PacketStore::alloc() {
	pktx px = freePkts->deq();
	if (px == 0) return 0;
	int b = freeBufs->deq();
	if (b == 0) {
		freePkts->enq(px); return 0;
	}
	ref[b].store(1);
	pkt[px].buffer = &buff[b];
	return px;
}

/** Release the storage used by a packet.
 *  Also releases the associated buffer, if no clones are using it.
 *  @param px is the packet number of the packet to be released
 */
void PacketStore::free(pktx px) {
	if (px < 1 || px > N || pkt[px].buffer == 0) return;
	int b = pkt[px].buffer - buff;
	pkt[px].buffer = 0; freePkts->enq(px);
	if ((--ref[b]) == 0) freeBufs->enq(b);
}

/** Make a "clone" of an existing packet.
 *  The clone shares the same buffer as the original.
 *  Its header is initialized to match the original.
 *  @param px is the packet number of the packet to be cloned
 *  @return the index of the new packet or 0 on failure
 */
pktx PacketStore::clone(pktx px) {
	if (px < 1 || px > N || pkt[px].buffer == 0) return 0;
	pktx cx = freePkts->deq();
	if (cx == 0) return 0;
	pkt[cx] = pkt[px];
	int b = pkt[px].buffer - buff; ref[b]++;
	return cx;
}

/** Allocate a new packet with the same content as p.
 *  A new buffer is allocated for this packet.
 *  @return the index of the new packet.
 */
pktx PacketStore::fullCopy(pktx px) {
	int cx = alloc();
	if (cx == 0) return 0;
	pkt[cx] = pkt[px];
	uint32_t* pp = (uint32_t*) pkt[px].buffer;
	uint32_t* cp = (uint32_t*) pkt[cx].buffer;
	int len = (pkt[px].length+3)/4;
	std::copy(pp, &pp[len], cp);
	return cx;
}

} // ends namespace
