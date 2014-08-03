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
 */
RouteTable::RouteTable(int maxRtx1, fAdr_t myAdr1, ComtreeTable *ctt1)
		      : maxRtx(maxRtx1), myAdr(myAdr1), ctt(ctt1) {
	rteMap = new HashMap<uint64_t,Vset,Hash::u64>(maxRtx,false);
	clMap  = new HashMap<uint64_t,Vset,Hash::u64>(maxRtx,false);
}
	
/** Destructor for RouteTable, frees dynamic storage. */
RouteTable::~RouteTable() { delete rteMap; delete clMap; }

/** Add a new route to the table.
 *  @param comt is the comtree number for the route
 *  @param adr is the destination address for the route
 *  @param cLnk is a comtree link number for a link in the comtree;
 *  for multicast routes, this designates an initial subscriber link;
 *  if cLnk=0, there is no initial subscriber link
 *  @return the index of the new route, or 0 if the operation fails
 */
int RouteTable::addRoute(comt_t comt, fAdr_t adr, int cLnk) {
	Vset links;
	int rtx = rteMap->put(rmKey(comt,adr),links);
	if (rtx == 0) return 0;
	if (cLnk == 0) return rtx;
	rteMap->getValue(rtx).insert(cLnk);
	// now update clMap
	uint64_t kee = cmKey(comt,cLnk);
	int x = clMap->find(kee);
	if (x == 0) {
		Vset new_routes;
		x = clMap->put(kee,new_routes);
	}
	Vset& routes = clMap->getValue(x);
	routes.insert(rtx);
	return rtx;
}

/** Remove a route from the table.
 *  @param rtx is the route index of the route to be removed.
 */
void RouteTable::removeRoute(int rtx) {
	if (!validRtx(rtx)) return;
	Vset& lset = rteMap->getValue(rtx);
	for (int cLnk = lset.first(); cLnk != 0; cLnk = lset.next(cLnk)) {
		uint64_t kee = cmKey(getComtree(rtx),cLnk);
		Vset& routes = clMap->get(kee);
		routes.remove(rtx);
		if (routes.size() == 0) clMap->remove(kee);
	}
	lset.clear();
	rteMap->remove(rteMap->getKey(rtx));
}

/** Remove a comtree link from all routes that use it.
 *  This method also removes routes for which the given cLnk is
 *  the only one used by the route.
 *  @param comt is a comtree number
 *  @param cLnk is a comtree link number for comt
 */
void RouteTable::purge(comt_t comt, int cLnk) {
	uint64_t kee = cmKey(comt,cLnk);
        Vset& routes = clMap->get(kee);
	for (int rtx = routes.first(); rtx != 0; rtx = routes.next(rtx)) {
		Vset& lset = rteMap->getValue(rtx);
		lset.remove(cLnk);
		if (lset.size() == 0)
			rteMap->remove(rteMap->getKey(rtx));
        }
        routes.clear();
	clMap->remove(kee);
}

/** Read an entry from an input stream and add a routing table entry for it.
 *  An entry consists of a comtree number, a forest address and
 *  either a single link number, or a comma-separated list of links.
 *  The forest address may be a unicast address in dotted decimal
 *  notation, or a multicast address, which is a single negative integer.
 *  An entry must appear on one line. Lines may contain comments,
 *  which start with a # sign and continue to the end of the line.
 *  @param in is an open input stream
 *  @return true on success, false on failure
 */
bool RouteTable::readRoute(istream& in) {
	int comt, lnk; fAdr_t adr;
        Util::skipBlank(in);
        if (!Util::readInt(in, comt) || !Forest::readForestAdr(in,adr)) 
                return false;
	int rtx = addRoute(comt,adr,0);
	if (rtx == 0) return false;
	if (Forest::mcastAdr(adr)) {
		do {
			if (!Util::readInt(in,lnk)) {
				removeRoute(rtx); return false;
			}
			int cLnk = ctt->getClnkNum(comt,lnk);
			if (cLnk == 0) return false;
			addLink(rtx,cLnk);
		} while (Util::verify(in,','));
	} else {
		if (!Util::readInt(in,lnk)) {
			removeRoute(rtx); return false;
		}
		Util::nextLine(in);
		int cLnk = ctt->getClnkNum(comt,lnk);
		if (cLnk == 0) return false;
		setLink(rtx,cLnk);
	}
	Util::nextLine(in);
	return true;
}

/** Read routing table entries from input stream.
 *  The first input line must contain an integer, giving the number
 *  of entries to be read.
 *  @param in is an open input stream
 */
bool RouteTable::read(istream& in) {
	int num;
	Util::skipBlank(in);
	if (!Util::readInt(in,num)) return false;
	Util::nextLine(in);
	for (int i = 1; i <= num; i++) {
		if (!readRoute(in)) {
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
string RouteTable::entry2string(int rtx) const {
	stringstream ss;
	ss << getComtree(rtx) << " "
   	   << Forest::fAdr2string(getAddress(rtx)) << " ";
	if (noLinks(rtx)) { ss << "-\n"; return ss.str(); }
	Vset& lset = rteMap->getValue(rtx);
	for (int cLnk = lset.first(); cLnk != 0; cLnk = lset.next(cLnk)) {
		if (cLnk != lset.first()) ss << ",";
		int ctx = ctt->getComtIndex(getComtree(rtx));
		ss << ctt->getLink(ctx, cLnk);
	}
	ss << endl;
	return ss.str();
}

/** Create a string representing the table.
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string RouteTable::toString() const {
	stringstream ss;
	ss << rteMap->size() << endl;
	for (int rtx = firstRtx(); rtx != 0; rtx = nextRtx(rtx))
                ss << entry2string(rtx);
	return ss.str();
}

} // ends namespace

