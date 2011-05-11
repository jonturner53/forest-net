// Header file for pktStore class.
// 
// Maintains a set of packets with selected header fields and a
// separate set of buffers. Each packet is associated with some
// buffer, but a buffer may be associated with several packets
// (to support multicast).

#ifndef PKTSTORE_H
#define PKTSTORE_H

#include "forest.h"
#include "list.h"

#include "header.h"

typedef int packet;

class pktStore {
public:
                pktStore(int=100000, int=50000);
                ~pktStore();
        packet  alloc();                // return new packet
        void    free(packet);           // return packet to free list
        packet  clone(packet);          // create packet copy, sharing buffer

        // pack/unpack header fields to/from buffer
        void    unpack(packet);         
        void    pack(packet);        
        // error checking
        bool    hdrErrCheck(packet); 
        bool    payErrCheck(packet);
        void    hdrErrUpdate(packet);    
        void    payErrUpdate(packet);   

        header& hdr(packet);            // return header for packet
        buffer_t& buffer(packet);       // return reference to packet's buffer
        uint32_t* payload(packet);      // return pointer to start of payload
private:
        int     N;                      // number of packets we have room for
        int     M;                      // number of buffers we have room for
        int     n;                      // number of packets in use
        int     m;                      // number of buffers in use

        header *phdr;                   // phdr[i] = header for packet i
        int     *pb;                    // pb[i] = index of packet i's buffer

        buffer_t *buff;                 // array of packet buffers
        int     *ref;                   // array of reference counts for buffers

        list    *freePkts;              // list of free packets
        list    *freeBufs;              // list of free buffers
};

// Return reference to packet header
inline header& pktStore::hdr(packet p) { return phdr[p]; }

// Return reference to buffer for packet p.
// This is for use of the IO routines (recvfrom, sendto).
inline buffer_t& pktStore::buffer(packet p) {
	return buff[pb[p]];
}

// Return pointer to start of payload for p.
inline uint32_t* pktStore::payload(packet p) {
	return &buff[pb[p]][HDR_LENG/sizeof(uint32_t)];
}

inline void pktStore::unpack(packet p) { (hdr(p)).unpack(buffer(p)); }
inline void pktStore::pack(packet p)   { (hdr(p)).pack(buffer(p)); }

inline bool pktStore::hdrErrCheck(packet p) {
	return (hdr(p)).hdrErrCheck(buffer(p));
}
inline bool pktStore::payErrCheck(packet p) {
	return (hdr(p)).payErrCheck(buffer(p));
}
inline void pktStore::hdrErrUpdate(packet p) {
	(hdr(p)).hdrErrUpdate(buffer(p));
}
inline void pktStore::payErrUpdate(packet p) {
	(hdr(p)).payErrUpdate(buffer(p));
}

#endif
