#include "lcTbl.h"

lcTbl::lcTbl(int maxlc1) : maxlc(maxlc1) {
// Constructor for lcTbl, allocates space and initializes table.
	maxlc = min(MAXLC,maxlc);
	lct = new lctEntry[maxlc+1];
	numlc = 0;

	for (int i = 1; i <= maxlc; i++) {
		lct[i].ipa = Null; // null ip address identifies unused entry
		lct[i].voqlen = 0;
		lct[i].inbklg = 0;
		lct[i].outbklg = 0;
	}
}
	
lcTbl::~lcTbl() { delete [] lct; }

int lcTbl::lookup(ipa_t ipa) {
// Return index of link number matching peer (IP,port) pair.
// If no match, return Null.
	for (int i = 1; i <= numlc; i++) {
		if (lct[i].ipa == ipa) return i;
	}
	return Null;
}

int lcTbl::getEntry(istream& is) {
// Read an entry from is and store it in the link table.
// Return the link number for the new link.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete link table entry.
// Each entry consists of a linecard #, the IP address of the
// machine that implements that linecard, plus a max bit
// rate for the link (in Kb/s) and a maximum packet rate (in p/s).
//
// If the linecard number specified in the input is already in use,
// the call to getEntry will fail, in which case Null is returned.
// The call can also fail if the input is not formatted correctly.
//
	int lc, brate, prate;
	ipa_t ipa;

	misc::skipBlank(is);
	if ( !misc::getNum(is,lc)   || !misc::getIpAdr(is,ipa) || 
	     !misc::getNum(is,brate) || !misc::getNum(is,prate)) {
		return Null;
	}
	misc::cflush(is,'\n');

	if (lc < 1 || lc > numlc) return Null;
	if (lct[lc].ipa != 0) return Null;
        lct[lc].ipa  = ipa;
	lct[lc].maxbitrate = brate; lct[lc].maxpktrate = prate;
	lct[lc].iPkt = 0; lct[lc].oPkt = 0;
	lct[lc].iByt = 0; lct[lc].oByt = 0;

	return lc;
}

bool operator>>(istream& is, lcTbl& lct) {
// Read link table entries from the input. The first line must contain an
// integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
	int num;
 	misc::skipBlank(is);
        if (!misc::getNum(is,num)) return false;
        misc::cflush(is,'\n');
	lct.numlc = min(num,MAXLC);
	while (num--) {
		if (lct.getEntry(is) == Null) return false;
	}
	return true;
}

void lcTbl::putEntry(ostream& os, int i) const {
// Print entry for link i
	char tempstr[50];
	os << setw(2) << i << " ";
	// ip addresses for linecard
	os << ((lct[i].ipa >> 24) & 0xFF) << "." 
	   << ((lct[i].ipa >> 16) & 0xFF) << "." 
	   << ((lct[i].ipa >>  8) & 0xFF) << "." 
	   << ((lct[i].ipa      ) & 0xFF) ;
	// bitrate, pktrate, mindelta
	os << " " << setw(6) << lct[i].maxbitrate
	   << " " << setw(6) << lct[i].maxpktrate
	   << endl;
}

ostream& operator<<(ostream& os, const lcTbl& lct) {
// Output human readable representation of link table.
	for (int i = 1; i <= lct.numlc; i++) 
		if (lct.valid(i)) lct.putEntry(os,i);
	return os;
}
