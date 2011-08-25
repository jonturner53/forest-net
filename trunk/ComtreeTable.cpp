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
	
ComtreeTable::~ComtreeTable() { delete [] tbl; delete comtMap; delete clMap; }

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
 *
 *  @param ctx is the index of the entry to be deleted
 *  @return true on success, false on failure (that is, if there
 *  was no such entry in the first place)
 */
void ComtreeTable::removeEntry(int ctx) {
	if (!validComtIndex(ctx)) return;
	
	set<int>& comtLinks = *tbl[ctx].comtLinks;
	set<int>::iterator p;
	for (p = comtLinks.begin(); p != comtLinks.end(); p++)
		removeLink(ctx,*p);

	comtMap->dropPair(key(tbl[ctx].comt));
	delete tbl[ctx].comtLinks;
	delete tbl[ctx].rtrLinks;
	delete tbl[ctx].coreLinks;
}

/** Add the link to the set of valid links for specified entry
 *
 *  @param ctx is number of table entry to be modified
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

        if (rflg) tbl[ctx].rtrLinks->insert(cLnk);
        if (cflg) tbl[ctx].coreLinks->insert(cLnk);

        return true;
}

/** Remove a comtree link from the set of valid links for a specified entry.
 *
 *  @param ctx is number of table entry to be modified
 *  @param cLnk is the number of the comtree link to removed
 */
void ComtreeTable::removeLink(int ctx, int cLnk) {
        if (!validComtIndex(ctx) || !validComtLink(cLnk)) return;

        tbl[ctx].comtLinks->erase(cLnk);
        tbl[ctx].rtrLinks->erase(cLnk);
        tbl[ctx].coreLinks->erase(cLnk);

	clMap->dropPair(key(getComtree(ctx),getLink(cLnk)));
}

/** Perform consistency check on table entry.
 *
 *  @param ctx is the number of the entry to be checked
 *  @return true if the entry is consistent, else false
 */
bool ComtreeTable::checkEntry(int ctx) const {
	int i, n; uint16_t lnkvec[Forest::MAXLNK+1];

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

/** Read a list of links.
 */
void ComtreeTable::readLinks(istream& in, set<int>& links) {
	do {
		int lnk;
		if (!Misc::readNum(in,lnk)) return;
		links.insert(lnk);
	} while (Misc::verify(in,','));
}

/** Read a comtree from in and initialize its table entry.
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
		addLink(ctx,lnk,rtrFlag,coreFlag);
	}
	setPlink(ctx,plnk); // must be done after links are defined

	if (!checkEntry(ctx)) { removeEntry(ctx); return false; }
	
        return true;
}

/** Read comtree table entries from input stream. The first line must contain
 *  an integer, giving the number of entries to be read. The input may
 *  include blank lines and comment lines (any text starting with '#').
 *  Each entry must be on a line by itself (possibly with a trailing comment).
 *  An entry consists of a comtree number, a core flag (0 or 1), the link
 *  number of the link leading to the parent, a queue number, a queue quantum,
 *  and three comma-separated lists of links.
 */
bool ComtreeTable::read(istream& in) {
	int num;
	Misc::skipBlank(in);
	if (!Misc::readNum(in,num)) return false;
	Misc::cflush(in,'\n');
	for (int i = 1; i <= num; i++) {
		if (!readEntry(in)) {
			cerr << "Error reading comtree table entry # "
			     << i << endl;
			return false;
		}
	}
        return true;
}

/** Write entry for link i
 */
void ComtreeTable::writeEntry(ostream& out, int ctx) const {
	out << setw(9) << getComtree(ctx) << " "  
	   << setw(6) << inCore(ctx)
	   << setw(8) << getPlink(ctx) << "    ";

	set<int>& comtLinks = *tbl[ctx].comtLinks;
	set<int>::iterator p = comtLinks.begin();
	while (p != comtLinks.end()) {
		out << getLink(*p++);
		if (p != comtLinks.end()) out << ",";
	}

	out << "   ";
	set<int>& coreLinks = *tbl[ctx].coreLinks;
	p = coreLinks.begin();
	while (p != coreLinks.end()) {
		out << getLink(*p++);
		if (p != coreLinks.end()) out << ",";
	}
	out << endl;
}

/** Output human readable representation of comtree table.
 */
void ComtreeTable::write(ostream& out) const {
	out << comtMap->size() << endl;
	out << "# comtree  coreFlag  pLink  links"
	    << "            coreLinks" << endl;
	for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx))
                writeEntry(out,ctx);
}
