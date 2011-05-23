#include "lnkTbl.h"

lnkTbl::lnkTbl(int nlnk1) : nlnk(nlnk1) {
// Constructor for lnkTbl, allocates space and initializes table.
	nlnk = min(MAXLNK,nlnk);
	ld = new lnkdata[nlnk+1];
	ht = new UiHashTbl(nlnk);

	iPkt = 0; oPkt = 0; iByt = 0; oByt = 0;
	irPkt = 0; orPkt = 0; icPkt = 0; ocPkt = 0;

	// Null peer address identifies unused entry
	for (int i = 1; i <= nlnk; i++) { disable(i); }
};
	
lnkTbl::~lnkTbl() { delete [] ld; delete ht; }

bool lnkTbl::addEntry(int lnk, int intf, ntyp_t pTyp, ipa_t pipa, fAdr_t pfa) {
// Add a link table entry for the specified lnk on the specified interface.
// PTyp is the peer's node type, pipa is the peer's IP address,
// and pfa is the peer's forest address.
// Return true on success, false on failure.
        uint32_t x = (pTyp != ROUTER ? pfa : pipa); 
        if (!ht->insert(hashkey(pipa,x),lnk)) return false;
        ld[lnk].intf = intf; ld[lnk].pipa = pipa; ld[lnk].padr = pfa;
        ld[lnk].ptyp = pTyp; ld[lnk].pipp = (pTyp != ROUTER ? 0 : FOREST_PORT);
        enable(lnk); // mark as valid entry (even tho some fields not yet set)
        return true;
}

bool lnkTbl::removeEntry(int lnk) {
// Remove the table entry for lnk. Return true on success, false on failure.
	if (!valid(lnk)) return false;
	uint32_t x = (ld[lnk].ptyp != ROUTER ?
                       ld[lnk].padr : ld[lnk].pipa);
	ht->remove(hashkey(ld[lnk].pipa,x));
	disable(lnk);  // mark entry as invalid
}

bool lnkTbl::checkEntry(int te) {
// Return true if entry is consistent, else false.
	// the forest address of every peer must be a valid unicast address
	if (!forest::ucastAdr(peerAdr(te))) return false;

	// the forest address of restricted destination must be unicast
	// or zero
	if (peerDest(te) != 0 && !forest::ucastAdr(peerDest(te))) return false;

	// only a router may use the forest port number
	if (peerPort(te) == FOREST_PORT && peerTyp(te) != ROUTER)
                return false;

	return true;
}

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
	ipa_t pipa; ipp_t pipp;
	ntyp_t t; int pa, pd;
	string typStr;

	Misc::skipBlank(is);
	if ( !Misc::readNum(is,lnk) ||
	     !Misc::readNum(is,intf) ||
	     !Np4d::readIpAdr(is,pipa) || !Misc::verify(is,':') ||
             !Misc::readNum(is,pipp) ||
	     !Misc::readWord(is,typStr) ||
	     !forest::getForestAdr(is,pa) ||
	     !forest::getForestAdr(is,pd) ||
	     !Misc::readNum(is,brate) || !Misc::readNum(is,prate)) {
		return Null;
	}
	Misc::cflush(is,'\n');

             if (typStr == "client") t = CLIENT;
        else if (typStr == "server") t = SERVER;
        else if (typStr == "router") t = ROUTER;
        else if (typStr == "controller") t = CONTROLLER;
        else return Null;

	if (!addEntry(lnk,intf,t,pipa,pa)) return Null;
	peerPort(lnk) =  pipp;
	peerDest(lnk) = pd;
	bitRate(lnk) =  brate; pktRate(lnk) = prate;

	if (!checkEntry(lnk)) { removeEntry(lnk); return Null; }

	return lnk;
}

bool operator>>(istream& is, lnkTbl& lt) {
// Read link table entries from the input. The first line must contain an
// integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
	int num;
 	Misc::skipBlank(is);
        if (!Misc::readNum(is,num)) return false;
        Misc::cflush(is,'\n');
	for (int i = 1; i <= num; i++) {
		if (lt.getEntry(is) == Null) {
			cerr << "Error reading link table entry # "
			     << i << endl;
			return false;
		}
	}
	return true;
}

void lnkTbl::putEntry(ostream& os, int i) const {
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
        case CLIENT: 	  os << " client    "; break;
        case SERVER: 	  os << " server    "; break;
        case ROUTER: 	  os << " router    "; break;
        case CONTROLLER:  os << " controller"; break;
        default: fatal("lnkTbl::putEntry: undefined type");
        }
        os << " " << (ld[i].padr  >> 16)
           << "." << (ld[i].padr  & 0xffff)
           << " " << (ld[i].dadr  >> 16)
           << "." << (ld[i].dadr  & 0xffff);
	// bitrate, pktrate, mindelta
	os << " " << setw(6) << ld[i].bitrate
	   << " " << setw(6) << ld[i].pktrate
	   << " " << setw(6) << ld[i].mindelta
	   << endl;
}

ostream& operator<<(ostream& os, const lnkTbl& lt) {
// Output human readable representation of link table.
	for (int i = 1; i <= lt.nlnk; i++) 
		if (lt.valid(i)) lt.putEntry(os,i);
	return os;
}
