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
 *  @param numPkts is the number of packets to allocate space for
 *  @param numBufs is the number of buffers to allocate space for
 */
PacketStore::PacketStore(int numPkts, int numBufs) : N(numPkts), M(numBufs) {
	n = m = 0;
	pkt = new Packet[N+1];
	buff = new buffer_t[M+1];
	ref = new atomic<int>[M+1];
	freePkts = new Stack<int>(N);
	freeBufs = new Stack<int>(M);

	foo = new int*[10];
	pxCache = new Stack<int>*[MAX_CACHE+1];
	bxCache = new Stack<int>*[MAX_CACHE+1];
	nextCache = 1;

	int i;
	for (i = 1; i <= N; i++) { freePkts->push(i); pkt[i].buffer = 0; }
	for (i = 1; i <= M; i++) { freeBufs->push(i); ref[i].store(1); }
	pkt[0].buffer = 0; ref[0].store(0);
};
	
PacketStore::~PacketStore() {
	delete [] pkt; delete [] buff; delete [] ref;
	delete freePkts; delete freeBufs;
}

int PacketStore::newCache() {
	if (nextCache == MAX_CACHE) return 0;
	pxCache[nextCache] = new Stack<int>(CACHE_SIZE);
	bxCache[nextCache] = new Stack<int>(CACHE_SIZE);
	pxCache[nextCache]->xferIn(*freePkts, MAX_CACHE/2);
	bxCache[nextCache]->xferIn(*freeBufs, MAX_CACHE/2);
	return nextCache++;
}

/** Allocate a new packet and buffer.
 *  @return the packet number or 0, if no more packets available
 */
pktx PacketStore::alloc() {
	unique_lock<mutex> lck(mtx);
	if (freePkts->empty()) return 0;
	pktx px = freePkts->pop();
	if (freeBufs->empty()) {
		freePkts->push(px); return 0;
	}
	int bx = freeBufs->pop();
	pkt[px].buffer = &buff[bx];
	return px;
}


/** Allocate a new packet and buffer, using a cache.
 *  @param cx is the index of the cache assigned to the calling thread
 *  @return the packet number or 0, if no more packets available
 */
pktx PacketStore::alloc(int cx) {
	bool noPkts = pxCache[cx]->empty();
	bool noBufs = bxCache[cx]->empty();
	if (noPkts || noBufs) {
		unique_lock<mutex> lck(mtx);
		if (noPkts) {
			if (pxCache[cx]->xferIn(*freePkts,MAX_CACHE/2) == 0)
				return 0;
		}
		if (noBufs) {
			if (bxCache[cx]->xferIn(*freeBufs,MAX_CACHE/2) == 0)
				return 0;
		}
	}
	int px = pxCache[cx]->pop();
	int bx = bxCache[cx]->pop();
	pkt[px].buffer = &buff[bx];
        return px;
}


/** Release the storage used by a packet.
 *  Also releases the associated buffer, if no clones are using it.
 *  @param px is the packet number of the packet to be released
 */
void PacketStore::free(pktx px) {
	if (px < 1 || px > N || pkt[px].buffer == 0) return;
	unique_lock<mutex> lck(mtx);
	int bx = pkt[px].buffer - buff;
	pkt[px].buffer = 0; freePkts->push(px);
	if (ref[bx] == 1) freeBufs->push(bx);
	else --ref[bx];
}

/** Release the storage used by a packet, using a cache.
 *  Also releases the associated buffer, if no clones are using it.
 *  @param px is the packet number of the packet to be released
 *  @param cx is the index of the cache assigned to the calling thread
 */
void PacketStore::free(pktx px, int cx) {
	if (px < 1 || px > N || pkt[px].buffer == 0) return;
	int bx = pkt[px].buffer - buff;
	pkt[px].buffer = 0;

	if (ref[bx].load() > 1) {
		if (!pxCache[cx]->push(px)) {
			unique_lock<mutex> lck(mtx);
			freePkts->xferIn(*pxCache[cx],CACHE_SIZE/2);
			pxCache[cx]->push(px);
		}
		ref[bx]--;
		return;
	}
	if (!pxCache[cx]->push(px)) {
		unique_lock<mutex> lck(mtx);
		freePkts->xferIn(*pxCache[cx],CACHE_SIZE/2);
		pxCache[cx]->push(px);
		if (!bxCache[cx]->push(bx)) {
			freeBufs->xferIn(*bxCache[cx],CACHE_SIZE/2);
			bxCache[cx]->push(bx);
		}
	}
	return;
}

/** Make a "clone" of an existing packet.
 *  The clone shares the same buffer as the original.
 *  Its header is initialized to match the original.
 *  @param px is the packet number of the packet to be cloned
 *  @return the index of the new packet or 0 on failure
 */
pktx PacketStore::clone(pktx px) {
	if (px < 1 || px > N || pkt[px].buffer == 0) return 0;
	unique_lock<mutex> lck(mtx);
	if (freePkts->empty()) return 0;
	pktx ppx = freePkts->pop();
	lck.unlock();
	pkt[ppx] = pkt[px];
	int bx = pkt[px].buffer - buff; ref[bx]++;
	return ppx;
}

/** Make a "clone" of an existing packet using a cache.
 *  The clone shares the same buffer as the original.
 *  Its header is initialized to match the original.
 *  @param px is the packet number of the packet to be cloned
 *  @param cx is the index of the cache assigned to the calling thread
 *  @return the index of the new packet or 0 on failure
 */
pktx PacketStore::clone(pktx px, int cx) {
	if (px < 1 || px > N || pkt[px].buffer == 0) return 0;
	if (pxCache[cx]->empty()) {
		unique_lock<mutex> lck(mtx);
		if (pxCache[cx]->xferIn(*freePkts,CACHE_SIZE/2) == 0)
			return 0;
	}
	pktx ppx = pxCache[cx]->pop();
	pkt[ppx] = pkt[px];
	int bx = pkt[px].buffer - buff; ref[bx]++;
	return ppx;
}

/** Allocate a new packet with the same content as p.
 *  A new buffer is allocated for this packet.
 *  @return the index of the new packet.
 */
pktx PacketStore::fullCopy(pktx px) {
	int ppx = alloc();
	if (ppx == 0) return 0;
	pkt[ppx] = pkt[px];
	uint32_t* pp = (uint32_t*) pkt[px].buffer;
	uint32_t* ppp = (uint32_t*) pkt[ppx].buffer;
	int len = (pkt[px].length+3)/4;
	std::copy(pp, &pp[len], ppp);
	return ppx;
}

/** Allocate a new packet with the same content as p using a cache.
 *  A new buffer is allocated for this packet.
 *  @param px is the index of the packet to be copied
 *  @param cx is the index of the cache assigned to the calling thread
 *  @return the index of the new packet.
 */
pktx PacketStore::fullCopy(pktx px, int cx) {
	int ppx = alloc(cx);
	if (ppx == 0) return 0;
	pkt[ppx] = pkt[px];
	uint32_t* pp = (uint32_t*) pkt[px].buffer;
	uint32_t* ppp = (uint32_t*) pkt[ppx].buffer;
	int len = (pkt[px].length+3)/4;
	std::copy(pp, &pp[len], ppp);
	return ppx;
}

} // ends namespace
