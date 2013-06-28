/** @file IoProcessor.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "IoProcessor.h"

namespace forest {


// Constructor for IoProcessor, allocates space and initializes private data
IoProcessor::IoProcessor(int maxIface1, IfaceTable *ift1, LinkTable *lt1,
			 PacketStore *ps1, StatsModule *sm1)
			 : maxIface(maxIface1), ift(ift1),
			   lt(lt1), ps(ps1), sm(sm1) {
	nRdy = 0; maxSockNum = -1;
	sockets = new fd_set;
	sock = new int[maxIface+1];
	for (int i = 0; i <= maxIface; i++) sock[i] = -1;
	bootSock = -1;
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
	sock[i] = Np4d::datagramSocket();
	if (sock[i] < 0) {
		cerr << "IoProcessor::setup: socket call failed\n";
                return false;
        }
	maxSockNum = max(maxSockNum, sock[i]);

	// bind it to an address
        if (!Np4d::bind4d(sock[i], ift->getIpAdr(i),0)) {
		string s;
		cerr << "IoProcessor::setup: bind call failed for "
		     << Np4d::ip2string(ift->getIpAdr(i),s)
		     << " check interface's IP address\n";
                return false;
        }
	ift->setPort(i,Np4d::getSockPort(sock[i]));

	// send dummy packet to NetMgr - hack to trigger NAT in SPP
	int zero = 0;
	Np4d::sendto4d(sock[i], (void *) &zero, sizeof(zero),
				nmIp, Forest::NM_PORT);
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
string s;
cerr << "binding to " << Np4d::ip2string(bootIp,s) << endl;
        if (!Np4d::bind4d(bootSock, bootIp, 0)) {
		cerr << "IoProcessor::setupBootIp: bind call failed, "
		     << "check boot IP address\n";
                return false;
        }
cerr << "success\n";
	return true;
}

void IoProcessor::closeBootSock() {
	close(bootSock); bootSock = -1;
}

// Return next waiting packet or 0 if there is none. 
pktx IoProcessor::receive() { 
	if (bootSock >= 0) {
		int nbytes;	  	// number of bytes in received packet
	
		pktx px = ps->alloc();
	        if (px == 0) { return 0; }
	        Packet& p = ps->getPacket(px);
	        buffer_t& b = *p.buffer;
	
		ipa_t sIpAdr; ipp_t sPort;
		nbytes = Np4d::recvfrom4d(bootSock, (void *) &b[0], 1500,
					  sIpAdr, sPort);
		p.bufferLen = nbytes;
		if (nbytes < 0) {
			if (errno == EAGAIN) {
				ps->free(px); return 0;
			}
			fatal("IoProcessor::receive: error in recvfrom call");
		}
		if (sIpAdr != nmIp || sPort != Forest::NM_PORT) {
			ps->free(px); return 0;
		}
		p.unpack();
/*
string s;
cerr << "iop received " << " nbytes=" << nbytes << endl;
cerr << p.toString(s) << endl;
if (p.type == Forest::CONNECT) {
uint64_t x = ntohl(p.payload()[0]); x <<= 32; x |= ntohl(p.payload()[1]);
cerr << x << endl;
}
*/
	        if (!p.hdrErrCheck()) { ps->free(px); return 0; }
        	p.tunIp = sIpAdr; p.tunPort = sPort;
		p.inLink = 0;
		return px;
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

	pktx px = ps->alloc();
        if (px == 0) return 0;
        Packet& p = ps->getPacket(px);
        buffer_t& b = *p.buffer;

	ipa_t sIpAdr; ipp_t sPort;
	nbytes = Np4d::recvfrom4d(sock[cIf], (void *) &b[0], 1500,
				  sIpAdr, sPort);
	if (nbytes < 0) fatal("IoProcessor::receive: error in recvfrom call");

	p.unpack();
/*
string s;
cerr << "iop received from (" << Np4d::ip2string(sIpAdr,s) << ",";
cerr << sPort << ") nbytes=" << nbytes << endl;
cerr << p.toString(s);
if (p.type == Forest::CONNECT) {
uint64_t x = ntohl(p.payload()[0]); x <<= 32; x |= ntohl(p.payload()[1]);
cerr << x << endl;
}
*/

        if (!p.hdrErrCheck()) { ps->free(px); return 0; }
	lnk = lt->lookup(sIpAdr, sPort);
	if (lnk == 0 && p.type == Forest::CONNECT
		     && p.length == Forest::OVERHEAD+8) {
		uint64_t nonce = ntohl(p.payload()[0]); nonce <<= 32;
		nonce |= ntohl(p.payload()[1]);
		lnk = lt->lookup(nonce); // check for "startup" entry
	}
        if (lnk == 0 || cIf != lt->getIface(lnk)) {
		string s;
		cerr << "IoProcessor::receive: bad packet: lnk=" << lnk << " "
		     << p.toString(s);
		cerr << "sender=(" << Np4d::ip2string(sIpAdr,s) << ","
		     << sPort << ")\n";
		ps->free(px); return 0;
	}
        
        p.inLink = lnk; p.bufferLen = nbytes;
        p.tunIp = sIpAdr; p.tunPort = sPort;

        sm->cntInLink(lnk,Forest::truPktLeng(nbytes),
		      (lt->getPeerType(lnk) == Forest::ROUTER));
        return px;
}

void IoProcessor::send(pktx px, int lnk) {
// Send packet on specified link and recycle storage.
	Packet p = ps->getPacket(px);
string s;
	if (lnk == 0) { // means we're booting and this is going to NetMgr
		int rv, lim = 0;
/*
cerr << "iop sending to (" << Np4d::ip2string(nmIp,s) << ",";
cerr << Forest::NM_PORT << ") \n" << p.toString(s);
if (p.type == Forest::CONNECT) {
uint64_t x = ntohl(p.payload()[0]); x <<= 32; x |= ntohl(p.payload()[1]);
cerr << x << endl;
}
*/
		do {
			rv = Np4d::sendto4d(bootSock,
				(void *) p.buffer, p.length,
				nmIp, Forest::NM_PORT);
		} while (rv == -1 && errno == EAGAIN && lim++ < 10);
		if (rv == -1) {
			fatal("IoProcessor:: send: failure in sendto");
		}
		ps->free(px);
		return;
	}
	ipa_t farIp = lt->getPeerIpAdr(lnk);
	ipp_t farPort = lt->getPeerPort(lnk);
	if (farIp == 0 || farPort == 0) { ps->free(px); return; }

	int rv, lim = 0;
/*
{
cerr << "iop sending to (" << Np4d::ip2string(farIp,s) << ",";
cerr << farPort << ") \n" << p.toString(s);
if (p.type == Forest::CONNECT) {
uint64_t x = ntohl(p.payload()[0]); x <<= 32; x |= ntohl(p.payload()[1]);
cerr << x << endl;
}
}
*/
	do {
		rv = Np4d::sendto4d(sock[lt->getIface(lnk)],
			(void *) p.buffer, p.length,
			farIp, farPort);
	} while (rv == -1 && errno == EAGAIN && lim++ < 10);
	if (rv == -1) {
		cerr << "IoProcessor:: send: failure in sendto (errno="
		     << errno << ")\n";
		exit(1);
	}
	sm->cntOutLink(lnk,Forest::truPktLeng(p.length),
		       (lt->getPeerType(lnk) == Forest::ROUTER));
	ps->free(px);
}

} // ends namespace

