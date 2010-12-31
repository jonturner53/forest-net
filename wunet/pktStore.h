// Header file for pktStore class.
// 
// Maintains a set of packets with selected header fields and a
// separate set of buffers. Each packet is associated with some
// buffer, but a buffer may be associated with several packets
// (to support multicast).

#ifndef PKTSTORE_H
#define PKTSTORE_H

#include "wunet.h"
#include "list.h"

struct pktData {
	uint16_t lng;			// length field from wunet header
	ptyp_t 	typ;			// type field from wunet header
	vnet_t	vnet;			// vnet identifier
	wuAdr_t	sadr;			// source address
	wuAdr_t	dadr;			// destination address
	int	inLnk;			// link on which packet arrived
	uint16_t ioBytes;		// number of bytes received
	int	buf;			// buff[pd[p].buf] = buffer # for pkt p
};					//          0 for unused buffers

class pktStore {
public:
		pktStore(int=100000, int=50000);
		~pktStore();
	int	alloc();		// return index to new packet
	void	free(int);		// return packet to free list
	int	clone(int);		// create packet copy, sharing buffer
	buffer_t *buffer(int) const;	// return pointer to packet's buffer

	void	unpack(int);		// unpack header fields from buffer
	void	pack(int);		// pack header fields into buffer

	uint16_t leng(int) const;	// return length field
	ptyp_t  ptyp(int) const;	// return packet type field
	vnet_t	vnet(int) const;	// return vnet field
	wuAdr_t	srcAdr(int) const;	// return source address field
	wuAdr_t	dstAdr(int) const;	// return destination address field
	int	inLink(int) const;	// return link on which packet arrived
	uint16_t ioBytes(int) const;	// return number of bytes received

	void	setLeng(int, uint16_t);	// set length field
	void	setPtyp(int, ptyp_t);	// set packet type field
	void	setVnet(int, vnet_t);	// set vnet field
	void	setSrcAdr(int, wuAdr_t);// set source address field
	void	setDstAdr(int, wuAdr_t);// set destination address field
	void	setInLink(int, int); 	// set input link number
	void	setIoBytes(int, uint16_t); // set # of bytes received

	void	print(ostream&, int) const; // print packet for debugging
private:
	int	N;			// number of packets we have room for
	int	M;			// number of buffers we have room for
	int	n;			// number of packets in use
	int	m;			// number of buffers in use

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

// Header field access methods
inline uint16_t pktStore::leng(int p) const 		{ return pd[p].lng; }
inline ptyp_t pktStore::ptyp(int p) const 		{ return pd[p].typ; }
inline vnet_t pktStore::vnet(int p) const 		{ return pd[p].vnet; }
inline wuAdr_t pktStore::srcAdr(int p) const 		{ return pd[p].sadr; }
inline wuAdr_t pktStore::dstAdr(int p) const 		{ return pd[p].dadr; }
inline int pktStore::inLink(int p) const 		{ return pd[p].inLnk; }
inline uint16_t pktStore::ioBytes(int p) const 		{ return pd[p].ioBytes; }

// Header field modification methods
inline void pktStore::setLeng(int p, uint16_t x)  	{ pd[p].lng = x; }
inline void pktStore::setPtyp(int p, ptyp_t x)  	{ pd[p].typ = x; }
inline void pktStore::setVnet(int p, vnet_t x)		{ pd[p].vnet = x; }
inline void pktStore::setSrcAdr(int p, wuAdr_t x)	{ pd[p].sadr = x; }
inline void pktStore::setDstAdr(int p, wuAdr_t x)	{ pd[p].dadr = x; }
inline void pktStore::setInLink(int p, int x)	{ pd[p].inLnk = x; }
inline void pktStore::setIoBytes(int p,uint16_t x)	{ pd[p].ioBytes = x; }

#endif
