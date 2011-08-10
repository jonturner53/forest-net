/** @file RouteTable.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RouteTable.h"

/** Constructor for RouteTable, allocates space and initializes table.
 *  Nte1 is the number of routing table entries.
 */
RouteTable::RouteTable(int nte1, fAdr_t myAdr1, LinkTable* lt1,
		       ComtreeTable* ctt1, QuManager* qm1)
		      : nte(nte1), myAdr(myAdr1), lt(lt1), ctt(ctt1), qm(qm1) {
	int i;
	tbl = new rtEntry[nte+1];
	ht = new UiHashTbl(nte);
	for (i = 0; i <= nte; i++) tbl[i].ct = 0;    // ct=0 for unused entry
	for (i = 1; i < nte; i++) {
		tbl[i].lnks = i+1;  // link unused entries
		tbl[i].qn = 0;	    // for qn=0 packets go to default for comt
	}
	free = 1; tbl[nte].lnks = 0;
};
	
RouteTable::~RouteTable() { delete [] tbl; }

int RouteTable::lookup(comt_t comt, fAdr_t adr) {
// Perform a lookup in the routing table. Comt is the comtree number,
// adr is the destination address. Returned value is the index
// of the table entry, or 0 if there is no match.
	//return ht->lookup(hashkey(comt,adr));

	uint64_t x = hashkey(comt,adr);
	int te = ht->lookup(x);
	uint64_t z = ht->getKey(te);
	if (te == 0) return 0;
	if (x != z)
		cerr << "hash lookup on " << x << " returns " << te
		     << " for which stored key is " << z << endl;
	else if (tbl[te].ct != comt)
		cerr << "hash is ok but stored comtree value " << tbl[te].ct
		     << " does not match " << comt << endl;
	return te;

}

int RouteTable::getLinks(int te, uint16_t lnks[], int limit) const {
// Return a vector of links for a given multicast table entry.
// N is the size of lnkVec. Return the number of links, or 0
// if this is  not a multicast table entry.
	if (!Forest::mcastAdr(tbl[te].adr)) return 0;
	int vec = tbl[te].lnks >> 1; int i = 0; int lnk = 1;
	while (lnk <= limit && vec != 0) {
		if (vec & 1) lnks[i++] = lnk;
		vec >>= 1; lnk++;
	}
	return i;
}

int RouteTable::addEntry(comt_t comt, fAdr_t adr, int lnk, int qnum) {
// Insert routing table entry for given comtree and address,
// with specified link and queue number. For unicast addresses,
// store link number. For multicast addresses, set bit in a bit
// vector representing a set of links. For multicast entries,
// if lnk=0, the bit vector should be all zeros (empty set).
// Return index of routing table entry or 0 on failure.
	if (free == 0) return 0;
	int te = free; free = tbl[free].lnks;

	if (ht->insert(hashkey(comt,adr),te)) {
		tbl[te].ct = comt;
		tbl[te].qn = qnum;
		if (Forest::mcastAdr(adr)) {
			tbl[te].adr = adr;
			tbl[te].lnks = (lnk == 0 ? 0 : 1 << lnk);
		} else {
			int zip = Forest::zipCode(adr);
			tbl[te].adr =  (zip == Forest::zipCode(myAdr) ?
					adr : Forest::forestAdr(zip,0));
			tbl[te].lnks = lnk;
		}
		return te;
	} else {
		free = te; return 0;
	}
}

bool RouteTable::compareEntry(comt_t comt, fAdr_t adr, int lnk, int qnum) {	
	int te = ht->lookup(hashkey(comt,adr));
	if(te == 0) return false;
	return tbl[te].ct == comt &&
	       tbl[te].qn == qnum &&
	       tbl[te].adr == adr &&
	       tbl[te].lnks == lnk;
}

bool RouteTable::removeEntry(int te) {
// Remove entry te from routing table.
	ht->remove(hashkey(tbl[te].ct,tbl[te].adr));
	tbl[te].ct = 0; tbl[te].lnks = free; free = te;
}

bool RouteTable::checkEntry(int te) const {
// Return true if consistent, else false;
	int i, n; uint16_t lnkvec[Forest::MAXLNK];
	// comtrees in routing table must be valid
	int ctte = ctt->lookup(getComtree(te));
	if (ctte == 0) return false;

	// only multicast addresses can have more than one link
	n = getLinks(te,lnkvec,Forest::MAXLNK);
	if (!Forest::mcastAdr(getAddress(te))) return (n == 1 ? true : false);

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

bool RouteTable::readEntry(istream& in) {
// Read an entry from in and create a routing table entry for it.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete routing table entry.
	int comt, lnk, qnum, quant, te; fAdr_t adr;

        Misc::skipBlank(in);
        if (!Misc::readNum(in, comt) || !Forest::readForestAdr(in,adr) ||
	    !Misc::readNum(in,qnum) || !Misc::readNum(in,quant)) {
                return false;
        }
	if ((te = addEntry(comt,adr,0,qnum)) == 0) return false;
	if (Forest::mcastAdr(adr)) {
		do {
			if (!Misc::readNum(in,lnk)) {
				removeEntry(te); return false;
			}
			addLink(te,lnk);
			if (qnum != 0) qm->setQuantum(lnk,qnum,quant);
		} while (Misc::verify(in,','));
	} else {
		if (!Misc::readNum(in,lnk)) return false;
		Misc::cflush(in,'\n');
		setLink(te,lnk);
	}
	Misc::cflush(in,'\n');

	if (!checkEntry(te)) { removeEntry(te); return false; }
	return true;
}

/** Read routing table entrys from input stream. The first line must contain
 *  an integer, giving the number of entries to be read. The input may
 *  include blank lines and comment lines (any text starting with '#').
 *  Each entry must be on a line by itself (possibly with a trailing comment).
 *  Each entry consists of a comtree number, an address and one or more
 *  link numbers. The comtree number is a decimal number. The address
 *  is in the form a.b where a and b are both decimal values. For multicast
 *  addresses, the a part must be at least 32,768 (2^15). The link numbers
 *  are given as decimal values. If the address is a unicast address,
 *  only the first link number on the line is considered.
 */
bool RouteTable::read(istream& in) {
	int num;
	Misc::skipBlank(in);
	if (!Misc::readNum(in,num)) return false;
	Misc::cflush(in,'\n');
	for (int i = 1; i <= num; i++) {
		if (!readEntry(in)) {
			cerr << "Error in route table entry # " << i << endl;
			return false;
		}
	}
        return true;
}

void RouteTable::writeEntry(ostream& out, int te) const {
// Print entry number te.
	out << setw(4) << te << ": " << getComtree(te) << " ";
	if (Forest::mcastAdr(getAddress(te))) {
	   	out << getAddress(te) << " " << getQnum(te) << " ";
		bool first = true;
		if (getLinks(te) == 0) { out << "-\n"; return; }
		for (int i = 1; i <= 31; i++) {
			if (getLinks(te) & (1 << i)) {
				if (first) first = false;
				else out << ",";
				out << i;
			}
		}
		out << endl;
	} else {
	   	out << Forest::zipCode(getAddress(te)) << "."
		   << Forest::localAdr(getAddress(te))
		   << " " << getQnum(te) << " " <<  getLinks(te) << endl;
	}
}

void RouteTable::write(ostream& out) const {
// Output human readable representation of routing table.
        for (int i = 1; i <= nte; i++) {
                if (valid(i)) writeEntry(out,i);
	}
}
