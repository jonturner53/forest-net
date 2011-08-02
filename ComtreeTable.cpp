/** @file ComtreeTable.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ComtreeTable.h"

/** Constructor for ComtreeTable, allocates space and initializes table. */
ComtreeTable::ComtreeTable(int maxte1, fAdr_t myAdr1,
			   LinkTable *lt1, QuManager *qm1)
		 	  : maxte(maxte1), myAdr(myAdr1), lt(lt1), qm(qm1) {
	tbl = new tblEntry[maxte+1];
	ht = new UiHashTbl(maxte);

	free = 1;
	for (int entry = 1; entry <= maxte; entry++) {
		tbl[entry].links = entry+1;
		tbl[entry].qn = 0;		// 0 for invalid entry
	}
	tbl[maxte].links = 0;
};
	
ComtreeTable::~ComtreeTable() { delete [] tbl; delete ht; }

/** Add a new entry to the table.
 *
 *  Attempts to add a new table entry. Can fail if the specified comtree
 *  number is already in use, or if the table has run out of space.
 *
 *  @param ct is the number of the comtree for which an entry is to be added
 *  @return the index of the new table entry or 0 on failure
 */
int ComtreeTable::addEntry(comt_t ct) {
	if (ht->lookup(ct) != 0) return 0;
	if (free == 0) return 0;
	int entry = free; free = tbl[free].links;
	if (!ht->insert(hashkey(ct),entry)) {
		free = entry; return 0;
	}
	tbl[entry].comt = ct;
	tbl[entry].links = tbl[entry].rlinks = 0;
	tbl[entry].llinks = tbl[entry].clinks = 0;
	tbl[entry].cFlag = false; tbl[entry].plnk = 0;
	tbl[entry].qn = 1;
	return entry;
}

/** Remove a table entry.
 *
 *  @param entry is the index of the entry to be deleted
 *  @return true on success, false on failure (that is, if there
 *  was no such entry in the first place)
 */
bool ComtreeTable::removeEntry(int entry) {
	if (!valid(entry)) return false;
	ht->remove(hashkey(tbl[entry].comt));
	tbl[entry].qn = 0; tbl[entry].links = free; free = entry;
}

/** Perform consistency check on table entry.
 *
 *  Does a series of consistency checks using the comtree
 *  entry plus information about the comtree links obtained
 *  from the link table.
 *
 *  @param is the number of the entry to be checked
 *  @return true if the entry is consistent, else false
 */
bool ComtreeTable::checkEntry(int entry) const {
	int i, n; uint16_t lnkvec[Forest::MAXLNK+1];

	if (!valid(entry)) return false;
	// every link in comtree entry must be valid
	n = getLinks(entry,lnkvec,Forest::MAXLNK);
	for (i = 0; i < n; i++) {
		if (!lt->valid(lnkvec[i])) return false;
	}
	// every core link must be a comtree link
	n = getClinks(entry,lnkvec,Forest::MAXLNK);
	for (i = 0; i < n; i++) {
		if (!isLink(entry,lnkvec[i])) return false;
	}
	// parent of a core router must be a core router
	int plnk = getPlink(entry);
	if (getCoreFlag(entry)) {
		if (plnk != 0 && !isClink(entry,plnk)) return false;
	} else {
		// and a non-core router has at most one core link
		if (n > 1) return false;
		if (plnk == 0 || 
		    (n == 1 && lnkvec[0] != plnk))
			return false;
	}
	// every router link must be a comtree link
	// also, peer must be a router
	n = getRlinks(entry,lnkvec,Forest::MAXLNK);
	for (i = 0; i < n; i++) {
		if (!isLink(entry,lnkvec[i])) return false;
		if (lt->getPeerType(lnkvec[i]) != ROUTER) return false;
	}
	// parent must be a router
	if (plnk != 0 && !isRlink(entry,plnk)) return false;
	return true;
}


/** Helper function for links, rlinks and clinks.
 *  @param vec is a bit vector representing a set of links.
 *  @param lnks points to an output array of link numbers
 *  @param limit is a limit on the number of link numbers to write
 *  @return the number of links in the returned array
 */
