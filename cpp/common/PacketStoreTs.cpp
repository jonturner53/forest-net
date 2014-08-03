/** @file PacketStoreTs.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "PacketStoreTs.h"

namespace forest {

/** Constructor allocates space and initializes free list.
 *  @param N1 is number of packets to allocate space for
 */
PacketStoreTs::PacketStoreTs(int N1) : N(N1) {
	n = 0;
	pkt = new Packet[N+1]; 
	buff = new buffer_t[N+1]; 
	freePkts = new List(N); 

	for (int i = 1; i <= N; i++) {
		pkt[i].buffer = &buff[i];
		freePkts->addLast(i);
	}

	pthread_mutex_init(&lock,NULL);
};
	
PacketStoreTs::~PacketStoreTs() {
	delete [] pkt; delete [] buff; delete freePkts; 
}

/** Allocate a new packet and buffer.
 *  @return the packet number or 0, if no more packets available
 */
pktx PacketStoreTs::alloc() {
	pthread_mutex_lock(&lock);

	pktx px;
	if (freePkts->empty()) { 
		px = 0;
	} else {
		px = freePkts->get(1); freePkts->removeFirst(); n++;
	}

	pthread_mutex_unlock(&lock);
	if (px == 0) {
		cerr << "PacketStoreTs::alloc: no packets left to "
			"allocate\n";
	}
	return px;
}

/** Release the storage used by a packet.
 *  Also releases the associated buffer, if no clones are using it.
 *  @param p is the packet number of the packet to be released
 */
void PacketStoreTs::free(pktx px) {
	pthread_mutex_lock(&lock);

	if (px >= 1 && px <= N && !freePkts->member(px)) {
		freePkts->addFirst(px); n--;
	}

	pthread_mutex_unlock(&lock);
}

/** Allocate a new packet that with the same content as px.
 *  A new buffer is allocated for this packet.
 *  @return the index of the new packet.
 */
pktx PacketStoreTs::fullCopy(pktx px) {
        int px1 = alloc();
        if (px1 == 0) return 0;
	Packet& p = getPacket(px); Packet& p1 = getPacket(px1);
	buffer_t* tmp = p1.buffer; p1 = p; p1.buffer = tmp;
        int len = (p.length+3)/4;
        buffer_t& buf = *p.buffer; buffer_t& buf1 = *p1.buffer;
        for (int i = 0; i < len; i++) buf1[i] = buf[i];
        return px1;
}

} // ends namespace
