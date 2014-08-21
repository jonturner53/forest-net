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
#include "Forest.h"
#include "Dlist.h"
#include "Hash.h"
#include "HashMap.h"
#include "LinkTable.h"

using namespace grafalgo;

namespace forest {


/** Class that implements a table of information on comtrees.
 *
 *  Table entries are accessed using a "comtree index", which
 *  can be obtained using the getCtx() method.
 *
 *  Information about the use of a link in a comtree can be
 *  accessed using a comtree link number. For example, using
 *  the comtree link number, you can get the queue identifier that
 *  is used for packets in a given comtree that are sent on
 *  on the link. Comtree link numbers can be obtained using
 *  the getClnkInfo() method.
 */
class ComtreeTable {
public:
	/// comtree link information
	struct ClnkInfo {		
	public:
		ClnkInfo();
		~ClnkInfo();
	string	toString() const;
	friend ostream& operator<<(ostream& out, const ClnkInfo& cli) {
                return out << cli.toString();
	}

	fAdr_t	dest;			///< if non-zero, allowed dest address
	int	qnum;			///< queue number used by comtree
	RateSpec rates;			///< rate spec for link (up=in,down=out)
	};

	///< comtree table entry
	struct Entry {
	public:
		Entry();
		~Entry();
	Entry&	operator=(const Entry&);
	Entry&	operator=(Entry&&);
	string	toString() const;
	friend	ostream& operator<<(ostream& out, const Entry& e) {
                return out << e.toString();
	}

	int	pLnk;			///< parent link in comtree
	int	pClnk;			///< corresponding cLnk value
	bool	coreFlag;		///< true if this router is in core

	HashMap<int,ClnkInfo,Hash::s32> *clMap;
					///< maps link# to comtree link info
	Dlist	*rtrLinks;		///< comtree links to other routers
	Dlist	*coreLinks;		///< comtree links to core routers
	};

		ComtreeTable(int, int);
		~ComtreeTable();

	// predicates
	bool	validComtree(comt_t) const;		
	bool	validCtx(int) const;		
	bool	validClnk(int, int) const;		
	bool	checkEntry(int) const;
	bool	inCore(int) const;
	bool	isLink(int, int) const;
	bool	isRtrLink(int, int) const;
	bool	isCoreLink(int, int) const;

	// iteration methods
	int	firstComt() const;
	int	nextComt(int) const;
	int	firstComtLink(int) const;
	int	nextComtLink(int, int) const;
	int	firstRtrLink(int) const;
	int	nextRtrLink(int, int) const;
	int	firstCoreLink(int) const;
	int	nextCoreLink(int, int) const;

	// access routines 
	int	getComtIndex(comt_t) const;		
	Entry&	getEntry(int) const;
	comt_t	getComtree(int) const;
	int	getLink(int, int) const;
	int	getPlink(int) const;
	int	getPClnk(int) const;
	int	getLinkCount(int) const;
	int	getClnkNum(int, int) const;
	ClnkInfo& getClnkInfo(int, int) const;
	int	getLinkQ(int, int) const;
	int	getClnkQ(int, int) const;
	fAdr_t	getDest(int, int) const;
	RateSpec getRates(int, int) const;
	const Dlist& getComtList(int) const;

	// add/remove/modify table entries
	int	addEntry(comt_t);
	bool	removeEntry(int);
	bool 	addLink(int, int, bool, bool);
	bool 	removeLink(int, int);
	void	setCoreFlag(int, bool);
	void	setPlink(int, int);
	void	setLinkQ(int, int, int);
	void	purgeLink(int);

	// input/output of table contents 
	bool 	read(istream&);
	string	toString() const;
	string	entry2string(int) const;
private:
	int	maxLnk;			///< maximum link number
	int	maxCtx;			///< maximum comtree index
	HashMap<comt_t,Entry,Hash::u32> *comtMap;
					///< maps comtree number to table entry
	Dlist	*comtList;		///< comtList[lnk] lists all comtrees
					///< that use lnk

