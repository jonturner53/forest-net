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
#include "Packet.h"
#include "RateSpec.h"
#include "StatCounts.h"
#include "Hash.h"
#include "HashSet.h"
#include "HashMap.h"

namespace forest {

/** Maintains information about a Forest router's virtual links.
 */
class LinkTable {
public:
	/// link table entry
	class Entry {		
	public:
	int	iface;			///< interface number for link
	ipa_t	peerIp;			///< IP address of peer endpoint
	ipp_t	peerPort;		///< peer port number
	sockaddr_in sa;			///< socket address of peer
	Forest::ntyp_t peerType;	///< node type of peer
	fAdr_t	peerAdr;		///< peer's forest address
	bool	isConnected;		///< true if link is connected
	uint64_t nonce;			///< used by peer when connecting
	RateSpec rates;			///< rate spec for link rates
	RateSpec availRates;		///< rate spec for available rates
	StatCounts stats;		///< rate statistics for link

		Entry();
		Entry(const Entry&);

	string	toString() const;
	friend	ostream& operator<<(ostream& out, const Entry& a) {
                return out << a.toString();
        }
	};

	/////////////////////////////////////////
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
	int 	lookup(uint64_t) const;
	int	lookup(fAdr_t) const;

	Entry&	getEntry(int) const;
	int	maxLink();

	// modifiers
	void	setPeerAdr(int, fAdr_t);
	int	addEntry(int,ipa_t,ipp_t,uint64_t);
	bool	remapEntry(int,ipa_t,ipp_t);
	bool	remapEntry(int, ipa_t, ipp_t, uint64_t); 
	bool	revertEntry(int);
	bool	removeEntry(int);		
	bool	connect(int, ipa_t, ipp_t);
	void	countIncoming(int lnk, int leng) {
		Entry& e = getEntry(lnk);
		e.stats.updateIn(leng);
		if (e.peerType == Forest::ROUTER)
			rtrStats.updateIn(leng);
		else
			leafStats.updateIn(leng);
	}
	void	countOutgoing(int lnk, int leng) {
		Entry& e = getEntry(lnk);
		e.stats.updateOut(leng);
		if (e.peerType == Forest::ROUTER)
			rtrStats.updateOut(leng);
		else
			leafStats.updateOut(leng);
	}
	void	getStats(int, StatCounts&) const;
	void	getStats(StatCounts&, StatCounts&) const;

	// io routines
	bool read(istream&);
	string	link2string(int) const;
	string	toString() const;

	// packing entry in a packet
	char*	pack(int, char*) const;
	char*	unpack(int, char*);

private:
	int	maxLnk;			///< maximum link number
	StatCounts rtrStats;		///< rates to/from other routers
	StatCounts leafStats;		///< rates to/from leaf nodes

	/// map from remote peer's (ip,port) pair to entry
	HashMap<uint64_t,Entry,Hash::u64> *map;
	/// extra map from peer addr to link #
	HashSet<fAdr_t,Hash::s32> *padrMap;

	// helper functions
	uint64_t hashkey(ipa_t, ipp_t) const;
	int	readEntry(istream&);	 	
};

inline LinkTable::Entry::Entry() {
	iface  = 0;
	peerIp = 0; peerPort = 0; peerType = Forest::UNDEF_NODE; peerAdr = 0;
	isConnected = false; nonce = 0;
}
	
inline LinkTable::Entry::Entry(const Entry& e) {
	iface  = e.iface;
	peerIp = e.peerIp; peerPort = e.peerPort;
	peerType = e.peerType; peerAdr = e.peerAdr;
	isConnected = e.isConnected; nonce = e.nonce;
	rates = e.rates; availRates = e.availRates;
}

inline string LinkTable::Entry::toString() const {
	stringstream ss;
	ss << setw(6) << iface << "  " << setw(12) << Np4d::ip2string(peerIp)
           << ":" << setw(5) << left << peerPort << "  "
           << setw(10) << left << Forest::nodeType2string(peerType)
           << " " << setw(10) << left <<Forest::fAdr2string(peerAdr)
           << " " << rates.toString() << " " << nonce;
	return ss.str();
}

/** Get the largest link number.
 *  @return the largest link number that can be used
 */
inline int LinkTable::maxLink() { return maxLnk; }

/** Determine if a link number is valid.
 *  @param lnk is a link number
 *  @return true if a link has been defined with the specified
 *  link number, else false
 */
inline bool LinkTable::valid(int lnk) const { return map->valid(lnk); }


/** Get the "first" link.
 *  This method is used to iterate through the links.
 *  The order of the links is arbitrary.
 *  @return the link number of the first link, or 0 if there are no links
 */
inline int LinkTable::firstLink() const { return map->first(); }

/** Get the "next" link.
 *  This method is used to iterate through the links.
 *  The order of the links is arbitrary.
 *  @param lnk is a link number
 *  @return the link number of the link following lnk,
 *  or 0 if there are no more links
 */
inline int LinkTable::nextLink(int lnk) const { return map->next(lnk); }

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
        return map->find(hashkey(ipa,ipp));
}

/** Get the link number based on a nonce.
 *  @param nonce is a value used to identify a new leaf when connecting.
 *  @return the number of the link that matches the given nonce,
 *  or 0 if there is no matching link
 */
inline int LinkTable::lookup(uint64_t nonce) const {
        return map->find(nonce);
}

/** Get the link number based on the peer's forest address.
 *  This method is only defined for links going to leaf nodes.
 *  @param nonce is a value used to identify a new leaf when connecting.
 *  @return the number of the link that matches the given nonce,
 *  or 0 if there is no matching link
 */
inline int LinkTable::lookup(fAdr_t peerAdr) const {
        return padrMap->find(peerAdr);
}

/** Get the table entry for a given link.
 *  @param lnk is a valid link number
 *  @return a reference to the entry for lnk
 */
inline LinkTable::Entry& LinkTable::getEntry(int lnk) const {
	return map->getValue(lnk);
}

/** Get a sample of the link statistics.
 *  @param lnk is a link number
 *  @param stats is a reference used to return the value of the link statistics
 */
inline void LinkTable::getStats(int lnk, StatCounts& stats) const {
	stats = getEntry(lnk).stats;
}

/** Get a sample of the router statistics.
 *  @param lnk is a link number
 *  @param stats is a reference used to return the value of the link statistics
 */
inline void LinkTable::getStats(StatCounts& rtrStats, StatCounts& leafStats)
				const {
	rtrStats = this->rtrStats; leafStats = this->leafStats;
}

} // ends namespace


#endif
