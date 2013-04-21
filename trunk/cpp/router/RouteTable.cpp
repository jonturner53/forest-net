/** @file RouteTable.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RouteTable.h"

namespace forest {


/** Constructor for RouteTable, allocates space and initializes table.
 *  @param maxRtx1 is the maximum number of entries in the table
 *  @param myAdr1 is the forest of the address of the router
 *  @param ctt1 is a pointer to the comtree table
 */
RouteTable::RouteTable(int maxRtx1, fAdr_t myAdr1, ComtreeTable* ctt1)
		      : maxRtx(maxRtx1), myAdr(myAdr1), ctt(ctt1) {
	tbl = new RouteEntry[maxRtx+1];
	rteMap = new IdMap(maxRtx);
};
	
/** Destructor for RouteTable, frees dynamic storage. */
RouteTable::~RouteTable() { delete [] tbl; delete rteMap; }

/** Add a new route to the table.
 *  @param comt is the comtree number for the route
 *  @param adr is the destination address for the route
 *  @param cLnk is a comtree link number for a link in the comtree;
 *  for multicast routes, this designates an initial subscriber link;
 *  if cLnk=0, there is no initial subscriber link
 *  @return the index of the new route, or 0 if the operation fails
 */
int RouteTable::addEntry(comt_t comt, fAdr_t adr, int cLnk) {
	if (!ctt->validComtLink(cLnk)) return 0;
	int rtx = rteMap->addPair(key(comt,adr));
	if (rtx == 0) return 0;
	tbl[rtx].ct = comt;
	if (Forest::mcastAdr(adr)) {
		tbl[rtx].adr = adr;
		tbl[rtx].links = new set<int>();
		if (cLnk != 0) {
			tbl[rtx].links->insert(cLnk);
        		ctt->registerRte(cLnk,rtx);
		}
	} else {
		int zip = Forest::zipCode(adr);
		tbl[rtx].adr =  (zip == Forest::zipCode(myAdr) ?
				adr : Forest::forestAdr(zip,0));
		tbl[rtx].lnk = cLnk;
	}
	return rtx;
}

/** Remove a route from the table.
 *  @param rtx is the route index of the route to be removed.
 */
void RouteTable::removeEntry(int rtx) {
	if (!validRteIndex(rtx)) return;
	if (Forest::mcastAdr(tbl[rtx].adr)) {
		set<int>& links = *(tbl[rtx].links);
		set<int>::iterator lp;
		for (lp = links.begin(); lp != links.end(); lp++)
			ctt->deregisterRte(*lp,rtx);
		delete tbl[rtx].links;
	}
	rteMap->dropPair(key(tbl[rtx].ct,tbl[rtx].adr));
}

/** Remove all route table entries for a specific comtree.
 *  @param comt is a comtree number
 */
void RouteTable::purgeRoutes(comt_t comt) {
	int ctx = ctt->getComtIndex(comt);
	set<int>& links = ctt->getLinks(ctx);
	set<int>::iterator lp;
	for (lp = links.begin(); lp != links.end(); lp++) {
		int cLnk = *lp;
		set<int>& routes = ctt->getRteSet(cLnk);
		set<int>::iterator rp;
		int *rvec = new int[routes.size()]; int i = 0;
		for (rp = routes.begin(); rp != routes.end(); rp++)
			rvec[i++] = *rp;
		while (--i >= 0) removeEntry(rvec[i]);
		delete [] rvec;
	}
}

/** Read an entry from an input stream and add a routing table entry for it.
 *  An entry consists of a comtree number, a forest address and
 *  either a single link number, or a comma-separated list of links.
 *  The forest address may be a unicast address in dotted decimal
 *  notation, or a multicast address, which is a single negative integer.
 *  An entry must appear on one line. Lines may contain comments,
 *  which start wigth a # sign and continue to the end of the line.
 *  @param in is an open input stream
 *  @return true on success, false on failure
 */
bool RouteTable::readEntry(istream& in) {
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

/** Read routing table entries from input stream.
 *  The first input line must contain an integer, giving the number
 *  of entries to be read.
 *  @param in is an open input stream
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

/** Create a string representing a table entry.
 *  @param rtx is the route index for the route to be written
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& RouteTable::entry2string(int rtx, string& s) const {
	stringstream ss;
	ss << getComtree(rtx) << " ";
	fAdr_t adr = getAddress(rtx);
	if (Forest::mcastAdr(adr)) {
	   	ss << Forest::fAdr2string(adr,s) << " ";
		if (noLinks(rtx)) { ss << "-\n"; s = ss.str(); return s; }
		bool first = true;
		set<int>& subLinks = getSubLinks(rtx);
		set<int>::iterator p;
		for (p = subLinks.begin(); p != subLinks.end(); p++) {
			if (first) first = false;
			else ss << ",";
			ss << ctt->getLink(*p);
		}
	} else 
		ss << Forest::fAdr2string(adr,s) << " "
		    << ctt->getLink(getLink(rtx));
	ss << endl;
	s = ss.str();
	return s;
}

/** Create a string representing the table.
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& RouteTable::toString(string& s) const {
	stringstream ss;
	ss << rteMap->size() << endl;
	for (int rtx = firstRteIndex(); rtx != 0; rtx = nextRteIndex(rtx))
                ss << entry2string(rtx,s);
	s = ss.str();
	return s;
}

} // ends namespace

