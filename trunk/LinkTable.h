/** @file LinkTable.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef LINKTABLE_H
#define LINKTABLE_H

#include "CommonDefs.h"
#include "UiHashTbl.h"

/** Maintains information about a Forest router's virtual links.
 */
class LinkTable {
public:
		LinkTable(int=31);
		~LinkTable();

	/** access methods */
	int 	lookup(uint32_t, bool);
	int	getInterface(int) const;	
	ipa_t	getPeerIpAdr(int) const;	
	ipp_t 	getPeerPort(int) const;	
	ntyp_t	getPeerType(int) const;	
	fAdr_t 	getPeerAdr(int) const;	
	fAdr_t	getPeerDest(int) const;	
	int	getBitRate(int) const;	
	int	getPktRate(int) const;	
	int	getMinDelta(int) const;	

	/** predicates */
	bool	valid(int) const;
	bool	checkEntry(int);

	/** modifiers */
	int	alloc();
	bool	addEntry(int,int,ntyp_t,ipa_t,fAdr_t);
	bool	removeEntry(int);		
	void	enable(int);		
	void	disable(int);
	void 	setPeerPort(int, ipp_t);	
	void	setPeerType(int, ntyp_t);	
	void	setPeerDest(int, fAdr_t);	
	void	setBitRate(int, int);	
	void	setPktRate(int, int);	

	/** statistics routines */
	uint32_t iPktCnt(int) const;
	uint32_t oPktCnt(int) const;
	uint32_t iBytCnt(int) const;
	uint32_t oBytCnt(int) const;
	void	postIcnt(int, int);
	void	postOcnt(int, int);

	/** io routines */
	bool read(istream&);
	void write(ostream&) const;

private:
	int	nlnk;			///< max number of links in table

	// router level statistics counters
	uint32_t iPkt;			///< count of input packets
	uint32_t oPkt;			///< count of output packets
	uint64_t iByt;			///< count of input bytes
	uint64_t oByt;			///< count of output bytes
	uint32_t irPkt;			///< count of packets from other routers
	uint32_t orPkt;			///< count of packets to other routers
	uint32_t icPkt;			///< count of packets from clients
	uint32_t ocPkt;			///< count of packets to clients

	struct lnkdata {
	int	intf;			///< interface number for link
	ipa_t	pipa;			///< IP address of peer endpoint
	ipp_t	pipp;			///< peer port number
	ntyp_t	ptyp;			///< node type of peer
	fAdr_t	padr;			///< peer's forest address
	fAdr_t	dadr;			///< peer's allowed destination address
	int	bitrate;		///< max bit rate of link (MAC level)
	int	pktrate;		///< maximum packet rate of link
	int	mindelta;		///< minimum time between packets (us)
	uint32_t iPkt;			///< input packet counter
	uint32_t oPkt;			///< output packet counter
	uint64_t iByt;			///< input byte counter
	uint64_t oByt;			///< output byte counter
	};
	lnkdata	*ld;			///< ld[i] is link data for link i
	UiHashTbl *ht;			///< hash table for fast lookup

	// helper functions
	uint64_t hashkey(uint32_t,bool) const;
	int	readEntry(istream&);	 	
	void	writeEntry(ostream&, int) const;
};

inline bool LinkTable::valid(int i) const { return (ld[i].padr != 0); }
inline void LinkTable::enable(int i) { if (ld[i].padr == 0) ld[i].padr++; }
inline void LinkTable::disable(int i) { ld[i].padr = 0; }

/** getters */
inline int LinkTable::getInterface(int i) const { return ld[i].intf; }
inline ipa_t LinkTable::getPeerIpAdr(int i) const { return ld[i].pipa; }
inline ipp_t LinkTable::getPeerPort(int i) const { return ld[i].pipp; }
inline ntyp_t LinkTable::getPeerType(int i) const { return ld[i].ptyp; }
inline fAdr_t LinkTable::getPeerAdr(int i) const { return ld[i].padr; }
inline fAdr_t LinkTable::getPeerDest(int i) const { return ld[i].dadr; }
inline int LinkTable::getBitRate(int i) const { return ld[i].bitrate; }
inline int LinkTable::getPktRate(int i) const { return ld[i].pktrate; }
inline int LinkTable::getMinDelta(int i) const 	{
	return int(1000000.0/ld[i].pktrate);
}

/** setters */
inline void LinkTable::setPeerPort(int i, ipp_t pp) { ld[i].pipp = pp; }
inline void LinkTable::setPeerType(int i, ntyp_t nt) { ld[i].ptyp = nt; }
inline void LinkTable::setPeerDest(int i, fAdr_t pd) { ld[i].dadr = pd; }
inline void LinkTable::setBitRate(int i, int br) { ld[i].bitrate = br; }
inline void LinkTable::setPktRate(int i, int pr) { ld[i].pktrate = pr; }

/** statistics routines */
inline uint32_t LinkTable::iPktCnt(int i) const {
	return (i > 0 ? ld[i].iPkt :
		(i == 0 ? iPkt :
		 (i == -1 ? irPkt : icPkt)));
}
inline uint32_t LinkTable::oPktCnt(int i) const {
	return (i > 0 ? ld[i].oPkt :
		(i == 0 ? oPkt :
		 (i == -1 ? orPkt : ocPkt)));
}
inline uint32_t LinkTable::iBytCnt(int i) const {
	return (i > 0 ? ld[i].iByt : iByt);
}
inline uint32_t LinkTable::oBytCnt(int i) const {
	return (i > 0 ? ld[i].oByt : oByt);
}
inline void LinkTable::postIcnt(int i, int leng) {
	int len = Forest::truPktLeng(leng);
	ld[i].iPkt++; ld[i].iByt += len;
	iPkt++; iByt += len;
	if (ld[i].ptyp == ROUTER) irPkt++;
	if (ld[i].ptyp == CLIENT) icPkt++;
}
inline void LinkTable::postOcnt(int i, int leng) {
	int len = Forest::truPktLeng(leng);
	ld[i].oPkt++; ld[i].oByt += len;
	oPkt++; oByt += len;
	if (ld[i].ptyp == ROUTER) orPkt++;
	if (ld[i].ptyp == CLIENT) { ocPkt++;
	}
}

/** Compute key for hash lookup
 */
inline uint64_t LinkTable::hashkey(uint32_t x, bool leafLink) const {
	return ((leafLink ? 0x12345678ull : 0x89ABCDEFull) << 32) | x;
}

/** Get the link number associated with a given peer address.
 *  @param leafLink is true if the peer is a leaf (not a router);
 *  if leafLink is false, the peer is assumed to be a router
 *  @param x if leafLink is true, x is the forest address of the
 *  peer, if leafLink is false, x is the IP address of the peer
 *  @return the matching link number or 0 if no match
 */
inline int LinkTable::lookup(uint32_t x, bool leafLink) {
        int lnk = ht->lookup(hashkey(x,leafLink));
	if (lnk == 0) return 0;
	if ((leafLink && ld[lnk].ptyp == ROUTER) ||
	    (!leafLink && ld[lnk].ptyp != ROUTER))
		return 0;
	return lnk;
}


#endif
