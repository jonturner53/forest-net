/** @file PacketStoreTs.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef PACKETSTORETS_H
#define PACKETSTORETS_H

#include "CommonDefs.h"
#include "UiList.h"
#include "PacketHeader.h"

typedef int packet;

/** Maintains a set of packets with selected header fields and a
 *  separate set of buffers. For use in multi-threaded contexts.
 *  The object is locked when allocating, de-allocating, or copying
 *  a packet. No locking is done for other methods. This is fine
 *  so long as no two threads attempt to access the same packet
 *  concurrently.
 */
class PacketStoreTs {
public:
                PacketStoreTs(int=10000);
                ~PacketStoreTs();

	// getters 
        PacketHeader& getHeader(packet) const;
        buffer_t& getBuffer(packet) const;      
        uint32_t* getPayload(packet) const;    

	// allocate/free packets 
        packet  alloc();           
        void    free(packet);     
	packet	fullCopy(packet);

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
        int     n;                      ///< number of packets in use

        PacketHeader *phdr;             ///< phdr[i] = header for packet i
        buffer_t *buff;                 ///< array of packet buffers

        UiList    *freePkts;            ///< list of free packets/buffers

	pthread_mutex_t lock;		///< used during allocate/free
};

/** Get reference to packet header.
 *  @param p is a packet number
 *  @return a reference to the packet header for p
 */
inline PacketHeader& PacketStoreTs::getHeader(packet p) const {
	return phdr[p];
}

/** Get reference to packet buffer.
 *  @param p is a packet number
 *  @return a reference to the buffer for packet p
 */
inline buffer_t& PacketStoreTs::getBuffer(packet p) const {
	return buff[p];
}

/** Get pointer to start of a packet payload.
 *  @param p is a packet number
 *  @return a pointer to first word of the payload for p
 */
inline uint32_t* PacketStoreTs::getPayload(packet p) const {
	return &buff[p][Forest::HDR_LENG/sizeof(uint32_t)];
}

/** Unpack the header fields for a packet from its buffer.
 *  @param p is the packet whose header fields are to be unpacked
 */
inline void PacketStoreTs::unpack(packet p) {
	getHeader(p).unpack(getBuffer(p));
}

/** Pack header fields into a packet's buffer.
 *  @param p is the packet whose buffer is to be packed from its header
 */
inline void PacketStoreTs::pack(packet p) {
	getHeader(p).pack(getBuffer(p));
}

/** Check the header error check field of a packet.
 *  @param p is a packet number
 */
inline bool PacketStoreTs::hdrErrCheck(packet p) {
	return getHeader(p).hdrErrCheck(getBuffer(p));
}

/** Check the payload error check field of a packet.
 *  @param p is a packet number
 */
inline bool PacketStoreTs::payErrCheck(packet p) {
	return getHeader(p).payErrCheck(getBuffer(p));
}

/** Update the header error check field of a packet.
 *  Computes error check over the header fields as they appear
 *  in the packet buffer.
 *  @param p is a packet number
 */
inline void PacketStoreTs::hdrErrUpdate(packet p) {
	getHeader(p).hdrErrUpdate(getBuffer(p));
}

/** Update the payload error check field of a packet.
 *  Computes error check over the entire packet payload.
 *  @param p is a packet number
 */
inline void PacketStoreTs::payErrUpdate(packet p) {
	getHeader(p).payErrUpdate(getBuffer(p));
}

#endif
