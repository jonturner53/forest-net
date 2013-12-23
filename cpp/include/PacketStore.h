/** @file PacketStore 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef PACKETSTORE_H
#define PACKETSTORE_H

#include "Forest.h"
#include "UiList.h"
#include "Packet.h"

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
                PacketStore(int=100000, int=50000);
                ~PacketStore();

	// getters 
        Packet& getPacket(pktx) const;
	int	getFreeSpace() const;

	// setters 
        //void setPacket(pktx, const Packet&);

	// allocate/free packets 
        pktx  alloc();           
        void  free(pktx);     
        pktx  clone(pktx);   
        pktx  fullCopy(pktx);   

private:
        int     N;                      ///< number of packets we have room for
        int     M;                      ///< number of buffers we have room for
        int     n;                      ///< number of packets in use
        int     m;                      ///< number of buffers in use

        Packet	*pkt;             	///< hkt[i] = packet with index i
        int     *pb;                    ///< pb[i] = index of packet i's buffer

        buffer_t *buff;                 ///< array of packet buffers
        int     *ref;                   ///< array of ref counts for buffers

        UiList    *freePkts;              ///< list of free packets
        UiList    *freeBufs;              ///< list of free buffers
};

/** Get reference to packet header.
 *  @param px is a packet index
 *  @return a reference to the packet for px
 */
inline Packet& PacketStore::getPacket(pktx px) const {
	return pkt[px];
}

inline int PacketStore::getFreeSpace() const { return min(N-n,M-m); }

} // ends namespace


#endif
