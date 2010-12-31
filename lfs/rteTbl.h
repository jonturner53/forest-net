// Header file for rteTbl class.
//
// Maintains set of tuples of the form (dest, pleng, link1, link2, link3)
// where dest is an IP destination address, pleng specifies the number of
// bits of the address that are relevant here (the prefix length), and the
// the links are possible nexthop links that can be used to reach the destination,
// in order of preference. All are expected to connect to routers
// that are "closer" to the destination than "this router".
// If any of the links is zero, the remaining ones are ignored.
//

#ifndef RTETBL_H
#define RTETBL_H

#include "lfs.h"
#include "lmp.h"
#include "fltrTbl.h"
#include "qMgr.h"

class rteTbl {
public:
		rteTbl(int,ipa_t,lnkTbl*,qMgr*);
		~rteTbl();

	int	lookup(ipa_t); 			// lookup entry
	bool	valid(int) const;	 	// return true for valid entry

	// retrieve fields from given entry
	ipa_t	prefix(int) const;		// return prefix for this entry
	int	prefLeng(int) const;		// return prefix length
	int& 	link(int,int);			// return reference to link

	// modify table
	int	addEntry(ipa_t,int); 		// add new entry
	bool	removeEntry(int);		// remove an entry
	bool	checkEntry(int) const;		// perform consistency check
	void	sort();				// sort entries

	// io routines for reading/writing table
	bool 	getEntry(istream&);	
	friend	bool operator>>(istream&, rteTbl&); 
	void	putEntry(ostream&, int) const; 
	friend	ostream& operator<<(ostream&, const rteTbl&);

	static const int maxNhops=3;
private:
	int	nte;			// max number of table entries
	int	myAdr;			// address of this router in overlay

	struct rtEntry {
	ipa_t	pfx;			// prefix
	int	pfxlng;			// prefix length
	int	nh[maxNhops+1];		// list of next hops
	};

	rtEntry *tbl;			// vector of table entries
	lmp  *lmpt;			// longest matching pfx data structure
	lnkTbl*	lt;			// pointer to link table
	qMgr*	qm;			// pointer to queue manager

	int free;			// first unused entry
};

inline bool rteTbl::valid(int te) const {
	return 1 <= te && te <= nte && tbl[te].pfx != 0;
}

inline ipa_t rteTbl::prefix(int te) const {
	assert(valid(te));
	return tbl[te].pfx;
}

inline int rteTbl::prefLeng(int te) const {
	assert(valid(te));
	return tbl[te].pfxlng;
}

// Return the i-th choice link (if any) or zero.
inline int& rteTbl::link(int te, int i) {
	assert(valid(te) && 1 <= i && i <= maxNhops);
	return tbl[te].nh[i];
}

#endif
