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
	int	getIface(int) const;	
	ipa_t	getPeerIpAdr(int) const;	
	ipp_t 	getPeerPort(int) const;	
	ntyp_t	getPeerType(int) const;	
	fAdr_t 	getPeerAdr(int) const;	
	int	getBitRate(int) const;	
	int	getPktRate(int) const;	
	int	getNsPerByte(int) const;	
	int	getMinDelta(int) const;	
	int	getFreeLink() const;

	// modifiers
	bool	addEntry(int,int,ntyp_t,ipa_t,fAdr_t);
	void	removeEntry(int);		
	void 	setPeerPort(int, ipp_t);	
	void	setPeerType(int, ntyp_t);	
	void	setBitRate(int, int);	
	void	setPktRate(int, int);	

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

/** getters */
inline int LinkTable::getIface(int i) const { return lnkTbl[i].iface; }
inline ipa_t LinkTable::getPeerIpAdr(int i) const { return lnkTbl[i].peerIp; }
inline ipp_t LinkTable::getPeerPort(int i) const { return lnkTbl[i].peerPort; }
inline ntyp_t LinkTable::getPeerType(int i) const { return lnkTbl[i].peerType; }
inline fAdr_t LinkTable::getPeerAdr(int i) const { return lnkTbl[i].peerAdr; }
inline int LinkTable::getBitRate(int i) const { return lnkTbl[i].bitRate; }
inline int LinkTable::getPktRate(int i) const { return lnkTbl[i].pktRate; }
inline int LinkTable::getFreeLink() const { return links->firstOut(); }

/** setters */
inline void LinkTable::setPeerPort(int i, ipp_t pp) { lnkTbl[i].peerPort = pp; }
inline void LinkTable::setPeerType(int i, ntyp_t nt) { lnkTbl[i].peerType = nt; }
inline void LinkTable::setBitRate(int i, int br) { lnkTbl[i].bitRate = br; }
inline void LinkTable::setPktRate(int i, int pr) { lnkTbl[i].pktRate = pr; }


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
	bool rtrLink = (ipp == Forest::ROUTER_PORT);
        int lnk = ht->lookup(hashkey(ipa,ipp));
	if (lnk == 0) return 0;
	if (( rtrLink && lnkTbl[lnk].peerType != ROUTER) ||
	    (!rtrLink && lnkTbl[lnk].peerType == ROUTER))
		return 0;
	return lnk;
}

#endif
