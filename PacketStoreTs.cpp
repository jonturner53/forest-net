/** @file PacketStoreTs.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "PacketStoreTs.h"

/** Constructor allocates space and initializes free list.
 *  @param N1 is number of packets to allocate space for
 */
PacketStoreTs::PacketStoreTs(int N1) : N(N1) {
	n = 0;
	phdr = new PacketHeader[N+1]; 
	buff = new buffer_t[N+1]; 
	freePkts = new UiList(N); 

	for (int i = 1; i <= N; i++) freePkts->addLast(i);

	pthread_mutex_init(&lock,NULL);
};
	
PacketStoreTs::~PacketStoreTs() {
	delete [] phdr; delete [] buff; delete freePkts; 
}

/** Allocate a new packet and buffer.
 *  @return the packet number or 0, if no more packets available
 */
packet PacketStoreTs::alloc() {
	pthread_mutex_lock(&lock);

	packet p;
	if (freePkts->empty()) { 
		p = 0;
	} else {
		p = freePkts->get(1); freePkts->removeFirst(); n++;
	}

	pthread_mutex_unlock(&lock);
	if (p == 0) {
		cerr << "PacketStoreTs::alloc: no packets left to "
			"allocate\n";
	}
	return p;
}

/** Release the storage used by a packet.
 *  Also releases the associated buffer, if no clones are using it.
 *  @param p is the packet number of the packet to be released
 */
void PacketStoreTs::free(packet p) {
	pthread_mutex_lock(&lock);

	if (p >= 1 && p <= N && !freePkts->member(p)) {
		freePkts->addFirst(p); n--;
	}

	pthread_mutex_unlock(&lock);
}

/** Allocate a new packet that with the same content as p.
 *  A new buffer is allocated for this packet.
 *  @return the index of the new packet.
 */
packet PacketStoreTs::fullCopy(packet p) {
        int p1 = alloc();
        if (p1 == 0) return 0;
	PacketHeader& h = getHeader(p);
	PacketHeader& h1 = getHeader(p1);
        int len = (h.getLength()+3)/4;
        buffer_t& buf = getBuffer(p); buffer_t& buf1 = getBuffer(p1);
        for (int i = 0; i < len; i++) buf1[i] = buf[i];
	h1 = h;
        return p1;
}

