/** @file IoProcessor.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "IoProcessor.h"

// Constructor for IoProcessor, allocates space and initializes private data
IoProcessor::IoProcessor(LinkTable *lt1, PacketStore *ps1) : lt(lt1), ps(ps1) {
	nRdy = 0; maxSockNum = -1;
	sockets = new fd_set;
	for (int i = 0; i <= Forest::MAXINTF; i++) ift[i].ipa = 0;
}

IoProcessor::~IoProcessor() {
	for (int i = 1; i <= Forest::MAXINTF; i++) 
		if (valid(i)) close(ift[i].sock);
	delete sockets;
}

/** Get the default interface.
 *  @return the first valid interface in the interface table
 */
int IoProcessor::getDefaultIface() const {
	for (int i = 1; i <= Forest::MAXINTF; i++)
		if (valid(i)) return i;
}

bool IoProcessor::setup(int i) {
// Setup an interface. Return true on success, false on failure.

	// create datagram socket
	ift[i].sock = Np4d::datagramSocket();
	if (ift[i].sock < 0) {
		cerr << "IoProcessor::setup: socket call failed\n";
                return false;
        }
	maxSockNum = max(maxSockNum, ift[i].sock);

	// bind it to an address/port
        if (!Np4d::bind4d(ift[i].sock, ift[i].ipa, Forest::ROUTER_PORT)) {
		cerr << "IoProcessor::setup: bind call failed, "
		     << "check interface's IP address\n";
                return false;
        }
	return true;
}

int IoProcessor::receive() { 
// Return next waiting packet or 0 if there is none. 
	if (nRdy == 0) { // if no ready interface check for new arrivals
		FD_ZERO(sockets);
		for (int i = 1; i <= Forest::MAXINTF; i++) {
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
			cerr << "IoProcessor::receive: select failed "
			     << cnt-1 << " times\n";
		}
		if (nRdy < 0) {
			fatal("IoProcessor::receive: select failed");
		}
		if (nRdy == 0) return 0;
		cIf = 0;
	}
	while (1) { // find next ready interface
		cIf++;
		if (cIf > Forest::MAXINTF) return 0; // should never reach here
		if (valid(cIf) && FD_ISSET(ift[cIf].sock,sockets)) {
			nRdy--; break;
		}
	}
	// Now, read the packet from the interface
	int nbytes;	  	// number of bytes in received packet
	int lnk;	  	// # of link on which packet received

	packet p = ps->alloc();
        if (p == 0) return 0;
        PacketHeader& h = ps->getHeader(p);
        buffer_t& b = ps->getBuffer(p);

	ipa_t sIpAdr; ipp_t sPort;
	nbytes = Np4d::recvfrom4d(ift[cIf].sock, (void *) &b[0], 1500,
				  sIpAdr, sPort);
	if (nbytes < 0) fatal("IoProcessor::receive: error in recvfrom call");

	ps->unpack(p);
        if (!ps->hdrErrCheck(p)) { ps->free(p); return 0; }
	lnk = (sPort == Forest::ROUTER_PORT ?
	       lt->lookup(sIpAdr,false) : lt->lookup(h.getSrcAdr(),true));
        if (lnk == 0 || cIf != lt->getInterface(lnk) ||
	    (sPort != lt->getPeerPort(lnk) && lt->getPeerPort(lnk) != 0)) {
		ps->free(p); return 0;
	}
        
        h.setIoBytes(nbytes);
        h.setInLink(lnk);
        h.setTunSrcIp(sIpAdr);
        h.setTunSrcPort(sPort);

        lt->postIcnt(lnk,nbytes);
        return p;
}

void IoProcessor::send(int p, int lnk) {
// Send packet on specified link and recycle storage.
	ipp_t farPort = lt->getPeerPort(lnk);
	if (farPort == 0) { ps->free(p); return; }

	ipa_t farIp = lt->getPeerIpAdr(lnk);
	int length = ps->getHeader(p).getLength();

	int rv, lim = 0;
	do {
		rv = Np4d::sendto4d(ift[lt->getInterface(lnk)].sock,
			(void *) ps->getBuffer(p), length,
			farIp, farPort);
	} while (rv == -1 && errno == EAGAIN && lim++ < 10);
	if (rv == -1) {
		cerr << "IoProcessor:: send: failure in sendto (errno="
		     << errno << ")\n";
		exit(1);
	}
	lt->postOcnt(lnk,length);
	ps->free(p);
}

