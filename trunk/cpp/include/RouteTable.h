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
#include "Forest.h"
#include "Util.h"
#include "Hash.h"
#include "HashMap.h"
#include "ComtreeTable.h"

namespace forest {

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
 *  which can be obtained using the getRtx() method.
 */
class RouteTable {
public:
		RouteTable(int, fAdr_t, ComtreeTable*);
		~RouteTable();

	// predicates
	bool	validRtx(int) const;
	bool	isLink(int,int) const;
	bool	noLinks(int) const;

	// iteration methods
	int	firstRtx() const;
	int	nextRtx(int) const;
	int	firstComtLink(int) const;
	int	nextComtLink(int, int) const;

	// access methods
	int	getRtx(comt_t, fAdr_t) const;
	comt_t	getComtree(int) const;
	fAdr_t	getAddress(int) const;	
	int 	getLinkCount(int) const; 		

	// modifiers
	bool	addLink(int,int);
	void	removeLink(int,int);	
	int	addRoute(comt_t,fAdr_t,int);
	void	removeRoute(int);
	void	purge(comt_t, int);
	void	setLink(int,int);

	// input/output
	bool 	read(istream&);
	string	toString() const;
	string	entry2string(int) const; 
private:
	int	maxRtx;			///< max number of table entries
	int	myAdr;			///< address of this router
	ComtreeTable *ctt;		///< pointer to comtree table

	typedef HashSet<int32_t,Hash::s32> Vset;

	/// map (comtree,adr) pair to list of cLnks
	HashMap<uint64_t,Vset,Hash::u64> *rteMap;

	// map (comtree,link) to list of rtx values (routes that use link)
	HashMap<uint64_t,Vset,Hash::u64> *clMap;

