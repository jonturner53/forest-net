#include "ioProc.h"

// Constructor for ioProc, allocates space and initializes private data
ioProc::ioProc(lnkTbl *lt1, pktStore *ps1) : lt(lt1), ps(ps1) {
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
	sa.sin_port = htons(FOREST_PORT);
        if (bind(ift[i].sock, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		cerr << "ioProc::setup: bind call failed, "
		     << "check interface's IP address\n";
                return false;
        }
	return true;
}

int ioProc::receive() { 
// Return next waiting packet or Null if there is none. 
	if (nRdy == 0) { // if no ready interface check for new arrivals
		FD_ZERO(sockets);
		for (int i = 1; i <= MAXINT; i++) {
			if (valid(i)) {	
				FD_SET(ift[i].sock,sockets);
			}
		}
		struct timeval zero; zero.tv_sec = zero.tv_usec = 0;
		int cnt = 0;
		do {
			nRdy = select(maxSockNum+1,sockets,
			       (fd_set *)NULL, (fd_set *)NULL, &zero);
		} while (nRdy < 0 && cnt++ < 10);
		if (cnt > 5) {
			cerr << "ioProc::receive: select failed "
			     << cnt-1 << " times\n";
		}
		if (nRdy < 0) {
			fatal("ioProc::receive: select failed");
		}
		if (nRdy == 0) return Null;
		cIf = 0;
	}
	while (1) { // find next ready interface
		cIf++;
		if (cIf > MAXINT) return Null; // should never reach here
		if (valid(cIf) && FD_ISSET(ift[cIf].sock,sockets)) {
			nRdy--; break;
		}
	}
	// Now, read the packet from the interface
	int nbytes;	  	// number of bytes in received packet
	sockaddr_in ssa;  	// socket address of sender
	socklen_t ssaLen;       // length of sender's socket address structure
	int lnk;	  	// # of link on which packet received
	ipa_t sIpAdr; ipp_t sPort; // IP address and port number of sender

	packet p = ps->alloc();
        if (p == 0) return 0;
        header& h = ps->hdr(p);
        buffer_t& b = ps->buffer(p);
        ssaLen = sizeof(ssa);

	nbytes = recvfrom(ift[cIf].sock, (void *) &b[0], 1500, 0,
	 	 	  (struct sockaddr *) &ssa, &ssaLen);
	if (nbytes < 0) fatal("ioProc::receive: error in recvfrom call");

	sIpAdr = ntohl(ssa.sin_addr.s_addr);
	sPort = ntohs(ssa.sin_port);

	ps->unpack(p);
        if (!ps->hdrErrCheck(p) ||
            (lnk = lt->lookup(cIf,sIpAdr,sPort,h.srcAdr())) == 0) {
                ps->free(p); return 0;
        }
        h.ioBytes() = nbytes;
        h.inLink() = lnk;
        h.tunSrcIp() = sIpAdr;
        h.tunSrcPort() = sPort;
        lt->postIcnt(lnk,nbytes);
        return p;
}

void ioProc::send(int p, int lnk) {
// Send packet on specified link and recycle storage.

	dsa.sin_addr.s_addr = htonl(lt->peerIpAdr(lnk));
	dsa.sin_port = htons(lt->peerPort(lnk));
	header h = ps->hdr(p);

	if (dsa.sin_port != 0) {
		int rv, lim = 0;
		do {
			rv = sendto(ift[lt->interface(lnk)].sock,
				(void *) ps->buffer(p), h.leng(),0,
			    	(struct sockaddr *) &dsa, sizeof(dsa));
		} while (rv == -1 && errno == EAGAIN && lim++ < 10);
		if (rv == -1) {
			cerr << "ioProc:: send: failure in sendto (errno="
			     << errno << ")\n";
			exit(1);
		}
		lt->postOcnt(lnk,h.leng());
	}
	ps->free(p);
}

bool ioProc::addEntry(int ifnum, ipa_t ipa, int brate, int prate) {
// Allocate and initialize a new interface table entry.
// Return true on success.
	if (ifnum < 1 || ifnum > MAXINTF) return false;
	if (valid(ifnum)) return false;
	ift[ifnum].ipa = ipa;
	ift[ifnum].maxbitrate = brate; ift[ifnum].maxpktrate = prate;
	return true;
}

void ioProc::removeEntry(int ifnum) {
	if (ifnum >= 0 && ifnum <= MAXINTF)
		ift[ifnum].ipa = 0;
}

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
