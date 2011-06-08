/** @file RouteTable.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef ROUTETABLE_H
#define ROUTETABLE_H

#include "CommonDefs.h"
#include "UiHashTbl.h"
#include "ComtreeTable.h"
#include "QuManager.h"

/** Header file for RouteTable class.
 * 
 *  Maintains set of tuples of the form (comtree, address, qnum, links)
 *  where the links is the either a single link, in the case of a unicast
 *  entry, or a set of links, in the case of a multicast entry.
 *  Qnum designates the queue to use for this route. If qnum=0,
 *  the default queue for the comtree is used.
 * 
 *  Main methods
 *    lookup - returns index of table entry, for a (comtree, address) pair
 *    link - returns the link for a given unicast entry
 *    links - returns a vector of links for a given multicast entry
 *    addEntry - adds new entry for triple (comtree,dest addr,qnum,output link)
 *    removeEntry - removes an existing entry
 */
class RouteTable {
public:
		RouteTable(int,fAdr_t,LinkTable*,ComtreeTable*,QuManager*);
		~RouteTable();

	/** access methods */
	int	lookup(comt_t, fAdr_t);
	comt_t	getComtree(int) const;
	fAdr_t	getAddress(int) const;	
	int	getQnum(int) const;	
	int 	getLink(int) const; 		
	int 	getLinks(int) const; 		
	int 	getLinks(int,uint16_t*,int) const; 

	/** predicates */
	bool	valid(int) const;
	bool	isLink(int,int) const;
	bool	noLinks(int) const;
	bool	checkEntry(int) const;	

	/** modifiers */
	int	addEntry(comt_t,fAdr_t,int,int);
	bool	removeEntry(int);
	bool	addLink(int,int);
	bool	removeLink(int,int);	
	bool	setLink(int,int);
	void	setQnum(int,int);		

	/** input/output */
	bool read(istream&);
	void write(ostream&) const;
private:
	int	nte;			///< max number of table entries
	int	myAdr;			///< address of this router

	struct rtEntry {
	comt_t	ct;			///< comtree number
	fAdr_t	adr;			///< destination address
	int	qn;			///< queue number (if 0 use default)
	int 	lnks;			///< link or links
	};

	rtEntry *tbl;			///< vector of table entries
	UiHashTbl *ht;			///< hash table to speed up access
	LinkTable* lt;			///< pointer to link table
	ComtreeTable* ctt;		///< pointer to comtree table
	QuManager* qm;			///< pointer to queue manager

	int free;			///< first unused entry

	// helper functions
	uint64_t hashkey(comt_t, fAdr_t);  ///< return hash key
	bool 	readEntry(istream&);	
	void	writeEntry(ostream&, int) const; 
};

inline bool RouteTable::valid(int te) const {
	return 1 <= te && te <= nte && tbl[te].ct != 0;
}

// Return true if lnk is a valid link for this table entry.
// Assumes a valid multicast table entry.
inline bool RouteTable::isLink(int te, int lnk) const {
	assert(valid(te));
	return (tbl[te].lnks & (1 << lnk)) ? true : false;
}

// Return true if no valid links for this multicast entry.
inline bool RouteTable::noLinks(int te) const {
	assert(valid(te)); return tbl[te].lnks == 0;
}

inline comt_t RouteTable::getComtree(int te) const {
	assert(valid(te)); return tbl[te].ct;
}
inline fAdr_t RouteTable::getAddress(int te) const {
	assert(valid(te)); return tbl[te].adr;
}
inline int RouteTable::getQnum(int te) const {
	assert(valid(te)); return tbl[te].qn;
}

inline int RouteTable::getLink(int te) const {
// Return link for a unicast table entry.
// Return 0 if this is a multicast entry.
	assert(valid(te));
	if (Forest::mcastAdr(tbl[te].adr)) return 0;
	return tbl[te].lnks;
}

inline int RouteTable::getLinks(int te) const {
// Return link vector for a unicast table entry.
// Return 0 if this is a unicast entry.
	assert(valid(te));
	return (Forest::mcastAdr(getAddress(te)) ? tbl[te].lnks : 0);
}

inline bool RouteTable::setLink(int te, int lnk) {
// Set the link field for a unicast table entry. Return true on success,
// false if entry is not a unicast entry.
	assert(valid(te));
	if (Forest::mcastAdr(tbl[te].adr)) return false;
	tbl[te].lnks = lnk;
	return true;
}

inline void RouteTable::setQnum(int te, int q) {
// Set the queue number.
	assert(valid(te)); tbl[te].qn = q;
}

inline bool RouteTable::addLink(int te, int lnk) {
// Add a link to a multicast table entry. Return true on success,
// false if entry is not a multicast entry.
	assert(valid(te));
	if (Forest::mcastAdr(tbl[te].adr)) {
		tbl[te].lnks |= (1 << lnk); return true;
	} else return false;
}

inline bool RouteTable::removeLink(int te, int lnk) {
// Remove a link from a multicast table entry. Return true on success,
// false if entry is not a multicast entry.
	assert(valid(te));
	if (Forest::mcastAdr(tbl[te].adr)) {
		tbl[te].lnks &= (~(1 << lnk)); return true;
	} else return false;
}

inline uint64_t RouteTable::hashkey(comt_t ct, fAdr_t fa) {
// Return the hashkey for the specified comtree and address
	if (!Forest::mcastAdr(fa)) {
		int zip = Forest::zipCode(fa);
		if (zip != Forest::zipCode(myAdr))
			fa = Forest::forestAdr(zip,0);
	}
	return (uint64_t(ct) << 32) | fa;

}

#endif
