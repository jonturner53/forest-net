/** @file LinkTable.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "LinkTable.h"

namespace forest {

/** Constructor for LinkTable, allocates space and initializes table. */
LinkTable::LinkTable(int maxLnk1) : maxLnk(maxLnk1) {
	lnkTbl = new LinkInfo[maxLnk+1];
	links = new UiSetPair(maxLnk);
	ht = new UiHashTbl(maxLnk);
	padrMap = new UiHashTbl(maxLnk);
};
	
/** Destructor for LinkTable, frees dynamic storage. */
LinkTable::~LinkTable() {
	delete [] lnkTbl; delete links; delete ht; delete padrMap;
}

/** Add a link table entry.
 *  @param lnk is the number of the link for which an entry is requested;
 *  if the lnk == 0, a free link is allocated for this entry;
 *  if lnk != 0, it must specify a link that is not already in use
 *  @param peerIp is the IP address of the peer node
 *  @param peerPort is the port number of the peer node
 *  @return the link number of the entry on success, or 0 on failure.
 */
int LinkTable::addEntry(int lnk, ipa_t peerIp, ipp_t peerPort, uint64_t nonce) {
	if (lnk == 0) lnk = links->firstOut();
	if (lnk == 0 || !links->isOut(lnk)) return 0;
	if (peerIp != 0 && peerPort != 0) {
		if (ht->lookup(hashkey(peerIp, peerPort)) != 0) return 0;
		if (!ht->insert(hashkey(peerIp, peerPort),lnk)) return 0;
	} else {
		if (ht->lookup(nonce) != 0) return 0;
		if (!ht->insert(nonce,lnk)) return 0;
	}
	links->swap(lnk);
	lnkTbl[lnk].peerIp = peerIp;
	lnkTbl[lnk].peerPort = peerPort;
	lnkTbl[lnk].nonce = nonce;
	lnkTbl[lnk].peerAdr = 0;
	setIface(lnk,0);
	setPeerType(lnk,Forest::UNDEF_NODE);
	getRates(lnk).set(Forest::MINBITRATE,Forest::MINBITRATE,
			  Forest::MINPKTRATE,Forest::MINPKTRATE);
	getAvailRates(lnk).set(	Forest::MINBITRATE,Forest::MINBITRATE,
			 	Forest::MINPKTRATE,Forest::MINPKTRATE);
	lnkTbl[lnk].comtSet = new set<int>();
        return lnk;
}

/** Remap an entry added earlier using a nonce.
 *  @param peerIp is the IP address of this link's peer
 *  @param peerPort is the port number of this link's peer
 *  @return true on success, false on failure
 */
bool LinkTable::remapEntry(int lnk, ipa_t peerIp, ipp_t peerPort) {
	if (!links->isIn(lnk)) return false;
	if (ht->lookup(lnkTbl[lnk].nonce) != lnk) return false;
	ht->remove(lnkTbl[lnk].nonce);
	if (!ht->insert(hashkey(peerIp, peerPort),lnk)) {
		ht->insert(lnkTbl[lnk].nonce,lnk); return false;
	}
	lnkTbl[lnk].peerIp = peerIp;
	lnkTbl[lnk].peerPort = peerPort;
        return true;
}

/** Revert an entry that was remapped earlier.
 *  @return true on success, false on failure
 */
bool LinkTable::revertEntry(int lnk) {
	if (!links->isIn(lnk)) return false;
	ipa_t peerIp = lnkTbl[lnk].peerIp;
	ipp_t peerPort = lnkTbl[lnk].peerPort;
	if (ht->lookup(hashkey(peerIp, peerPort)) != lnk) return false;
	ht->remove(hashkey(peerIp, peerPort));
	if (!ht->insert(lnkTbl[lnk].nonce,lnk)) {
		ht->insert(hashkey(peerIp, peerPort),lnk);
		return false;
	}
	lnkTbl[lnk].peerIp = 0;
	lnkTbl[lnk].peerPort = 0;
        return true;
}

/** Set the port number for the peer at the far end of a link.
 *  @param lnk is a valid link number
 *  @param peerPort is the port number of the peer node
 *  @return true on success, false on failure
bool LinkTable::setPeerIpPort(int lnk, ipa_t peerIp, ipp_t peerPort) {
	if (!valid(lnk)) return false;
	ht->remove(hashkey(getPeerIpAdr(lnk),getPeerPort(lnk)));
	if (!ht->insert(hashkey(peerIp, peerPort),lnk)) {
		ht->insert(hashkey(getPeerIpAdr(lnk), getPeerPort(lnk)),lnk);
		return false;
	}
	lnkTbl[lnk].peerIp = peerIp;
	lnkTbl[lnk].peerPort = peerPort;
	return true;
}
 */

