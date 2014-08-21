/** @file ComtreeTable.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ComtreeTable.h"

namespace forest {

/** Constructor for ComtreeTable, allocates space and initializes table.
 *  @param maxCtx is the maximum number of entries
 *  @param lt1 is a pointer to the router's link table
 */
ComtreeTable::ComtreeTable(int maxLnk1, int maxCtx1)
	: maxLnk(maxLnk1), maxCtx(maxCtx1) {
	comtMap = new HashMap<comt_t,Entry,Hash::u32>(maxCtx,false);
	comtList = new Dlist[maxLnk];
	for (int i = 1; i <= maxLnk; i++) comtList[i].resize(maxCtx);
}
	
/** Destructor for ComtreeTable, frees dynamic storage */
ComtreeTable::~ComtreeTable() {
	delete comtMap; delete [] comtList;
}

/** Add a link to the set of links for a comtree.
 *
 *  @param ctx is comtree index of the comtree to be modified
 *  @param link is the number of the link to add
 *  @param rflg is true if far end of link is another router
 *  @param cflg is true if far end of link is a core router for this comtree
 *  @return true on success, else false
 */
bool ComtreeTable::addLink(int ctx, int lnk, bool rflg, bool cflg) {
	if (!validCtx(ctx)) return false;
	Entry& e = getEntry(ctx);
	ClnkInfo cli;
	int cLnk = e.clMap->put(lnk,cli);
	if (cLnk == 0) return false;

	if (rflg) e.rtrLinks->addLast(cLnk);
	if (cflg) e.coreLinks->addLast(cLnk);
	comtList[lnk].addLast(ctx);

	return true;
}

/** Remove a comtree link from the set of valid links for a comtree.
 *  @param ctx is comtree index of the comtree to be modified
 *  @param cLnk is the number of the comtree link to removed
 *  @param return true on success, false on failure
 */
bool ComtreeTable::removeLink(int ctx, int cLnk) {
	if (!validCtx(ctx)) return false;
	Entry& e = getEntry(ctx);
	if (!e.clMap->valid(cLnk)) return false;

	int lnk = e.clMap->getKey(cLnk);
	if (lnk == e.pLnk) {
		return removeEntry(ctx);
	}
	e.clMap->remove(lnk);
	e.rtrLinks->remove(cLnk);
	e.coreLinks->remove(cLnk);
	comtList[lnk].remove(ctx);
	return true;
}

/** Add a new entry to the table.
 *
 *  Attempts to add a new table entry. Can fail if the specified comtree
 *  number is already in use, or if the table has run out of space.
 *
 *  @param comt is the number of the comtree for which an entry is to be added
 *  @return the index of the new table entry or 0 on failure
 */
int ComtreeTable::addEntry(comt_t comt) {
	Entry e;
	int ctx = comtMap->put(comt,e);
	return ctx;
}

/** Remove a table entry.
 *  This method removes all comtree links from the comtree, before
 *  removing the comtree itself. This requires that the comtree links
 *  have previously had all their routes removed.
 *  @param ctx is the index of the entry to be deleted
 *  @return true on success, false on failure
 */
bool ComtreeTable::removeEntry(int ctx) {
	if (!validCtx(ctx)) return true;
	Entry& e = getEntry(ctx);

	// first remove ctx from comtList[lnk] for all links in comtree
	for (int clx = e.clMap->first(); clx != 0; clx = e.clMap->next(clx)) {
		int lnk = e.clMap->getKey(clx);
		comtList[lnk].remove(ctx);
	}

	// now clear the map and lists, then drop comt from comtree mapping
	e.clMap->clear(); e.rtrLinks->clear(); e.coreLinks->clear();
	
	int comt = comtMap->getKey(ctx);
	comtMap->remove(comt);
	return true;
}

/** Remove a link from all comtrees that currently use it.
 *  Caller is assumed to hold locks on the ComtreeTable.
 *  @param lnk is the number of the link to be purged
 */
void ComtreeTable::purgeLink(int lnk) {
	int ctx = comtList[lnk].first();
	while (ctx != 0 ) {
		Entry& e = getEntry(ctx);
		if (lnk == e.pLnk) { // skip for now
			ctx = comtList[lnk].next(ctx);
		} else {
			int cLnk = e.clMap->find(lnk);
			e.clMap->remove(lnk);
			e.rtrLinks->remove(cLnk);
			e.coreLinks->remove(cLnk);
			// now, carefully remove ctx from comtList[lnk]
			int x = ctx;
			ctx = comtList[lnk].next(ctx);
			comtList[lnk].remove(x);
		}
	}
	// now remove comtrees for which lnk==pLnk
	// note that removeEntry updates comtList[lnk]
	while (!comtList[lnk].empty()) {
		removeEntry(comtList[lnk].first());
	}
}

