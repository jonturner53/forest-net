/** @file PacketStore.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "PacketStore.h"

/** Constructor allocates space and initializes free lists.
 *  @param N1 is number of packets to allocate space for
 *  @param M1 is the number of buffers to allocate space for
 */
PacketStore::PacketStore(int N1, int M1) : N(N1), M(M1) {
	n = m = 0;
	pkt = new Packet[N+1]; pb = new int[N+1];
	buff = new buffer_t[M+1]; ref = new int[M+1];
	freePkts = new UiList(N); freeBufs = new UiList(M);

	int i;
	for (i = 1; i <= N; i++) { freePkts->addLast(i); pb[i] = 0; }
	for (i = 1; i <= M; i++) { freeBufs->addLast(i); ref[i] = 0; }
	pb[0] = ref[0] = 0;
};
	
PacketStore::~PacketStore() {
	delete [] pkt; delete [] pb; delete [] buff; delete [] ref;
	delete freePkts; delete freeBufs;
}

/** Allocate a new packet and buffer.
 *  @return the packet number or 0, if no more packets available
 */
pktx PacketStore::alloc() {
	if (freePkts->empty() || freeBufs->empty()) return 0;
	n++; m++;
	pktx px = freePkts->get(1); freePkts->removeFirst();
	int b = freeBufs->get(1); freeBufs->removeFirst();
	//pb[px] = b;
	ref[b] = 1;
	pkt[px].buffer = &buff[b];
	return px;
}

/** Release the storage used by a packet.
 *  Also releases the associated buffer, if no clones are using it.
 *  @param px is the packet number of the packet to be released
 */
void PacketStore::free(pktx px) {
	if (px < 1 || px > N || freePkts->member(px)) return;
	int b = pkt[px].buffer - buff;
	freePkts->addFirst(px); n--;
	if ((--ref[b]) == 0) { freeBufs->addFirst(b); m--; }
}

/** Make a "clone" of an existing packet.
 *  The clone shares the same buffer as the original.
 *  Its header is initialized to match the original.
 *  @param px is the packet number of the packet to be cloned
 *  @return the index of the new packet or 0 on failure
 */
pktx PacketStore::clone(pktx px) {
	if (freePkts->empty()) return 0;
	n++;
	pktx px1 = freePkts->get(1); freePkts->removeFirst();
	pkt[px1] = pkt[px];
	int b = pkt[px].buffer - buff; ref[b]++;
	return px1;
}

/** Allocate a new packet with the same content as p.
 *  A new buffer is allocated for this packet.
 *  @return the index of the new packet.
 */
pktx PacketStore::fullCopy(pktx px) {
	int px1 = alloc();
	if (px1 == 0) return 0;
	Packet& p = getPacket(px); Packet& p1 = getPacket(px1);
	buffer_t* tmp = p1.buffer; p1 = p; p1.buffer = tmp;
	int len = (p.length+3)/4;
        buffer_t& buf = *p.buffer; buffer_t& buf1 = *p1.buffer;
        for (int i = 0; i < len; i++) buf1[i] = buf[i];
	return px1;
}
