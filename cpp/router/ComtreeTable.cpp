/** @file ComtreeTable.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ComtreeTable.h"

/** Constructor for ComtreeTable, allocates space and initializes table. */
ComtreeTable::ComtreeTable(int maxCtx1, int maxComtLink1, LinkTable *lt1)
			   : maxCtx(maxCtx1), maxComtLink(maxComtLink1),
			     lt(lt1) {
	tbl = new TblEntry[maxCtx+1];
	comtMap = new IdMap(maxCtx);
	clTbl = new ComtLinkInfo[maxComtLink+1];
	clMap = new IdMap(maxComtLink);
};
	
/** Destructor for ComtreeTable, frees dynamic storage */
ComtreeTable::~ComtreeTable() {
	delete [] tbl; delete comtMap; delete [] clTbl; delete clMap;
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
	int ctx = comtMap->addPair(key(comt));
	if (ctx == 0) return 0;

	tbl[ctx].comt = comt;
	tbl[ctx].plnk = 0;
	tbl[ctx].cFlag = false;
	tbl[ctx].comtLinks = new set<int>();
	tbl[ctx].rtrLinks = new set<int>();
	tbl[ctx].coreLinks = new set<int>();

	return ctx;
}

/** Remove a table entry.
 *  This method requires that all links have been previously
 *  removed from the comtree;
 *  @param ctx is the index of the entry to be deleted
 *  @return true on success, false on failure
 */
bool ComtreeTable::removeEntry(int ctx) {
	if (!validComtIndex(ctx)) return true;
	
	set<int>& comtLinks = *tbl[ctx].comtLinks;
	set<int>::iterator p;
	// copy out the comtree links, then remove them
	// so long as none has any registered routes
	int *clnks = new int[comtLinks.size()];
	int i = 0;
	for (p = comtLinks.begin(); p != comtLinks.end(); p++) {
		if (clTbl[*p].rteSet->size() != 0) return false;
		clnks[i++] = *p;
	}
	while(--i >= 0) removeLink(ctx,clnks[i]);
	delete [] clnks;

	comtMap->dropPair(key(tbl[ctx].comt));
	delete tbl[ctx].comtLinks;
	delete tbl[ctx].rtrLinks;
	delete tbl[ctx].coreLinks;
	return true;
}

/** Add the link to the set of links for a comtree.
 *
 *  @param ctx is comtree index of the comtree to be modified
 *  @param link is the number of the link to add
 *  @param rflg is true if far end of link is another router
 *  @param cflg is true if far end of link is a core router for this comtree
 */
bool ComtreeTable::addLink(int ctx, int lnk, bool rflg, bool cflg) {
        if (!validComtIndex(ctx)) return false;

	int cLnk = clMap->addPair(key(getComtree(ctx),lnk));
	if (cLnk == 0) return false;

	tbl[ctx].comtLinks->insert(cLnk);
	clTbl[cLnk].ctx = ctx; clTbl[cLnk].lnk = lnk;
	clTbl[cLnk].dest = 0; clTbl[cLnk].qnum = 0;
	clTbl[cLnk].rteSet = new set<int>;
	lt->registerComt(lnk,ctx);

        if (rflg) tbl[ctx].rtrLinks->insert(cLnk);
        if (cflg) tbl[ctx].coreLinks->insert(cLnk);

        return true;
}

/** Remove a comtree link from the set of valid links for a comtree.
 *  This method requires that there are no multicast routes
 *  registered with this comtree.
 *  @param ctx is comtree index of the comtree to be modified
 *  @param cLnk is the number of the comtree link to removed
 *  @param return true on success, false on failure
 */
bool ComtreeTable::removeLink(int ctx, int cLnk) {
        if (!validComtIndex(ctx) || !validComtLink(cLnk))
		return false;

	if (clTbl[cLnk].rteSet->size() != 0) return false;
        tbl[ctx].comtLinks->erase(cLnk);
        tbl[ctx].rtrLinks->erase(cLnk);
        tbl[ctx].coreLinks->erase(cLnk);
        delete clTbl[cLnk].rteSet;
	lt->deregisterComt(getLink(cLnk),ctx);

	clMap->dropPair(key(getComtree(ctx),getLink(cLnk)));
	return true;
}

/** Perform consistency check on table entry.
 *
 *  @param ctx is the number of the entry to be checked
 *  @return true if the entry is consistent, else false
 */
