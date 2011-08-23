/** @file ComtreeTable.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef COMTREETABLE_H
#define COMTREETABLE_H

#include <set>
#include "CommonDefs.h"
#include "IdMap.h"
#include "LinkTable.h"

/** Class that implements a table of information on comtrees.
 *
 *  Provides a lookup method that uses a hash table to
 *  map a comtree number to an "entry number".
 *  The entry number can then be used to access the fields in
 *  an individual entry.
 */
class ComtreeTable {
public:
		ComtreeTable(int,int,LinkTable*);
		~ComtreeTable();

	// predicates
	bool	validComtree(comt_t) const;		
	bool	validComtIndex(int) const;		
	bool	validComtLink(int) const;		
	bool	checkEntry(int) const;
	bool	inCore(int) const;
	bool	isLink(int, int) const;
	bool	isRtrLink(int, int) const;
	bool	isRtrLink(int) const;
	bool	isCoreLink(int, int) const;
	bool	isCoreLink(int) const;

	// iteration methods
	int	firstComtIndex() const;
	int	nextComtIndex(int) const;

	// access routines 
	int	getComtIndex(comt_t) const;		
	comt_t	getComtree(int) const;
	int	getLink(int) const;
	int	getPlink(int) const;
	int	getLinkCount(int) const;
	int	getComtLink(int, int) const;
	int	getPCLnk(int) const;
	int	getLinkQ(int) const;
	int	getDest(int) const;
	int	getInBitRate(int) const;
	int	getInPktRate(int) const;
	int	getOutBitRate(int) const;
	int	getOutPktRate(int) const;
	set<int>& getLinks(int) const;
	set<int>& getRtrLinks(int) const;
	set<int>& getCoreLinks(int) const;

	// add/remove/modify table entries
	int	addEntry(comt_t);
	void	removeEntry(int);
	bool 	addLink(int, int, bool, bool);
	void 	removeLink(int, int);
	void	setCoreFlag(int, bool);
	void	setPlink(int, int);
	void	setLinkQ(int, int);
	void	setInBitRate(int, int);
	void	setInPktRate(int, int);
	void	setOutBitRate(int, int);
	void	setOutPktRate(int, int);

	// input/output of table contents 
	bool 	read(istream&);
	void	write(ostream&) const;
private:
	int	maxCtx;			///< largest comtree index
	int	maxComtLink;		///< largest comtLink

	struct TblEntry { 		///< comtree table entry
	int	comt;			///< comtree for this table entry
	int	plnk;			///< parent link in comtree
	int	pCLnk;			///< corresponding cLnk value
	bool	cFlag;			///< true if this router is in core
	set<int> *comtLinks;		///< list of comtLink #s in comtree
	set<int> *rtrLinks;		///< comtLinks to other routers
	set<int> *coreLinks;		///< comtLinks to core routers
	};
	TblEntry *tbl;
	IdMap *comtMap;			///< maps comtree numbers to indices

	struct ComtLinkInfo {		
	int	ctx;			///< index of comtree for this comtLink
	int	lnk;			///< actual link # for this comtLink
	fAdr_t	dest;			///< if non-zero, allowed dest address
	int	qnum;			///< queue number used by comtree
	int	inBitRate;		///< max incoming bit rate
	int	inPktRate;		///< max incoming packet rate
	int	outBitRate;		///< max outgoing bit rate
	int	outPktRate;		///< max outgoing packet rate
	};
	ComtLinkInfo *clTbl;		///< cLnkTbl[cl] has info on comtLink cl
	IdMap *clMap;			///< maps (comtree,link)->comtLink

	LinkTable *lt;

	/** helper functions */
	uint64_t key(comt_t) const;
	uint64_t key(comt_t, int) const;
	bool 	readEntry(istream&);
	void	writeEntry(ostream&, const int) const;
	void	readLinks(istream&, set<int>&);	
	void	writeLinks(ostream&,int) const;
};

inline bool ComtreeTable::validComtree(comt_t comt) const {
	return comtMap->validKey(key(comt));
}

inline bool ComtreeTable::validComtIndex(int ctx) const {
	return comtMap->validId(ctx);
}

inline bool ComtreeTable::validComtLink(int cLnk) const {
	return clMap->validId(cLnk);
}

inline bool ComtreeTable::inCore(int ctx) const { return tbl[ctx].cFlag; }

/** Determine if a given link is part of a given comtree.
 *  @param entry is the entry number
 *  @param lnk is the link number
 *  @return true if the specified link is part of the comtree
 */
inline bool ComtreeTable::isLink(int ctx, int lnk) const {
	if (!validComtIndex(ctx)) return false;
	return clMap->validKey(key(tbl[ctx].comt,lnk));
}

/** Determine if a given link connects to another router.
 *  @param ctx is the entry number
 *  @param lnk is the link number
 *  @return true if the specified link connects to another router
 */
inline bool ComtreeTable::isRtrLink(int ctx, int lnk) const {
	if (!validComtIndex(ctx)) return false;
	return isRtrLink(clMap->getId(key(tbl[ctx].comt,lnk)));
}

inline bool ComtreeTable::isRtrLink(int cLnk) const {
	if (cLnk == 0) return false;
	set<int>& rl = *tbl[clTbl[cLnk].ctx].rtrLinks;
	return (rl.find(cLnk) != rl.end());
}

/** Determine if a given link connects to a core node.
 *  @param ctx is the entry number
 *  @param lnk is the link number
 *  @return true if the peer node for the link is in the comtree core
 */
