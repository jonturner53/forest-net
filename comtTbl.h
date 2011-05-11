// Header file for comtTbl class.
//
// Maintains set of tuples of the form (comt, links)
// where comt is the comt number for this entry and
// links is the set of all incident links that are
// in the comt. In this version, links is implemented
// as a 32 bit vector, limiting the number of links
// to 32 (well actually 31, as we don't use 0).
// Also, this version only supports comt numbers up to
//

#ifndef COMTTBL_H
#define COMTTBL_H

#include "forest.h"
#include "qMgr.h"

class comtTbl {
public:
		comtTbl(int, fAdr_t, lnkTbl*, qMgr*);
		~comtTbl();

	int	lookup(comt_t);			// return comtree's table entry
	int	addEntry(comt_t);	 	 // add table entry
	bool	removeEntry(int);		// remove table entry
	bool	checkEntry(int) const;		// perform consistency checks
	void 	addLink(int, int, bool, bool); 	// add link to table entry
	void 	removeLink(int, int);		// remove link from table entry
	
	// access various fields in comtree table entry
	int	comtree(int) const;		// return comtree for entry
	bool&	coreFlag(int) const;		// return flag that determines if core node
	int&	plink(int) const;		// return the link to parent
	int&	qnum(int) const;		// return comtree q number

	// test various properties
	bool	valid(int) const;		// return true for valid entry
	bool	isLink(int,int) const;		// return true for valid link
	bool	isRlink(int,int) const;		// true for link to rtr
	bool	isLlink(int,int) const;		// true for link to local rtr
	bool	isClink(int,int) const;		// true for core link
	bool	inComt(int,int) const;		// return true if link in comt

	// return various sets of links
	int	links(int,uint16_t*,int) const;	// return vector of links
	int	rlinks(int,uint16_t*,int) const;// return vec of rtr links
	int	llinks(int,uint16_t*,int) const;// return vec of local rtr links
	int	clinks(int,uint16_t*,int) const;// return vec of core links

	// input/output of table content
	friend	bool operator>>(istream&, comtTbl&); // read in table
	friend	ostream& operator<<(ostream&, const comtTbl&);
private:
	int	maxte;			// largest table entry number
	struct tblEntry { 
		int comt;		// comtree for this table entry
		int plnk;		// parent link in comtree
		bool cFlag;		// true if this router is in core
		int qn;			// number of comtree queue
		int links;		// bit vector of links
		int rlinks;		// bit vec of links to other routers
		int llinks;		// bit vec of links to local routers
		int clinks;		// bit vec to neighboring core routers
	} *tbl;				// tbl[i] contains data for table entry
	int free;			// start of free list

	fAdr_t	myAdr;			// forest address of this router
	lnkTbl	*lt;			// pointer to link table
	qMgr	*qm;			// pointer to queue manager
	hashTbl	*ht;			// pointer to hash table for fast lookup

	// helper functions
	uint64_t hashkey(comt_t) const;	// for using hash table
	int	listLinks(int,uint16_t*,int) const; // for links,...
	bool 	getComt(istream&);		// read table entry from input
	void	putComt(ostream&, int) const; // print a single entry
	int	getLinks(istream&);		// for getComt
	void	putLinks(ostream&,int) const;	// for putComt
};

inline void comtTbl::addLink(int ctte, int lnk, bool rflg, bool cflg) {
// Add the link to the set of valid links for ctte.
// Rflg is true if far end of link is another router.
// Cflg is true if far end of link is a core router for this comtree.
	if (ctte > 0 && ctte <= maxte && valid(ctte)) {
		tbl[ctte].links |= (1 << lnk);
		if (rflg) tbl[ctte].rlinks |= (1 << lnk);
		if (rflg && cflg) tbl[ctte].clinks |= (1 << lnk);
	}
}

inline void comtTbl::removeLink(int ctte, int lnk) {
// Remove the link from the set of valid links for ctte.
	if (ctte > 0 && ctte <= maxte && valid(ctte)) {
		tbl[ctte].links &= ~(1 << lnk);
		tbl[ctte].rlinks &= ~(1 << lnk);
		tbl[ctte].clinks &= ~(1 << lnk);
	}
}

// return number of the comtree for given entry
inline int comtTbl::comtree(int ctte) const {
	assert(valid(ctte)); return tbl[ctte].comt;
}

// return the parent link for the comt
inline int& comtTbl::plink(int ctte) const {
	assert(valid(ctte)); return tbl[ctte].plnk;
}

// return true if this node is a core router
inline bool& comtTbl::coreFlag(int ctte) const {
	assert(valid(ctte)); return tbl[ctte].cFlag;
}

// return the queue number for the comtree
inline int& comtTbl::qnum(int ctte) const {
	assert(valid(ctte)); return tbl[ctte].qn;
}

// return true if ctte is a valid entry
inline bool comtTbl::valid(int ctte) const {
	return ctte > 0 && ctte <= maxte && tbl[ctte].qn != 0;
}
// return true for valid link
inline bool comtTbl::isLink(int ctte, int lnk) const {
	return (tbl[ctte].links & (1 << lnk)) ? true : false;
}
// return true for link to rtr
inline bool comtTbl::isRlink(int ctte, int lnk) const {
	return (tbl[ctte].rlinks & (1 << lnk)) ? true : false;
}
// return true for link to rtr in same zip code
inline bool comtTbl::isLlink(int ctte, int lnk) const {
	return (tbl[ctte].llinks & (1 << lnk)) ? true : false;
}
// return true for core link
inline bool comtTbl::isClink(int ctte, int lnk) const {
	return (tbl[ctte].clinks & (1 << lnk)) ? true : false;
}

// return true for link in comtree
inline bool comtTbl::inComt(int ctte, int lnk) const {
	return ctte > 0 && ctte <= maxte && valid(ctte)
		        && (tbl[ctte].links & (1<<lnk));
}

inline int comtTbl::links(int ctte, uint16_t* lnks, int limit) const {
// Return all the comtree links for the specified comtree.
// Note that this requires a comtree table entry number.
// Lnks points to a vector that will hold the links on return.
// The limit argument specifies the maximum number of links that
// may be returned. If the table entry specifies more links, the
// extra ones will be ignored. The value returned by the function
// is the number of links that were returned.
        if (ctte < 1 || ctte > maxte || !valid(ctte)) return 0;
	return listLinks(tbl[ctte].links,lnks,limit);
}

inline int comtTbl::rlinks(int ctte, uint16_t* lnks, int limit) const {
// Return all the comtree links that go to other routers.
        if (ctte < 1 || ctte > maxte || !valid(ctte)) return 0;
	return listLinks(tbl[ctte].rlinks,lnks,limit);
}

inline int comtTbl::llinks(int ctte, uint16_t* lnks, int limit) const {
// Return all the comtree links that go to other routers in same zip code
        if (ctte < 1 || ctte > maxte || !valid(ctte)) return 0;
	return listLinks(tbl[ctte].llinks,lnks,limit);
}

inline int comtTbl::clinks(int ctte, uint16_t* lnks, int limit) const {
// Return all the comtree links that go to core routers.
        if (ctte < 1 || ctte > maxte || !valid(ctte)) return 0;
	return listLinks(tbl[ctte].clinks,lnks,limit);
}

// Compute key for hash lookup
inline uint64_t comtTbl::hashkey(comt_t ct) const {
        return (uint64_t(ct) << 32) | ct;
}

// Return index of table entry for given comtree
// If no match, return Null.
inline int comtTbl::lookup(comt_t ct) {
        return ht->lookup(hashkey(ct));
}

#endif
