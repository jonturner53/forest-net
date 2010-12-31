#include "ioProc.h"

// Constructor for ioProc, allocates space and initializes private data
ioProc::ioProc(ipa_t mip1, ipp_t mp1, lnkTbl *lt1, pktStore *ps1,
		lcTbl *lct1, int myLcn1)
		: myIpAdr(mip1) , myPort(mp1), lt(lt1), ps(ps1),
		  lct(lct1), myLcn(myLcn1) {
	// initialize socket address structures
	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(WUNET_PORT);
	sa.sin_addr.s_addr = htonl(myIpAdr);
	bzero(&dsa, sizeof(dsa));
	dsa.sin_family = AF_INET;
}

// Null destructor
ioProc::~ioProc() { }

bool ioProc::init() {
// Initialize IO. Return true on success, false on failure.
// Configure socket for non-blocking access, so that we don't
// block when there are no input packets available.
	// create datagram socket
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		cerr << "ioProc::init: socket call failed\n";
                return false;
        }
	// bind it to the socket address structure
        if (bind(sock, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		cerr << "ioProc::init: bind call failed, check router's IP address\n";
                return false;
        }
	// make socket nonblocking
	int flags;
	if ((flags = fcntl(sock, F_GETFL, 0)) < 0) {
		cerr << "ioProc::init: first fcntl call failed\n";
		return false;
	}
	flags |= O_NONBLOCK;
	if ((flags = fcntl(sock, F_SETFL, flags)) < 0) {
		cerr << "ioProc::init: second fcntl call failed\n";
		return false;
	}
	return true;
}

int ioProc::receive() { 
// Return next waiting packet or Null if there is none. 
	int nbytes;	  	// number of bytes in received packet
	sockaddr_in ssa;  	// socket address of sender
	socklen_t ssaLen;       // length of sender's socket address structure
	int lnk;	  	// # of link on which packet received
	ipa_t sIpAdr; ipp_t sPort;

	int p = ps->alloc();
	if (p == Null) return Null;
	buffer_t* bp = ps->buffer(p);

	ssaLen = sizeof(ssa); 
	nbytes = recvfrom(sock, (void *) bp, 1500, 0,
	 	 	  (struct sockaddr *) &ssa, &ssaLen);
	if (nbytes < 0) {
		if (errno == EWOULDBLOCK) {
			ps->free(p); return Null;
		}
		fatal("ioProc::receive: error in recvfrom call");
	}

	sIpAdr = ntohl(ssa.sin_addr.s_addr);
	sPort = ntohs(ssa.sin_port);

	lnk = 0;
	if (myLcn != 0) lnk = lct->lookup(sIpAdr); // check for another linecard
	if (lnk == 0) {
		lnk = lt->lookup(sIpAdr, sPort);
		if (lnk == 0) { // no matching link
			// so, look for a place-holder with 0 for peerPort
			// this allows hosts to use ephemeral ports
			lnk = lt->lookup(sIpAdr, 0);
			if (lnk == 0) { ps->free(p); return Null; }
			lt->setPeerPort(lnk,sPort);
		}
		if (myLcn != 0 && lnk != myLcn) { ps->free(p); return Null; }
	}

	ps->setIoBytes(p,nbytes);
	ps->setInLink(p,lnk);
	lt->postIcnt(lnk,nbytes);
	return p;
}

void ioProc::send(int p, int lnk) {
// Send packet on specified link and recycle storage.

	if (myLcn == 0 || lnk == myLcn) {
		dsa.sin_addr.s_addr = htonl(lt->peerIpAdr(lnk));
		dsa.sin_port = htons(lt->peerPort(lnk));
	} else {
		dsa.sin_addr.s_addr = htonl(lct->ipAdr(lnk));
		dsa.sin_port = htons(WUNET_PORT);
	}

	if (dsa.sin_port != 0) {
		int rv, lim = 0;
		do {
			rv = sendto(sock,(void *) ps->buffer(p),ps->leng(p),0,
			    	(struct sockaddr *) &dsa, sizeof(dsa));
		} while (rv == -1 && errno == EAGAIN && lim++ < 100);
		if (rv == -1) {
			cerr << "ioProc:: send: failure in sendto (errno="
			     << errno << ")\n";
			exit(1);
		}
		lt->postOcnt(lnk,ps->leng(p));
	}
	ps->free(p);
}