/** Perform consistency check on table entry.
 *
 *  @param ctx is the number of the entry to be checked
 *  @return true if the entry is consistent, else false
 */
bool ComtreeTable::checkEntry(int ctx) const {
	if (!validCtx(ctx)) return false;
	Entry& e = getEntry(ctx);

	for (int cLnk = firstRtrLink(ctx); cLnk != 0;
		 cLnk = nextRtrLink(ctx,cLnk)) {
		if (!e.clMap->valid(cLnk)) return false;
	}

	for (int cLnk = firstCoreLink(ctx); cLnk != 0;
		 cLnk = nextCoreLink(ctx,cLnk)) {
		if (!e.rtrLinks->member(cLnk)) return false;
	}

	// parent must be a router
	int plnk = getPlink(ctx);
	if (plnk != 0 && !isRtrLink(ctx,plnk)) return false;

	if (inCore(ctx)) {
		// parent of a core router must be a core router
		if (plnk != 0 && !isCoreLink(ctx,plnk)) return false;
	} else {
		// and a non-core router has at most one core link
		int n = e.coreLinks->length();
		if (n > 1) return false;
		// and if it has a core link, it must be the parent link
		if (n == 1 && !isCoreLink(ctx,plnk)) return false;
	}
	return true;
}

/** Read an entry from an input stream and initialize its table entry.
 *  The entry must be on a line by itself (possibly with a trailing comment).
 *  An entry consists of a comtree number, a core flag (0 or 1), the link
 *  number of the link leading to the parent, and two comma-separated lists
 *  of links.
 *  @param in is an open input stream
 */
bool ComtreeTable::readEntry(istream& in) {
	int comt, plnk;
	Entry e;

	Util::skipBlank(in);
	if (!Util::readInt(in, comt) || comt < 1) return false;
	if (Util::verify(in,'*')) e.coreFlag = true;
	if (!Util::readInt(in,plnk)) return false;

	fAdr_t defDest; RateSpec defRates;
	if (!Forest::readForestAdr(in,defDest) || !defRates.read(in))
		return false;

	if (!Util::verify(in,'{')) return false;
	ClnkInfo cli;
	while (true) {
		if (Util::verify(in,'}')) break;
		int lnk;
		if (!Util::readInt(in,lnk)) return false;
		bool isRouter = false; bool isCore = false;
		if (Util::verify(in,'+')) isRouter = true;
		else if (Util::verify(in,'*')) isRouter = isCore = true;
		// check for optional dest, rates
		fAdr_t dest = defDest; RateSpec rates=defRates;
		if (Util::verify(in,'[')) {
			if (!rates.read(in)) {
				if (!Forest::readForestAdr(in,dest))
					return false;
				rates.read(in);
			}
			if (!Util::verify(in,']')) return false;
		}
		cli.dest = dest; cli.rates = rates;
		int cLnk = e.clMap->put(lnk,cli);
		if (isRouter) e.rtrLinks->addLast(cLnk);
		if (isCore) e.coreLinks->addLast(cLnk);
	}
	Util::nextLine(in);

	int ctx = comtMap->put(comt,e);
	if (ctx == 0) return false;

	setPlink(ctx,plnk); // must be done after links are defined

	if (!checkEntry(ctx)) { removeEntry(ctx); return false; }
	
	return true;
}

/** Read comtree table entries from an input stream.
 *  The first line of the input must contain
 *  an integer, giving the number of entries to be read. The input may
 *  include blank lines and comment lines (any text starting with '#').
 *  The operation may fail if the input is misformatted or if
 *  an entry does not pass some basic sanity checks.
 *  In this case, an error message is printed that identifies the
 *  entry on which a problem was detected
 *  @param in is an open input stream
 *  @return true on success, false on failure
 */
bool ComtreeTable::read(istream& in) {
	int num;
	Util::skipBlank(in);
	if (!Util::readInt(in,num)) return false;
	Util::nextLine(in);
	for (int i = 1; i <= num; i++) {
		if (!readEntry(in)) {
			cerr << "ComtreeTable::read: could not read "
			     << i << "-th comtree\n";
			return false;
		}
	}
	return true;
}

/** Create a string representing a table entry
 *  @param ctx is the comtree index of the comtree to be written
 *  @return the string
 */
string ComtreeTable::entry2string(int ctx) const {
	stringstream ss;
	if (!validCtx(ctx)) return "";
	Entry& e = getEntry(ctx);
	int comt = comtMap->getKey(ctx);

	ss << comt << e.toString() << endl; 
	return ss.str();
}

/** Create a string representation of the table
 *  @return the string
 */
string ComtreeTable::toString() const {
	stringstream ss;
	ss << comtMap->size() << endl;
	ss << "# comtree  coreFlag  pLink  links" << endl;
	for (int ctx = firstComt(); ctx != 0; ctx = nextComt(ctx))
		ss << entry2string(ctx);
	return ss.str();
}

} // ends namespace

