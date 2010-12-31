#include "lnkTbl.h"

lnkTbl::lnkTbl(int nlnk1) : nlnk(nlnk1) {
// Constructor for lnkTbl, allocates space and initializes table.
	nlnk = min(MAXLNK,nlnk);
	ld = new lnkdata[nlnk+1];

	// Null peer address identifies unused entry
	for (int i = 1; i <= nlnk; i++) { ld[i].padr = Null; }
};
	
lnkTbl::~lnkTbl() { delete [] ld; }

int lnkTbl::lookup(ipa_t pipa, ipp_t pipp) {
// Return index of link number matching peer (IP,port) pair.
// If no match, return Null.
	for (int i = 1; i <= nlnk; i++) {
		if (ld[i].pipa == pipa && ld[i].pipp == pipp)
			return i;
	}
	return Null;
}

int lnkTbl::getEntry(istream& is) {
// Read an entry from is and store it in the link table.
// Return the link number for the new link.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete link table entry.
// Each entry consists of a link#, the IP address and port number
// of the peer, the type of the peer, the wunet address
// of the peer. Finally, the entry includes a max bit
// rate for the link (in Kb/s) and a maximum packet rate (in p/s).
//
// If the link number specified in the input is already in use,
// the call to getEntry will fail, in which case Null is returned.
// The call can also fail if the input is not formatted correctly.
//
	int lnk, brate, prate, suc, pred;
	ipa_t pipa; ipp_t pipp;
	ntyp_t t; int pa;
	string typStr;

	misc::skipBlank(is);
	if ( !misc::getNum(is,lnk) ||
	     !misc::getIpAdr(is,pipa) || !misc::verify(is,':') ||
             !misc::getNum(is,pipp) ||
	     !misc::getWord(is,typStr) ||
             !misc::getNum(is,pa) ||
	     !misc::getNum(is,brate) || !misc::getNum(is,prate)) {
		return Null;
	}
	misc::cflush(is,'\n');

             if (typStr == "router") t = ROUTER;
        else if (typStr == "host") t = HOST;
        else return Null;

	if (ld[lnk].pipp != 0) return Null;
        ld[lnk].pipa  = pipa; ld[lnk].pipp  = pipp;
        ld[lnk].ptyp  = t; ld[lnk].padr  = pa;
	ld[lnk].bitrate = brate; ld[lnk].pktrate = prate;
	ld[lnk].mindelta = (prate != 0 ? int(1000000.0/prate) : 100000);
	ld[lnk].iPkt = 0; ld[lnk].oPkt = 0;
	ld[lnk].iByt = 0; ld[lnk].oByt = 0;

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
	while (num--) {
		if (lt.getEntry(is) == Null) return false;
	}
	return true;
}

void lnkTbl::putEntry(ostream& os, int i) const {
// Print entry for link i
	char tempstr[50];
	os << setw(2) << i << " ";
	// ip addresses and ports identifying link
	os << ((ld[i].pipa >> 24) & 0xFF) << "." 
	   << ((ld[i].pipa >> 16) & 0xFF) << "." 
	   << ((ld[i].pipa >>  8) & 0xFF) << "." 
	   << ((ld[i].pipa      ) & 0xFF) << ":" << ld[i].pipp;
	// peer type and address information
        switch (ld[i].ptyp) {
        case ROUTER: 	  os << " router"; break;
        case HOST: 	  os << " host"; break;
        default: fatal("lnkTbl::putEntry: undefined type");
        }
        os << " " << ld[i].padr;
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