inline bool ComtreeTable::isCoreLink(int ctx, int lnk) const {
	if (!validComtIndex(ctx)) return false;
	return isCoreLink(clMap->getId(key(tbl[ctx].comt,lnk)));
}

inline bool ComtreeTable::isCoreLink(int cLnk) const {
	if (cLnk == 0) return false;
	set<int>& cl = *tbl[clTbl[cLnk].ctx].coreLinks;
	return (cl.find(cLnk) != cl.end());
}
	
inline int ComtreeTable::firstComtIndex() const {
	return comtMap->firstId();
}

inline int ComtreeTable::nextComtIndex(int ctx) const {
	return comtMap->nextId(ctx);
}

/** Get the comtree number for a given table entry.
 *  @param ctx is the comtree index
 *  @return the comtree number
 */
inline comt_t ComtreeTable::getComtree(int ctx) const {
	return tbl[ctx].comt;
}

inline int ComtreeTable::getComtIndex(comt_t comt) const {
	return comtMap->getId(key(comt));
}

/** Get the parent link for a given table entry.
 *  @param ctx is the entry number
 *  @return the link leading to the parent (in the comtree) of this router;
 *  returns 0 if the router is the root of the comtree
 */
inline int ComtreeTable::getPlink(int ctx) const { return tbl[ctx].plnk; }


inline int ComtreeTable::getLinkCount(int ctx) const {
	return tbl[ctx].comtLinks->size();
}

inline int ComtreeTable::getComtLink(int comt, int lnk) const {
	return clMap->getId(key(comt,lnk));
}

inline int ComtreeTable::getPCLnk(int ctx) const { return tbl[ctx].pCLnk; }

inline int ComtreeTable::getLink(int cLnk) const {
	return clTbl[cLnk].lnk;
}

inline int ComtreeTable::getLinkQ(int cLnk) const {
	return clTbl[cLnk].qnum;
}

inline int ComtreeTable::getDest(int cLnk) const {
	return clTbl[cLnk].dest;
}

inline int ComtreeTable::getInBitRate(int cLnk) const {
	return clTbl[cLnk].inBitRate;
}

inline int ComtreeTable::getInPktRate(int cLnk) const {
	return clTbl[cLnk].inPktRate;
}

inline int ComtreeTable::getOutBitRate(int cLnk) const {
	return clTbl[cLnk].outBitRate;
}

inline int ComtreeTable::getOutPktRate(int cLnk) const {
	return clTbl[cLnk].outPktRate;
}

inline set<int>& ComtreeTable::getLinks(int ctx) const {
	return *tbl[ctx].comtLinks;
}

inline set<int>& ComtreeTable::getRtrLinks(int ctx) const {
	return *tbl[ctx].rtrLinks;
}

inline set<int>& ComtreeTable::getCoreLinks(int ctx) const {
	return *tbl[ctx].coreLinks;
}


/** Set the parent link for a given table entry.
 *  @param ctx is the entry number
 *  @param plink is the number of the link to this router's parent
 *  in the comtree
 */
inline void ComtreeTable::setPlink(int ctx, int plink) {
	if (!validComtIndex(ctx)) return;
	int cLnk = clMap->getId(key(tbl[ctx].comt,plink));
	if (!isRtrLink(ctx,plink)) return;
	tbl[ctx].plnk = plink; tbl[ctx].pCLnk =cLnk;
}

/** Set the core flag for a given table entry.
 *  @param ctx is the entry number
 *  @param f is the new value of the core flag for this entry
 */
inline void ComtreeTable::setCoreFlag(int ctx, bool f) {
	if (validComtIndex(ctx)) tbl[ctx].cFlag = f;
}

/** Set the queue number for a comtree link.
 *  @param cLnk is a comtree link number
 *  @param q is a queue number
 */
inline void ComtreeTable::setLinkQ(int cLnk, int q) {
	if (validComtLink(cLnk)) clTbl[cLnk].qnum = q;
}

/** Set the incoming bit rate for a comtree link.
 *  @param cLnk is a comtree link number
 *  @param br is a bit rate
 */
inline void ComtreeTable::setInBitRate(int cLnk, int br) {
	if (validComtLink(cLnk)) clTbl[cLnk].inBitRate = br;
}

/** Set the incoming packet rate for a comtree link.
 *  @param cLnk is a comtree link number
 *  @param pr is a packet rate
 */
inline void ComtreeTable::setInPktRate(int cLnk, int pr) {
	if (validComtLink(cLnk)) clTbl[cLnk].inPktRate = pr;
}

/** Set the outgoing bit rate for a comtree link.
 *  @param cLnk is a comtree link number
 *  @param br is a bit rate
 */
inline void ComtreeTable::setOutBitRate(int cLnk, int br) {
	if (validComtLink(cLnk)) clTbl[cLnk].outBitRate = br;
}

/** Set the outgoing packet rate for a comtree link.
 *  @param cLnk is a comtree link number
 *  @param pr is a packet rate
 */
inline void ComtreeTable::setOutPktRate(int cLnk, int pr) {
	if (validComtLink(cLnk)) clTbl[cLnk].outPktRate = pr;
}

/** Compute key for use with comtMap.
 *  @param ct is a comtree number
 *  @return a 64 bit hash key
 */
inline uint64_t ComtreeTable::key(comt_t ct) const {
        return (uint64_t(ct) << 32) | ct;
}

/** Compute key for use with clMap.
 *  @param ct is a comtree number
 *  @param lnk is a link number
 *  @return a 64 bit hash key
 */
inline uint64_t ComtreeTable::key(comt_t ct, int lnk) const {
        return (uint64_t(ct) << 32) | lnk;
}

#endif