/** Set the Forest address of the peer for a given link.
 *  @param lnk is a valid link number
 *  @param adr is a Forest unicast address
 */
void LinkTable::setPeerAdr(int lnk, fAdr_t adr) {
	if (!valid(lnk)) return;
	if (getPeerAdr(lnk) != 0 && getPeerType(lnk) != Forest::ROUTER) {
		uint64_t x = lnkTbl[lnk].peerAdr; x <<= 32;
		x |= lnkTbl[lnk].peerAdr;
		padrMap->remove(x);
	}
	if (adr != 0 && getPeerType(lnk) != Forest::ROUTER) {
		uint64_t x = adr; x <<= 32; x |= adr;
		padrMap->insert(x,lnk);
	}
	lnkTbl[lnk].peerAdr = adr;
}


/** Remove the table entry for a link.
 *  @param lnk is the link number for the entry to be deleted
 */
void LinkTable::removeEntry(int lnk) {
	if (!links->isIn(lnk)) return;
	ht->remove(hashkey(getPeerIpAdr(lnk),getPeerPort(lnk)));
	delete lnkTbl[lnk].comtSet;
	links->swap(lnk); 
}

/** Check if a table entry is consistent.
 *  @param lnk is the link number for the entry to be checked
 *  @return true if entry is consistent, else false
 */
bool LinkTable::checkEntry(int lnk) {
	// the forest address of every peer must be a valid unicast address
	if (!Forest::validUcastAdr(getPeerAdr(lnk))) return false;

	// only a router may use the forest port number
	if (getPeerPort(lnk) == Forest::ROUTER_PORT &&
	    getPeerType(lnk) != Forest::ROUTER)
                return false;

	return true;
}

/** Read an entry from an input stream and store it in the link table.
 *  Each entry must be on its own line, possibly followed by a comment.
 *  A comment begins with a # sign and continues to the end of the link.
 *  A non-blank line that does not being with a comment is assumed to
 *  contain an entry.
 *  Each entry consists of a link#, the IP address of the local
 *  endpoint, the IP address and port number of the peer,
 *  the type of the peer, the forest address of the peer,
 *  the maximum bit rate for the link (in Kb/s) and 
 *  its maximum packet rate (in p/s).
 * 
 *  If the link number specified in the input is already in use,
 *  the call to readEntry will fail, in which case 0 is returned.
 *  The call can also fail if the input is not formatted correctly.
 *  @param in is an open input stream
 *  @return the link number of the entry, or 0 if the operation fails
 */
int LinkTable::readEntry(istream& in) {
	int lnk, iface; RateSpec rs;
	ipa_t peerIp; ipp_t peerPort;
	Forest::ntyp_t peerType; int peerAdr;
	string typStr;

	Misc::skipBlank(in);
	if ( !Misc::readNum(in,lnk) ||
	     !Misc::readNum(in,iface) ||
	     !Np4d::readIpAdr(in,peerIp) || !Misc::verify(in,':') ||
             !Misc::readNum(in,peerPort) ||
	     !Misc::readWord(in,typStr) ||
	     !Forest::readForestAdr(in,peerAdr) ||
	     !rs.read(in)) {
		return 0;
	}
	Misc::cflush(in,'\n');

	peerType = Forest::getNodeType(typStr);
	if (peerType == Forest::UNDEF_NODE) return 0;

	if (!addEntry(lnk,peerIp,peerPort,0)) return 0;
	setIface(lnk,iface); 
        setPeerType(lnk, peerType);
	setPeerAdr(lnk, peerAdr);
	getRates(lnk) = rs; getAvailRates(lnk) = rs;

	if (!checkEntry(lnk)) { removeEntry(lnk); return 0; }

	return lnk;
}

/** Read link table entries from the input.
 *  The first line must contain an
 *  integer, giving the number of entries to be read. The input may
 *  include blank lines and comment lines (any text starting with '#').
 *  Each entry must be on a line by itself (possibly with a trailing comment).
 *  If an error is encountered, a message is sent to cerr identifying the
 *  the input entry that caused the error.
 *  @param in is an open input stream
 *  @return true on success, false on failure
 */
bool LinkTable::read(istream& in) {
	int num;
 	Misc::skipBlank(in);
        if (!Misc::readNum(in,num)) return false;
        Misc::cflush(in,'\n');
	for (int i = 1; i <= num; i++) {
		if (readEntry(in) == 0) {
			cerr << "LinkTable::read: could not read "
			     << i << "-th table entry" << endl;
			return false;
		}
	}
	return true;
}

