#include "rteTbl.h"

rteTbl::rteTbl(int nte1, qMgr* qm1) : nte(nte1), qm(qm1) {
// Constructor for rteTbl, allocates space and initializes table.
// Nte1 is the number of routing table entries.
	int i;
	tbl = new rtEntry[nte+1];
	ht = new hashTbl(nte);
	for (i = 0; i <= nte; i++) tbl[i].vn = 0;    // vn=0 for unused entry
	for (i = 1; i < nte; i++) {
		tbl[i].lnks = i+1;  // link unused entries
		tbl[i].qn = 0;	    // for qn=0 packets go to default for vnet
	}
	free = 1; tbl[nte].lnks = Null;
};
	
rteTbl::~rteTbl() { delete [] tbl; }

int rteTbl::lookup(vnet_t vnet, wuAdr_t adr) {
// Perform a lookup in the routing table. Vnet is the vnet number,
// adr is the destination address. Returned value is the index
// of the table entry, or 0 if there is no match.
	uint64_t key = vnet; key = (key << 32) | adr;
	return ht->lookup(key);
}

int rteTbl::links(int te, uint16_t lnkVec[], int n) const {
// Return a vector of links for a given multicast table entry.
// N is the size of lnkVec. Return the number of links, or 0
// if this is  not a multicast table entry.
	if (ucastAdr(tbl[te].adr)) return 0;
	int j = 0;
	for (int i = 1; i <= 31 && j < n; i++) {
		if (tbl[te].lnks & (1 << i)) lnkVec[j++] = i;
	}
	return j;
}

int rteTbl::addEntry(vnet_t vnet, wuAdr_t adr, int lnk, int qnum) {
// Insert routing table entry for given vnet and address,
// with specified link and queue number. For unicast addresses,
// store link number. For multicast addresses, set bit in a bit
// vector representing a set of links. For multicast entries,
// if lnk=0, the bit vector should be all zeros (empty set).
// Return index of routing table entry or Null on failure.
	if (free == Null) return Null;
	int te = free; free = tbl[free].lnks;

	uint64_t key = vnet; key = (key << 32) | adr;
	if (ht->insert(key,te)) {
		tbl[te].vn = vnet; tbl[te].adr = adr; tbl[te].qn = qnum;
		if (ucastAdr(adr) || lnk == 0)
			tbl[te].lnks = lnk;
		else
			tbl[te].lnks = (1 << lnk);
		return te;
	} else {
		free = te;
		return Null;
	}
}

bool rteTbl::removeEntry(int te) {
// Remove entry te from routing table.
	uint64_t key = tbl[te].vn; key = (key << 32) | tbl[te].adr;
	ht->remove(key);
	tbl[te].vn = 0; tbl[te].lnks = free; free = te;
}

bool rteTbl::getEntry(istream& is) {
// Read an entry from is and create a routing table entry for it.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete routing table entry.
	int vnet, adr, lnk, qnum, quant, te;

        misc::skipBlank(is);
        if (!misc::getNum(is, vnet) || !misc::getNum(is,adr) ||
	    !misc::getNum(is,qnum) || !misc::getNum(is,quant)) {
                return false;
        }
	if ((te = addEntry(vnet,adr,0,qnum)) == 0) return false;
	if (ucastAdr(adr)) {
		if (!misc::getNum(is,lnk)) return false;
		misc::cflush(is,'\n');
		setLink(te,lnk);
	} else {
		do {
			if (!misc::getNum(is,lnk)) {
				removeEntry(te); return false;
			}
			addLink(te,lnk);
			if (qnum != 0) qm->setQuantum(lnk,qnum,quant);
		} while (misc::verify(is,','));
	}
	misc::cflush(is,'\n');
	return true;
}

bool operator>>(istream& is, rteTbl& rt) {
// Read routing table entrees from input stream. The first line must contain
// an integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
// Each entry consists of a vnet number, an address and one or more
// link numbers. The vnet number is a decimal number. The address
// is in the form a.b where a and b are both decimal values. For multicast
// addresses, the a part must be at least 32,768 (2^15). The link numbers
// are given as decimal values. If the address is a unicast address,
// only the first link number on the line is considered.
	int num;
	misc::skipBlank(is);
	if (!misc::getNum(is,num)) return false;
	misc::cflush(is,'\n');
	while (num--) {
		if (!rt.getEntry(is)) return false;
	}
        return true;
}

void rteTbl::putEntry(ostream& os, int te) const {
// Print entry number te.
	os << setw(4) << te << ": " << tbl[te].vn << " " 
	   << tbl[te].adr << " " << tbl[te].qn << " ";
	if (ucastAdr(tbl[te].adr)) {
		os << tbl[te].lnks << endl;
	} else {
		bool first = true;
		for (int i = 1; i <= 31; i++) {
			if (tbl[te].lnks & (1 << i)) {
				if (first) first = false;
				else os << ",";
				os << i;
			}
		}
		os << endl;
	}
}

ostream& operator<<(ostream& os, const rteTbl& rt) {
// Output human readable representation of routing table.
        for (int i = 1; i <= rt.nte; i++) {
                if (rt.valid(i)) rt.putEntry(os,i);
	}
	return os;
}
