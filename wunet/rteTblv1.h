// Header file for rteTbl class.
//
// Maintains set of tuples of the form (vnet, address, link)
// where the link is the link to which a packet should be forwarded.
//
// Main methods
//   lookup - returns index of table entry, for a (vnet, address) pair
//   link - returns the link for a given entry
//   addEntry - adds new entry for triple (vnet, dest addr, output link)
//   removeEntry - removes an existing entry
//

#ifndef RTETBL_H
#define RTETBL_H

#include "wunet.h"

class rteTbl {
public:
		rteTbl(int);
		~rteTbl();

	int	lookup(vnet_t, wuAdr_t); 	// lookup entry
	bool	valid(int) const;	 	// return true for valid entry

	// retrieve fields from given entry
	vnet_t	vnet(int) const;		// return vnet for this entry
	wuAdr_t	address(int) const;		// return address for this entry
	int 	link(int) const; 		// retrieve link

	// modify table
	bool	addEntry(vnet_t, wuAdr_t, int);	// add new entry
	bool	removeEntry(int);		// remove an entry
	void	setLink(int, int);		// set link for unicast entry

	// io routines for reading/writing table
	bool 	getEntry(istream&);	
	friend	bool operator>>(istream&, rteTbl&); 
	void	putEntry(ostream&, int) const; 
	friend	ostream& operator<<(ostream&, const rteTbl&);
private:
	int	nte;			// max number of table entries
	int	maxInUse;		// largest index of an in-use entry

	struct rtEntry {
	vnet_t	vn;			// vnet number
	wuAdr_t	adr;			// destination address
	int 	lnk;			// link
	};

	rtEntry *tbl;			// vector of table entries
};

inline bool rteTbl::valid(int te) const { return (tbl[te].vn != 0); }
inline vnet_t rteTbl::vnet(int te) const { return tbl[te].vn; }
inline wuAdr_t rteTbl::address(int te) const { return tbl[te].adr; }
inline int rteTbl::link(int te) const { return (tbl[te].lnk); }
inline void rteTbl::setLink(int te, int lnk) { tbl[te].lnk = lnk; }

#endif
