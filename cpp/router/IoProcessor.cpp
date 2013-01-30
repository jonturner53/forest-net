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
	bootSock = -1;
	#ifdef PROFILING // MAH
    timer_send = new Timer("IoProcessor::send()");
    timer_receive = new Timer("IoProcessor::receive() excluding when no pkts received");
    timer_np4d_sendto4d = new Timer("IoProcessor::receive() -> timer_np4d_sendto4d()");
    timer_np4d_recvfrom4d = new Timer("IoProcessor::receive() -> timer_np4d_recvfrom4d()");
	#endif
}

IoProcessor::~IoProcessor() {
	int iface;
	for (iface = ift->firstIface(); iface != 0;
	     iface = ift->nextIface(iface))
		close(sock[iface]);
	delete sockets;
	#ifdef PROFILING // MAH
    cout << *timer_send << endl;
    cout << *timer_np4d_sendto4d << endl;
    cout << *timer_receive << endl;
    cout << *timer_np4d_recvfrom4d << endl;
    delete timer_send; delete timer_receive;
    delete timer_np4d_sendto4d; delete timer_np4d_recvfrom4d; 
	#endif
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
		string s;
		cerr << "IoProcessor::setup: bind call failed for "
		     << Np4d::ip2string(ift->getIpAdr(i),s) << ":"
		     << Forest::ROUTER_PORT << ", check interface's IP address\n";
                return false;
        }
	return true;
}

bool IoProcessor::setupBootSock(ipa_t bootIp, ipa_t nmIp1) {
	nmIp = nmIp1;
	// open datagram socket
	bootSock = Np4d::datagramSocket();
	if (bootSock < 0) {
		cerr << "IoProcessor::setupBootSock: socket call failed\n";
		return false;
	}
	// bind it to the bootIp
        if (!Np4d::bind4d(bootSock, bootIp, 0)) {
		cerr << "IoProcessor::setupBootIp: bind call failed, "
		     << "check boot IP address\n";
                return false;
        }
	return true;
}

void IoProcessor::closeBootSock() {
	close(bootSock); bootSock = -1;
}

int IoProcessor::receive() { 
    #ifdef PROFILING
    timer_receive->start();
    #endif
// Return next waiting packet or 0 if there is none. 
	if (bootSock >= 0) {
		int nbytes;	  	// number of bytes in received packet
	
		packet p = ps->alloc();
	        if (p == 0) { return 0; }
	        PacketHeader& h = ps->getHeader(p);
	        buffer_t& b = ps->getBuffer(p);
	
		ipa_t sIpAdr; ipp_t sPort;
		#ifdef PROFILING
        timer_np4d_recvfrom4d->start();
        #endif
		nbytes = Np4d::recvfrom4d(bootSock, (void *) &b[0], 1500,
					  sIpAdr, sPort);
		#ifdef PROFILING
        timer_np4d_recvfrom4d->stop();
        #endif
		h.setIoBytes(nbytes);
		if (nbytes < 0) {
			if (errno == EAGAIN) {
				ps->free(p); return 0;
			}
			fatal("IoProcessor::receive: error in recvfrom call");
		}
		if (sIpAdr != nmIp || sPort != Forest::NM_PORT) {
			ps->free(p); return 0;
		}
		ps->unpack(p);
	        if (!ps->hdrErrCheck(p)) { ps->free(p); return 0; }
        	h.setTunSrcIp(sIpAdr); h.setTunSrcPort(sPort);
		h.setInLink(0); h.setIoBytes(nbytes);
		#ifdef PROFILING // MAH
        timer_receive->stop();
        #endif
		return p;
	}
	// proceed to normal case, when we're not booting
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
	#ifdef PROFILING
    timer_np4d_recvfrom4d->start();
    #endif
	nbytes = Np4d::recvfrom4d(sock[cIf], (void *) &b[0], 1500,
				  sIpAdr, sPort);
	#ifdef PROFILING
    timer_np4d_recvfrom4d->stop();
    #endif
	if (nbytes < 0) fatal("IoProcessor::receive: error in recvfrom call");

	ps->unpack(p);
        if (!ps->hdrErrCheck(p)) { ps->free(p); return 0; }
	lnk = lt->lookup(sIpAdr, sPort);
	if (lnk == 0 && h.getPtype() == CONNECT)
		lnk = lt->lookup(sIpAdr, 0); // check for "startup" entry
        if (lnk == 0 || cIf != lt->getIface(lnk) ||
	    (sPort == Forest::ROUTER_PORT && lt->getPeerType(lnk) != ROUTER) ||
	    (sPort != Forest::ROUTER_PORT && lt->getPeerType(lnk) == ROUTER)) {
		string s;
		cerr << "bad packet " << h.toString(ps->getBuffer(p),s) << endl;
		ps->free(p); return 0;
	}
        
        h.setInLink(lnk); h.setIoBytes(nbytes);
        h.setTunSrcIp(sIpAdr); h.setTunSrcPort(sPort);

        sm->cntInLink(lnk,Forest::truPktLeng(nbytes),
		      (lt->getPeerType(lnk) == ROUTER));
		#ifdef PROFILING // MAH
        timer_receive->stop();
        #endif
        return p;
}

// Send packet on specified link and recycle storage.
void IoProcessor::send(int p, int lnk) {
    #ifdef PROFILING
    timer_send->start();
    #endif
	if (bootSock >= 0) {
		int rv, lim = 0;
		int length = ps->getHeader(p).getLength();
		do {
		    #ifdef PROFILING
            timer_np4d_sendto4d->start();
            #endif
			rv = Np4d::sendto4d(bootSock,
				(void *) ps->getBuffer(p), length,
				nmIp, Forest::NM_PORT);
			#ifdef PROFILING
			timer_np4d_sendto4d->stop();
			#endif
		} while (rv == -1 && errno == EAGAIN && lim++ < 10);
		if (rv == -1) {
			fatal("IoProcessor:: send: failure in sendto");
		}
		ps->free(p);
	    #ifdef PROFILING
	    timer_send->stop();
	    #endif
		return;
	}
	ipp_t farPort = lt->getPeerPort(lnk);
	#ifdef PROFILING
	if (farPort == 0) {
	     ps->free(p); 
         timer_send->stop();
         return;
    }
    #else
	if (farPort == 0) { ps->free(p); return; }
    #endif

	ipa_t farIp = lt->getPeerIpAdr(lnk);
	int length = ps->getHeader(p).getLength();

	int rv, lim = 0;
	do {
		#ifdef PROFILING
		timer_np4d_sendto4d->start();
		#endif
		rv = Np4d::sendto4d(sock[lt->getIface(lnk)],
			(void *) ps->getBuffer(p), length,
			farIp, farPort);
		#ifdef PROFILING
	    timer_np4d_sendto4d->stop();
		#endif
	} while (rv == -1 && errno == EAGAIN && lim++ < 10);
	if (rv == -1) {
		cerr << "IoProcessor:: send: failure in sendto (errno="
		     << errno << ")\n";
		exit(1);
	}
	sm->cntOutLink(lnk,Forest::truPktLeng(length),
		       (lt->getPeerType(lnk) == ROUTER));
	ps->free(p);
	#ifdef PROFILING
	timer_send->stop();
	#endif
}
