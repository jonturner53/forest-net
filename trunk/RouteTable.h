// Header file for rteTbl class.
//
// Maintains set of tuples of the form (comtree, address, qnum, links)
// where the links is the either a single link, in the case of a unicast
// entry, or a set of links, in the case of a multicast entry.
// Qnum designates the queue to use for this route. If qnum=0,
// the default queue for the comtree is used.
//
// Main methods
//   lookup - returns index of table entry, for a (comtree, address) pair
//   link - returns the link for a given unicast entry
//   links - returns a vector of links for a given multicast entry
//   addEntry - adds new entry for triple (comtree,dest addr,qnum,output link)
//   removeEntry - removes an existing entry
//

#ifndef RTETBL_H
#define RTETBL_H

#include "forest.h"
#include "UiHashTbl.h"
#include "comtTbl.h"
#include "qMgr.h"

class rteTbl {
public:
		rteTbl(int,fAdr_t,lnkTbl*,ComtTbl*,qMgr*);
		~rteTbl();

	int	lookup(comt_t, fAdr_t); 	// lookup entry
	bool	valid(int) const;	 	// return true for valid entry
	bool	isLink(int,int) const;		// return true for valid link
	bool	noLinks(int) const;		// return true if no valid links

	// retrieve fields from given entry
	comt_t	comtree(int) const;		// return comtree for this entry
	fAdr_t	address(int) const;		// return address for this entry
	int&	qnum(int) const;		// return queue for this route
	int 	link(int) const; 		// retrieve link
	int 	links(int,uint16_t*,int) const; // retrieve list of links

	// modify table
	int	addEntry(comt_t,fAdr_t,int,int); // add new entry
	bool	removeEntry(int);		// remove an entry
	bool	checkEntry(int) const;		// perform consistency check
	bool	setLink(int,int);		// set link for unicast entry
	bool	addLink(int,int);		// add link for multicast entry
	bool	removeLink(int,int);		// remove link from multicast entry

	// io routines for reading/writing table
	friend	bool operator>>(istream&, rteTbl&); 
	friend	ostream& operator<<(ostream&, const rteTbl&);
private:
	int	nte;			// max number of table entries
	int	myAdr;			// address of this router

	struct rtEntry {
	comt_t	ct;			// comtree number
	fAdr_t	adr;			// destination address
	int	qn;			// queue number (if 0 use default)
	int 	lnks;			// link or links
	};

	rtEntry *tbl;			// vector of table entries
	UiHashTbl *ht;			// hash table to speed up access
	lnkTbl*	lt;			// pointer to link table
	ComtTbl* ctt;			// pointer to comtree table
	qMgr*	qm;			// pointer to queue manager

	int free;			// first unused entry

	// helper functions
	uint64_t hashkey(comt_t, fAdr_t);  // return hash key
	bool 	getEntry(istream&);	
	void	putEntry(ostream&, int) const; 
};

inline bool rteTbl::valid(int te) const {
	return 1 <= te && te <= nte && tbl[te].ct != 0;
}

// Return true if lnk is a valid link for this table entry.
// Assumes a valid multicast table entry.
inline bool rteTbl::isLink(int te, int lnk) const {
	assert(valid(te));
	return (tbl[te].lnks & (1 << lnk)) ? true : false;
}

// Return true if no valid links for this multicast entry.
inline bool rteTbl::noLinks(int te) const {
	assert(valid(te)); return tbl[te].lnks == 0;
}

inline comt_t rteTbl::comtree(int te) const {
	assert(valid(te)); return tbl[te].ct;
}
inline fAdr_t rteTbl::address(int te) const {
	assert(valid(te)); return tbl[te].adr;
}
inline int& rteTbl::qnum(int te) const {
	assert(valid(te)); return tbl[te].qn;
}

inline int rteTbl::link(int te) const {
// Return link for a unicast table entry.
// Return 0 if this is a multicast entry.
	assert(valid(te));
	if (forest::mcastAdr(tbl[te].adr)) return 0;
	return tbl[te].lnks;
}

inline bool rteTbl::setLink(int te, int lnk) {
// Set the link field for a unicast table entry. Return true on success,
// false if entry is not a unicast entry.
	assert(valid(te));
	if (forest::mcastAdr(tbl[te].adr)) return false;
	tbl[te].lnks = lnk;
	return true;
}

inline bool rteTbl::addLink(int te, int lnk) {
// Add a link to a multicast table entry. Return true on success,
// false if entry is not a multicast entry.
	assert(valid(te));
	if (forest::ucastAdr(tbl[te].adr)) return false;
	tbl[te].lnks |= (1 << lnk);
	return true;
}

inline bool rteTbl::removeLink(int te, int lnk) {
// Remove a link from a multicast table entry. Return true on success,
// false if entry is not a multicast entry.
	assert(valid(te));
	if (forest::ucastAdr(tbl[te].adr)) return false;
	tbl[te].lnks &= (~(1 << lnk));
	return true;
}

inline uint64_t rteTbl::hashkey(comt_t ct, fAdr_t fa) {
// Return the hashkey for the specified comtree and address
	return  (uint64_t(ct) << 32) |
		(forest::ucastAdr(fa) &&
		    (forest::zipCode(fa) != forest::zipCode(myAdr)) ?
			(fa & 0xffff0000) : fa);
}

#endif