bool ComtreeTable::checkEntry(int ctx) const {
	if (!validComtIndex(ctx)) return false;

	// every router link must be a comtree link
	set<int>& rtrLinks  = *tbl[ctx].rtrLinks;
	set<int>& coreLinks = *tbl[ctx].coreLinks;
	set<int>::iterator p;
	for (p = rtrLinks.begin(); p != rtrLinks.end(); p++) {
		if (!isLink(ctx,getLink(*p))) return false;
	}
	// and every core link must be a router link
	for (p = coreLinks.begin(); p != coreLinks.end(); p++) {
		if (!isRtrLink(ctx,getLink(*p))) return false;
	}

	// parent must be a router
	int plnk = getPlink(ctx);
	if (plnk != 0 && !isRtrLink(ctx,plnk)) return false;

	if (inCore(ctx)) {
		// parent of a core router must be a core router
		if (plnk != 0 && !isCoreLink(ctx,plnk)) return false;
	} else {
		// and a non-core router has at most one core link
		int n = tbl[ctx].coreLinks->size();
		if (n > 1) return false;
		// and if it has a core link, it must be the parent link
		if (n == 1 && !isCoreLink(ctx,plnk)) return false;
	}
	return true;
}

/** Read a list of links from an input stream.
 *  The list consists of integers separated by commas.
 *  The first number that is not immediately followed by a comma
 *  is assumed to be the last one on the list.
 *  @param in is an open input stream
 *  @param links is a reference to a set of integers; on return,
 *  it will contain the list of link numbers read from the input
 */
void ComtreeTable::readLinks(istream& in, set<int>& links) {
	do {
		int lnk;
		if (!Misc::readNum(in,lnk)) return;
		links.insert(lnk);
	} while (Misc::verify(in,','));
}

/** Read an entry from an input stream and initialize its table entry.
 *  The entry must be on a line by itself (possibly with a trailing comment).
 *  An entry consists of a comtree number, a core flag (0 or 1), the link
 *  number of the link leading to the parent, and two comma-separated lists
 *  of links.
 *  @param in is an open input stream
 */
bool ComtreeTable::readEntry(istream& in) {
	int ct, cFlg, plnk;

        Misc::skipBlank(in);
        if (!Misc::readNum(in, ct) || ct < 1 || 
	    !Misc::readNum(in, cFlg) ||
	    !Misc::readNum(in, plnk))
                return false;

	set<int> comtLinks; readLinks(in, comtLinks);
	set<int> coreLinks; readLinks(in, coreLinks);

	Misc::cflush(in,'\n');

	int ctx = addEntry(ct);
	if (ctx == 0) return false;
	setCoreFlag(ctx,cFlg);

	set<int>::iterator p;
	for (p = comtLinks.begin(); p != comtLinks.end(); p++) {
		// if the link is in the core, also add it to the router list
		int lnk = *p;
		bool rtrFlag = (lt->getPeerType(lnk) == ROUTER);
		bool coreFlag = (coreLinks.find(lnk) != coreLinks.end());
		if (coreFlag && !rtrFlag) return false;
		if (!addLink(ctx,lnk,rtrFlag,coreFlag)) return false;
		int cLnk = getComtLink(ct,lnk);
		getRates(cLnk).set(Forest::MINBITRATE,Forest::MINBITRATE,
				   Forest::MINPKTRATE,Forest::MINPKTRATE);
	}
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
	Misc::skipBlank(in);
	if (!Misc::readNum(in,num)) return false;
	Misc::cflush(in,'\n');
	for (int i = 1; i <= num; i++) {
		if (!readEntry(in)) {
			cerr << "ComtreeTable::read: could not read "
			     << i << "-th comtree\n";
			return false;
		}
	}
        return true;
}

/** Write a table entry to an output stream.
 *  @param ctx is the comtree index of the comtree to be written
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& ComtreeTable::entry2string(int ctx,string& s) const {
	stringstream ss;
	ss << setw(9) << getComtree(ctx) << " "  
	  << setw(6) << inCore(ctx)
	  << setw(8) << getPlink(ctx) << "    ";

	set<int>& comtLinks = *tbl[ctx].comtLinks;
	set<int>::iterator p = comtLinks.begin();
	while (p != comtLinks.end()) {
		ss << getLink(*p++);
		if (p != comtLinks.end()) ss << ",";
	}

	ss << "   ";
	set<int>& coreLinks = *tbl[ctx].coreLinks;
	p = coreLinks.begin();
	if (p == coreLinks.end()) ss << "0";
	while (p != coreLinks.end()) {
		ss << getLink(*p++);
		if (p != coreLinks.end()) ss << ",";
	}
	ss << endl;
	s = ss.str();
	return s;
}

/** Create a string representation of the table
 *  @param s is a reference to a string in which the result is returned
 *  @return a reference to s
 */
string& ComtreeTable::toString(string& s) const {
	stringstream ss;
	ss << comtMap->size() << endl;
	ss << "# comtree  coreFlag  pLink  links"
	   << "            coreLinks" << endl;
	for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx))
                ss << entry2string(ctx,s);
	s = ss.str();
	return s;
}
