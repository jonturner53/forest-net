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
#include "Forest.h"
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
	RateSpec& getRates(int) const;	
	RateSpec& getAvailRates(int) const;	
	set<int>& getComtSet(int) const;

	// modifiers
	int	addEntry(int,ipa_t,ipp_t);
	void	removeEntry(int);		
	bool 	setPeerPort(int, ipp_t);	
	void 	setIface(int, int);	
	void	setPeerType(int, ntyp_t);	
	void	setPeerAdr(int, fAdr_t);	
	bool	registerComt(int,int);
	bool	deregisterComt(int,int);

	// io routines
	bool read(istream&);
	string& toString(string&) const;

private:
	int	maxLnk;			///< maximum link number

	struct LinkInfo {		///< information about a link
	int	iface;			///< interface number for link
	ipa_t	peerIp;			///< IP address of peer endpoint
	ipp_t	peerPort;		///< peer port number
	ntyp_t	peerType;		///< node type of peer
	fAdr_t	peerAdr;		///< peer's forest address
	RateSpec rates;			///< rate spec for link rates
	RateSpec availRates;		///< rate spec for available rates
	set<int> *comtSet;		///< set of comtrees containing link
	};
	LinkInfo *lnkTbl;		///< lnkTbl[i] is link info for link i
	UiSetPair *links;		///< sets for in-use and unused links
	UiHashTbl *ht;			///< hash table for fast lookup

	// helper functions
	uint64_t hashkey(ipa_t, ipp_t) const;
	int	readEntry(istream&);	 	
	string&	entry2string(int, string&) const;
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

/** Get the rates for a given link.
 *  @param lnk is a valid link number
 *  @return a reference to the rate spec for the link;
 *  currently links are symmetric and up and down fields must
 *  be set to identical values; treat "up" as input, "down" as output
 */
inline RateSpec& LinkTable::getRates(int lnk) const {
	return lnkTbl[lnk].rates;
}

/** Get the available rates for a given link.
 *  @param lnk is a valid link number
 *  @return a reference to the available rate spec for the link;
 *  the "up" fields correspond to input, "down" fields to output
 */
inline RateSpec& LinkTable::getAvailRates(int lnk) const {
	return lnkTbl[lnk].availRates;
}

/** Get the set of comtrees registered for a link.
 *  This method is provided to allow the caller to efficiently
 *  examine the elements of a comtree set. It should not be
 *  used to modify the set.
 *  @param lnk is a valid link number
 *  @return a reference to the set of comtrees registered for this link
 */
inline set<int>& LinkTable::getComtSet(int lnk) const {
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

inline bool LinkTable::registerComt(int lnk, int ctx) {
	if (!valid(lnk)) return false;
	lnkTbl[lnk].comtSet->insert(ctx);
	return true;
}

inline bool LinkTable::deregisterComt(int lnk, int ctx) {
	if (!valid(lnk)) return false;
	lnkTbl[lnk].comtSet->erase(ctx);
	return true;
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
