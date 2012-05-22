/** @file RouteTable.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef ROUTETABLE_H
#define ROUTETABLE_H

#include <set>
#include "CommonDefs.h"
#include "IdMap.h"
#include "ComtreeTable.h"

typedef set<int>::iterator RteLink; 

/** Maintains a set of routes. A unicast route is a triple
 *  (comtree, address, comtreeLink) where comtree is the comtree number
 *  associated with the route, address is a unicast address and
 *  comtreeLink is the comtree link number for some link in the
 *  comtree (comtree link numbers are maintained by the ComtreeTable).
 * 
 *  A multicast route is similar, except in this case the address
 *  is a multicast address and comtreeLink is replaced by a list
 *  of comtree link numbers that represents the subscribers to
 *  the multicast address.
 *
 *  The data for a route is accessed using its "route index",
 *  which can be obtained using the getRteIndex() method.
 */
class RouteTable {
public:
		RouteTable(int,fAdr_t,ComtreeTable*);
		~RouteTable();

	// predicates
	bool	validRteIndex(int) const;
	bool	isLink(int,int) const;
	bool	noLinks(int) const;

	// iteration methods
	int	firstRteIndex() const;
	int	nextRteIndex(int) const;

	// access methods
	int	getRteIndex(comt_t, fAdr_t) const;
	comt_t	getComtree(int) const;
	fAdr_t	getAddress(int) const;	
	int 	getLink(int) const; 		
	set<int>& getSubLinks(int) const;

	// modifiers
	int	addEntry(comt_t,fAdr_t,int);
	void	removeEntry(int);
	void	purgeRoutes(comt_t);
	bool	addLink(int,int);
	void	removeLink(int,int);	
	void	setLink(int,int);

	// input/output
	bool 	read(istream&);
	void 	write(ostream&) const;
	void	writeEntry(ostream&, int) const; 
private:
	int	maxRtx;			///< max number of table entries
	int	myAdr;			///< address of this router

	struct RouteEntry {		///< routing table entry
	comt_t	ct;			///< comtree number
	fAdr_t	adr;			///< destination address
	int 	lnk;			///< comtree link for unicast routes
	set<int> *links;		///< comtree links for multicast routes
	};
	RouteEntry *tbl;		///< table of routes
	IdMap	*rteMap;		///< map (comtree,adr) pair to index

	ComtreeTable *ctt;	

	// helper functions
	uint64_t key(comt_t, fAdr_t) const;  ///< key for use with rteMap
	bool 	readEntry(istream&);	
};

/** Verify that a route index is valid.
 *  @param rtx is an index into the routing table
 *  @return true if rtx corresponds to a valid route, else false
 */
inline bool RouteTable::validRteIndex(int rtx) const {
	return rteMap->validId(rtx);
}

/** Determine if a comtree link is in a multicast route.
 *  @param rtx is a route index
 *  @param cLnk is a comtree link number
 *  @return true if the route with index rtx if lnk is one of the links
 *  in the multicast route
 *  Assumes rtx is a valid index that refers to a multicast route
 */
inline bool RouteTable::isLink(int rtx, int cLnk) const {
	set<int>& lset = *tbl[rtx].links;
	return lset.find(cLnk) != lset.end();
}

/** Determine if a multicast route has no links.
 *  @return true if the route has no links, else false
 */
inline bool RouteTable::noLinks(int rtx) const {
	return tbl[rtx].links->size() == 0;
}

/** Get the first route index.
 *  This method is used to iterate through all the routes in the table.
 *  The order of routes is arbitrary.
 *  @return the first route index, or 0 if there are none
 */
inline int RouteTable::firstRteIndex() const {
	return rteMap->firstId();
}

/** Get the next route index following a given route index.
 *  This method is used to iterate through all the routes in the table.
 *  The order of routes is arbitrary.
 *  @param rtx is a route index
 *  @return the next route index following rtx, or 0 if there is none
 */
inline int RouteTable::nextRteIndex(int rtx) const {
	return rteMap->nextId(rtx);
}

/** Get the route index for a given comtree and destination address.
 *  @param comt is a comtree number
 *  @param adr is a forest address
 *  @return the associated comtree index or 0 if there is none
 */
inline int RouteTable::getRteIndex(comt_t comt, fAdr_t adr) const {
        return rteMap->getId(key(comt,adr));
}   

/** Get the comtree number for a given route.
 *  @param rtx is a route index
 *  @return the comtree that the route was defined for
 */
inline comt_t RouteTable::getComtree(int rtx) const {
	return tbl[rtx].ct;
}

/** Get the destination address for a given route.
 *  @param rtx is a route index
 *  @return the address that the route was defined for
 */
inline fAdr_t RouteTable::getAddress(int rtx) const {
	return tbl[rtx].adr;
}

/** Get the comtree link number for a given unicast route.
 *  @param rtx is a route index
 *  @return the comtree link number for the outgoing link
 */
inline int RouteTable::getLink(int rtx) const {
	if (Forest::mcastAdr(tbl[rtx].adr)) return 0;
	return tbl[rtx].lnk;
}

/** Get a reference to the set of subscriber links for a given multicast route.
 *  This method is provided to allow client programs to efficiently
 *  iterate through all the subscriber links in the route.
 *  It must not be used to modify the set of subscriber links.
 *  @param rtx is a route index
 *  @return a reference to a set of integers, where each integer in the
 *  set is a comtree link number
 */
inline set<int>& RouteTable::getSubLinks(int rtx) const {
	return *(tbl[rtx].links);
}

/** Set the link for a unicast route.
 *  @param rtx is the route number
 *  @param cLnk is the comtree link number for the new outgoing link
 */
inline void RouteTable::setLink(int rtx, int cLnk) {
	if (!validRteIndex(rtx) || !ctt->validComtLink(cLnk) ||
	    Forest::mcastAdr(tbl[rtx].adr))
		return;
	tbl[rtx].lnk = cLnk;
}

/** Add a subscriber link to a multicast route.
 *  @param rtx is the route number
 *  @param cLnk is the comtree link number for the new subscriber
 *  @return true on success, false on failure
 */
inline bool RouteTable::addLink(int rtx, int cLnk) {
	if (!validRteIndex(rtx) || !ctt->validComtLink(cLnk) ||
	    !Forest::mcastAdr(tbl[rtx].adr))
		return false;
	tbl[rtx].links->insert(cLnk);
	ctt->registerRte(cLnk,rtx);
	return true;
}

/** Remove a subscriber link from a multicast route.
 *  @param rtx is the route number
 *  @param cLnk is the comtree link number for the link to be removed
 *  @return true on success, false on failure
 */
inline void RouteTable::removeLink(int rtx, int cLnk) {
	if (!validRteIndex(rtx) || !ctt->validComtLink(cLnk) ||
	    !Forest::mcastAdr(tbl[rtx].adr)) {
		return;
	}
	tbl[rtx].links->erase(cLnk);
	ctt->deregisterRte(cLnk,rtx);
	if (tbl[rtx].links->size() == 0) // no subscribers lefr
		removeEntry(rtx);
}

/** Compute a key for a specified (comtree,address) pair.
 *  @param comt is a comtree number
 *  @param adr is an address
 *  @return a 64 bit integer suitable for use as a lookup key
 */
inline uint64_t RouteTable::key(comt_t comt, fAdr_t adr) const {
	if (!Forest::mcastAdr(adr)) {
		int zip = Forest::zipCode(adr);
		if (zip != Forest::zipCode(myAdr))
			adr = Forest::forestAdr(zip,0);
	}
	return (uint64_t(comt) << 32) | (uint64_t(adr) & 0xffffffff);
}

#endif
