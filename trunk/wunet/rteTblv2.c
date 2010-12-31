#include "rteTbl.h"

rteTbl::rteTbl(int nte1) : nte(nte1) {
// Constructor for rteTbl, allocates space and initializes table.
// Nte1 is the number of routing table entries.
	int i;
	tbl = new rtEntry[nte+1];
	ht = new hashTbl(nte);
	for (i = 0; i <= nte; i++) tbl[i].vn = 0;    // vn=0 for unused entry
	for (i = 1; i < nte; i++) tbl[i].lnk = i+1;  // link unused entries
	free = 1; tbl[nte].lnk = Null;
};
	
rteTbl::~rteTbl() { delete [] tbl; }

int rteTbl::lookup(vnet_t vnet, wuAdr_t adr) {
// Perform a lookup in the routing table. Vnet is the vnet number,
// adr is the destination address. Returned value is the index
// of the table entry, or 0 if there is no match.
	uint64_t key = vnet; key = (key << 32) | adr;
	return ht->lookup(key);
}

bool rteTbl::addEntry(vnet_t vnet, wuAdr_t adr, int lnk) {
// Insert routing table entry for given vnet and address,
// with specified link. Return index of routing table entry.
// No check is made for an existing entry that matches.
// Return true on success, false on failure.

	if (free == Null) return false;
	int te = free; free = tbl[free].lnk;

	uint64_t key = vnet; key = (key << 32) | adr;
	if (ht->insert(key,te)) {
		tbl[te].vn = vnet; tbl[te].adr = adr; tbl[te].lnk = lnk;
		return true;
	} else {
		free = te;
		return false;
	}
}

bool rteTbl::removeEntry(int te) {
// Remove entry te from routing table.
	uint64_t key = tbl[te].vn; key = (key << 32) | tbl[te].adr;
	ht->remove(key);
	tbl[te].vn = 0; tbl[te].lnk = free; free = te;
}

bool rteTbl::getEntry(istream& is) {
// Read an entry from is and create a routing table entry for it.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete routing table entry.
	int vnet, adr, lnk;

        misc::skipBlank(is);
        if (!misc::getNum(is, vnet) || 
	    !misc::getNum(is,adr) ||
            !misc::getNum(is,lnk)) {
                return false;
        }
	misc::cflush(is,'\n');
	return addEntry(vnet,adr,lnk);
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
	os << setw(4) << te << ": " 
	   << tbl[te].vn << " " 
	   << tbl[te].adr << " " 
	   << tbl[te].lnk << endl;
}

ostream& operator<<(ostream& os, const rteTbl& rt) {
// Output human readable representation of routing table.
        for (int i = 1; i <= rt.nte; i++) {
                if (rt.valid(i)) rt.putEntry(os,i);
	}
	return os;
}
