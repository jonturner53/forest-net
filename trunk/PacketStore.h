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

	/** getters */
        PacketHeader& getHeader(packet) const;
        buffer_t& getBuffer(packet) const;      
        uint32_t* getPayload(packet) const;    

	/** setters */
        void setHeader(packet, const PacketHeader&);

	/** allocate/free packets */
        packet  alloc();           
        void    free(packet);     
        packet  clone(packet);   

        /** pack/unpack header fields to/from buffer */
        void    unpack(packet);         
        void    pack(packet);        

        /** error checking */
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

// Return reference to packet header
inline PacketHeader& PacketStore::getHeader(packet p) const {
	return phdr[p];
}

// Return reference to buffer for packet p.
// This is for use of the IO routines (recvfrom, sendto).
inline buffer_t& PacketStore::getBuffer(packet p) const {
	return buff[pb[p]];
}

// Return pointer to start of payload for p.
inline uint32_t* PacketStore::getPayload(packet p) const {
	return &buff[pb[p]][Forest::HDR_LENG/sizeof(uint32_t)];
}

inline void PacketStore::setHeader(packet p, const PacketHeader& h) {
	phdr[p] = h;
}

inline void PacketStore::unpack(packet p) {
	getHeader(p).unpack(getBuffer(p));
}
inline void PacketStore::pack(packet p) {
	getHeader(p).pack(getBuffer(p));
}

inline bool PacketStore::hdrErrCheck(packet p) {
	return getHeader(p).hdrErrCheck(getBuffer(p));
}
inline bool PacketStore::payErrCheck(packet p) {
	return getHeader(p).payErrCheck(getBuffer(p));
}
inline void PacketStore::hdrErrUpdate(packet p) {
	getHeader(p).hdrErrUpdate(getBuffer(p));
}
inline void PacketStore::payErrUpdate(packet p) {
	getHeader(p).payErrUpdate(getBuffer(p));
}

#endif
