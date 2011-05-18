// Header file for lnkTbl class, which stores information
// about all the links incident to a given router.
//

#ifndef LNKTBL_H
#define LNKTBL_H

#include "forest.h"
#include "hashTbl.h"

class lnkTbl {
public:
		lnkTbl(int=31);
		~lnkTbl();
	int	lookup(int,ipa_t,ipp_t,fAdr_t);	// retrieve matching entry
	bool	addEntry(int,int,ntyp_t,ipa_t,fAdr_t);	// create new entry
	bool	removeEntry(int);		// remove entry from table
	bool	checkEntry(int);		// perform consistency check
	bool	valid(int) const;		// return true for valid entries
	void	enable(int);			// make table entry valid
	void	disable(int);			// make table entry invalid
	// routines for accessing/changing various vields
	// note that peer's IP address and forest address may not
	// be changed, as hash access depends on these
	int	interface(int) const;	
	ipa_t	peerIpAdr(int) const;	
	ipp_t& 	peerPort(int);	
	ntyp_t&	peerTyp(int);	
	fAdr_t 	peerAdr(int) const;	
	fAdr_t&	peerDest(int);	
	int&	bitRate(int);	
	int&	pktRate(int);	
	int	minDelta(int) const;	
	// statistics routines
	uint32_t iPktCnt(int) const;
	uint32_t oPktCnt(int) const;
	uint32_t iBytCnt(int) const;
	uint32_t oBytCnt(int) const;
	void	postIcnt(int, int);
	void	postOcnt(int, int);
	// io routines
	friend	bool operator>>(istream&, lnkTbl&);
	friend	ostream& operator<<(ostream&, const lnkTbl&);
private:
	int	nlnk;			// max number of links in table

	// router level statistics counters
	uint32_t iPkt;			// count of input packets
	uint32_t oPkt;			// count of output packets
	uint64_t iByt;			// count of input bytes
	uint64_t oByt;			// count of output bytes
	uint32_t irPkt;			// count of packets from other routers
	uint32_t orPkt;			// count of packets to other routers
	uint32_t icPkt;			// count of packets from clients
	uint32_t ocPkt;			// count of packets to clients

	struct lnkdata {
	int	intf;			// interface number for link
	ipa_t	pipa;			// IP address of peer endpoint
	ipp_t	pipp;			// peer port number
	ntyp_t	ptyp;			// node type of peer
	fAdr_t	padr;			// peer's forest address
	fAdr_t	dadr;			// peer's allowed destination address
	int	bitrate;		// maximum bit rate of link (MAC level)
	int	pktrate;		// maximum packet rate of link
	int	mindelta;		// minimum time between packets (us)
	uint32_t iPkt;			// input packet counter
	uint32_t oPkt;			// output packet counter
	uint64_t iByt;			// input byte counter
	uint64_t oByt;			// output byte counter
	};
	lnkdata	*ld;			// ld[i] is link data for link i
	hashTbl *ht;			// hash table for fast lookup

	// helper functions
	uint64_t hashkey(ipa_t,uint32_t) const;
	int	getEntry(istream&);	 	
	void	putEntry(ostream&, int) const;
};

inline bool lnkTbl::valid(int i) const { return (ld[i].padr != 0); }
inline void lnkTbl::enable(int i) { if (ld[i].padr == 0) ld[i].padr++; }
inline void lnkTbl::disable(int i) { ld[i].padr = 0; }

// Routines to access various fields in lnkTbl entries
inline int lnkTbl::interface(int i) const 	{ return ld[i].intf; }
inline ipa_t lnkTbl::peerIpAdr(int i) const 	{ return ld[i].pipa; }
inline ipp_t& lnkTbl::peerPort(int i) 		{ return ld[i].pipp; }
inline ntyp_t& lnkTbl::peerTyp(int i) 	 	{ return ld[i].ptyp; }
inline fAdr_t lnkTbl::peerAdr(int i) const 	{ return ld[i].padr; }
inline fAdr_t& lnkTbl::peerDest(int i) 	 	{ return ld[i].dadr; }
inline int& lnkTbl::bitRate(int i) 		{ return ld[i].bitrate; }
inline int& lnkTbl::pktRate(int i) 		{ return ld[i].pktrate; }
inline int lnkTbl::minDelta(int i) const 	{
	return int(1000000.0/ld[i].pktrate);
}

// statistics routines
inline uint32_t lnkTbl::iPktCnt(int i) const {
	return (i > 0 ? ld[i].iPkt :
		(i == 0 ? iPkt :
		 (i == -1 ? irPkt : icPkt)));
}
inline uint32_t lnkTbl::oPktCnt(int i) const {
	return (i > 0 ? ld[i].oPkt :
		(i == 0 ? oPkt :
		 (i == -1 ? orPkt : ocPkt)));
}
inline uint32_t lnkTbl::iBytCnt(int i) const {
	return (i > 0 ? ld[i].iByt : iByt);
}
inline uint32_t lnkTbl::oBytCnt(int i) const {
	return (i > 0 ? ld[i].oByt : oByt);
}
inline void lnkTbl::postIcnt(int i, int leng) {
	int len = forest::truPktLeng(leng);
	ld[i].iPkt++; ld[i].iByt += len;
	iPkt++; iByt += len;
	if (ld[i].ptyp == ROUTER) irPkt++;
	if (ld[i].ptyp == CLIENT) icPkt++;
}
inline void lnkTbl::postOcnt(int i, int leng) {
	int len = forest::truPktLeng(leng);
	ld[i].oPkt++; ld[i].oByt += len;
	oPkt++; oByt += len;
	if (ld[i].ptyp == ROUTER) orPkt++;
	if (ld[i].ptyp == CLIENT) { ocPkt++;
	}
}

// Compute key for hash lookup
inline uint64_t lnkTbl::hashkey(ipa_t x, uint32_t y) const {
	return (uint64_t(x) << 32) | y;
}

// Return index of link for a packet received on specified interface,
// with specified source IP (address,port) pair and the given
// Forest source address. If no match, return Null.
inline int lnkTbl::lookup(int intf, ipa_t pipa, ipp_t pipp, fAdr_t srcAdr) {
        ipa_t x = (pipp != FOREST_PORT ? srcAdr : pipa);
        int te = ht->lookup(hashkey(pipa,x));
        if (te !=0 && intf == interface(te) &&
            (pipp == peerPort(te) || peerPort(te) == 0))
                return te;
        return 0;
}

#endif
