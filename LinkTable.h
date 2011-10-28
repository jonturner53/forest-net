/** @file LinkTable.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef LINKTABLE_H
#define LINKTABLE_H

#include <set>
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
	int	getAvailInBitRate(int) const;	
	int	getAvailInPktRate(int) const;	
	int	getAvailOutBitRate(int) const;	
	int	getAvailOutPktRate(int) const;	
	set<int>& getComtSet(int);

	// modifiers
	int	addEntry(int,ipa_t,ipp_t);
	void	removeEntry(int);		
	bool 	setPeerPort(int, ipp_t);	
	void 	setIface(int, int);	
	void	setPeerType(int, ntyp_t);	
	void	setPeerAdr(int, fAdr_t);	
	void	setBitRate(int, int);	
	void	setPktRate(int, int);	
	bool	setAvailInBitRate(int, int);	
	bool	setAvailInPktRate(int, int);	
	bool	setAvailOutBitRate(int, int);	
	bool	setAvailOutPktRate(int, int);	
	bool	addAvailInBitRate(int, int);	
	bool	addAvailInPktRate(int, int);	
	bool	addAvailOutBitRate(int, int);	
	bool	addAvailOutPktRate(int, int);	
	void	registerComt(int);
	void	deregisterComt(int);

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
	int	avInBitRate;		///< available input bit rate of link
	int	avInPktRate;		///< available input packet rate
	int	avOutBitRate;		///< available output bit rate
	int	avOutPktRate;		///< available output packet rate
	set<int> *comtSet;		///< set of comtrees containing link
	};
	LinkInfo *lnkTbl;		///< lnkTbl[i] is link info for link i
	UiSetPair *links;		///< sets for in-use and unused links
	UiHashTbl *ht;			///< hash table for fast lookup

	// helper functions
	uint64_t hashkey(ipa_t, ipp_t) const;
	int	readEntry(istream&);	 	
	void	writeEntry(ostream&, int) const;
};

/** Determine if a link number is valid.
 *  @param lnk is a link number
 *  @return true if a link has been defined with the specified
 *  link number, else false
 */
inline bool LinkTable::valid(int lnk) const { return links->isIn(lnk); }

/** Get the "first" link.
 *  This method is used to iterate through the links.
 *  The order of the links is arbitrary.
 *  @return the link number of the first link, or 0 if there are no links
 */
inline int LinkTable::firstLink() const {
	return links->firstIn();
}

/** Get the "next" link.
 *  This method is used to iterate through the links.
 *  The order of the links is arbitrary.
 *  @param lnk is a link number
 *  @return the link number of the link following lnk,
 *  or 0 if there are no more links
 */
inline int LinkTable::nextLink(int lnk) const {
	return links->nextIn(lnk);
}

/** Get the IP address of the peer for a given link.
 *  @param lnk is a valid link number
 *  @return the IP address for the far end of the link
 */
inline ipa_t LinkTable::getPeerIpAdr(int lnk) const {
	return lnkTbl[lnk].peerIp;
}

/** Get the port number of the peer for a given link.
 *  @param lnk is a valid link number
 *  @return the port number for the far end of the link
 */
inline ipp_t LinkTable::getPeerPort(int lnk) const {
	return lnkTbl[lnk].peerPort;
}

/** Get the interface number for a given link.
 *  @param lnk is a valid link number
 *  @return the number of the interface that the link is assigned to
 */
inline int LinkTable::getIface(int lnk) const {
	return lnkTbl[lnk].iface;
}

/** Get the node type of the peer for a given link.
 *  @param lnk is a valid link number
 *  @return the type of the node at the far end of the link
 */
inline ntyp_t LinkTable::getPeerType(int lnk) const {
	return lnkTbl[lnk].peerType;
}

/** Get the Forest address of the peer for a given link.
 *  @param lnk is a valid link number
 *  @return the Forest address for the far end of the link
 */
inline fAdr_t LinkTable::getPeerAdr(int lnk) const {
	return lnkTbl[lnk].peerAdr;
}

/** Get the bit rate of a given link.
 *  @param lnk is a valid link number
 *  @return the bit rate for the link
 */
