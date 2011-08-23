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

/** Header file for RouteTable class.
 * 
 *  Maintains set of tuples of the form (comtree, address, links)
 *  where the links is the either a single link, in the case of a unicast
 *  entry, or a set of links, in the case of a multicast entry.
 */
class RouteTable {
public:
		RouteTable(int,fAdr_t,ComtreeTable*);
		~RouteTable();

	// predicates
	bool	validRteIndex(int) const;
	bool	isLink(int,int) const;
	bool	noLinks(int) const;
	//bool    compareEntry(comt_t,fAdr_t,int);

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
	bool	addLink(int,int);
	void	removeLink(int,int);	
	void	setLink(int,int);

	// input/output
	bool read(istream&);
	void write(ostream&) const;
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
	void	writeEntry(ostream&, int) const; 
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

// Return true if no valid links for this multicast entry.
inline bool RouteTable::noLinks(int rtx) const {
	return tbl[rtx].links->size() == 0;
}

inline int RouteTable::firstRteIndex() const {
	return rteMap->firstId();
}

inline int RouteTable::nextRteIndex(int rtx) const {
	return rteMap->nextId(rtx);
}

// Perform a lookup in the routing table. Comt is the comtree number,
// adr is the destination address. Returned value is the index
// of the table entry, or 0 if there is no match. 
inline int RouteTable::getRteIndex(comt_t comt, fAdr_t adr) const {
        return rteMap->getId(key(comt,adr));
}   

inline comt_t RouteTable::getComtree(int rtx) const {
	return tbl[rtx].ct;
}
inline fAdr_t RouteTable::getAddress(int rtx) const {
	return tbl[rtx].adr;
}

// Return comtree link for a unicast table entry.
// Return 0 if this is a multicast entry.
inline int RouteTable::getLink(int rtx) const {
	if (Forest::mcastAdr(tbl[rtx].adr)) return 0;
	return tbl[rtx].lnk;
}

inline set<int>& RouteTable::getSubLinks(int rtx) const {
	return *tbl[rtx].links;
}
	

inline void RouteTable::setLink(int rtx, int cLnk) {
// Set the link field for a unicast table entry. Return true on success,
// false if entry is not a unicast entry.
	if (Forest::mcastAdr(tbl[rtx].adr)) return;
	tbl[rtx].lnk = cLnk;
}

// Add a comtree link to a multicast table entry. Return true on success,
// false if entry is not a multicast entry.
inline bool RouteTable::addLink(int rtx, int cLnk) {
	if (!Forest::mcastAdr(tbl[rtx].adr)) return false;
	tbl[rtx].links->insert(cLnk);
	return true;
}

// Remove a comtree link from a multicast table entry. Return true on success,
// false if entry is not a multicast entry.
inline void RouteTable::removeLink(int rtx, int cLnk) {
	if (!Forest::mcastAdr(tbl[rtx].adr)) return;
	tbl[rtx].links->erase(cLnk);
}

inline uint64_t RouteTable::key(comt_t ct, fAdr_t fa) const {
// Return the key for the specified comtree and address
	if (!Forest::mcastAdr(fa)) {
		int zip = Forest::zipCode(fa);
		if (zip != Forest::zipCode(myAdr))
			fa = Forest::forestAdr(zip,0);
	}
	return (uint64_t(ct) << 32) | (uint64_t(fa) & 0xffffffff);

}

#endif
