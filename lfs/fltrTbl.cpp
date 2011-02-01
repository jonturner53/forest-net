#include "fltrTbl.h"

fltrTbl::fltrTbl(int maxte1, ipa_t myAdr1, lnkTbl *lt1)
		 : maxte(maxte1), myAdr(myAdr1), lt(lt1) {
// Constructor for fltrTbl, allocates space and initializes table.
	tbl = new tblEntry[maxte+1];
	ht = new hashTbl(maxte);

	free = 1;
	for (int i = 1; i <= maxte; i++) {
		tbl[i].lnk = i+1;	// link entries in free list
		tbl[i].src = 0;		// 0 for invalid entry
	}
	tbl[maxte].lnk = Null;
};
	
fltrTbl::~fltrTbl() { delete [] tbl; delete ht; }

int fltrTbl::addEntry(ipa_t src, ipa_t dst) {
// Add the specified comtree to the set of comtrees in use.
// Return the index of the table entry or Null on failure.
	if (ht->lookup(hashkey(src,dst)) != Null) return Null;
	if (free == Null) return Null;
// temporary hack;
//	int te = free; free = tbl[free].lnk;
int te = 1; while (te <= maxte && tbl[te].src != 0) te++;
if (te > maxte) return Null;
	if (!ht->insert(hashkey(src,dst),te)) {
		free = te; return Null;
	}
	tbl[te].src = src; tbl[te].dst = dst;
	link(te) = 0; qnum(te) = 0;
	return te;
}

bool fltrTbl::removeEntry(int te) {
// Remove an entry from the table of filters
// Return true on success, false on failure.
	if (te == Null) return false;
	ht->remove(hashkey(tbl[te].src,tbl[te].dst));
	tbl[te].src = 0;
// temporary hack
//	tbl[te].lnk = free; free = te;
	return true;
}

bool fltrTbl::checkEntry(int te) const {
// Return true if table entry is consistent, else false.
// Add checks later
	return true;
}

bool fltrTbl::getFltr(istream& is) {
// Read a filter from is and initialize its table entry.

// Todo - Add an inLink field at the start of the line.
	int lnk, qn, fRate;
	ipa_t src, dst;

        misc::skipBlank(is);
        if (!misc::getIpAdr(is, src) || !misc::getIpAdr(is, dst) || 
	    !misc::getNum(is, lnk) || !misc::getNum(is, qn) ||
	    !misc::getNum(is, fRate)) {
                return false;
        }
	misc::cflush(is,'\n');

	int te = addEntry(src,dst);
	if (te == Null) return false;
	link(te) = lnk; qnum(te) = qn; rate(te) = fRate;

	if (!checkEntry(te)) { removeEntry(te); return false; }

        return true;
}

bool operator>>(istream& is, fltrTbl& ft) {
// Read filter table entrees from input stream. The first line must contain
// an integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
// An entry consists of an IP source address, destination address, link number,
// queue number and a queue quantum.
	int num;
return true; // ignore prespecified filters
	misc::skipBlank(is);
	if (!misc::getNum(is,num)) return false;
	misc::cflush(is,'\n');
	for (int i = 1; i <= num; i++) {
		if (!ft.getFltr(is)) {
			cerr << "Error reading filter table entry # "
			     << i << endl;
			return false;
		}
	}
        return true;
}

void fltrTbl::putFltr(ostream& os, int fte) {
// Print entry fte
	os << fte << ": ";
	os << tbl[fte].inlnk << " ";
	os << ((tbl[fte].src >> 24) & 0xFF) << "."
	   << ((tbl[fte].src >> 16) & 0xFF) << "."
	   << ((tbl[fte].src >> 8) & 0xFF) << "."
	   << ((tbl[fte].src) & 0xFF) << " ";
	os << ((tbl[fte].dst >> 24) & 0xFF) << "."
	   << ((tbl[fte].dst >> 16) & 0xFF) << "."
	   << ((tbl[fte].dst >> 8) & 0xFF) << "."
	   << ((tbl[fte].dst) & 0xFF) << " ";
	os << setw(2) << link(fte) << " "
	   << setw(3) << qnum(fte) << " "
	   << setw(3) << rate(fte) ;
}

ostream& operator<<(ostream& os, fltrTbl& ft) {
// Output human readable representation of comtree table.
        for (int i = 1; i <= ft.maxte; i++) {
                if (ft.valid(i)) {
			ft.putFltr(os,i); os << endl;
		}
	}
	return os;
}
