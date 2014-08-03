/** @file PacketStoreTs.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef PACKETSTORETS_H
#define PACKETSTORETS_H

#include "Forest.h"
#include "List.h"
#include "Packet.h"

namespace forest {


typedef int pktx;

/** Maintains a set of packets with selected header fields and a
 *  separate set of buffers. For use in multi-threaded contexts.
 *  The object is locked when allocating, de-allocating, or copying
 *  a packet. No locking is done for other methods. This is fine
 *  so long as no two threads attempt to access the same packet
 *  concurrently. Also note that this version of packet store
 *  does not support multiple packets that reference the same buffer,
 *  as it is intended for use with end systems, not routers.
 */
class PacketStoreTs {
public:
                PacketStoreTs(int=10000);
                ~PacketStoreTs();

	// getters 
        Packet& getPacket(pktx) const;
        //buffer_t& getBuffer(pktx) const;      
        //uint32_t* getPayload(pktx) const;    

	// allocate/free packets 
        pktx  alloc();           
        void    free(pktx);     
	pktx	fullCopy(pktx);

        // pack/unpack header fields to/from buffer 
        void    unpack(pktx);         
        void    pack(pktx);        

        // error checking 
        //bool    hdrErrCheck(packet); 
        //bool    payErrCheck(packet);
        //void    hdrErrUpdate(packet);    
        //void    payErrUpdate(packet);   

private:
        int     N;                      ///< number of packets we have room for
        int     n;                      ///< number of packets in use

        Packet *pkt;             	///< pkt[i] = packet i
        buffer_t *buff;                 ///< buff[i] = buffer for packet i

        List    *freePkts;            ///< list of free packets/buffers

	pthread_mutex_t lock;		///< used during allocate/free
};

/** Get reference to packet header.
 *  @param px is a packet number
 *  @return a reference to the packet header for p
 */
inline Packet& PacketStoreTs::getPacket(pktx px) const {
	return pkt[px];
}

/** Get reference to packet buffer.
 *  @param px is a packet number
 *  @return a reference to the buffer for packet px
inline buffer_t& PacketStoreTs::getBuffer(pktx px) const {
	return buff[px];
}
 */

/** Get pointer to start of a packet payload.
 *  @param p is a packet number
 *  @return a pointer to first word of the payload for p
inline uint32_t* PacketStoreTs::getPayload(pktx px) const {
	return &buff[p][Forest::HDR_LENG/sizeof(uint32_t)];
}
 */

/** Unpack the header fields for a packet from its buffer.
 *  @param p is the packet whose header fields are to be unpacked
inline void PacketStoreTs::unpack(pktx px) {
	getHeader(p).unpack(getBuffer(p));
}
 */

/** Pack header fields into a packet's buffer.
 *  @param p is the packet whose buffer is to be packed from its header
inline void PacketStoreTs::pack(pktx px) {
	getHeader(p).pack(getBuffer(p));
}
 */

/** Check the header error check field of a packet.
 *  @param p is a packet number
inline bool PacketStoreTs::hdrErrCheck(pktx px) {
	return getHeader(p).hdrErrCheck(getBuffer(p));
}
 */

/** Check the payload error check field of a packet.
 *  @param p is a packet number
inline bool PacketStoreTs::payErrCheck(pktx px) {
	return getHeader(p).payErrCheck(getBuffer(p));
}
 */

/** Update the header error check field of a packet.
 *  Computes error check over the header fields as they appear
 *  in the packet buffer.
 *  @param p is a packet number
inline void PacketStoreTs::hdrErrUpdate(pktx px) {
	getHeader(p).hdrErrUpdate(getBuffer(p));
}
 */

/** Update the payload error check field of a packet.
 *  Computes error check over the entire packet payload.
 *  @param p is a packet number
inline void PacketStoreTs::payErrUpdate(pktx px) {
	getHeader(p).payErrUpdate(getBuffer(p));
}
 */


} // ends namespace

#endif