inline int LinkTable::getBitRate(int lnk) const {
	return lnkTbl[lnk].bitRate;
}

/** Get the packet rate of a given link.
 *  @param lnk is a valid link number
 *  @return the packet rate for the link
 */
inline int LinkTable::getPktRate(int lnk) const {
	return lnkTbl[lnk].pktRate;
}

/** Get the available incoming bit rate of a given link.
 *  @param lnk is a valid link number
 *  @return the available incoming bit rate for the link
 */
inline int LinkTable::getAvailInBitRate(int lnk) const {
	return lnkTbl[lnk].avInBitRate;
}

/** Get the available incoming packet rate of a given link.
 *  @param lnk is a valid link number
 *  @return the available incoming packet rate for the link
 */
inline int LinkTable::getAvailInPktRate(int lnk) const {
	return lnkTbl[lnk].avInPktRate;
}

/** Get the available outcoming bit rate of a given link.
 *  @param lnk is a valid link number
 *  @return the available outcoming bit rate for the link
 */
inline int LinkTable::getAvailOutBitRate(int lnk) const {
	return lnkTbl[lnk].avOutBitRate;
}

/** Get the available outcoming bit rate of a given link.
 *  @param lnk is a valid link number
 *  @return the available outcoming bit rate for the link
 */
inline int LinkTable::getAvailOutPktRate(int lnk) const {
	return lnkTbl[lnk].avOutPktRate;
}

/** Get the set of comtrees registered for a link.
 *  This method is provided to allow the caller to efficiently
 *  examine the elements of a comtree set. It should not be
 *  used to modify the set.
 *  @param lnk is a valid link number
 *  @return a reference to the set of comtrees registered for this link
 */
inline set<int>& LinkTable::getComtSet(int lnk) {
	return *(lnkTbl[lnk].comtSet);
}

/** Set the interface number for a given link.
 *  @param lnk is a valid link number
 *  @param iface is an interface number
 */
inline void LinkTable::setIface(int lnk, int iface) {
	if (valid(lnk)) lnkTbl[lnk].iface = iface;
}

/** Set the type of the peer for a given link.
 *  @param lnk is a valid link number
 *  @param nt is a node type
 */
inline void LinkTable::setPeerType(int lnk, ntyp_t nt) {
	if (valid(lnk)) lnkTbl[lnk].peerType = nt;
}

/** Set the Forest address of the peer for a given link.
 *  @param lnk is a valid link number
 *  @param adr is a Forest unicast address
 */
inline void LinkTable::setPeerAdr(int lnk, fAdr_t adr) {
	if (valid(lnk)) lnkTbl[lnk].peerAdr =adr;
}

/** Set the bit rate for a given link.
 *  If the specified rate is outside the allowed range, it is set
 *  to the endpoint of the range.
 *  @param lnk is a valid link number
 *  @param br is the new bit rate
 */
inline void LinkTable::setBitRate(int lnk, int br) {
	if (valid(lnk))
		lnkTbl[lnk].bitRate = min(Forest::MAXBITRATE,
				      max(Forest::MINBITRATE,br));
}

/** Set the packet rate for a given link.
 *  If the specified rate is outside the allowed range, it is set
 *  to the endpoint of the range.
 *  @param lnk is a valid link number
 *  @param pr is the new packet rate
 */
inline void LinkTable::setPktRate(int lnk, int pr) {
	if (valid(lnk))
		lnkTbl[lnk].pktRate = min(Forest::MAXPKTRATE,
				      max(Forest::MINPKTRATE,pr));
}

/** Set the available incoming bit rate for a link.
 *  @param lnk is a valid link number
 *  @param br is the new available bit rate in the input direction
 *  @return true on success, false on failure
 */
inline bool LinkTable::setAvailInBitRate(int lnk, int br) {
	if (!valid(lnk) || br > lnkTbl[lnk].bitRate) return false;
	lnkTbl[lnk].avInBitRate = max(0,br);
	return true;
}

/** Set the available incoming packet rate for a link.
 *  @param lnk is a valid link number
 *  @param br is the new available packet rate in the input direction
 *  @return true on success, false on failure
 */
