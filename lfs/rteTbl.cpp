#include "rteTbl.h"

rteTbl::rteTbl(int nte1, ipa_t myAdr1, lnkTbl* lt1, qMgr* qm1)
		: nte(nte1), myAdr(myAdr1), lt(lt1), qm(qm1) {
// Constructor for rteTbl, allocates space and initializes table.
// Nte1 is the number of routing table entries.
	int i;
	tbl = new rtEntry[nte+1];
	lmpt = new lmp(nte);
	for (i = 0; i <= nte; i++) tbl[i].pfx = 0;    // pfx=0 for unused entry
	for (i = 1; i < nte; i++) {
		tbl[i].nh[0] = i+1;  // link unused entries using nh[0] field
	}
	free = 1; tbl[nte].nh[0] = Null;
};
	
rteTbl::~rteTbl() { delete [] tbl; }

int rteTbl::lookup(ipa_t adr) {
// Perform a lookup in the routing table for the given address.
// Returned value is the index of the table entry, or 0 if there is no match.
	return lmpt->lookup(adr);
}

int rteTbl::addEntry(ipa_t pref, int lng) {
// Insert routing table entry for given prefix and length.
// Return index of routing table entry or Null on failure.
	if (free == Null) return Null;
	int te = free; free = tbl[free].nh[0];

	if (lmpt->insert(pref,lng,te)) {
		tbl[te].pfx = pref;
		tbl[te].pfxlng = lng;
		tbl[te].nh[1] = tbl[te].nh[2] = tbl[te].nh[3] = 0;
		return te;
	} else {
		free = te; return Null;
	}
}

bool rteTbl::removeEntry(int te) {
// Remove entry te from routing table.
	lmpt->remove(tbl[te].pfx,tbl[te].pfxlng);
	tbl[te].pfx = 0; tbl[te].nh[0] = free; free = te;
}

void rteTbl::sort() {
// Sort route table entries in decreasing order of their prefix length
	int i,j;
	// first remove everything from lmpt
	for (i = 1; i <= nte; i++)
		if (valid(i)) lmpt->remove(tbl[i].pfx,tbl[i].pfxlng);
	// sort the table
	for (i = 1; i <= nte; i++) {
		for (j = i+1; j <= nte; j++) {
			if ((!valid(i) && valid(j)) ||
			    valid(i) && valid(j) &&
			    tbl[j].pfxlng > tbl[i].pfxlng) {
				rtEntry tmp = tbl[i];
				tbl[i] = tbl[j];
				tbl[j] = tmp;
			}
		}
	}
	// rebuild lmpt
	for (i = 1; i <= nte; i++)
		if (valid(i))
			lmpt->insert(tbl[i].pfx,tbl[i].pfxlng,i);
}

bool rteTbl::checkEntry(int te) const {
// Return true if consistent, else false;
	return tbl[te].nh[1]> 0;
}

bool rteTbl::getEntry(istream& is) {
// Read an entry from is and create a routing table entry for it.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete routing table entry.
// which consists of an IP address prefix and length, plus
// up to three next hop links. 
	ipa_t pref; int prefLng, te;

        misc::skipBlank(is);
        if (!misc::getIpAdr(is, pref) || !misc::verify(is,'/') ||
	    !misc::getNum(is,prefLng)) 
                return false;
	if (!ucastAdr(pref) || prefLng < 0 || prefLng > 32) return false;
	if ((te = addEntry(pref,prefLng)) == 0) return false;
	int l1, l2, l3; l1 = l2 = l3 = 0;
	misc::getNum(is,l1); misc::getNum(is,l2); misc::getNum(is,l3);
	if (l1 < 0 || l1 > MAXLNK || l2 < 0 || l2 > MAXLNK ||
	    l3 < 0 || l3 > MAXLNK) return false;
	link(te,1) = l1; link(te,2) = l2; link(te,3) = l3;
	misc::cflush(is,'\n');

	if (!checkEntry(te)) { removeEntry(te); return false; }
	return true;
}

bool operator>>(istream& is, rteTbl& rt) {
// Read routing table entries from input stream. The first line must contain
// an integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
	int num;
	misc::skipBlank(is);
	if (!misc::getNum(is,num)) return false;
	misc::cflush(is,'\n');
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
	os << setw(4) << te << ": "
	   << ((tbl[te].pfx >> 24) & 0xff) << "."
	   << ((tbl[te].pfx >> 16) & 0xff) << "."
	   << ((tbl[te].pfx >>  8) & 0xff) << "."
	   << ((tbl[te].pfx      ) & 0xff) << "/" << tbl[te].pfxlng << " "
	   << tbl[te].nh[1] << " " << tbl[te].nh[2] << " " << tbl[te].nh[3];
	os << endl;
}

ostream& operator<<(ostream& os, const rteTbl& rt) {
// Output human readable representation of routing table.
        for (int i = 1; i <= rt.nte; i++) {
                if (rt.valid(i)) rt.putEntry(os,i);
	}
	return os;
}
