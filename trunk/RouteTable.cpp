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
RouteTable::RouteTable(int maxRtx1, fAdr_t myAdr1, ComtreeTable* ctt1)
		      : maxRtx(maxRtx1), myAdr(myAdr1), ctt(ctt1) {
	int i;
	tbl = new RouteEntry[maxRtx+1];
	rteMap = new IdMap(maxRtx);
};
	
RouteTable::~RouteTable() { delete [] tbl; delete rteMap; }

// Insert routing table entry for given comtree and address,
// with specified comtree link. For multicast entries,
// if lnk=0, make the link set empty.
// Return index of routing table entry or 0 on failure.
int RouteTable::addEntry(comt_t comt, fAdr_t adr, int cLnk) {
	int rtx = rteMap->addPair(key(comt,adr));
	if (rtx == 0) return 0;
	tbl[rtx].ct = comt;
	if (Forest::mcastAdr(adr)) {
		tbl[rtx].adr = adr;
		tbl[rtx].links = new set<int>();
		if (cLnk != 0) tbl[rtx].links->insert(cLnk);
	} else {
		int zip = Forest::zipCode(adr);
		tbl[rtx].adr =  (zip == Forest::zipCode(myAdr) ?
				adr : Forest::forestAdr(zip,0));
		tbl[rtx].lnk = cLnk;
	}
	return rtx;
}

// Remove entry rtx from routing table.
void RouteTable::removeEntry(int rtx) {
	if (!validRteIndex(rtx)) return;
	if (Forest::mcastAdr(tbl[rtx].adr)) delete tbl[rtx].links;
	rteMap->dropPair(key(tbl[rtx].ct,tbl[rtx].adr));
}

/*
bool RouteTable::compareEntry(comt_t comt, fAdr_t adr, int lnk) {
        int rtx = rteMap->getId(hashkey(comt,adr));
        if(rtx == 0) return false;
        return tbl[te].ct == comt && tbl[te].adr == adr && tbl[te].lnk == lnk;
}
*/

bool RouteTable::readEntry(istream& in) {
// Read an entry from in and create a routing table entry for it.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete routing table entry.
	int comt, lnk; fAdr_t adr;
        Misc::skipBlank(in);
        if (!Misc::readNum(in, comt) || !Forest::readForestAdr(in,adr)) 
                return false;
	int rtx = addEntry(comt,adr,0);
	if (rtx == 0) return false;
	if (Forest::mcastAdr(adr)) {
		do {
			if (!Misc::readNum(in,lnk)) {
				removeEntry(rtx); return false;
			}
			int cLnk = ctt->getComtLink(comt,lnk);
			if (cLnk == 0) return false;
			addLink(rtx,cLnk);
		} while (Misc::verify(in,','));
	} else {
		if (!Misc::readNum(in,lnk)) {
			removeEntry(rtx); return false;
		}
		Misc::cflush(in,'\n');
		int cLnk = ctt->getComtLink(comt,lnk);
		if (cLnk == 0) return false;
		setLink(rtx,cLnk);
	}
	Misc::cflush(in,'\n');
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

// Print entry number rtx.
void RouteTable::writeEntry(ostream& out, int rtx) const {
	string s;
	out << getComtree(rtx) << " ";
	fAdr_t adr = getAddress(rtx);
	if (Forest::mcastAdr(adr)) {
	   	out << Forest::fAdr2string(adr,s) << " ";
		if (noLinks(rtx)) { out << "-\n"; return; }
		bool first = true;
		set<int>& rteLinks = getRteLinks(rtx);
		set<int>::iterator p;
		for (p = rteLinks.begin(); p != rteLinks.end(); p++) {
			if (first) first = false;
			else out << ",";
			out << ctt->getLink(*p);
		}
	} else 
		out << Forest::fAdr2string(adr,s) << " " << getLink(rtx);
	out << endl;
}

void RouteTable::write(ostream& out) const {
// Output human readable representation of routing table.
	out << rteMap->size() << endl;
	for (int rtx = firstRteIndex(); rtx != 0; rtx = nextRteIndex(rtx))
                writeEntry(out,rtx);
}