	// helper functions
	uint64_t rmKey(comt_t, fAdr_t) const;  ///< key for rteMap
	uint64_t cmKey(comt_t, int32_t) const;  ///< key for clMap
	bool 	readRoute(istream&);	
};

/** Verify that a route index is valid.
 *  @param rtx is an index into the routing table
 *  @return true if rtx corresponds to a valid route, else false
 */
inline bool RouteTable::validRtx(int rtx) const {
	return rteMap->valid(rtx);
}

/** Determine if a comtree link is in a multicast route.
 *  @param rtx is a route index
 *  @param cLnk is a comtree link number
 *  @return true if the route with index rtx if lnk is one of the links
 *  in the multicast route
 *  Assumes rtx is a valid index that refers to a multicast route
 */
inline bool RouteTable::isLink(int rtx, int cLnk) const {
	Vset& lset = rteMap->getValue(rtx);
	return lset.contains(cLnk);
}

/** Determine if a multicast route has no links.
 *  @return true if the route has no links, else false
 */
inline bool RouteTable::noLinks(int rtx) const {
	Vset& lset = rteMap->getValue(rtx);
	return lset.empty();
}

/** Get the first route index.
 *  This method is used to iterate through all the routes in the table.
 *  The order of routes is arbitrary.
 *  @return the first route index, or 0 if there are none
 */
inline int RouteTable::firstRtx() const {
	return rteMap->first();
}

/** Get the next route index following a given route index.
 *  This method is used to iterate through all the routes in the table.
 *  The order of routes is arbitrary.
 *  @param rtx is a route index
 *  @return the next route index following rtx, or 0 if there is none
 */
inline int RouteTable::nextRtx(int rtx) const {
	return rteMap->next(rtx);
}

/** Get the first comtree link for a given route.
 *  This method is used to iterate through all the links in the route.
 *  The order of comtree links is arbitrary.
 *  @return the first route index, or 0 if there are none
 */
inline int RouteTable::firstComtLink(int rtx) const {
	return rteMap->getValue(rtx).first();
}

/** Get the next comtree link following a given comtree link.
 *  This method is used to iterate through all the links in a route.
 *  The order of comtree links is arbitrary.
 *  @param rtx is a route index
 *  @param cLnk is a comtree link number
 *  @return the next comtree link number for the given route following cLnk,
 *  or 0 if there is no next comtree link
 */
inline int RouteTable::nextComtLink(int rtx, int cLnk) const {
	return rteMap->getValue(rtx).next(cLnk);
}

/** Get the route index for a given comtree and destination address.
 *  @param comt is a comtree number
 *  @param adr is a forest address
 *  @return the associated comtree index or 0 if there is none
 */
inline int RouteTable::getRtx(comt_t comt, fAdr_t adr) const {
        return rteMap->find(rmKey(comt,adr));
}   

/** Get the comtree number for a given route.
 *  @param rtx is a route index
 *  @return the comtree that the route was defined for
 */
inline comt_t RouteTable::getComtree(int rtx) const {
	uint64_t kee = rteMap->getKey(rtx);
	return (comt_t) (kee >> 32);
}

/** Get the destination address for a given route.
 *  @param rtx is a route index
 *  @return the address that the route was defined for
 */
inline fAdr_t RouteTable::getAddress(int rtx) const {
	uint64_t kee = rteMap->getKey(rtx);
	return (fAdr_t) (kee & 0xffffffff);
}

/** Get the number of links used by a route.
 *  @param rtx is a route index
 *  @return the number of outgoing links used by this route;
 *  for unicast routes, this should always be 1.
 */
inline int RouteTable::getLinkCount(int rtx) const {
	return rteMap->getValue(rtx).size();
}

/** Add a subscriber link to a multicast route.
 *  @param rtx is the route number
 *  @param cLnk is the comtree link number for the new subscriber
 *  @return true on success, false on failure
 */
inline bool RouteTable::addLink(int rtx, int cLnk) {
	if (!validRtx(rtx) || !Forest::mcastAdr(getAddress(rtx)))
		return false;
	Vset& lset = rteMap->getValue(rtx);
	lset.insert(cLnk);
	uint64_t kee = cmKey(getComtree(rtx),cLnk);
	int x = clMap->find(kee);
	if (x == 0) {
		Vset new_routes;
		x = clMap->put(kee,new_routes);
	}
	clMap->getValue(x).insert(rtx);
	return true;
}

/** Remove a subscriber link from a multicast route.
 *  @param rtx is the route number
 *  @param cLnk is the comtree link number for the link to be removed
 *  @return true on success, false on failure
 */
inline void RouteTable::removeLink(int rtx, int cLnk) {
	if (!validRtx(rtx) || !Forest::mcastAdr(getAddress(rtx)))
		return;
	Vset& lset = rteMap->getValue(rtx);
	lset.remove(cLnk);
	uint64_t kee = cmKey(getComtree(rtx),cLnk);
	Vset& routes = clMap->get(kee);
	routes.remove(rtx);
	if (lset.size() == 0) // no subscribers left
		rteMap->remove(rteMap->getKey(rtx));
	if (routes.size() == 0) // no routes mapped to cLnk
		clMap->remove(kee);
}

/** Set the link for a unicast route.
 *  @param rtx is the route number
 *  @param cLnk is the comtree link number for the new outgoing link
 */
inline void RouteTable::setLink(int rtx, int cLnk) {
	if (!validRtx(rtx) || Forest::mcastAdr(getAddress(rtx)))
		return;
	// assume link set is empty or has a single member
	Vset& lset = rteMap->getValue(rtx);
	if (lset.size() != 0) {
		int old_cLnk = lset.first();
		uint64_t kee = cmKey(getComtree(rtx),old_cLnk);
		Vset& routes = clMap->get(kee);
		routes.remove(rtx);
		if (routes.size() == 0) clMap->remove(kee);
		lset.remove(old_cLnk);
	}
	uint64_t kee = cmKey(getComtree(rtx),cLnk);
	int x = clMap->find(kee);
	if (x == 0) {
		Vset new_routes;
		x = clMap->put(kee,new_routes);
	}
	clMap->getValue(x).insert(rtx);
	lset.insert(cLnk);
}

/** Compute a key for use in the route map.
 *  @param comt is a comtree number
 *  @param adr is a forest address
 *  @return a 64 bit integer suitable for use as a lookup key
 */
inline uint64_t RouteTable::rmKey(comt_t comt, int32_t adr) const {
	// zero out local address parts for uncast addresses to non-local zip
	bool local = ((adr & 0xffff0000) ^ (myAdr & 0xffff0000)) == 0;
	if (!Forest::mcastAdr(adr) && !local)  adr &= 0xffff0000;
	return (uint64_t(comt) << 32) | (uint64_t(adr) & 0xffffffff);
}

/** Compute a key for use in the comtree link map.
 *  @param comt is a comtree number
 *  @param cLnk is a comtree link number
 *  @return a 64 bit integer suitable for use as a lookup key
 */
inline uint64_t RouteTable::cmKey(comt_t comt, int32_t cLnk) const {
	return (uint64_t(comt) << 32) | (uint64_t(cLnk) & 0xffffffff);
}

} // ends namespace


#endif
