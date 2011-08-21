/** @file PacketStore 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef PACKETSTORE_H
#define PACKETSTORE_H

#include "CommonDefs.h"
#include "UiList.h"
#include "PacketHeader.h"

typedef int packet;

/** Maintains a set of packets with selected header fields and a
 *  separate set of buffers. Each packet is associated with some
 *  buffer, but a buffer may be associated with several packets
 *  (to support multicast).
 */
class PacketStore {
public:
                PacketStore(int=100000, int=50000);
                ~PacketStore();

	// getters 
        PacketHeader& getHeader(packet) const;
        buffer_t& getBuffer(packet) const;      
        uint32_t* getPayload(packet) const;    

	// setters 
        void setHeader(packet, const PacketHeader&);

	// allocate/free packets 
        packet  alloc();           
        void    free(packet);     
        packet  clone(packet);   
        packet  fullCopy(packet);   

        // pack/unpack header fields to/from buffer 
        void    unpack(packet);         
        void    pack(packet);        

        // error checking 
        bool    hdrErrCheck(packet); 
        bool    payErrCheck(packet);
        void    hdrErrUpdate(packet);    
        void    payErrUpdate(packet);   

private:
        int     N;                      ///< number of packets we have room for
        int     M;                      ///< number of buffers we have room for
        int     n;                      ///< number of packets in use
        int     m;                      ///< number of buffers in use

        PacketHeader *phdr;             ///< phdr[i] = header for packet i
        int     *pb;                    ///< pb[i] = index of packet i's buffer

        buffer_t *buff;                 ///< array of packet buffers
        int     *ref;                   ///< array of ref counts for buffers

        UiList    *freePkts;              ///< list of free packets
        UiList    *freeBufs;              ///< list of free buffers
};

/** Get reference to packet header.
 *  @param p is a packet number
 *  @return a reference to the packet header for p
 */
inline PacketHeader& PacketStore::getHeader(packet p) const {
	return phdr[p];
}

/** Get reference to packet buffer.
 *  @param p is a packet number
 *  @return a reference to the buffer for packet p
inline buffer_t& PacketStore::getBuffer(packet p) const {
	return buff[pb[p]];
}

/** Get pointer to start of a packet payload.
 *  @param p is a packet number
 *  @return a pointer to first word of the payload for p
 */
inline uint32_t* PacketStore::getPayload(packet p) const {
	return &buff[pb[p]][Forest::HDR_LENG/sizeof(uint32_t)];
}

/** Set the header fields for a packet.
 *  @param p is the packet whose header is to be updated
 *  @param h is a reference to another header whose value is to be
 *  copied into the header for p
 */
inline void PacketStore::setHeader(packet p, const PacketHeader& h) {
	phdr[p] = h;
}

/** Unpack the header fields for a packet from its buffer.
 *  @param p is the packet whose header fields are to be unpacked
 */
inline void PacketStore::unpack(packet p) {
	getHeader(p).unpack(getBuffer(p));
}

/** Pack header fields into a packet's buffer.
 *  @param p is the packet whose buffer is to be packed from its header
 */
inline void PacketStore::pack(packet p) {
	getHeader(p).pack(getBuffer(p));
}

/** Check the header error check field of a packet.
 *  @param p is a packet number
 */
inline bool PacketStore::hdrErrCheck(packet p) {
	return getHeader(p).hdrErrCheck(getBuffer(p));
}

/** Check the payload error check field of a packet.
 *  @param p is a packet number
 */
inline bool PacketStore::payErrCheck(packet p) {
	return getHeader(p).payErrCheck(getBuffer(p));
}

/** Update the header error check field of a packet.
 *  Computes error check over the header fields as they appear
 *  in the packet buffer.
 *  @param p is a packet number
 */
inline void PacketStore::hdrErrUpdate(packet p) {
	getHeader(p).hdrErrUpdate(getBuffer(p));
}

/** Update the payload error check field of a packet.
 *  Computes error check over the entire packet payload.
 *  @param p is a packet number
 */
inline void PacketStore::payErrUpdate(packet p) {
	getHeader(p).payErrUpdate(getBuffer(p));
}

#endif
