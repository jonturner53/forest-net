/** @file ComtreeTable.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef COMTREETABLE_H
#define COMTREETABLE_H

#include "CommonDefs.h"
#include "QuManager.h"

/** Class that implements a table of information on comtrees.
 *
 *  Provides a lookup method that uses a hash table to
 *  map a comtree number to an "entry number".
 *  The entry number can then be used to access the fields in
 *  an individual entry.
 */
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
	int	getQuant(int) const;
	int 	getLinkCount(int) const;
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

	/** add/modify table entries */
	int	addEntry(comt_t);
	bool	removeEntry(int);
	bool	checkEntry(int) const;
	void 	addLink(int, int, bool, bool);
	void 	removeLink(int, int);
	void	setCoreFlag(int, bool);
	void	setPlink(int, int);
	void	setQnum(int, int);
	void	setQuant(int, int);

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
		int quant;		///< quantum associated with queue
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

/** Get the comtree number for a given table entry.
 *  @param entry is the entry number
 *  @return the comtree number
 */
inline comt_t ComtreeTable::getComtree(int entry) const {
	assert(valid(entry)); return tbl[entry].comt;
}

/** Get the parent link for a given table entry.
 *  @param entry is the entry number
 *  @return the link leading to the parent (in the comtree) of this router;
 *  returns 0 if the router is the root of the comtree
 */
inline int ComtreeTable::getPlink(int entry) const {
	assert(valid(entry)); return tbl[entry].plnk;
}

/** Get the core flag for a given table entry.
 *  @param entry is the entry number
 *  @return true if this router is in the core of the comtree, else false
 */
inline bool ComtreeTable::getCoreFlag(int entry) const {
	assert(valid(entry)); return tbl[entry].cFlag;
}

/** Get the queue number for a given table entry.
 *  @param entry is the entry number
 *  @return the queue used by packets sent on this comtree
 */
inline int ComtreeTable::getQnum(int entry) const {
	assert(valid(entry)); return tbl[entry].qn;
}

/** Get the quantum for this comtree.
 *  @param entry is the entry number
 *  @return the quantum associated with the comtree's queue
 */
inline int ComtreeTable::getQuant(int entry) const {
	assert(valid(entry)); return tbl[entry].quant;
}

/** Set the parent link for a given table entry.
 *  @param entry is the entry number
 *  @param p is the number of the link to this router's parent in the comtree
 */
inline void ComtreeTable::setPlink(int entry, int p) {
	assert(valid(entry)); tbl[entry].plnk = p;
}

/** Set the core flag for a given table entry.
 *  @param entry is the entry number
 *  @param f is the new value of the core flag for this entry
 */
inline void ComtreeTable::setCoreFlag(int entry, bool f) {
	assert(valid(entry)); tbl[entry].cFlag = f;
}

/** Set the queue number for a given table entry.
 *  @param entry is the entry number
 *  @param q is the number of the queue to use for packets in this comtree
 */
inline void ComtreeTable::setQnum(int entry, int q) {
	assert(valid(entry)); tbl[entry].qn = q;
}

/** Set the quantum for a given table entry.
 *  @param entry is the entry number
 *  @param q is the quantum for the comtree's queue
 */
inline void ComtreeTable::setQuant(int entry, int q) {
	assert(valid(entry)); tbl[entry].quant = q;
}

/** Determine if an entry number is valid or not.
 *  @param entry is the entry number
 *  @return true if the entry number is valid, else false
 */
inline bool ComtreeTable::valid(int entry) const {
	return entry > 0 && entry <= maxte && tbl[entry].qn != 0;
}

/** Determine if a given link is part of a given comtree.
 *  @param entry is the entry number
 *  @param lnk is the link number
 *  @return true if the specified link is part of the comtree
 */
inline bool ComtreeTable::isLink(int entry, int lnk) const {
	return (tbl[entry].links & (1 << lnk)) ? true : false;
}

/** Get the number of links in a specific comtree.
 *  @param entry is the entry number for the comgtree
 *  @return the number of links in this comtree
 */
inline int ComtreeTable::getLinkCount(int entry) const {
	int cnt = 0;
	for (uint32_t i = 1; i != 0; i <<= 1)
		if (tbl[entry].links & i) cnt++;
	return cnt;
}

/** Determine if a given link connects to another router.
 *  @param entry is the entry number
 *  @param lnk is the link number
 *  @return true if the specified link connects to another router
 */
inline bool ComtreeTable::isRlink(int entry, int lnk) const {
	return (tbl[entry].rlinks & (1 << lnk)) ? true : false;
}

/** Determine if a given link connects to a node in the same zip code
 *  @param entry is the entry number
 *  @param lnk is the link number
 *  @return true if the peer node at the far end of the link is in the
 *  same zip code
 */
inline bool ComtreeTable::isLlink(int entry, int lnk) const {
	return (tbl[entry].llinks & (1 << lnk)) ? true : false;
}

/** Determine if a given link connects to a core node.
 *  @param entry is the entry number
 *  @param lnk is the link number
 *  @return true if the peer node for the link is in the comtree core
 */
inline bool ComtreeTable::isClink(int entry, int lnk) const {
	return (tbl[entry].clinks & (1 << lnk)) ? true : false;
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

/** Get a vector of links to adjacent routers in the comtree.
 *  @param entry is the entry number
 *  @param lnks points to an array in which link numbers are stored
 *  @param limit is an upper bound on the number of links to be returned
 *  @return the number of links returned
 */
inline int ComtreeTable::getRlinks(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].rlinks,lnks,limit);
}

/** Get a vector of local links in the comtree.
 *  @param entry is the entry number
 *  @param lnks points to an array in which link numbers are stored
 *  @param limit is an upper bound on the number of links to be returned
 *  @return the number of links returned
 */
inline int ComtreeTable::getLlinks(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].llinks,lnks,limit);
}

/** Get a vector of links to adjacent core routers in the comtree.
 *  @param entry is the entry number
 *  @param lnks points to an array in which link numbers are stored
 *  @param limit is an upper bound on the number of links to be returned
 *  @return the number of links returned
 */
inline int ComtreeTable::getClinks(int entry, uint16_t* lnks, int limit) const {
        if (entry < 1 || entry > maxte || !valid(entry)) return 0;
	return listLinks(tbl[entry].clinks,lnks,limit);
}

/** Compute key for hash lookup.
 *  @param ct is a comtree number
 *  @return a 64 bit hash key
 */
inline uint64_t ComtreeTable::hashkey(comt_t ct) const {
        return (uint64_t(ct) << 32) | ct;
}

/** Lookup the table entry number for a given comtree.
 *  @param ct is a comtree number
 *  @return is the entry number for the given comtree, or zero if there is
 *  no match
 */
inline int ComtreeTable::lookup(comt_t ct) {
        return ht->lookup(hashkey(ct));
}

#endif