int ComtreeTable::listLinks(int vec, uint16_t* lnks, int limit) const {
	vec >>= 1; int i = 0; int lnk = 1;
	while (lnk <= limit && vec != 0) {
		if (vec & 1) lnks[i++] = lnk;
		vec >>= 1; lnk++;
	}
	return i;
}

/** Get a list of links from is and return it as a bit vector.
 */
int ComtreeTable::readLinks(istream& is) {
	int nlnks = 0; int lnk; int vec = 0;
	do {
		if (!Misc::readNum(is,lnk)) return 0;
		if (0 < lnk && lnk <= 31) { vec |= (1 << lnk); nlnks++; }
	} while (Misc::verify(is,','));
	return vec;
}

/** Read a comtree from is and initialize its table entry.
 */
bool ComtreeTable::readEntry(istream& is) {
	int ct, cFlg, plnk, qn, quant;

        Misc::skipBlank(is);
        if (!Misc::readNum(is, ct) || ct < 1 || 
	    !Misc::readNum(is, cFlg) ||
	    !Misc::readNum(is, plnk) ||
	    !Misc::readNum(is, qn) ||
	    !Misc::readNum(is, quant)) { 
                return false;
        }

	int  lnks = readLinks(is);
	int clnks = readLinks(is);
	Misc::cflush(is,'\n');

	// determine set of router links and local links
	// using link table and address information
	int rlnks = 0; int llnks = 0;
	uint16_t lnkvec[Forest::MAXLNK];
	int n = listLinks(lnks,lnkvec,Forest::MAXLNK);
	for (int i = 0; i < n; i++ ) {
		int j = lnkvec[i];
		if (lt->getPeerType(j) == ROUTER) {
			rlnks |= (1 << j);
			if (Forest::zipCode(lt->getPeerAdr(j))
			    == Forest::zipCode(myAdr))
				llnks |= (1 << j);
		}
	}

	int entry = addEntry(ct);
	if (entry == 0) return false;
	setCoreFlag(entry,cFlg);
	setPlink(entry,plnk); setQnum(entry,qn); setQuant(entry,quant);
	tbl[entry].links = lnks;   tbl[entry].rlinks = rlnks;
	tbl[entry].llinks = llnks; tbl[entry].clinks = clnks;

	if (!checkEntry(entry)) { removeEntry(entry); return false; }

	for (int i = 0; i < n; i++ ) qm->setQuantum(lnkvec[i],qn,quant);
	
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
bool ComtreeTable::readTable(istream& is) {
	int num;
	Misc::skipBlank(is);
	if (!Misc::readNum(is,num)) return false;
	Misc::cflush(is,'\n');
	for (int i = 1; i <= num; i++) {
		if (!readEntry(is)) {
			cerr << "Error reading comtree table entry # "
			     << i << endl;
			return false;
		}
	}
        return true;
}

/** Helper function for writeComt. Prints list of links. */
void ComtreeTable::writeLinks(ostream& out, int lnks) const {
	int i = 1; lnks >>= 1;
	if (lnks == 0) { out << "-"; return; }
	while (lnks != 0) {
		if (lnks & 1) {
			out << i;
			if ((lnks>>1) != 0) out << ",";
		}
		i++; lnks >>= 1;
	}
}

/** Write entry for link i
 */
void ComtreeTable::writeEntry(ostream& out, int entry) const {
	out << setw(9) << getComtree(entry) << " "  
	   << setw(6) << getCoreFlag(entry)
	   << setw(8) << getPlink(entry) << " "
	   << setw(6) << getQnum(entry) << " "
	   << setw(6) << getQuant(entry) << "   ";
	writeLinks(out,tbl[entry].links); out << "     ";
	writeLinks(out,tbl[entry].clinks); out << "\n";
}

/** Output human readable representation of comtree table.
 */
void ComtreeTable::writeTable(ostream& out) const {
	int cnt = 0;
        for (int i = 1; i <= maxte; i++)
                if (valid(i)) cnt++;
	out << cnt << endl;
	out << "# comtree  coreFlag  pLink  qNum  quant  links"
	    << "            coreLinks" << endl;
        for (int i = 1; i <= maxte; i++)
                if (valid(i)) writeEntry(out,i);
}
