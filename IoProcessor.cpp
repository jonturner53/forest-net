/** @file IoProcessor.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "IoProcessor.h"

// Constructor for IoProcessor, allocates space and initializes private data
IoProcessor::IoProcessor(int maxIface1, IfaceTable *ift1, LinkTable *lt1,
			 PacketStore *ps1, StatsModule *sm1)
			 : maxIface(maxIface1), ift(ift1),
			   lt(lt1), ps(ps1), sm(sm1) {
	nRdy = 0; maxSockNum = -1;
	sockets = new fd_set;
	sock = new int[maxIface+1];
}

IoProcessor::~IoProcessor() {
	int iface;
	for (iface = ift->firstIface(); iface != 0;
	     iface = ift->nextIface(iface))
		close(sock[iface]);
	delete sockets;
}

// Setup an interface. Return true on success, false on failure.
bool IoProcessor::setup(int i) {
	// create datagram socket
	sock[i]= Np4d::datagramSocket();
	if (sock[i] < 0) {
		cerr << "IoProcessor::setup: socket call failed\n";
                return false;
        }
	maxSockNum = max(maxSockNum, sock[i]);

	// bind it to an address/port
        if (!Np4d::bind4d(sock[i], ift->getIpAdr(i), Forest::ROUTER_PORT)) {
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
		for (int i = ift->firstIface(); i != 0; i = ift->nextIface(i)) {
			FD_SET(sock[i],sockets);
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
		if (ift->valid(cIf) && FD_ISSET(sock[cIf],sockets)) {
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
	nbytes = Np4d::recvfrom4d(sock[cIf], (void *) &b[0], 1500,
				  sIpAdr, sPort);
	if (nbytes < 0) fatal("IoProcessor::receive: error in recvfrom call");

	ps->unpack(p);
        if (!ps->hdrErrCheck(p)) { ps->free(p); return 0; }
	lnk = lt->lookup(sIpAdr, sPort);
	if (lnk == 0 && h.getPtype() == CONNECT)
		lnk = lt->lookup(sIpAdr, 0); // check for "startup" entry
        if (lnk == 0 || cIf != lt->getIface(lnk) ||
	    (sPort == Forest::ROUTER_PORT && lt->getPeerType(lnk) != ROUTER) ||
	    (sPort != Forest::ROUTER_PORT && lt->getPeerType(lnk) == ROUTER)) {
		ps->free(p); return 0;
	}
        
        h.setInLink(lnk); h.setIoBytes(nbytes);
        h.setTunSrcIp(sIpAdr); h.setTunSrcPort(sPort);

        sm->cntInLink(lnk,nbytes, (lt->getPeerType(lnk) == ROUTER));

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
		rv = Np4d::sendto4d(sock[lt->getIface(lnk)],
			(void *) ps->getBuffer(p), length,
			farIp, farPort);
	} while (rv == -1 && errno == EAGAIN && lim++ < 10);
	if (rv == -1) {
		cerr << "IoProcessor:: send: failure in sendto (errno="
		     << errno << ")\n";
		exit(1);
	}
	sm->cntOutLink(lnk,length, (lt->getPeerType(lnk) == ROUTER));
	ps->free(p);
}
