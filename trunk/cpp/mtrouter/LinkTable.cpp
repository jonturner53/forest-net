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
	map = new HashMap<uint64_t,Entry,Hash::u64>(maxLnk,false);
	padrMap = new HashSet<fAdr_t,Hash::s32>(maxLnk,false);
};
	
/** Destructor for LinkTable, frees dynamic storage. */
LinkTable::~LinkTable() {
	delete map; delete padrMap;
}

/** Add a link table entry.
 *  @param lnk is the number of the link for which an entry is requested;
 *  @param peerIp is the IP address of the peer node.
 *  @param peerPort is the port number of the peer node.
 *  @param nonce is the nonce that is used to connect the link.
 *  @return the link number of the entry on success, or 0 on failure.
 *  if the lnk == 0, a free link is allocated for this entry;
 *  if lnk != 0, it must specify a link that is not already in use;
 *  the link is created in the disconnected state and when in this state,
 *  the hash table entry used to lookup the link number is based on the nonce;
 *  when the link becomes connected, this hash table entry is replaced with
 *  one based on the peerIp and peerPort
 */
int LinkTable::addEntry(int lnk, ipa_t peerIp, ipp_t peerPort, uint64_t nonce) {
	Entry e;
	e.iface= 0; e.isConnected = false; e.nonce = nonce;
	e.peerIp = peerIp; e.peerPort = peerPort;
	Np4d::initSockAdr(e.peerIp,e.peerPort,e.sa);
	e.peerAdr = 0; e.peerType = Forest::UNDEF_NODE;
	e.rates.set(Forest::MINBITRATE,Forest::MINBITRATE,
		    Forest::MINPKTRATE,Forest::MINPKTRATE);
	e.availRates.set(Forest::MINBITRATE,Forest::MINBITRATE,
			 Forest::MINPKTRATE,Forest::MINPKTRATE);

	if (lnk == 0) lnk = map->put(nonce,e);
	else lnk = map->put(nonce,e,lnk);
        return lnk;
}

/** Connect a link that was previously disconnected.
 *  This involves removing the nonce-based hash-table entry and replacing
 *  it with an entry using the peerIp and peerPort
 *  @param peerIp is the IP address of this link's peer
 *  @param peerPort is the port number of this link's peer
 *  @return true on success, false on failure
 */
bool LinkTable::connect(int lnk, ipa_t peerIp, ipp_t peerPort) {
cerr << "connect " << Np4d::ip2string(peerIp) << " " << peerPort << endl;
	if (!valid(lnk)) return false;
	Entry& e = getEntry(lnk);
	if (e.isConnected == true) return false; // already connected
	if (map->find(e.nonce) != lnk) return false;
	if (!map->rekey(lnk, hashkey(peerIp,peerPort))) return false;
	e.peerIp = peerIp; e.peerPort = peerPort; e.isConnected = true;
	Np4d::initSockAdr(e.peerIp,e.peerPort,e.sa);
        return true;
}

/** Remove the table entry for a link.
 *  @param lnk is the link number for the entry to be deleted
 *  @return true on success, else false
 */
bool LinkTable::removeEntry(int lnk) {
	if (!valid(lnk)) return false;
	Entry& e = getEntry(lnk);
	if (e.isConnected == true)
		map->remove(hashkey(e.peerIp,e.peerPort));
	else
		map->remove(e.nonce);
	return true;
}

/** Set the Forest address of the peer for a given link.
 *  @param lnk is a valid link number
 *  @param adr is a Forest unicast address
 */
void LinkTable::setPeerAdr(int lnk, fAdr_t adr) {
	if (!valid(lnk)) return;
	Entry& e = getEntry(lnk);

	if (e.peerAdr != 0) padrMap->remove(e.peerAdr);
	if (adr != 0) padrMap->insert(adr,lnk);
	e.peerAdr = adr;
}

/** Check if a table entry is consistent.
 *  @param lnk is the link number for the entry to be checked
 *  @return true if entry is consistent, else false
 */
bool LinkTable::checkEntry(int lnk) {
	if (!valid(lnk)) return false;
	Entry& e = getEntry(lnk);
	// the forest address of every peer must be a valid unicast address
	if (!Forest::validUcastAdr(e.peerAdr)) return false;

	// only a router may use the forest port number
	if (e.peerPort == Forest::ROUTER_PORT && e.peerType != Forest::ROUTER)
                return false;
	return true;
}

/** Remap an entry added earlier using a nonce.
 *  This modifies the entry so it can be looked up using the
 *  peer's (ip,port) pair.
 *  @param peerIp is the IP address of this link's peer
 *  @param peerPort is the port number of this link's peer
 *  @return true on success, false on failure
 */
bool LinkTable::remapEntry(int lnk, ipa_t peerIp, ipp_t peerPort) {
cerr << "remap " << Np4d::ip2string(peerIp) << " " << peerPort << endl;
	if (!valid(lnk)) return false;
	Entry& lte = getEntry(lnk);
	if (map->getKey(lnk) != lte.nonce) return false;
	if (!map->rekey(lnk,hashkey(peerIp,peerPort))) return false;
	lte.peerIp = peerIp; lte.peerPort = peerPort;
	Np4d::initSockAdr(peerIp,peerPort,lte.sa);
        return true;
}

