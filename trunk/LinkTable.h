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
#include "PacketHeader.h"
#include "UiSetPair.h"
#include "UiHashTbl.h"

/** Maintains information about a Forest router's virtual links.
 */
class LinkTable {
public:
		LinkTable(int);
		~LinkTable();

	// predicates
	bool	valid(int) const;
	bool	checkEntry(int);

	// iteration methods
	int	firstLink() const;
	int	nextLink(int) const;

	// access methods
	int 	lookup(ipa_t, ipp_t) const;
	ipa_t	getPeerIpAdr(int) const;	
	ipp_t 	getPeerPort(int) const;	
	int	getIface(int) const;	
	ntyp_t	getPeerType(int) const;	
	fAdr_t 	getPeerAdr(int) const;	
	int	getBitRate(int) const;	
	int	getPktRate(int) const;	
	int	getAvailBitRate(int) const;	
	int	getAvailPktRate(int) const;	

	// modifiers
	int	addEntry(int,ipa_t,ipp_t);
	void	removeEntry(int);		
	bool 	setPeerPort(int, ipp_t);	
	void 	setIface(int, int);	
	void	setPeerType(int, ntyp_t);	
	void	setPeerAdr(int, fAdr_t);	
	void	setBitRate(int, int);	
	void	setPktRate(int, int);	
	bool	setAvailBitRate(int, int);	
	bool	setAvailPktRate(int, int);	
	bool	addAvailBitRate(int, int);	
	bool	addAvailPktRate(int, int);	

	// io routines
	bool read(istream&);
	void write(ostream&) const;

private:
	int	maxLnk;			///< maximum link number

	struct LinkInfo {		///< information about a link
	int	iface;			///< interface number for link
	ipa_t	peerIp;			///< IP address of peer endpoint
	ipp_t	peerPort;		///< peer port number
	ntyp_t	peerType;		///< node type of peer
	fAdr_t	peerAdr;		///< peer's forest address
	int	bitRate;		///< max bit rate of link (MAC level)
	int	pktRate;		///< maximum packet rate of link
	int	avBitRate;		///< available bit rate of link
	int	avPktRate;		///< available packet rate of link
	};
	LinkInfo *lnkTbl;		///< lnkTbl[i] is link info for link i
	UiSetPair *links;		///< sets for in-use and unused links
	UiHashTbl *ht;			///< hash table for fast lookup

	// helper functions
	uint64_t hashkey(ipa_t, ipp_t) const;
	int	readEntry(istream&);	 	
	void	writeEntry(ostream&, int) const;
};

inline bool LinkTable::valid(int i) const { return links->isIn(i); }

inline int LinkTable::firstLink() const {
	return links->firstIn();
}

inline int LinkTable::nextLink(int lnk) const {
	return links->nextIn(lnk);
}

// getters
inline ipa_t LinkTable::getPeerIpAdr(int i) const { return lnkTbl[i].peerIp; }
inline ipp_t LinkTable::getPeerPort(int i) const { return lnkTbl[i].peerPort; }
inline int LinkTable::getIface(int i) const { return lnkTbl[i].iface; }
inline ntyp_t LinkTable::getPeerType(int i) const { return lnkTbl[i].peerType; }
inline fAdr_t LinkTable::getPeerAdr(int i) const { return lnkTbl[i].peerAdr; }
inline int LinkTable::getBitRate(int i) const { return lnkTbl[i].bitRate; }
inline int LinkTable::getPktRate(int i) const { return lnkTbl[i].pktRate; }
inline int LinkTable::getAvailBitRate(int i) const {
	return lnkTbl[i].avBitRate;
}
inline int LinkTable::getAvailPktRate(int i) const {
	return lnkTbl[i].avPktRate;
}

// setters
inline void LinkTable::setIface(int i, int iface) { lnkTbl[i].iface = iface; }
inline void LinkTable::setPeerType(int i, ntyp_t nt) { lnkTbl[i].peerType =nt; }
inline void LinkTable::setPeerAdr(int i, fAdr_t adr) { lnkTbl[i].peerAdr =adr; }
inline void LinkTable::setBitRate(int i, int br) { lnkTbl[i].bitRate = br; }
inline void LinkTable::setPktRate(int i, int pr) { lnkTbl[i].pktRate = pr; }
inline bool LinkTable::setAvailBitRate(int i, int br) {
	if (br > lnkTbl[i].bitRate) return false;
	lnkTbl[i].avBitRate = max(0,br);
	return true;
}
inline bool LinkTable::setAvailPktRate(int i, int pr) {
	if (pr > lnkTbl[i].pktRate) return false;
	lnkTbl[i].avPktRate = max(0,pr);
	return true;
}
inline bool LinkTable::addAvailBitRate(int i, int br) {
	int s = br + lnkTbl[i].avBitRate;
	if (s > lnkTbl[i].bitRate) return false;
	lnkTbl[i].avBitRate = max(0,s);
	return true;
}
inline bool LinkTable::addAvailPktRate(int i, int pr) {
	int s = pr + lnkTbl[i].avPktRate;
	if (s > lnkTbl[i].pktRate) return false;
	lnkTbl[i].avPktRate = max(0,s);
	return true;
}


/** Compute key for hash lookup
 */
inline uint64_t LinkTable::hashkey(ipa_t ipa, ipp_t ipp) const {
	return (((uint64_t) ipa) << 32) | ((uint64_t) ipp);
}

/** Get the link number on which a packet arrived.
 *  @param h is a reference to a PacketHeader for an incoming packet
 *  @return the link number that matches h or 0 if none match
 */
inline int LinkTable::lookup(ipa_t ipa, ipp_t ipp) const {
        return ht->lookup(hashkey(ipa,ipp));
}

#endif
