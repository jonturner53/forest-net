#include "rteTbl.h"

rteTbl::rteTbl(int nte1, fAdr_t myAdr1, lnkTbl* lt1, ComtTbl* ctt1, qMgr* qm1)
		: nte(nte1), myAdr(myAdr1), lt(lt1), ctt(ctt1), qm(qm1) {
// Constructor for rteTbl, allocates space and initializes table.
// Nte1 is the number of routing table entries.
	int i;
	tbl = new rtEntry[nte+1];
	ht = new hashTbl(nte);
	for (i = 0; i <= nte; i++) tbl[i].ct = 0;    // ct=0 for unused entry
	for (i = 1; i < nte; i++) {
		tbl[i].lnks = i+1;  // link unused entries
		tbl[i].qn = 0;	    // for qn=0 packets go to default for comt
	}
	free = 1; tbl[nte].lnks = Null;
};
	
rteTbl::~rteTbl() { delete [] tbl; }

int rteTbl::lookup(comt_t comt, fAdr_t adr) {
// Perform a lookup in the routing table. Comt is the comtree number,
// adr is the destination address. Returned value is the index
// of the table entry, or 0 if there is no match.
	return ht->lookup(hashkey(comt,adr));
}

int rteTbl::links(int te, uint16_t lnks[], int limit) const {
// Return a vector of links for a given multicast table entry.
// N is the size of lnkVec. Return the number of links, or 0
// if this is  not a multicast table entry.
	if (forest::ucastAdr(tbl[te].adr)) return 0;
	int vec = tbl[te].lnks >> 1; int i = 0; int lnk = 1;
	while (lnk <= limit && vec != 0) {
		if (vec & 1) lnks[i++] = lnk;
		vec >>= 1; lnk++;
	}
	return i;
}

int rteTbl::addEntry(comt_t comt, fAdr_t adr, int lnk, int qnum) {
// Insert routing table entry for given comtree and address,
// with specified link and queue number. For unicast addresses,
// store link number. For multicast addresses, set bit in a bit
// vector representing a set of links. For multicast entries,
// if lnk=0, the bit vector should be all zeros (empty set).
// Return index of routing table entry or Null on failure.
	if (free == Null) return Null;
	int te = free; free = tbl[free].lnks;

	if (ht->insert(hashkey(comt,adr),te)) {
		tbl[te].ct = comt;
		tbl[te].adr = (forest::ucastAdr(adr) &&
				forest::zipCode(adr)!=forest::zipCode(myAdr)) ?
				(adr & 0xffff0000) : adr;
		tbl[te].qn = qnum;
		if (forest::ucastAdr(adr) || lnk == 0)
			tbl[te].lnks = lnk;
		else
			tbl[te].lnks = (1 << lnk);
		return te;
	} else {
		free = te; return Null;
	}
}

bool rteTbl::removeEntry(int te) {
// Remove entry te from routing table.
	ht->remove(hashkey(tbl[te].ct,tbl[te].adr));
	tbl[te].ct = 0; tbl[te].lnks = free; free = te;
}

bool rteTbl::checkEntry(int te) const {
// Return true if consistent, else false;
	int i, n; uint16_t lnkvec[MAXLNK];
	// comtrees in routing table must be valid
	int ctte = ctt->lookup(comtree(te));
	if (ctte == Null) return false;


	// only multicast addresses can have more than one link
	n = links(te,lnkvec,MAXLNK);
	if (forest::ucastAdr(address(te))) return (n == 1 ? true : false);

	// check links of multicast routes
	for (i = 0; i < n; i++) {
		// links mentioned in routing table must be valid
		if (!lt->valid(lnkvec[i])) return false;
		// links go only to children that are not in core
		if (lnkvec[i] == ctt->getPlink(ctte)) return false;
		if (ctt->isClink(ctte,lnkvec[i])) return false;
	}
	return true;
}

bool rteTbl::getEntry(istream& is) {
// Read an entry from is and create a routing table entry for it.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete routing table entry.
	int comt, lnk, qnum, quant, te; uint32_t adr;

        Misc::skipBlank(is);
        if (!Misc::readNum(is, comt) || !Misc::readNum(is,adr) ||
	    !Misc::readNum(is,qnum) || !Misc::readNum(is,quant)) {
                return false;
        }
	if ((te = addEntry(comt,adr,0,qnum)) == 0) return false;
	if (forest::ucastAdr(adr)) {
		if (!Misc::readNum(is,lnk)) return false;
		Misc::cflush(is,'\n');
		setLink(te,lnk);
	} else {
		do {
			if (!Misc::readNum(is,lnk)) {
				removeEntry(te); return false;
			}
			addLink(te,lnk);
			if (qnum != 0) qm->quantum(lnk,qnum) = quant;
		} while (Misc::verify(is,','));
	}
	Misc::cflush(is,'\n');

	if (!checkEntry(te)) { removeEntry(te); return false; }
	return true;
}

bool operator>>(istream& is, rteTbl& rt) {
// Read routing table entrys from input stream. The first line must contain
// an integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
// Each entry consists of a comtree number, an address and one or more
// link numbers. The comtree number is a decimal number. The address
// is in the form a.b where a and b are both decimal values. For multicast
// addresses, the a part must be at least 32,768 (2^15). The link numbers
// are given as decimal values. If the address is a unicast address,
// only the first link number on the line is considered.
	int num;
	Misc::skipBlank(is);
	if (!Misc::readNum(is,num)) return false;
	Misc::cflush(is,'\n');
	for (int i = 1; i <= num; i++) {
		if (!rt.getEntry(is)) {
			cerr << "Error in route table entry # " << i << endl;
			return false;
		}
	}
        return true;
}

void rteTbl::putEntry(ostream& os, int te) const {
// Print entry number te.
	os << setw(4) << te << ": " << tbl[te].ct << " ";
	if (forest::mcastAdr(tbl[te].adr)) {
	   	os << tbl[te].adr << " " << tbl[te].qn << " ";
		bool first = true;
		if (tbl[te].lnks == 0) { os << "-\n"; return; }
		for (int i = 1; i <= 31; i++) {
			if (tbl[te].lnks & (1 << i)) {
				if (first) first = false;
				else os << ",";
				os << i;
			}
		}
		os << endl;
	} else {
	   	os << forest::zipCode(tbl[te].adr) << "."
		   << forest::localAdr(tbl[te].adr)
		   << " " << tbl[te].qn << " " <<  tbl[te].lnks << endl;
	}
}

ostream& operator<<(ostream& os, const rteTbl& rt) {
// Output human readable representation of routing table.
        for (int i = 1; i <= rt.nte; i++) {
                if (rt.valid(i)) rt.putEntry(os,i);
	}
	return os;
}
