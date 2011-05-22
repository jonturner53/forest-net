/** \file ComtTbl.cpp */

#include "ComtTbl.h"

/** Constructor for ComtTbl, allocates space and initializes table. */
ComtTbl::ComtTbl(int maxte1, fAdr_t myAdr1, lnkTbl *lt1, qMgr *qm1)
		 : maxte(maxte1), myAdr(myAdr1), lt(lt1), qm(qm1) {
	tbl = new tblEntry[maxte+1];
	ht = new hashTbl(maxte);

	free = 1;
	for (int entry = 1; entry <= maxte; entry++) {
		tbl[entry].links = entry+1;
		tbl[entry].qn = 0;		// 0 for invalid entry
	}
	tbl[maxte].links = Null;
};
	
ComtTbl::~ComtTbl() { delete [] tbl; delete ht; }

/** Add the specified comtree to the set of comtrees in use.
 *  Return the index of the table entry or Null on failure.
 */
int ComtTbl::addEntry(comt_t ct) {
	if (ht->lookup(ct) != Null) return Null;
	if (free == Null) return Null;
	int entry = free; free = tbl[free].links;
	if (!ht->insert(hashkey(ct),entry)) {
		free = entry; return Null;
	}
	tbl[entry].links = tbl[entry].rlinks = 0;
	tbl[entry].llinks = tbl[entry].clinks = 0;
	tbl[entry].cFlag = false; tbl[entry].plnk = 0;
	tbl[entry].qn = 1;
	return entry;
}

/** Remove an entry from the table of comtrees.
 *  Return true on success, false on failure.
 */
bool ComtTbl::removeEntry(int entry) {
	if (entry == Null) return false;
	ht->remove(hashkey(tbl[entry].comt));
	tbl[entry].qn = 0; tbl[entry].links = free; free = entry;
}

/** Return true if table entry is consistent, else false.
 */
bool ComtTbl::checkEntry(int entry) const {
	int i, n; uint16_t lnkvec[MAXLNK+1];
	// every link in comtree entry must be valid
	n = links(entry,lnkvec,MAXLNK);
	for (i = 0; i < n; i++) {
		if (!lt->valid(lnkvec[i])) return false;
	}
	// every core link must be a comtree link
	n = clinks(entry,lnkvec,MAXLNK);
	for (i = 0; i < n; i++) {
		if (!isLink(entry,lnkvec[i])) return false;
	}
	// parent of a core router must be a core router
	int plnk = getPlink(entry);
	if (getCoreFlag(entry)) {
		if (plnk != Null && !isClink(entry,plnk)) return false;
	} else {
		// and a non-core router has at most one core link
		if (n > 1) return false;
		if (plnk == Null || 
		    (n == 1 && lnkvec[0] != plnk))
			return false;
	}
	// every router link must be a comtree link
	// also, peer must be a router
	n = rlinks(entry,lnkvec,MAXLNK);
	for (i = 0; i < n; i++) {
		if (!isLink(entry,lnkvec[i])) return false;
		if (lt->peerTyp(lnkvec[i]) != ROUTER) return false;
	}
	// parent must be a router
	if (plnk != Null && !isRlink(entry,plnk)) return false;
	return true;
}


/** Helper function for links, rlinks and clinks.
 */
int ComtTbl::listLinks(int vec, uint16_t* lnks, int limit) const {
	vec >>= 1; int i = 0; int lnk = 1;
	while (lnk <= limit && vec != 0) {
		if (vec & 1) lnks[i++] = lnk;
		vec >>= 1; lnk++;
	}
	return i;
}

/** Get a list of links from is and return it as a bit vector.
 */
int ComtTbl::readLinks(istream& is) {
	int nlnks = 0; int lnk; int vec = 0;
	do {
		if (!Misc::readNum(is,lnk)) return 0;
		if (0 < lnk && lnk <= 31) { vec |= (1 << lnk); nlnks++; }
	} while (Misc::verify(is,','));
	return vec;
}

/** Read a comtree from is and initialize its table entry.
 */
bool ComtTbl::readEntry(istream& is) {
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
	uint16_t lnkvec[MAXLNK];
	int n = listLinks(lnks,lnkvec,MAXLNK);
	for (int i = 0; i < n; i++ ) {
		int j = lnkvec[i];
		if (lt->peerTyp(j) == ROUTER) {
			rlnks |= (1 << j);
			if (forest::zipCode(lt->peerAdr(j))
			    == forest::zipCode(myAdr))
				llnks |= (1 << j);
		}
	}

	int entry = addEntry(ct);
	if (entry == Null) return false;
	tbl[entry].comt = ct; setCoreFlag(entry,cFlg);
	setPlink(entry,plnk); setQnum(entry,qn);
	tbl[entry].links = lnks;   tbl[entry].rlinks = rlnks;
	tbl[entry].llinks = llnks; tbl[entry].clinks = clnks;

	if (!checkEntry(entry)) { removeEntry(entry); return false; }

	for (int i = 0; i < n; i++ ) qm->quantum(lnkvec[i],qn) = quant;
	
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
bool ComtTbl::readTable(istream& is) {
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
void ComtTbl::writeLinks(ostream& os, int lnks) const {
	int i = 1; lnks >>= 1;
	if (lnks == 0) { os << "-"; return; }
	while (lnks != 0) {
		if (lnks & 1) {
			os << i;
			if ((lnks>>1) != 0) os << ",";
		}
		i++; lnks >>= 1;
	}
}

void ComtTbl::writeEntry(ostream& os, int entry) const {
// Print entry for link i
	os << setw(3) << getComtree(entry) << " "  
	   << (getCoreFlag(entry) ? "true  " : "false ")
	   << setw(2) << getPlink(entry) << " "
	   << setw(3) << getQnum(entry) << " ";
	writeLinks(os,tbl[entry].links); os << " ";
	writeLinks(os,tbl[entry].rlinks); os << " ";
	writeLinks(os,tbl[entry].clinks); os << "\n";
}

/** Output human readable representation of comtree table. */
void ComtTbl::writeTable(ostream& os) const {
        for (int i = 1; i <= maxte; i++) {
                if (valid(i)) writeEntry(os,i);
	}
}
