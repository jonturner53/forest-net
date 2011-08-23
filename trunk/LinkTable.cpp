/** @file LinkTable.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "LinkTable.h"

// Constructor for LinkTable, allocates space and initializes table.
LinkTable::LinkTable(int maxLnk1) : maxLnk(maxLnk1) {
	lnkTbl = new LinkInfo[maxLnk+1];
	links = new UiSetPair(maxLnk);
	ht = new UiHashTbl(maxLnk);
};
	
LinkTable::~LinkTable() { delete [] lnkTbl; delete links; delete ht; }

/** Add a link table entry.
 *  @param lnk is the number of the link for which an entry is requested;
 *  if the lnk == 0, a free link is allocated for this entry;
 *  if lnk != 0, it must specify a link that is not already in use
 *  @param peerIp is the IP address of the peer node
 *  @param peerPort is the port number of the peer node
 *  @return the link number of the entry on success, or 0 on failure.
 */
int LinkTable::addEntry(int lnk, ipa_t peerIp, ipp_t peerPort) {
	if (lnk == 0) lnk = links->firstOut();
	if (!links->isOut(lnk)) return 0;
	if (!ht->insert(hashkey(peerIp, peerPort),lnk)) return 0;

	links->swap(lnk);

	lnkTbl[lnk].peerIp = peerIp;
	setPeerPort(lnk,peerPort);
	setIface(lnk,0);
	setPeerAdr(lnk,0); setPeerType(lnk,UNDEF_NODE);
	setBitRate(lnk,Forest::MINBITRATE);
	setPktRate(lnk,Forest::MINPKTRATE);
	setAvailBitRate(lnk,Forest::MINBITRATE);
	setAvailPktRate(lnk,Forest::MINPKTRATE);
        return lnk;
}

bool LinkTable::setPeerPort(int lnk, ipp_t peerPort) {
	ht->remove(hashkey(getPeerIpAdr(lnk),getPeerPort(lnk)));
	if (!ht->insert(hashkey(getPeerIpAdr(lnk), peerPort),lnk))
		return false;
	lnkTbl[lnk].peerPort = peerPort;
	return true;
}

// Remove the table entry for lnk. Return true on success, false on failure.
void LinkTable::removeEntry(int lnk) {
	if (!links->isIn(lnk)) return;
	ht->remove(hashkey(getPeerIpAdr(lnk),getPeerPort(lnk)));
	links->swap(lnk); 
}

// Return true if entry is consistent, else false.
bool LinkTable::checkEntry(int lnk) {
	// the forest address of every peer must be a valid unicast address
	if (!Forest::validUcastAdr(getPeerAdr(lnk))) return false;

	// only a router may use the forest port number
	if (getPeerPort(lnk) == Forest::ROUTER_PORT &&
	    getPeerType(lnk) != ROUTER)
                return false;

	return true;
}

// Read an entry from is and store it in the link table.
// Return the link number for the new link.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete link table entry.
// Each entry consists of a link#, the IP address of the local
// endpoint, the IP address and port number of the peer,
// the type of the peer, the forest address of the peer
// and the allowed forest destination address for the peer.
// Finally, the entry includes a max bit rate for the link
// (in Kb/s) and a maximum packet rate (in p/s).
//
// If the link number specified in the input is already in use,
// the call to readEntry will fail, in which case 0 is returned.
// The call can also fail if the input is not formatted correctly.
//
int LinkTable::readEntry(istream& in) {
	int lnk, iface, bRate, pRate;
	ipa_t peerIp; ipp_t peerPort;
	ntyp_t peerType; int peerAdr;
	string typStr;

	Misc::skipBlank(in);
	if ( !Misc::readNum(in,lnk) ||
	     !Misc::readNum(in,iface) ||
	     !Np4d::readIpAdr(in,peerIp) || !Misc::verify(in,':') ||
             !Misc::readNum(in,peerPort) ||
	     !Misc::readWord(in,typStr) ||
	     !Forest::readForestAdr(in,peerAdr) ||
	     !Misc::readNum(in,bRate) || !Misc::readNum(in,pRate)) {
		return 0;
	}
	Misc::cflush(in,'\n');

	peerType = Forest::getNodeType(typStr);
	if (peerType == UNDEF_NODE) return 0;

	if (!addEntry(lnk,peerIp,peerPort)) return 0;
	setIface(lnk,iface); 
        setPeerType(lnk, peerType);
	setPeerAdr(lnk, peerAdr);
	setBitRate(lnk,bRate); setPktRate(lnk,pRate);

	if (!checkEntry(lnk)) { removeEntry(lnk); return 0; }

	return lnk;
}

// Read link table entries from the input. The first line must contain an
// integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
bool LinkTable::read(istream& in) {
	int num;
 	Misc::skipBlank(in);
        if (!Misc::readNum(in,num)) return false;
        Misc::cflush(in,'\n');
	for (int i = 1; i <= num; i++) {
		if (readEntry(in) == 0) {
			cerr << "Error reading link table entry # "
			     << i << endl;
			return false;
		}
	}
	return true;
}

void LinkTable::writeEntry(ostream& out, int i) const {
// Print entry for link i
	out << setw(5) << right << i << setw(6) << getIface(i) << "  ";
	
	string s;
	out << setw(12) << Np4d::ip2string(getPeerIpAdr(i),s)
	    << ":" << setw(5) << left << getPeerPort(i) << "  ";
	out << setw(10) << left << Forest::nodeType2string(getPeerType(i),s);

	out << " " << setw(10) << left << Forest::fAdr2string(getPeerAdr(i),s);
	
	out << " " << setw(6) << right << getBitRate(i)
	    << " " << setw(6) << right << getPktRate(i)
	    << endl;
}

void LinkTable::write(ostream& out) const {
// Output human readable representation of link table.
	out << links->getNumIn() << endl;
	out << "# link  iface    peerIp:port     peerType  peerAdr   ";
        out << "  bitRate pktRate\n";
	for (int i = firstLink(); i <= 0; i = nextLink(i)) 
		writeEntry(out,i);
}
