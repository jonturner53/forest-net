// Header file for pktStore class.
// 
// Maintains a set of packets with selected header fields and a
// separate set of buffers. Each packet is associated with some
// buffer, but a buffer may be associated with several packets
// (to support multicast).

#ifndef PKTSTORE_H
#define PKTSTORE_H

#include "lfs.h"
#include "list.h"

class pktStore {
public:
		pktStore(int=100000, int=50000);
		~pktStore();
	int	alloc();		// return index to new packet
	void	free(int);		// return packet to free list
	int	clone(int);		// create packet copy, sharing buffer
	buffer_t *buffer(int) const;	// return pointer to packet's buffer
	uint32_t *header(int) const;	// return int32 pointer to start of hdr
	uint32_t *payload(int) const;	// return int32 pointer to payload

	void	unpack(int);		// unpack header fields from buffer
	void	pack(int);		// pack header fields into buffer

	uint16_t leng(int) const;	// return length field
	uint8_t hleng(int) const;	// return header length field
	uint8_t proto(int) const;	// return protocol field
	ipa_t	srcAdr(int) const;	// return source address field
	ipa_t	dstAdr(int) const;	// return destination address field
	uint8_t optCode(int) const;	// return option code field
	uint8_t optLen(int) const;	// return option length
	uint8_t lfsOp(int) const;	// return LFS operation field
	uint8_t lfsFlags(int) const;	// return LFS flags
	int 	lfsRrate(int) const;	// return LFS requested rate
	int 	lfsArate(int) const;	// return LFS allocated rate
	int	inLink(int) const;	// return link on which packet arrived
	uint16_t ioBytes(int) const;	// return number of bytes received

	void	setLeng(int, uint16_t);	// set length field
	void	setHleng(int, uint8_t);	// set packet type field
	void	setProto(int, uint8_t);// set protocol field
	void	setSrcAdr(int, ipa_t);	// set source address field
	void	setDstAdr(int, ipa_t);	// set destination address field
	void	setOptCode(int, uint8_t); 	// set option code
	void	setOptLen(int, uint8_t); 	// set option length
	void	setLfsOp(int, uint8_t); 	// set LFS Operation field
	void	setLfsFlags(int, uint8_t); 	// set LFS flags
	void	setLfsRrate(int, int); 		// set LFS requested rate
	void	setLfsArate(int, int); 		// set LFS allocated rate
	void	setInLink(int, int); 		// set input link number
	void	setIoBytes(int, uint16_t); 	// set # of bytes received

        bool    hdrErrCheck(int) const; // return true if header check passes
        bool    payErrCheck(int) const; // return true if payload check passes
        void    hdrErrUpdate(int);      // update header check
        void    payErrUpdate(int);      // update payload check

	bool	getPacket(istream&, int); // get packet from input
	void	print(ostream&, int) const; // print packet
private:
	int	N;			// number of packets we have room for
	int	M;			// number of buffers we have room for
	int	n;			// number of packets in use
	int	m;			// number of buffers in use

	struct pktData {
	uint16_t lng;			// length field from IP header
	uint8_t hlng;			// header length field
	uint8_t proto;			// header length field
	ipa_t	sadr;			// source address
	ipa_t	dadr;			// destination address
	uint8_t	optCode;		// IP option code
	uint8_t	optLeng;		// IP option length
	uint8_t	lfsOp;			// LFS operation code
	uint8_t	lfsFlags;		// LFS flags
	int	lfsRrate;		// LFS requested rate
	int	lfsArate;		// LFS allocated rate
	int	lfsTrace;		// LFS trace field
	int	inLnk;			// link on which packet arrived
	uint16_t ioBytes;		// number of bytes received
	int	buf;			// buff[pd[p].buf] = buffer # for pkt p
	};				//                 0 for unused buffers

	pktData *pd;			// pd[i] = data for packet i
	buffer_t *buff;	 		// buff[b] = buffer number b
	uint16_t *ref;			// ref[b] = reference count for buffer b
					//          0 for unused buffers
	list	*freePkts;		// list of free packets
	list	*freeBufs;		// list of free buffers
	
};

// Return pointer to buffer for packet p.
// This is for use of the IO routines (recvfrom, sendto).
inline buffer_t* pktStore::buffer(int p) const { return &buff[pd[p].buf]; }
inline uint32_t* pktStore::header(int p) const {
	return (uint32_t *) &buff[pd[p].buf];
}
inline uint32_t* pktStore::payload(int p) const {
	return (uint32_t *) &header(p)[5];
}

// Header field access methods
inline int pktStore::leng(int p) const 		{ return pd[p].lng; }
inline int pktStore::hleng(int p) const 	{ return pd[p].hlng; }
inline uint8_t pktStore::proto(int p) const 	{ return pd[p].proto; }
inline ipa_t pktStore::srcAdr(int p) const 	{ return pd[p].sadr; }
inline ipa_t pktStore::dstAdr(int p) const 	{ return pd[p].dadr; }
inline uint8_t pktStore::optCode(int p) const 	{ return pd[p].optCode; }
inline uint8_t pktStore::optLeng(int p) const 	{ return pd[p].optLeng; }
inline uint8_t pktStore::lfsOp(int p) const 	{ return pd[p].lfsOp; }
inline uint8_t pktStore::lfsFlags(int p) const 	{ return pd[p].lfsFlags; }
inline int pktStore::lfsRrate(int p) const 	{ return pd[p].lfsRrate; }
inline int pktStore::lfsArate(int p) const 	{ return pd[p].lfsArate; }
inline int pktStore::lfsTrace(int p) const 	{ return pd[p].lfsTrace; }
inline int pktStore::inLink(int p) const 	{ return pd[p].inLnk; }
inline uint16_t pktStore::ioBytes(int p) const 	{ return pd[p].ioBytes; }

// Header field modification methods
inline void pktStore::setLeng(int p, uint16_t x) 	{ pd[p].lng = x; }
inline void pktStore::setHleng(int p, uint8_t x)  	{ pd[p].hlng = x; }
inline void pktStore::setProto(int p, uint8_t x)  	{ pd[p].proto = x; }
inline void pktStore::setSrcAdr(int p, fAdr_t x)	{ pd[p].sadr = x; }
inline void pktStore::setDstAdr(int p, fAdr_t x)	{ pd[p].dadr = x; }
inline void pktStore::setOptCode(int p, uint8_t x)	{ pd[p].optCode = x; }
inline void pktStore::setOptLeng(int p, uint8_t x)	{ pd[p].optLeng = x; }
inline void pktStore::setLfsOp(int p, uint8_t x)	{ pd[p].LfsOp = x; }
inline void pktStore::setLfsFlags(int p, uint8_t x)	{ pd[p].LfsFlags = x; }
inline void pktStore::setLfsRrate(int p, int x)		{ pd[p].LfsRrate = x; }
inline void pktStore::setLfsArate(int p, int x)		{ pd[p].LfsArate = x; }
inline void pktStore::setLfsTrace(int p, int x)		{ pd[p].LfsTrace = x; }
inline void pktStore::setInLink(int p, int x)		{ pd[p].inLnk = x; }
inline void pktStore::setIoBytes(int p,uint16_t x)	{ pd[p].ioBytes = x; }

#endif
