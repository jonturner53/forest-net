// Header file for rteTbl class.
//
// Maintains set of tuples of the form (vnet, address, qnum, links)
// where the links is the either a single link, in the case of a unicast
// entry, or a set of links, in the case of a multicast entry.
// Qnum designates the queue to use for this route. If qnum=0,
// the default queue for the vnet is used.
//
// Main methods
//   lookup - returns index of table entry, for a (vnet, address) pair
//   link - returns the link for a given unicast entry
//   links - returns a vector of links for a given multicast entry
//   addEntry - adds new entry for triple (vnet, dest addr, qnum, output link)
//   removeEntry - removes an existing entry
//

#ifndef RTETBL_H
#define RTETBL_H

#include "wunet.h"
#include "hashTbl.h"
#include "qMgr.h"

class rteTbl {
public:
		rteTbl(int,qMgr*);
		~rteTbl();

	int	lookup(vnet_t, wuAdr_t); 	// lookup entry
	bool	valid(int) const;	 	// return true for valid entry

	// retrieve fields from given entry
	vnet_t	vnet(int) const;		// return vnet for this entry
	wuAdr_t	address(int) const;		// return address for this entry
	int	qnum(int) const;		// return queue for this route
	int 	link(int) const; 		// retrieve link
	int 	links(int,uint16_t*,int) const; // retrieve list of links

	// modify table
	int	addEntry(vnet_t,wuAdr_t,int,int); // add new entry
	bool	removeEntry(int);		// remove an entry
	bool	setLink(int,int);		// set link for unicast entry
	bool	addLink(int,int);		// add link for multicast entry
	bool	removeLink(int,int);		// remove link from multicast entry
	bool	noLinks(int) const;		// return true if no links defined

	// io routines for reading/writing table
	bool 	getEntry(istream&);	
	friend	bool operator>>(istream&, rteTbl&); 
	void	putEntry(ostream&, int) const; 
	friend	ostream& operator<<(ostream&, const rteTbl&);
private:
	int	nte;			// max number of table entries

	struct rtEntry {
	vnet_t	vn;			// vnet number
	wuAdr_t	adr;			// destination address
	int	qn;			// queue number (if 0 use default)
	int 	lnks;			// link or links
	};

	rtEntry *tbl;			// vector of table entries
	hashTbl *ht;			// hash table to speed up access
	qMgr*	qm;			// pointer to queue manager

	int free;			// first unused entry
};

inline bool rteTbl::valid(int te) const { return (tbl[te].vn != 0); }
inline vnet_t rteTbl::vnet(int te) const { return tbl[te].vn; }
inline wuAdr_t rteTbl::address(int te) const { return tbl[te].adr; }
inline int rteTbl::qnum(int te) const { return tbl[te].qn; }

inline int rteTbl::link(int te) const {
// Return link for a unicast table entry.
// Return 0 if this is a multicast entry.
	if (mcastAdr(tbl[te].adr)) return 0;
	return tbl[te].lnks;
}

inline bool rteTbl::setLink(int te, int lnk) {
// Set the link field for a unicast table entry. Return true on success,
// false if entry is not a unicast entry.
	if (mcastAdr(tbl[te].adr)) return false;
	tbl[te].lnks = lnk;
	return true;
}

inline bool rteTbl::addLink(int te, int lnk) {
// Add a link to a multicast table entry. Return true on success,
// false if entry is not a multicast entry.
	if (ucastAdr(tbl[te].adr)) return false;
	tbl[te].lnks |= (1 << lnk);
	return true;
}

inline bool rteTbl::removeLink(int te, int lnk) {
// Remove a link from a multicast table entry. Return true on success,
// false if entry is not a multicast entry.
	if (ucastAdr(tbl[te].adr)) return false;
	tbl[te].lnks &= (~(1 << lnk));
	return true;
}

inline bool rteTbl::noLinks(int te) const {
// Return true if no links are defined for the given table entry,
// else false.
	return tbl[te].lnks == 0;
}

#endif
