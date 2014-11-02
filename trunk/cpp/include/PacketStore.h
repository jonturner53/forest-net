/** @file PacketStore 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef PACKETSTORE_H
#define PACKETSTORE_H

#include <thread>
#include <mutex>
#include <chrono>

using std::mutex;
using std::unique_lock;
using std::thread;

#include "Forest.h"
#include "List.h"
#include "Stack.h"
#include "Packet.h"
#include "NonblockingQ.h"

namespace forest {

typedef int pktx;

/** Maintains a set of packets with selected header fields and a
 *  separate set of buffers. Each packet is associated with some
 *  buffer, but a buffer may be associated with several packets
 *  (to support multicast).
 *  
 *  Packets are identified by an integer index.
 */
class PacketStore {
public:
                PacketStore(int=17, int=16);
                ~PacketStore();

        Packet& getPacket(pktx) const;

	int	newCache();

	// allocate/free packets 
        pktx  alloc();           
        pktx  alloc(int);           
        void  free(pktx);     
        void  free(pktx, int);     
        pktx  clone(pktx);   
        pktx  clone(pktx, int);   
        pktx  fullCopy(pktx);   
        pktx  fullCopy(pktx,int);   

	string toString() const;

private:
	static int const MAX_CACHE = 10;
	static int const CACHE_SIZE = 128;

        int     N;                      ///< number of packets we have room for
        int     M;                      ///< number of buffers we have room for
        int     n;                      ///< number of packets in use
        int     m;                      ///< number of buffers in use
	int	nextCache;		///< index of next available cache

        Packet	*pkt;             	///< pkt[i] = packet with index i
        buffer_t *buff;                 ///< array of packet buffers
        atomic<int> *ref;               ///< array of ref counts for buffers

        Stack<int> *freePkts;   	///< stack of free packet indexes
        Stack<int> *freeBufs;   	///< stack of free buffer indexes

	Stack<int> **pxCache;		///< array of pointers to packet caches
	Stack<int> **bxCache;		///< array of pointers to buffer caches

	mutex	mtx;
};

/** Get reference to packet header.
 *  @param px is a packet index
 *  @return a reference to the packet for px
 */
inline Packet& PacketStore::getPacket(pktx px) const {
	return pkt[px];
}

} // ends namespace


#endif
