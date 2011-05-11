/* Todo
 Longer term
 - Replace bit vectors with alternate representation.
   Needs fast membership test and fast iteration.
   Want to support large link counts, since clients have
   modest bandwidth needs. Also should be reasonably
   efficient when adding links, since clients can come
   and go frequently.

   One approach is to have two sets of links, one for
   connections to other routers (with links numbered 1-31)
   and a separate set for links to other endpoints.
   The router set could be implemented using a bit vector.
   The endpoint set could be implemented with a list,
   possibly augmented by hash table. Alternatively,
   for endpoints we could list their vnets in the
   link table and test for membership by consulting
   the link table entry.

   The more general approach would be to have a list
   for each vnet and a hash table of vnet/link pairs
   that we can use for fast membership tests. Main
   drawback is memory usage, but that may not be a
   good enough reason to let performance optimization
   impose restrictions on how links are used.
*/

#include "comtTbl.h"

comtTbl::comtTbl(int maxte1, fAdr_t myAdr1, lnkTbl *lt1, qMgr *qm1)
		 : maxte(maxte1), myAdr(myAdr1), lt(lt1), qm(qm1) {
// Constructor for comtTbl, allocates space and initializes table.
	tbl = new tblEntry[maxte+1];
	ht = new hashTbl(maxte);

	free = 1;
	for (int i = 1; i <= maxte; i++) {
		tbl[i].links = i+1;
		tbl[i].qn = 0;		// 0 for invalid entry
	}
	tbl[maxte].links = Null;
};
	
comtTbl::~comtTbl() { delete [] tbl; delete ht; }

int comtTbl::addEntry(comt_t ct) {
// Add the specified comtree to the set of comtrees in use.
// Return the index of the table entry or Null on failure.
	if (ht->lookup(ct) != Null) return Null;
	if (free == Null) return Null;
	int te = free; free = tbl[free].links;
	if (!ht->insert(hashkey(ct),te)) {
		free = te; return Null;
	}
	tbl[te].links = tbl[te].rlinks = tbl[te].llinks = tbl[te].clinks = 0;
	tbl[te].cFlag = false; tbl[te].plnk = 0;
	tbl[te].qn = 1;
	return te;
}

bool comtTbl::removeEntry(int te) {
// Remove an entry from the table of comtrees.
// Return true on success, false on failure.
	if (te == Null) return false;
	ht->remove(hashkey(tbl[te].comt));
	tbl[te].qn = 0; tbl[te].links = free; free = te;
}

bool comtTbl::checkEntry(int te) const {
// Return true if table entry is consistent, else false.
	int i, n; uint16_t lnkvec[MAXLNK+1];
	// every link in comtree entry must be valid
	n = links(te,lnkvec,MAXLNK);
	for (i = 0; i < n; i++) {
		if (!lt->valid(lnkvec[i])) return false;
	}
	// every core link must be a comtree link
	n = clinks(te,lnkvec,MAXLNK);
	for (i = 0; i < n; i++) {
		if (!isLink(te,lnkvec[i])) return false;
	}
	// parent of a core router must be a core router
	if (coreFlag(te)) {
		if (plink(te) != Null && !isClink(te,plink(te))) return false;
	} else {
		// and a non-core router has at most one core link
		if (n > 1) return false;
		if (plink(te) == Null || 
		    (n == 1 && lnkvec[0] != plink(te)))
			return false;
	}
	// every router link must be a comtree link
	// also, peer must be a router
	n = rlinks(te,lnkvec,MAXLNK);
	for (i = 0; i < n; i++) {
		if (!isLink(te,lnkvec[i])) return false;
		if (lt->peerTyp(lnkvec[i]) != ROUTER) return false;
	}
	// parent must be a router
	if (plink(te) != Null && !isRlink(te,plink(te))) return false;
	return true;
}


int comtTbl::listLinks(int vec, uint16_t* lnks, int limit) const {
// Helper function for links, rlinks and clinks.
	vec >>= 1; int i = 0; int lnk = 1;
	while (lnk <= limit && vec != 0) {
		if (vec & 1) lnks[i++] = lnk;
		vec >>= 1; lnk++;
	}
	return i;
}

int comtTbl::getLinks(istream& is) {
// Get a list of links from is and return it as a bit vector.
	int nlnks = 0; int lnk; int vec = 0;
	do {
		if (!misc::getNum(is,lnk)) return 0;
		if (0 < lnk && lnk <= 31) { vec |= (1 << lnk); nlnks++; }
	} while (misc::verify(is,','));
	return vec;
}

bool comtTbl::getComt(istream& is) {
// Read a comtree from is and initialize its table entry.
	int ct, cFlg, plnk, qn, quant;

        misc::skipBlank(is);
        if (!misc::getNum(is, ct) || ct < 1 || 
	    !misc::getNum(is, cFlg) ||
	    !misc::getNum(is, plnk) ||
	    !misc::getNum(is, qn) ||
	    !misc::getNum(is, quant)) { 
                return false;
        }

	int  lnks = getLinks(is);
	int clnks = getLinks(is);
	misc::cflush(is,'\n');

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

	int te = addEntry(ct);
	if (te == Null) return false;
	tbl[te].comt = ct;
	coreFlag(te) = cFlg; plink(te) = plnk; qnum(te) = qn;
	tbl[te].links = lnks; tbl[te].rlinks = rlnks;
	tbl[te].llinks = llnks; tbl[te].clinks = clnks;

	if (!checkEntry(te)) { removeEntry(te); return false; }

	for (int i = 0; i < n; i++ ) qm->quantum(lnkvec[i],qn) = quant;
	
        return true;
}

bool operator>>(istream& is, comtTbl& ctt) {
// Read comtree table entrees from input stream. The first line must contain
// an integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
// An entry consists of a comtree number, a core flag (0 or 1), the link
// number of the link leading to the parent, a queue number, a queue quantum,
// and three comma-separated lists of links.
	int num;
	misc::skipBlank(is);
	if (!misc::getNum(is,num)) return false;
	misc::cflush(is,'\n');
	for (int i = 1; i <= num; i++) {
		if (!ctt.getComt(is)) {
			cerr << "Error reading comtree table entry # "
			     << i << endl;
			return false;
		}
	}
        return true;
}

void comtTbl::putLinks(ostream& os, int lnks) const {
// Helper function for putComt. Prints list of links.
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

void comtTbl::putComt(ostream& os, int ctte) const {
// Print entry for link i
	os << setw(3) << comtree(ctte) << " "  
	   << (coreFlag(ctte) ? "true  " : "false ")
	   << setw(2) << plink(ctte) << " "
	   << setw(3) << qnum(ctte) << " ";
	putLinks(os,tbl[ctte].links); os << " ";
	putLinks(os,tbl[ctte].rlinks); os << " ";
	putLinks(os,tbl[ctte].clinks); os << "\n";
}

ostream& operator<<(ostream& os, const comtTbl& ctt) {
// Output human readable representation of comtree table.
        for (int i = 1; i <= ctt.maxte; i++) {
                if (ctt.valid(i)) {
			ctt.putComt(os,i);
		}
	}
	return os;
}