	/** helper functions */
	bool 	readEntry(istream&);
	void	readLinks(istream&, set<int>&);	
	string	links2string(int) const;
};

/** Constructor for ClnkInfo objects. */
inline ComtreeTable::ClnkInfo::ClnkInfo() {
	dest = 0; qnum = 0;
}

/** Destructor for ClnkInfo objects. */
inline ComtreeTable::ClnkInfo::~ClnkInfo() { }

/** Create a string representation of a ClnkInfo object.
 *  The route set is not included in the string, but its size is.
 *  @return the string
 */
inline string ComtreeTable::ClnkInfo::toString() const {
	string s = "[" + Forest::fAdr2string(dest) + " "
		   + rates.toString() + "]";
	return s;
}

/** Constructor for Entry objects. */
inline ComtreeTable::Entry::Entry() {
	pLnk = 0; pClnk = 0; coreFlag = false;
	clMap = new HashMap<int,ClnkInfo,Hash::s32>;
	rtrLinks = new Dlist(); coreLinks = new Dlist();
}

/** Destructor for Entry objects. */
inline ComtreeTable::Entry::~Entry() {
	delete clMap; delete rtrLinks; delete coreLinks;
}

/** Assignment operator for Entry objects (copy version).
 *  @param src is the object to be copied to this object
 *  @return a reference to this object.
 */
inline ComtreeTable::Entry& ComtreeTable::Entry::operator=(const Entry& src) {
	pLnk = src.pLnk; pClnk = src.pClnk; coreFlag = src.coreFlag;
	*clMap = *(src.clMap);
	*rtrLinks = *(src.rtrLinks);
	*coreLinks = *(src.coreLinks);
	return *this;
}

/** Assignment operator for Entry objects (move version).
 *  @param src is the object to be copied to this object
 *  @return a reference to this object.
 */
inline ComtreeTable::Entry& ComtreeTable::Entry::operator=(Entry&& src) {
	pLnk = src.pLnk; pClnk = src.pClnk; coreFlag = src.coreFlag;
	delete clMap; clMap = src.clMap; src.clMap = nullptr;
	delete rtrLinks; rtrLinks = src.rtrLinks; src.rtrLinks = nullptr;
	delete coreLinks; coreLinks = src.coreLinks; src.coreLinks = nullptr;
	return *this;
}

/** Create a string representation of an Entry object.
 *  Includes the core flag (leading asterisk if true), the parent link #
 *  and the list of links in the comtree.
 *  @return the string
 */
inline string ComtreeTable::Entry::toString() const {
	string s = (coreFlag ? "* " : " ") + to_string(pLnk) + " {";
	for (int cLnk = clMap->first(); cLnk != 0; cLnk = clMap->next(cLnk)) {
		ClnkInfo& cli = clMap->getValue(cLnk);
		if (cLnk != clMap->first()) s += " ";
		s += to_string(clMap->getKey(cLnk));
		if (coreLinks->member(cLnk)) s += "*";
		else if (rtrLinks->member(cLnk)) s += "+";
		s += "[" + Forest::fAdr2string(cli.dest) + " "
		  +  cli.rates.toString() + "]";
	}
	s += "}";
	return s;
}

/** Determine if the table has an entry for a given comtree.
 *  @param comt is a comtree number
 *  @return true if table contains an entry for comt, else false.
 */
inline bool ComtreeTable::validComtree(comt_t comt) const {
	return comtMap->contains(comt);
}

/** Determine if a comtree index is being used in this table.
 *  @param ctx is a comtree index
 *  @return true if the table contains an entry matching ctx, else false.
 */
inline bool ComtreeTable::validCtx(int ctx) const {
	return comtMap->valid(ctx);
}

/** Determine if a comtree link number is being used by a given comtree.
 *  @param ctx is a comtree index
 *  @param cLnk is a comtree link number
 *  @return true if cLnk is a valid comtree link number for the specified
 *  comtree, else false
 */
inline bool ComtreeTable::validClnk(int ctx, int cLnk) const {
	Entry& e = getEntry(ctx);
	return e.clMap->valid(cLnk);
}

/** Determine if "this node" is in the core of the comtree.
 *  @param ctx is a comtree index
 *  @return true if the router is in the core, else false
 */
inline bool ComtreeTable::inCore(int ctx) const {
	Entry& e = getEntry(ctx); return e.coreFlag;
}

/** Determine if a given link is part of a given comtree.
 *  @param entry is the comtree index
 *  @param lnk is the link number
 *  @return true if the specified link is part of the comtree
 */
inline bool ComtreeTable::isLink(int ctx, int lnk) const {
	if (!validCtx(ctx)) return false;
	Entry& e = getEntry(ctx);
	return e.clMap->contains(lnk);
}

/** Determine if a given comtree link connects to another router.
 *  @param ctx is the comtree index of the relevant comtree
 *  @param cLnk is the comtree link index
 *  @return true if the specified link connects to another router
 */
inline bool ComtreeTable::isRtrLink(int ctx, int cLnk) const {
	if (cLnk == 0 || !validCtx(ctx)) return false;
	Entry& e = getEntry(ctx);
	return e.rtrLinks->member(cLnk);
}

/** Determine if a given comtree link connects to a core node.
 *  @param ctx is the comtree index
 *  @param cLnk is the comtree link number
 *  @return true if the peer node for the link is in the comtree core
 */
inline bool ComtreeTable::isCoreLink(int ctx, int cLnk) const {
	if (!validCtx(ctx)) return false;
	Entry& e = getEntry(ctx);
	return e.coreLinks->member(cLnk);
}
	
/** Get the first comtree index.
 *  This method is used to iterate through all the comtrees.
 *  The order of the comtree indices is arbitrary.
 *  @return the first comtree index or 0, if no comtrees
 *  have been defined
 */
inline int ComtreeTable::firstComt() const {
	return comtMap->first();
}

/** Get the next comtree index.
 *  This method is used to iterate through all the comtrees.
 *  The order of the comtree indices is arbitrary.
 *  @param ctx is a comtree index
 *  @return the next comtree index following ctx, or 0 if there
 *  is no next index
 */
inline int ComtreeTable::nextComt(int ctx) const {
	return comtMap->next(ctx);
}

/** Get the comtree number for a given table entry.
 *  @param ctx is the comtree index
 *  @return the comtree number
 */
inline comt_t ComtreeTable::getComtree(int ctx) const {
	return comtMap->getKey(ctx);
}

/** Get the comtree table entry for a given comtree, using its index.
 *  @param cts is the comtree index
 *  @return a reference to the comtree table entry
 */
inline ComtreeTable::Entry& ComtreeTable::getEntry(int ctx) const {
	return comtMap->getValue(ctx);
}

/** Get the comtree index, based on the comtree number.
 *  @param comt is the comtree number
 *  @return the comtree index or 0 if not a valid comtree
 */
inline int ComtreeTable::getComtIndex(comt_t comt) const {
	return comtMap->find(comt);
}

/** Get the number of links that belong to this comtree.
 *  @param ctx is the comtree index
 *  @return the number of links in the comtree
 */
inline int ComtreeTable::getLinkCount(int ctx) const {
	Entry& e = getEntry(ctx);
	return e.clMap->size();
}

/** Get the comtree link number for a given (comtree, link) pair.
 *  @param comt is a comtree number
 *  @param lnk is a link number
 *  @return the comtree link number for the given pair, or 0 if there
 *  is no comtree link for the given pair
 */
inline int ComtreeTable::getClnkNum(int comt, int lnk) const {
	int ctx = getComtIndex(comt);
	if (ctx == 0) return 0;
	Entry& e = getEntry(ctx);
	return e.clMap->find(lnk);
}

/** Get the comtree link info for a specfic comtree link.
 *  @param ctx is a comtree index
 *  @param cLnk is a comtree link number for comtree ctx
 *  @return a reference to the ClnkInfo object for cLnk.
 */
inline ComtreeTable::ClnkInfo&
ComtreeTable::getClnkInfo(int ctx, int cLnk) const {
	Entry& e = getEntry(ctx);
	return e.clMap->getValue(cLnk);
}

/** Get the parent link for a comtree.
 *  @param ctx is the comtree index
 *  @return the link leading to the parent (in the comtree) of this router;
 *  returns 0 if the router is the root of the comtree
 */
inline int ComtreeTable::getPlink(int ctx) const { return getEntry(ctx).pLnk; }

/** Get the comtree link number for the parent link in a comtree.
 *  @param ctx is the comtree index
 *  @return the comtree link number for the link leading to the parent
 *  (in the comtree) of this router;
 *  returns 0 if the router is the root of the comtree
 */
inline int ComtreeTable::getPClnk(int ctx) const {
	return getEntry(ctx).pClnk;
}

/** Get the link number for a given comtree link.
 *  @param ctx is a comtree index
 *  @param cLnk is a comtree link number
 *  @return the number of the associated link
 */
inline int ComtreeTable::getLink(int ctx, int cLnk) const {
	if (cLnk == 0) return 0;
	Entry& e = getEntry(ctx);
	return e.clMap->getKey(cLnk);
}

/** Get the queue identifier for a given comtree link.
 *  @param ctx is a comtree index
 *  @param cLnk is a comtree link number
 *  @return the queue identifier assigned to cLnk
 */
inline int ComtreeTable::getClnkQ(int ctx, int cLnk) const {
	if (cLnk == 0) return 0;
	return getEntry(ctx).clMap->getValue(cLnk).qnum;
}

/** Get the queue identifier for a given comtree link.
 *  @param ctx is a comtree index
 *  @param lnk is a comtree link number
 *  @return the queue identifier assigned to cLnk
 */
inline int ComtreeTable::getLinkQ(int ctx, int lnk) const {
	if (lnk == 0) return 0;
	return getEntry(ctx).clMap->get(lnk).qnum;
}

/** Get the allowed destination for packets received on a given comtree link.
 *  @param ctx is a comtree index
 *  @param cLnk is a comtree link number
 *  @return the forest address of the designated "allowed destination"
 *  for this link
 */
inline fAdr_t ComtreeTable::getDest(int ctx, int cLnk) const {
	if (cLnk == 0) return 0;
	return getEntry(ctx).clMap->getValue(cLnk).dest;
}

/** Get the rate spec for a given comtree link.
 *  @param ctx is a comtree index
 *  @param cLnk is a comtree link number
 *  @return a reference to the rate spec for cLnk
 */
inline RateSpec ComtreeTable::getRates(int ctx, int cLnk) const {
	return getEntry(ctx).clMap->getValue(cLnk).rates;
}

/** Get a list of comtrees that use a specified link.
 *  @param lnk is a link number
 *  @return a constant reference to a list of the comtrees that use lnk.
 */
inline const Dlist& ComtreeTable::getComtList(int lnk) const {
	return comtList[lnk];
}

/** Get the first comtree link number for a given comtree.
 *  @param ctx is a comtree index
 *  @return the first comtree link number for the specified comtree
 */
inline int ComtreeTable::firstComtLink(int ctx) const {
	return getEntry(ctx).clMap->first();
}

/** Get the next comtree link number for a given comtree.
 *  @param ctx is a comtree index
 *  @param cLnk is a comtree link that is defined for the comtree
 *  @return the next comtree link number following cLnk
 */
inline int ComtreeTable::nextComtLink(int ctx, int cLnk) const {
	return getEntry(ctx).clMap->next(cLnk);
}

/** Get the first comtree link number going to a router.
 *  @param ctx is a comtree index
 *  @return the first comtree link number that goes to a router.
 */
inline int ComtreeTable::firstRtrLink(int ctx) const {
	return getEntry(ctx).rtrLinks->first();
}

/** Get the next comtree link number going to a router.
 *  @param ctx is a comtree index
 *  @param cLnk is a comtree link that is defined for the comtree
 *  @return the next comtree link number following cLnk
 */
inline int ComtreeTable::nextRtrLink(int ctx, int cLnk) const {
	return getEntry(ctx).rtrLinks->next(cLnk);
}

/** Get the first comtree link number going to a core router.
 *  @param ctx is a comtree index
 *  @return the first comtree link number that goes to a core router.
 */
inline int ComtreeTable::firstCoreLink(int ctx) const {
	return getEntry(ctx).coreLinks->first();
}

/** Get the next comtree link number going to a router.
 *  @param ctx is a comtree index
 *  @param cLnk is a comtree link that is defined for the comtree
 *  @return the next comtree link number following cLnk
 */
inline int ComtreeTable::nextCoreLink(int ctx, int cLnk) const {
	return getEntry(ctx).coreLinks->next(cLnk);
}

/** Set the parent link for a given table entry.
 *  @param ctx is the comtree index
 *  @param plink is the number of the link to this router's parent
 *  in the comtree
 */
inline void ComtreeTable::setPlink(int ctx, int plink) {
	if (!validCtx(ctx)) return;
	Entry& e = getEntry(ctx);
	if (plink == 0) { e.pLnk = 0; e.pClnk = 0; return; }
	int cLnk = e.clMap->find(plink);
	if (cLnk == 0 || !e.rtrLinks->member(cLnk)) return;
	e.pLnk = plink; e.pClnk = cLnk;
}

/** Set the core flag for a given table entry.
 *  @param ctx is the comtree index
 *  @param f is the new value of the core flag for this entry
 */
inline void ComtreeTable::setCoreFlag(int ctx, bool f) {
	if (validCtx(ctx)) getEntry(ctx).coreFlag = f;
}

/** Set the queue number for a comtree link.
 *  @param ctx is a comtree index
 *  @param cLnk is a comtree link number
 *  @param q is a queue number
 */
inline void ComtreeTable::setLinkQ(int ctx, int cLnk, int q) {
	if (!validCtx(ctx)) return;
	Entry& e = getEntry(ctx);
	if (validClnk(ctx,cLnk)) e.clMap->getValue(cLnk).qnum = q;
}

} // ends namespace

#endif