/** Revert an entry that was remapped earlier.
 *  This modifies the entry so it can be looked up using the nonce.
 *  @return true on success, false on failure
 */
bool LinkTable::revertEntry(int lnk) {
cerr << "revert " << lnk << endl;
	if (!valid(lnk)) return false;
	Entry& lte = getEntry(lnk);
	if (map->getKey(lnk) != hashkey(lte.peerIp, lte.peerPort)) return false;
	if (!map->rekey(lnk,lte.nonce)) return false;
	lte.peerIp = lte.peerPort = 0;
	Np4d::initSockAdr(0,0,lte.sa);
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
	ipa_t peerIp; int peerPort;
	Forest::ntyp_t peerType;
	int peerAdr; uint64_t nonce;
	string typStr;

	Util::skipBlank(in);
	if ( !Util::readInt(in,lnk) ||
	     !Util::readInt(in,iface) ||
	     !Np4d::readIpAdr(in,peerIp) || !Util::verify(in,':') ||
             !Util::readInt(in,peerPort) ||
	     !Util::readWord(in,typStr) ||
	     !Forest::readForestAdr(in,peerAdr) ||
	     !rs.read(in) || !Util::readInt(in,nonce)) {
		return 0;
	}
	Util::nextLine(in);

	peerType = Forest::getNodeType(typStr);
	if (peerType == Forest::UNDEF_NODE) return 0;

	if (!addEntry(lnk,peerIp,peerPort,nonce)) return 0;
	Entry& e = getEntry(lnk);
	e.iface = iface; 
        e.peerType = (Forest::ntyp_t) peerType; e.peerAdr = peerAdr;
	e.rates = rs; e.availRates = rs;

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
 	Util::skipBlank(in);
        if (!Util::readInt(in,num)) return false;
        Util::nextLine(in);
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
string LinkTable::link2string(int lnk) const {
	if (!valid(lnk)) { return ""; }
	stringstream ss;
	ss << setw(5) << right << lnk;
	ss << getEntry(lnk);
	return ss.str();
}

/** Create a string representing the table.
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string LinkTable::toString() const {
	stringstream ss;
	ss << map->size() << endl;
	ss << "# link  iface    peerIp:port     peerType  peerAdr   ";
        ss << "  rates      nonce\n";
	for (int i = firstLink(); i != 0; i = nextLink(i)) 
		ss << link2string(i) << endl;
	return ss.str();
}

/*
#define pack8(x)  (*buf = *((char*)  x), buf += 1)
#define pack16(x) (*buf = *((char*) htons(x)), buf += sizeof(uint16_t))
#define pack32(x) (*buf = *((char*) htonl(x)), buf += sizeof(uint32_t))
#define pack64(x) ( pack32(((x)>>32)&0xffffffff), pack32((x)&0xffffffff))
#define packRspec(x) (	pack32(x.bitRateUp), pack32(x.bitRateDown), \
                	pack32(x.pktRateUp), pack32(x.pktRateDown) )
*/

/** Pack a link table entry into a packet buffer.
 *  @return true on success, false on failure
char* LinkTable::pack(int lnk, char *buf) const {
	if (!valid(lnk)) return 0;
	Entry& e = getEntry(lnk);
	pack32(lnk);
	pack32(e.iface);

	pack32(e.peerIp);
	pack16(e.peerPort);
	uint16_t x = e.peerType; pack16(x);
	pack16(e.peerType);
	pack32(e.peerAdr);

	pack8(e.isConnected);
	pack64(e.nonce);

	packRspec(e.rates);
	packRspec(e.availRates);

	return buf;
}
 */

/*
#define unpack8(x) (x = *buf, buf += 1)
#define unpack16(x) (x = ntohs(*((uint16_t*) buf)), buf += sizeof(uint16_t))
#define unpack32(x) (x = ntohl(*((uint32_t*) buf)), buf += sizeof(uint32_t))
#define unpack64(x) (x  = ntohl(*((uint32_t*) buf)), buf += sizeof(uint32_t), \
		     x |= ntohl(*((uint32_t*) buf)), buf += sizeof(uint32_t))
#define unpackRspec(x) (pack32(x.bitRateUp), pack32(x.bitRateDown), \
                	pack32(x.pktRateUp), pack32(x.pktRateDown) )
 */

/** Unpack a link table entry from a packet buffer.
 *  @return on success, return a pointer to the next position in the buffer;
 *  on failure, return 0
char* LinkTable::unpack(int lnk, char *buf) {
	if (!valid(lnk)) return 0;
	int lnk1 unpack32(lnk1);
	if (lnk1 != lnk) return 0;

	Entry& e = getEntry(lnk);
	unpack32(e.iface);

	unpack32(e.peerIp);
	unpack16(e.peerPort);
	uint16_t x; unpack16(x) e.peerType = (Forest::ntyp_t) x;
	unpack32(e.peerAdr);

	unpack8(e.isConnected);
	unpack64(e.nonce);

	unpackRspec(e.rates);
	unpackRspec(e.availRates);

	return buf;
}
 */

} // ends namespace