inline bool LinkTable::setAvailInPktRate(int lnk, int pr) {
	if (!valid(lnk) || pr > lnkTbl[lnk].pktRate) return false;
	lnkTbl[lnk].avInPktRate = max(0,pr);
	return true;
}

/** Set the available outgoing bit rate for a link.
 *  @param lnk is a valid link number
 *  @param br is the new available bit rate in the output direction
 *  @return true on success, false on failure
 */
inline bool LinkTable::setAvailOutBitRate(int lnk, int br) {
	if (!valid(lnk) || br > lnkTbl[lnk].bitRate) return false;
	lnkTbl[lnk].avOutBitRate = max(0,br);
	return true;
}

/** Set the available outgoing packet rate for a link.
 *  @param lnk is a valid link number
 *  @param pr is the new available packet rate in the output direction
 *  @return true on success, false on failure
 */
inline bool LinkTable::setAvailOutPktRate(int lnk, int pr) {
	if (!valid(lnk) || pr > lnkTbl[lnk].pktRate) return false;
	lnkTbl[lnk].avOutPktRate = max(0,pr);
	return true;
}

/** Add to the available incoming bit rate for a link.
 *  @param lnk is a valid link number
 *  @param br is the amount to be added to the available bit rate
 *  in the input direction
 *  @return true on success, false on failure
 */
inline bool LinkTable::addAvailInBitRate(int lnk, int br) {
	if (!valid(lnk)) return false;
	int s = br + lnkTbl[lnk].avInBitRate;
	if (s < 0 || s > lnkTbl[lnk].bitRate) return false;
	lnkTbl[lnk].avInBitRate = s;
	return true;
}

/** Add to the available incoming packet rate for a link.
 *  @param lnk is a valid link number
 *  @param pr is the amount to be added to the available packet rate
 *  in the output direction
 *  @return true on success, false on failure
 */
inline bool LinkTable::addAvailInPktRate(int lnk, int pr) {
	if (!valid(lnk)) return false;
	int s = pr + lnkTbl[lnk].avInPktRate;
	if (s < 0 || s > lnkTbl[lnk].pktRate) return false;
	lnkTbl[lnk].avInPktRate = s;
	return true;
}

/** Add to the available outgoing bit rate for a link.
 *  @param lnk is a valid link number
 *  @param br is the amount to be added to the available bit rate
 *  in the input direction
 *  @return true on success, false on failure
 */
inline bool LinkTable::addAvailOutBitRate(int lnk, int br) {
	if (!valid(lnk)) return false;
	int s = br + lnkTbl[lnk].avOutBitRate;
	if (s < 0 || s > lnkTbl[lnk].bitRate) return false;
	lnkTbl[lnk].avOutBitRate = s;
	return true;
}

/** Add to the available outgoing packet rate for a link.
 *  @param lnk is a valid link number
 *  @param br is the amount to be added to the available packet rate
 *  in the output direction
 *  @return true on success, false on failure
 */
inline bool LinkTable::addAvailOutPktRate(int lnk, int pr) {
	if (!valid(lnk)) return false;
	int s = pr + lnkTbl[lnk].avOutPktRate;
	if (s < 0 || s > lnkTbl[lnk].pktRate) return false;
	lnkTbl[lnk].avOutPktRate = s;
	return true;
}

inline bool LinkTable::registerComt(int lnk) {
	if (!valid(lnk)) return false;
	lnkTbl[lnk].comtSet->insert(lnk);
}

inline bool LinkTable::deregisterComt(int lnk) {
	if (!valid(lnk)) return false;
	lnkTbl[lnk].comtSet->erase(lnk);
}

/** Compute key for hash lookup.
 *  @param ipa is an IP address
 *  @param ipp is an IP port number
 */
inline uint64_t LinkTable::hashkey(ipa_t ipa, ipp_t ipp) const {
	return (((uint64_t) ipa) << 32) | ((uint64_t) ipp);
}

/** Get the link number on which a packet arrived.
 *  @param ipa is an IP address
 *  @param ipp is an IP port number
 *  @return the number of the link that matches the given IP address
 *  and port, or 0 if there is no matching link
 */
inline int LinkTable::lookup(ipa_t ipa, ipp_t ipp) const {
        return ht->lookup(hashkey(ipa,ipp));
}

#endif
