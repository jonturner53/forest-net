#include "ioProc.h"

// Constructor for ioProc, allocates space and initializes private data
ioProc::ioProc(lnkTbl *lt1) : lt(lt1) {
	nRdy = 0; maxSockNum = -1;
	sockets = new fd_set;
	for (int i = 0; i <= MAXINT; i++) ift[i].ipa = 0;
	// initialize destination socket address structures
	bzero(&dsa, sizeof(dsa));
	dsa.sin_family = AF_INET;
}

// Null destructor
ioProc::~ioProc() { }

bool ioProc::setup(int i) {
// Setup an interface. Return true on success, false on failure.

	// create datagram socket
	if ((ift[i].sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		cerr << "ioProc::setup: socket call failed\n";
                return false;
        }
	maxSockNum = max(maxSockNum, ift[i].sock);

	// bind it to an address/port
	sockaddr_in sa;
	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(ift[i].ipa);
	sa.sin_port = htons(LFS_PORT);
        if (bind(ift[i].sock, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		cerr << "ioProc::setup: bind call failed, "
		     << "check interface's IP address\n";
                return false;
        }
	return true;
}

int ioProc::lookup(ipa_t ipa) {
// Return the interface # associated with local ip address ipa.
// Return 0, if no match.
	for (int i = 1; i <= MAXINT; i++)
		if (ift[i].ipa == ipa) return i;
	return 0;
}

bool ioProc::addEntry(int ifnum, ipa_t ipa, int brate, int prate) {
// Allocate and initialize a new interface table entry.
// Return true on success.
	if (ifnum < 1 || ifnum > MAXINT) return false;
	if (valid(ifnum)) return false;
	ift[ifnum].ipa = ipa;
	ift[ifnum].maxbitrate = brate; ift[ifnum].maxpktrate = prate;
}

void ioProc::removeEntry(int ifnum) { ift[ifnum].ipa = 0; }

bool ioProc::checkEntry(int ifnum) {
	if (ift[ifnum].maxbitrate < MINBITRATE ||
	    ift[ifnum].maxbitrate > MAXBITRATE ||
	    ift[ifnum].maxpktrate < MINPKTRATE ||
	    ift[ifnum].maxpktrate > MAXPKTRATE)
		return false;
	int br = 0; int pr = 0;
	for (int i = 1; i <= MAXLNK; i++ ) {
		if (!lt->valid(i)) continue;
		if (lt->interface(i) == ifnum) {
			br += lt->bitRate(i); pr += lt->pktRate(i);
		}
	}
	if (br > ift[ifnum].maxbitrate || pr > ift[ifnum].maxpktrate)
		return false;
	return true;
}

int ioProc::getEntry(istream& is) {
// Read an entry from is and store it in the interface table.
// Return the interface number for the new entry.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete link table entry.
// Each entry consists of an interface#, the max bitrate of the
// interface (in Kb/s) and the max packet rate (in p/s).
//
// If the interface number specified in the input is already in use,
// the call to getEntry will fail, in which case Null is returned.
// The call can also fail if the input is not formatted correctly.
//
// GetEntry also opens a socket for each new interface and
// initializes the sock field of the innterface table entry.
//
	int ifnum, brate, prate, suc, pred;
	ipa_t ipa;
	ntyp_t t; int pa1, pa2, da1, da2;
	string typStr;

	misc::skipBlank(is);
	if ( !misc::getNum(is,ifnum) || !misc::getIpAdr(is,ipa) ||
	     !misc::getNum(is,brate) || !misc::getNum(is,prate)) {
		return Null;
	}
	misc::cflush(is,'\n');

	if (!addEntry(ifnum,ipa,brate,prate)) return Null;
	if (!checkEntry(ifnum)) {
		removeEntry(ifnum); return Null;
	}
	if (setup(ifnum)) return ifnum;
	removeEntry(ifnum);
	return Null;
}

bool operator>>(istream& is, ioProc& iop) {
// Read interface table entries from the input. The first line must contain an
// integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
	int num;
 	misc::skipBlank(is);
        if (!misc::getNum(is,num)) return false;
        misc::cflush(is,'\n');
	for (int i = 1; i <= num; i++) {
		if (iop.getEntry(is) == Null) {
			cerr << "Error in interface table entry #"
			     << i << endl;
			return false;
		}
	}
	return true;
}

void ioProc::putEntry(ostream& os, int i) const {
// Print entry for interface i
	os << setw(2) << i << " ";
	// ip address of interface
	os << ((ift[i].ipa >> 24) & 0xFF) << "." 
	   << ((ift[i].ipa >> 16) & 0xFF) << "." 
	   << ((ift[i].ipa >>  8) & 0xFF) << "." 
	   << ((ift[i].ipa      ) & 0xFF);
	// bitrate, pktrate
	os << " " << setw(6) << ift[i].maxbitrate
	   << " " << setw(6) << ift[i].maxpktrate
	   << endl;
}

ostream& operator<<(ostream& os, const ioProc& iop) {
// Output human readable representation of link table.
	for (int i = 1; i <= iop.MAXINT; i++) 
		if (iop.valid(i)) iop.putEntry(os,i);
	return os;
}
