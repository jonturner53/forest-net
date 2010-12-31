// Header file for fltrTbl class.
//
// Maintains set of tuples of the form (ipsrc, ipdst, link, queue)
// where ipsrc and ipdst are the IP source and destination
// addresses for this filter, link is the link the packet
// should be forwarded on and queue is the number of the
// queue on that link.
//

#ifndef FLTRTBL_H
#define FLTRTBL_H

#include "lfs.h"
#include "qMgr.h"

class fltrTbl {
public:
		fltrTbl(int, ipa_t, lnkTbl*, qMgr*);
		~fltrTbl();

	int	lookup(ipa_t,ipa_t);		// return entry for src/dst
	int	addEntry(ipa_t,ipa_t); 		// add table entry
	bool	removeEntry(int);		// remove table entry
	bool	checkEntry(int) const;		// perform consistency checks
	bool	valid(int) const;		// return true for valid entry

	// access various fields in filter table entry
	ipa_t	src(int) const;		// return source adr for entry
	ipa_t	dst(int) const;		// return dest adr for entry
	int&	link(int);		// return the link for entry
	int&	qnum(int);		// return q number
	int&	rate(int);		// return allocated rate
	int&	inLink(int);		// return input link for the filter

	bool 	getFltr(istream&);		// read table entry from input
	friend	bool operator>>(istream&, fltrTbl&); // read in table
	void	putFltr(ostream&, int); 	// print a single entry
	friend	ostream& operator<<(ostream&, fltrTbl&);
private:
	int	maxte;			// largest table entry number
	struct tblEntry { 
		ipa_t src;		// IP source address
		ipa_t dst;		// IP destination address
		int lnk;		// link to next hop
		int qn;			// queue number
		int rate;		// allocated rate (firm)
		int inlnk;		// input link
	} *tbl;				// tbl[i] contains data for table entry
	int free;			// start of free list

	ipa_t	myAdr;			// IP address of this router (in overlay)
	lnkTbl	*lt;			// pointer to link table
	qMgr	*qm;			// pointer to queue manager
	hashTbl	*ht;			// pointer to hash table for fast lookup

	// helper functions
	uint64_t hashkey(ipa_t,ipa_t) const;	// for using hash table
};

// return true if fte is a valid entry
inline bool fltrTbl::valid(int fte) const {
	return fte > 0 && fte <= maxte && tbl[fte].src != 0;
}

// return the nexthop link for the entry
inline int& fltrTbl::link(int fte) {
	if (valid(fte)) return tbl[fte].lnk;
}

// return the queue number for the entry
inline int& fltrTbl::qnum(int fte) {
	if (valid(fte)) return tbl[fte].qn;
}

// return the rate for the filter
inline int& fltrTbl::rate(int fte) {
	if (valid(fte)) return tbl[fte].rate;
}

// return the input link for the filter
inline int& fltrTbl::inLink(int fte) {
	if (valid(fte)) return tbl[fte].inlnk;
}

// Compute key for hash lookup
inline uint64_t fltrTbl::hashkey(ipa_t src, ipa_t dst) const {
        return (uint64_t(src) << 32) | dst;
}

// Return index of table entry for given src/dst pair
// If no match, return Null.
inline int fltrTbl::lookup(ipa_t src, ipa_t dst) {
        return ht->lookup(hashkey(src,dst));
}

#endif