/** Create a string representing a table entry.
 *  @param lnk is the number of the link to be written out
 *  @param s is a reference to a string in which the result is returned
 *  @return a reference to s
 */
string& LinkTable::link2string(int lnk, string& s) const {
	if (!valid(lnk)) { s = ""; return s; }
	stringstream ss;
	ss << setw(5) << right << lnk << setw(6) << getIface(lnk) << "  ";
	
	ss << setw(12) << Np4d::ip2string(getPeerIpAdr(lnk),s)
	   << ":" << setw(5) << left << getPeerPort(lnk) << "  ";
	ss << setw(10) << left << Forest::nodeType2string(getPeerType(lnk),s);
	ss << " " << setw(10) << left <<Forest::fAdr2string(getPeerAdr(lnk),s);
	ss << " " << getRates(lnk).toString(s);
	ss << " " << getAvailRates(lnk).toString(s);
	ss << " " << lnkTbl[lnk].comtCount;
	s = ss.str();
	return s;
}

/** Create a string representing the table.
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& LinkTable::toString(string& s) const {
	stringstream ss;
	ss << links->getNumIn() << endl;
	ss << "# link  iface    peerIp:port     peerType  peerAdr   ";
        ss << "  rates      avail rates      comtree count\n";
	for (int i = firstLink(); i != 0; i = nextLink(i)) 
		ss << link2string(i,s) << endl;
	s = ss.str();
	return s;
}

#define pack8(x)  (*buf = *((char*)  x), buf += 1)
#define pack16(x) (*buf = *((char*) htons(x)), buf += sizeof(uint16_t))
#define pack32(x) (*buf = *((char*) htonl(x)), buf += sizeof(uint32_t))
#define pack64(x) ( pack32(((x)>>32)&0xffffffff), pack32((x)&0xffffffff))
#define packRspec(x) (	pack32(x.bitRateUp), pack32(x.bitRateDown), \
                	pack32(x.pktRateUp), pack32(x.pktRateDown) )

/** Pack a link table entry into a packet buffer.
 *  Omit the comtSet, but include the number of elements it contains.
 *  @return true on success, false on failure
 */
char* LinkTable::pack(int lnk, char *buf) const {
	if (!valid(lnk)) return 0;
	pack32(lnk);
	pack32(lnkTbl[lnk].iface);

	pack32(lnkTbl[lnk].peerIp);
	pack16(lnkTbl[lnk].peerPort);
	uint16_t x = lnkTbl[lnk].peerType; pack16(x);
	pack16(lnkTbl[lnk].peerType);
	pack32(lnkTbl[lnk].peerAdr);

	pack8(lnkTbl[lnk].status);
	pack64(lnkTbl[lnk].nonce);

	packRspec(lnkTbl[lnk].rates);
	packRspec(lnkTbl[lnk].availRates);

	pack32(lnkTbl[lnk].comtCount);
	return buf;
}

#define unpack8(x) (x = *buf, buf += 1)
#define unpack16(x) (x = ntohs(*((uint16_t*) buf)), buf += sizeof(uint16_t))
#define unpack32(x) (x = ntohl(*((uint32_t*) buf)), buf += sizeof(uint32_t))
#define unpack64(x) (x  = ntohl(*((uint32_t*) buf)), buf += sizeof(uint32_t), \
		     x |= ntohl(*((uint32_t*) buf)), buf += sizeof(uint32_t))
#define unpackRspec(x) (pack32(x.bitRateUp), pack32(x.bitRateDown), \
                	pack32(x.pktRateUp), pack32(x.pktRateDown) )

/** Unpack a link table entry from a packet buffer.
 *  @return true on success, false on failure
 */
char* LinkTable::unpack(int lnk, char *buf) {
	if (!valid(lnk)) return 0;
	unpack32(lnk);
	unpack32(lnkTbl[lnk].iface);

	unpack32(lnkTbl[lnk].peerIp);
	unpack16(lnkTbl[lnk].peerPort);
	uint16_t x; unpack16(x); lnkTbl[lnk].peerType = (Forest::ntyp_t) x;
	unpack32(lnkTbl[lnk].peerAdr);

	unpack8(lnkTbl[lnk].status);
	unpack64(lnkTbl[lnk].nonce);

	unpackRspec(lnkTbl[lnk].rates);
	unpackRspec(lnkTbl[lnk].availRates);

	unpack32(lnkTbl[lnk].comtCount);
	return buf;
}

} // ends namespace