bool IoProcessor::addEntry(int ifnum, ipa_t ipa, int brate, int prate) {
// Allocate and initialize a new interface table entry.
// Return true on success.
	if (ifnum < 1 || ifnum > Forest::MAXINTF) return false;
	if (valid(ifnum)) return false;
	ift[ifnum].ipa = ipa;
	ift[ifnum].maxbitrate = brate; ift[ifnum].maxpktrate = prate;
	return true;
}
bool IoProcessor::lookupEntry(int ifnum, ipa_t ipa, int brate, int prate) {
	if(ifnum < 1 || ifnum > Forest::MAXINTF) return false;
	if(!valid(ifnum)) return false;
	return ift[ifnum].ipa == ipa &&
	       ift[ifnum].maxbitrate == brate &&
	       ift[ifnum].maxpktrate == prate;
}
void IoProcessor::removeEntry(int ifnum) {
	if (ifnum >= 0 && ifnum <= Forest::MAXINTF)
		ift[ifnum].ipa = 0;
}

bool IoProcessor::checkEntry(int ifnum) {
	if (ift[ifnum].maxbitrate < Forest::MINBITRATE ||
	    ift[ifnum].maxbitrate > Forest::MAXBITRATE ||
	    ift[ifnum].maxpktrate < Forest::MINPKTRATE ||
	    ift[ifnum].maxpktrate > Forest::MAXPKTRATE)
		return false;
	int br = 0; int pr = 0;
	for (int i = 1; i <= Forest::MAXLNK; i++ ) {
		if (!lt->valid(i)) continue;
		if (lt->getInterface(i) == ifnum) {
			br += lt->getBitRate(i); pr += lt->getPktRate(i);
		}
	}
	if (br > ift[ifnum].maxbitrate || pr > ift[ifnum].maxpktrate)
		return false;
	return true;
}

int IoProcessor::readEntry(istream& in) {
// Read an entry from in and store it in the interface table.
// Return the interface number for the new entry.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete link table entry.
// Each entry consists of an interface#, the max bitrate of the
// interface (in Kb/s) and the max packet rate (in p/s).
//
// If the interface number specified in the input is already in use,
// the call to readEntry will fail, in which case 0 is returned.
// The call can also fail if the input is not formatted correctly.
//
// GetEntry also opens a socket for each new interface and
// initializes the sock field of the innterface table entry.
//
	int ifnum, brate, prate; ipa_t ipa;

	Misc::skipBlank(in);
	if ( !Misc::readNum(in,ifnum) || !Np4d::readIpAdr(in,ipa) ||
	     !Misc::readNum(in,brate) || !Misc::readNum(in,prate)) {
		return 0;
	}
	Misc::cflush(in,'\n');

	if (!addEntry(ifnum,ipa,brate,prate)) return 0;
	if (!checkEntry(ifnum)) {
		removeEntry(ifnum); return 0;
	}
	if (setup(ifnum)) return ifnum;
	removeEntry(ifnum);
	return 0;
}

bool IoProcessor::read(istream& in) {
// Read interface table entries from the input. The first line must contain an
// integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
	int num;
 	Misc::skipBlank(in);
        if (!Misc::readNum(in,num)) return false;
        Misc::cflush(in,'\n');
	for (int i = 1; i <= num; i++) {
		if (readEntry(in) == 0) {
			cerr << "Error in interface table entry #"
			     << i << endl;
			return false;
		}
	}
	return true;
}

void IoProcessor::writeEntry(ostream& out, int i) const {
// Print entry for interface i
	out << setw(2) << i << " ";
	Np4d::writeIpAdr(out,ift[i].ipa);
	out << " " << setw(6) << ift[i].maxbitrate
	   << " " << setw(6) << ift[i].maxpktrate
	   << endl;
}

void IoProcessor::write(ostream& out) const {
// Output human readable representation of link table.
	for (int i = 1; i <= Forest::MAXINTF; i++) 
		if (valid(i)) writeEntry(out,i);
}
