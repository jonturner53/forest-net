#include "lnkTbl.h"

lnkTbl::lnkTbl(int nlnk1) : nlnk(nlnk1) {
// Constructor for lnkTbl, allocates space and initializes table.
	nlnk = min(MAXLNK,nlnk);
	ld = new lnkdata[nlnk+1];
	ht = new hashTbl(nlnk);

	// Null peer address identifies unused entry
	for (int i = 1; i <= nlnk; i++) { disable(i); }
};
	
lnkTbl::~lnkTbl() { delete [] ld; delete ht; }

bool lnkTbl::addEntry(int lnk, int intf, ipa_t pipa, ipa_t plfs, ntyp_t pTyp) {
// Add a link table entry for the lnk. The local endpoint's interface number
// and peer information of the entry are specified. Other fields
// can be initialized using the various set methods.
// Return true on success, false on failure.
	if (valid(lnk)) return false;
	ipa_t x = (pTyp != ROUTER ? plfs : pipa);
	if (!ht->insert(hashkey(pipa,x),lnk)) return false;
	ld[lnk].intf = intf; ld[lnk].pipa = pipa; ld[lnk].padr = plfs;
	ld[lnk].ptyp = pTyp; ld[lnk].pipp = (pTyp != ROUTER ? 0 : LFS_PORT);
	enable(lnk); // mark as valid entry (even tho some fields not yet set)
	return true;
}

bool lnkTbl::removeEntry(int lnk) {
// Remove the table entry for lnk. Return true on success, false on failure.
	if (!valid(lnk)) return false;
	uint32_t x = (ld[lnk].pTyp != ROUTER ?
			ld[lnk].padr : ipAdr(ld[lnk].intf));
	ht->remove(hashkey(ld[lnk].pipa,x));
	disable(lnk);  // mark entry as invalid
}

bool lnkTbl::checkEntry(int te) {
// Return true if entry is consistent, else false.
	// the lfs address of every peer must be a unicast address
	if (!ucastAdr(peerAdr(te))) return false;
	if (peerPort(te) == LFS_PORT && peerTyp(te) != ROUTER)
		return false;
	return true;
}

/*
bool lnkTbl::peerIpAdr(int lnk, ipa_t pipa) {
// Set peer's IP address. This requires changing hash table.
	lnkdata x = ld[lnk];
	if (!removeEntry(lnk)) return false;
	if (!ht->insert(hashkey(x.intf,pipa,x.pipp),lnk)) return false;
	x.pipa = pipa; ld[lnk] = x;
	return true;
}

bool lnkTbl::peerPort(int lnk, ipp_t pipp) {
// Set peer's IP port number. This requires changing hash table.
	lnkdata x = ld[lnk];
	if (!removeEntry(lnk)) return false;
	if (!ht->insert(hashkey(x.intf,x.pipa,pipp),lnk)) return false;
	x.pipp = pipp; ld[lnk] = x;
	return true;
}
*/

int lnkTbl::getEntry(istream& is) {
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
// the call to getEntry will fail, in which case Null is returned.
// The call can also fail if the input is not formatted correctly.
//
	int lnk, intf, brate, prate;
	ipa_t pipa, plfsa; ipp_t pipp;
	ntyp_t t;
	string typStr;

	misc::skipBlank(is);
	if ( !misc::getNum(is,lnk) ||
	     !misc::getNum(is,intf) ||
	     !misc::getIpAdr(is,pipa) || !misc::verify(is,':') ||
             !misc::getNum(is,pipp) ||
	     !misc::getWord(is,typStr) ||
	     !misc::getIpAdr(is,plfsa) ||
	     !misc::getNum(is,brate) || !misc::getNum(is,prate)) {
		return Null;
	}
	misc::cflush(is,'\n');

             if (typStr == "endsys") t = ENDSYS;
        else if (typStr == "router") t = ROUTER;
        else if (typStr == "controller") t = CONTROLLER;
        else return Null;

	if (!addEntry(lnk,intf,pipa,plfsa,t)) return Null;
	setPeerPort(lnk,pipp);
	bitRate(lnk) = brate; pktRate(lnk) = prate;

	if (!checkEntry(lnk)) { removeEntry(lnk); return Null; }

	return lnk;
}

bool operator>>(istream& is, lnkTbl& lt) {
// Read link table entries from the input. The first line must contain an
// integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
	int num;
 	misc::skipBlank(is);
        if (!misc::getNum(is,num)) return false;
        misc::cflush(is,'\n');
	for (int i = 1; i <= num; i++) {
		if (lt.getEntry(is) == Null) {
			cerr << "Error reading link table entry # "
			     << i << endl;
			return false;
		}
	}
	return true;
}

void lnkTbl::putEntry(ostream& os, int i) {
// Print entry for link i
	char tempstr[50];
	os << setw(2) << i << " " << ld[i].intf << " ";
	// peer ip address and port
	os << ((ld[i].pipa >> 24) & 0xFF) << "." 
	   << ((ld[i].pipa >> 16) & 0xFF) << "." 
	   << ((ld[i].pipa >>  8) & 0xFF) << "." 
	   << ((ld[i].pipa      ) & 0xFF) << ":" << ld[i].pipp;
	// peer type and address information
        switch (ld[i].ptyp) {
        case ENDSYS: 	  os << " endsys    "; break;
        case ROUTER: 	  os << " router    "; break;
        case CONTROLLER:  os << " controller"; break;
        default: fatal("lnkTbl::putEntry: undefined type");
        }
	os << ((ld[i].padr >> 24) & 0xFF) << "." 
	   << ((ld[i].padr >> 16) & 0xFF) << "." 
	   << ((ld[i].padr >>  8) & 0xFF) << "." 
	   << ((ld[i].padr      ) & 0xFF);
	// bitrate, pktrate, mindelta
	os << " " << setw(6) << bitRate(i)
	   << " " << setw(6) << pktRate(i) 
	   << " " << setw(6) << minDelta(i)
	   << endl;
}

ostream& operator<<(ostream& os, lnkTbl& lt) {
// Output human readable representation of link table.
	for (int i = 1; i <= lt.nlnk; i++) 
		if (lt.valid(i)) lt.putEntry(os,i);
	return os;
}
