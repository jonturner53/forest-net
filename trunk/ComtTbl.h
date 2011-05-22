/** \file ComtTbl.h
 * Comtree table class.
 *
 */

#ifndef COMTTBL_H
#define COMTTBL_H

#include "forest.h"
#include "qMgr.h"

class ComtTbl {
public:
		ComtTbl(int, fAdr_t, lnkTbl*, qMgr*);
		~ComtTbl();

	int	lookup(comt_t);			///< return comtree's table entry
	int	addEntry(comt_t);	 	///< add table entry
	bool	removeEntry(int);		///< remove table entry
	bool	checkEntry(int) const;		///< perform consistency checks
	void 	addLink(int, int, bool, bool); 	///< add link to comtree
	void 	removeLink(int, int);		///< remove link from comtree
	
	/** getters and setters */
	comt_t	getComtree(int) const;
	bool	getCoreFlag(int) const;
	int	getPlink(int) const;
	int	getQnum(int) const;

	void	setCoreFlag(int, bool);
	void	setPlink(int, int);
	void	setQnum(int, int);

	/** test various properties */
	bool	valid(int) const;		///< return true for valid entry
	bool	isLink(int,int) const;		///< return true for valid link
	bool	isRlink(int,int) const;		///< true for link to rtr
	bool	isLlink(int,int) const;		///< true for link to local rtr
	bool	isClink(int,int) const;		///< true for core link
	bool	inComt(int,int) const;		///< return true if link in comt

	/** return various sets of links */
	int	links(int,uint16_t*,int) const;	///< return vector of links
	int	rlinks(int,uint16_t*,int) const;///< return vec of rtr links
	int	llinks(int,uint16_t*,int) const;///< return vec of local rtr links
	int	clinks(int,uint16_t*,int) const;///< return vec of core links

	/** input/output of table contents */
	bool 	readTable(istream&);
	void	writeTable(ostream&) const;
private:
	int	maxte;			///< largest table entry number
	struct tblEntry { 
		int comt;		///< comtree for this table entry
		int plnk;		///< parent link in comtree
		bool cFlag;		///< true if this router is in core
		int qn;			///< number of comtree queue
		int links;		///< bit vector of links
		int rlinks;		///< bit vec of links to other routers
		int llinks;		///< bit vec of links to local routers
		int clinks;		///< bit vec to neighboring core routers
	} *tbl;				///< tbl[i] contains data for table entry
	int free;			///< start of free list

	fAdr_t	myAdr;			///< forest address of this router
	lnkTbl	*lt;			///< pointer to link table
	qMgr	*qm;			///< pointer to queue manager
	hashTbl	*ht;			///< pointer to hash table for fast lookup

	/** helper functions */
	uint64_t hashkey(comt_t) const;
	int	listLinks(int,uint16_t*,int) const;
	bool 	readEntry(istream&);
	void	writeEntry(ostream&, const int) const;
	int	readLinks(istream&);	
	void	writeLinks(ostream&,int) const;
};

/** Add the link to the set of valid links for specified entry
 *
 *  @param entry is number of table entry to be modified
 *  @param link is the number of the link to add
 *  @param rflg is true if far end of link is another router
 *  @param cflg is true if far end of link is a core router for this comtree
 */
inline void ComtTbl::addLink(int entry, int lnk, bool rflg, bool cflg) {
	if (entry > 0 && entry <= maxte && valid(entry)) {
		tbl[entry].links |= (1 << lnk);
		if (rflg) tbl[entry].rlinks |= (1 << lnk);
		if (rflg && cflg) tbl[entry].clinks |= (1 << lnk);
	}
}

/** Remove the link from the set of valid links for specified entry.
 *
 *  @param entry is number of table entry to be modified
 *  @param link is the number of the link to add
 */
inline void ComtTbl::removeLink(int entry, int lnk) {
	if (entry > 0 && entry <= maxte && valid(entry)) {
		tbl[entry].links &= ~(1 << lnk);
		tbl[entry].rlinks &= ~(1 << lnk);
		tbl[entry].clinks &= ~(1 << lnk);
	}
}

inline comt_t ComtTbl::getComtree(int entry) const {
	assert(valid(entry)); return tbl[entry].comt;
}

inline int ComtTbl::getPlink(int entry) const {
	assert(valid(entry)); return tbl[entry].plnk;
}

inline bool ComtTbl::getCoreFlag(int entry) const {
	assert(valid(entry)); return tbl[entry].cFlag;
}

inline int ComtTbl::getQnum(int entry) const {
	assert(valid(entry)); return tbl[entry].qn;
}

inline void ComtTbl::setPlink(int entry, int p) {
	assert(valid(entry)); tbl[entry].plnk = p;
}

inline void ComtTbl::setCoreFlag(int entry, bool f) {
	assert(valid(entry)); tbl[entry].cFlag = f;
}

inline void ComtTbl::setQnum(int entry, int q) {
	assert(valid(entry)); tbl[entry].qn = q;
}

inline bool ComtTbl::valid(int entry) const {
	return entry > 0 && entry <= maxte && tbl[entry].qn != 0;
}

/** return true for valid link */
inline bool ComtTbl::isLink(int entry, int lnk) const {
	return (tbl[entry].links & (1 << lnk)) ? true : false;
}

/** return true for link to rtr */
inline bool ComtTbl::isRlink(int entry, int lnk) const {
	return (tbl[entry].rlinks & (1 << lnk)) ? true : false;
}

/** return true for link to rtr in same zip code */
inline bool ComtTbl::isLlink(int entry, int lnk) const {
	return (tbl[entry].llinks & (1 << lnk)) ? true : false;
}

/** return true for core link */
inline bool ComtTbl::isClink(int entry, int lnk) const {
	return (tbl[entry].clinks & (1 << lnk)) ? true : false;
}

/** return true for link in comtree */
inline bool ComtTbl::inComt(int entry, int lnk) const {
	return entry > 0 && entry <= maxte && valid(entry)
		        && (tbl[entry].links & (1<<lnk));
}

/** Return all the comtree links for the specified comtree. 
 *  Note that this requires a comtree table entry number. 
 *  Lnks points to a vector that will hold the links on return. 
 *  The limit argument specifies the maximum number of links that
 *  may be returned. If the table entry specifies more links, the 
 *  extra ones will be ignored. The value returned by the function 
 *  is the number of links that were returned. 
 */
inline int ComtTbl::links(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].links,lnks,limit);
}

/** Return all the comtree links that go to other routers. */
inline int ComtTbl::rlinks(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].rlinks,lnks,limit);
}

/** Return all the comtree links that go to other routers in same zip code */
inline int ComtTbl::llinks(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].llinks,lnks,limit);
}

/** Return all the comtree links that go to core routers. */
inline int ComtTbl::clinks(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].clinks,lnks,limit);
}

/** Compute key for hash lookup */
inline uint64_t ComtTbl::hashkey(comt_t ct) const {
        return (uint64_t(ct) << 32) | ct;
}

/** Return index of table entry for given comtree 
 *  If no match, return Null. 
 */
inline int ComtTbl::lookup(comt_t ct) {
        return ht->lookup(hashkey(ct));
}

#endif
