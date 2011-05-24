/** \file ComtreeTable.h
 * Comtree table class.
 *
 */

#ifndef COMTREETABLE_H
#define COMTREETABLE_H

#include "CommonDefs.h"
#include "QuManager.h"

class ComtreeTable {
public:
		ComtreeTable(int, fAdr_t, LinkTable*, QuManager*);
		~ComtreeTable();

	/** access routines */
	int	lookup(comt_t);		
	comt_t	getComtree(int) const;
	bool	getCoreFlag(int) const;
	int	getPlink(int) const;
	int	getQnum(int) const;
	int	getLinks(int,uint16_t*,int) const;	
	int	getRlinks(int,uint16_t*,int) const;
	int	getLlinks(int,uint16_t*,int) const;
	int	getClinks(int,uint16_t*,int) const;

	/** predicates */
	bool	valid(int) const;		
	bool	isLink(int,int) const;	
	bool	isRlink(int,int) const;
	bool	isLlink(int,int) const;
	bool	isClink(int,int) const;
	bool	inComt(int,int) const;	


	/** add/modify table entries */
	int	addEntry(comt_t);
	bool	removeEntry(int);
	bool	checkEntry(int) const;
	void 	addLink(int, int, bool, bool);
	void 	removeLink(int, int);
	void	setCoreFlag(int, bool);
	void	setPlink(int, int);
	void	setQnum(int, int);

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
	} *tbl;				///< tbl[i] contains data for entry i
	int free;			///< start of free list

	fAdr_t	myAdr;			///< forest address of this router
	LinkTable *lt;			///< pointer to link table
	QuManager *qm;			///< pointer to queue manager
	UiHashTbl *ht;			///< hash table for fast lookup

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
inline void ComtreeTable::addLink(int entry, int lnk, bool rflg, bool cflg) {
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
inline void ComtreeTable::removeLink(int entry, int lnk) {
	if (entry > 0 && entry <= maxte && valid(entry)) {
		tbl[entry].links &= ~(1 << lnk);
		tbl[entry].rlinks &= ~(1 << lnk);
		tbl[entry].clinks &= ~(1 << lnk);
	}
}

inline comt_t ComtreeTable::getComtree(int entry) const {
	assert(valid(entry)); return tbl[entry].comt;
}

inline int ComtreeTable::getPlink(int entry) const {
	assert(valid(entry)); return tbl[entry].plnk;
}

inline bool ComtreeTable::getCoreFlag(int entry) const {
	assert(valid(entry)); return tbl[entry].cFlag;
}

inline int ComtreeTable::getQnum(int entry) const {
	assert(valid(entry)); return tbl[entry].qn;
}

inline void ComtreeTable::setPlink(int entry, int p) {
	assert(valid(entry)); tbl[entry].plnk = p;
}

inline void ComtreeTable::setCoreFlag(int entry, bool f) {
	assert(valid(entry)); tbl[entry].cFlag = f;
}

inline void ComtreeTable::setQnum(int entry, int q) {
	assert(valid(entry)); tbl[entry].qn = q;
}

inline bool ComtreeTable::valid(int entry) const {
	return entry > 0 && entry <= maxte && tbl[entry].qn != 0;
}

/** return true for valid link */
inline bool ComtreeTable::isLink(int entry, int lnk) const {
	return (tbl[entry].links & (1 << lnk)) ? true : false;
}

/** return true for link to rtr */
inline bool ComtreeTable::isRlink(int entry, int lnk) const {
	return (tbl[entry].rlinks & (1 << lnk)) ? true : false;
}

/** return true for link to rtr in same zip code */
inline bool ComtreeTable::isLlink(int entry, int lnk) const {
	return (tbl[entry].llinks & (1 << lnk)) ? true : false;
}

/** return true for core link */
inline bool ComtreeTable::isClink(int entry, int lnk) const {
	return (tbl[entry].clinks & (1 << lnk)) ? true : false;
}

/** return true for link in comtree */
inline bool ComtreeTable::inComt(int entry, int lnk) const {
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
inline int ComtreeTable::getLinks(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].links,lnks,limit);
}

/** Return all the comtree links that go to other routers. */
inline int ComtreeTable::getRlinks(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].rlinks,lnks,limit);
}

/** Return all the comtree links that go to other routers in same zip code */
inline int ComtreeTable::getLlinks(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].llinks,lnks,limit);
}

/** Return all the comtree links that go to core routers. */
inline int ComtreeTable::getClinks(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].clinks,lnks,limit);
}

/** Compute key for hash lookup */
inline uint64_t ComtreeTable::hashkey(comt_t ct) const {
        return (uint64_t(ct) << 32) | ct;
}

/** Return index of table entry for given comtree 
 *  If no match, return Null. 
 */
inline int ComtreeTable::lookup(comt_t ct) {
        return ht->lookup(hashkey(ct));
}

#endif
