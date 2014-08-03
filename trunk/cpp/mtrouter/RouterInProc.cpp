/** @file RouterInProc.cpp 
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RouterInProc.h"

using namespace forest;

namespace forest {

RouterInProc::RouterInProc(Router *rtr1) : rtr(rtr1) {
	nRdy = 0; maxSockNum = -1;
	sockets = new fd_set;

	// setup thread pool
	tpool = new ThreadInfo[numThreads+1];
	for (int i = 1; i <= numThreads; i++) {
		tpool[i].q.resize(100);
		tpool[i].rc = RouterControl(rtr,i,tpool[i].q,retQ);
		tpool[i].thred = thread(RouterControl::start,&tpool[i].rc);
	}

	comtSet = new HashSet<comt_t>(Hash::hash_u32,numThreads,false);
	rptr = new Repeater(numThreads);
	repH = new RepeatHandler(maxReplies);
}

RouterInProc::~RouterInProc() {
	delete sockets; delete [] tpool; 
	delete comtSet; delete rptr; delete repH;
}

/** Start input processor.
 *  Start() is a static method used to initiate a separate thread.
 */
bool RouterInProc::start(RouterInProc *self) { self->run(); }

/** Main input processing loop.
 */
void RouterInProc::run() {
	nanoseconds temp = high_resolution_clock::now() - rtr->tZero;
	now = temp.count();

	if (rtr->booting) {
		if (!bootStart()) {
			Util::fatal("RouterInProc::run: could not initiate "
				    "remote boot");
		}
	}

	uint64_t statsTime = 0;		// time statistics were last processed
	bool didNothing;
	high_resolution_clock::time_point
		finishTime = now + nanoseconds(rtr->runLength);
	while (finishTime.count == 0 || now < finishTime) {
		temp = high_resolution_clock::now() - rtr->tZero;
		now = temp.count();
		didNothing  = !inBound();
		didNothing &= !outBound();

		if (didNothing) {
			pair<int,int> pp = rptr->overdue(now);
			if (pp.first > 0) {
				pktx cx = ps.clone(pp.first);
				if (cx != 0) forward(cx);
				didNothing = false;
			} else if (pp.first < 0) {
				// no more retries, return to thread
				// with a NO_REPLY mode
				int px = -pp.first;
				Packet& p = ps->getPacket(px);
				CtlPkt cp(p);
				cp.mode = CtlPkt::NO_REPLY;
				cp.fmtBase();
				p.length = Forest::OVERHEAD + cp.paylen;
				p.pack();
				tpool[pp.second].q.enq(px);
				didNothing = false;
			}
		}

		if (didNothing) {
			// check for old entries in RepeatHandler
			int px = repH.expired(now);
			if (px != 0) {
				ps->free(px); didNothing = false;
			}
		}

		// every 300 ms, update statistics and check for un-acked
		// control packets
		if (now - statsTime > 300*1000000) {
			rtr->sm->record(now); statsTime = now;
			didNothing = false;
		}

		// if did nothing on that pass, sleep for a millisecond.
		if (didNothing) {
			this_thread::sleep_for(milliseconds(1));
		}
	}
}

bool RouterInProc::bootStart() {
	// open datagram socket
	bootSock = Np4d::datagramSocket();
	if (bootSock < 0) {
		cerr << "RouterInProc::bootStart: socket call failed\n";
		return false;
	}
	// bind it to the bootIp
        if (!Np4d::bind4d(bootSock, bootIp, 0)) {
		cerr << "RouterInProc::bootStart: bind call failed, "
		     << "check boot IP address\n";
                return false;
        }

	// send boot request to net manager
	int px = ps->alloc();
	if (px == 0) {
		Util::fatal("RouterInProc::bootStart: no packets left");
	}
	Packet& p = ps->getPacket(px);
	CtlPkt cp(CtlPkt::BOOT_ROUTER, CtlPkt::REQUEST, rtr->seqNum++);
	cp.payoad = p.payload; cp.pack();

	for (int i = 0; i <= 3; i++) {
		bootSend(px);
		for (int j = 0; j < 9; j++) {
			this_thread::sleep_for(milliseconds(100));
			int rx = bootReceive();
			if (rx == 0) continue;
			Packet& reply = ps->getPacket(rx);
			CtlPkt cpr(reply);
			if (cpr.type == CtlPkt::BOOT_REQUEST &&
			    cpr.mode == CtlPkt::POS_REPLY)
				return true;
			// discard other packets until boot reply comes in
			ps->free(rx);
		}
	}

	return false;
}

/** During boot process, return next waiting packet or 0 if there is none. 
 */
pktx RouterInProc::bootReceive() { 
	int nbytes;	  	// number of bytes in received packet

	pktx px = ps->alloc();
        if (px == 0) return 0;
        Packet& p = ps->getPacket(px);

	ipa_t sIpAdr; ipp_t sPort;
	nbytes = Np4d::recvfrom4d(bootSock, (void *) p.buffer, 1500,
				  sIpAdr, sPort);
	p.bufferLen = nbytes;
	if (nbytes < 0) {
		if (errno == EAGAIN) {
			ps->free(px); return 0;
		}
		Util::fatal("RouterInProc::bootReceive:receive: error in "
			    "recvfrom call");
	}
	if (sIpAdr != nmIp || sPort != Forest::NM_PORT) {
		ps->free(px); return 0;
	}
	p.unpack();
        if (!p.hdrErrCheck() ||
	    p.srcAdr != rtr->nmAdr || p.type != Forest::NET_SIG) {
		ps->free(px); return 0;
	}
       	p.tunIp = sIpAdr; p.tunPort = sPort; p.inLink = 0;
	return px;
}

/** Send packet to net manager during boot process.
 *  @param px is the index of the packet to send.
 */
void RouterInProc::bootSend(pktx px) {
	Packet p = ps->getPacket(px);
	p.srcAdr = rtr->myAdr;
	p.dstAdr = rtr->nmAdr;
	p.comtree = 0;
	p.pack();
	int rv, lim = 0;
	do {
		rv = Np4d::sendto4d(bootSock,(void *) p.buffer, p.length,
				    nmIp, Forest::NM_PORT);
	} while (rv == -1 && errno == EAGAIN && lim++ < 10);
	if (rv == -1) {
		Util::fatal("RouterInProc:: send: failure in sendto");
	}
	ps->free(px);
	return;
}

/** Check for an inbound packet and process it. 
 *  @return true if a packet was processed, else false
 */
bool RouterInProc::inBound() {
	pktx px;
	if (rtr->booting) {
		px = bootReceive();
		if (px == 0) return false;
		Packet& p = ps->getPacket(px);
		CtlPkt cp(p);
		if (cp.type == CtlPkt::BOOT_COMPLETE &&
		    cp.mode == CtlPkt::REQUEST) {
			cp.mode = CtlPkt::POS_REPLY;
			cp.fmtBase(); 
			rtr->booting = false;
			bootSend(px); return true;
		} else if (cp.type == CtlPkt::BOOT_ABORT &&
		           cp.mode == CtlPkt::REQUEST) {
			cp.mode = CtlPkt::POS_REPLY; cp.fmtBase(); 
			bootSend(px);
			Util::fatal("RouterInProc::inBound: remote boot "
				    "aborted by NetMgr");
		}
	} else {
		px = receive();
		if (px == 0) return false;
	}
	Packet& p = ps->getPacket(px);
	p.outLink = 0;
	int ptype = p.type;
	p.rcvSeqNum = ++rcvSeqNum;
        pktLog->log(px,p.inLink,false,now);
	// lock ctt
	int ctx = ctt->getComtIndex(p.comtree);
	if (!pktCheck(px,ctx)) {
		// unlock ctt
		ps->free(px);
	} else if (p.dstAdr != myAdr && 
		   (p.typ == Forest::CLIENT_DATA ||
		    p.typ == Forest::NET_SIG)) {
		// unlock ctt
		forward(px);
	} else if (p.dstAdr == myAdr &&
		   (p.typ == Forest::NET_SIG || p.typ == Forest::CLIENT_SIG)) {
		CtlPkt cp(p);
		if (cp.mode != CtlPkt::REQUEST) {
			// reply to a request sent earlier
			pair<int,int> pp = rptr.deleteMatch(p.srcAdr,cp.seqNum);
			if (pp.first == 0) { // no matching request
				ps->free(px); return true;
			}
			ps->free(pp->first); // free saved copy of request
			// pass reply to responsible thread; remember rcvSeqNum
			tpool[pp.second].rcvSeqNum = p.rcvSeqNum;
			tpool[pp.second].q.enq(px);
			return true;
		}
		if ((pktx sx = replyH->find(p.srcAdr,cp.seqNum)) != 0) {
			// repeat of a request we've already received
			ps->free(px);
			Packet& saved = ps->getPacket(sx);
			CtlPkt scp(saved);
			if (scp.mode != CtlPkt::REQUEST) {
				// already replied to this request, reply again
				pktx cx = ps->clone(sx);
				forward(cx);
			}
			return true;
		}
		// new request packet
		if (Forest::isSigComt(p.comtree)) {
			// so not a comtree control packet
			// assign worker thread
			thx = freeThreads.first();
			if (thx == 0) {
				// no threads available to handle it
				// return negative reply
				cp.fmtError("to busy to handle request, "
					    "retry later");
				p.dstAdr = p.srcAdr;
				p.srcAdr = rtr->myAdr;
				p.pack();
				forward(px);
				return true;
			}
			freeThreads.removeFirst();
			tpool[thx].rcvSeqNum = p.rcvSeqNum;
			// save a copy, so we can detect repeats
			pktx cx = ps->clone(px);
			pktx ox = repH.saveReq(cx,p.srcAdr,cp.seqNum,now);
			if (ox != 0) { // old entry was removed to make room
				ps->free(ox);
			}
			// and send original to the thread
			tpool[thx].q.enq(px);
			return true;
		} else {
			// request for changing comtree
			index thx = comtSet->find(p.comtree);
			if (thx == 0) {
				// no thread assigned to this comtree yet
				// assign a worker thread
				thx = freeThreads.first();
				if (thx == 0) {
					cp.fmtError("to busy to handle request,"
						    " retry later");
					p.dstAdr = p.srcAdr;
					p.srcAdr = rtr->myAdr;
					p.pack();
					forward(px);
					return true;
				}
				freeThreads.removeFirst();
				comtSet->insert(p.comtree,thx);
			}
			tpool[thx].rcvSeqNum = p.rcvSeqNum;
			tpool[thx].q.enq(px);
			return true;
		}
	} else {
		cerr << "RouterInProc::inBound: unrecognized packet "
			+ p.toString() << endl;
		rtr->ps->free(px);
	}
	return true;
}

/** Check for an outbound packet and process it. 
 *  @return true if a packet was processed, else false
 */
bool RouterInProc::outBound() {
	if (retQ.empty()) return false;
	pair<int,int> retp = retQ.deq();

	int thx = retp.first(); // index of sending thread
	pktx px = retp.second(); // packet index of outgoing packet
	
	Packet& p = ps->getPacket(px);
	if (thx < 0) {
		// means thread is ready to be released and packet is
		// a dummy used to pass rcvSeqNum back to ensure clean
		// deallocation of thread
		//
		// needed to handle comtree signalling
		thx = -thx; // restore proper thread index
		if (tpool[thx].rcvSeqNum == p.rcvSeqNum) {
			if (comtSet->valid(thx))
				comtSet->remove(comtSet->retrieve(thx));
			freeThreads->addFirst(thx);
		}
		ps->free(px); return true;
	}
	if (p.type != Forest::CLIENT_SIG || p.type != Forest::NET_SIG) {
		forward(px); return true;
	}
	CtlPkt cp(p);
	if (cp.mode == CtlPkt::REQUEST) {
		// assign sequence number to packet
		cp.seqNum = rtr->nextSeqNum();
		cp.updateSeqNum();
		// make and send copy
		pktx cx = ps->clone(px);
		forward(cx);
		// save original request packet
		rptr.saveReq(px, cp.seqNum, now, thx);
		return true;
	}
	// it's a reply, send copy, save original in repeat handler and
	// recycle corresponding request that was stored in repeat handler
	pktx cx = ps->clone(px);
	forward(cx);
	int sx = repH.saveRep(px, px.dstAdr, cp.seqNum);
	if (sx != 0) ps->free(sx);
	return true;
}

// Return next waiting packet or 0 if there is none. 
pktx RouterInProc::receive() { 
	if (nRdy == 0) { // if no ready interface check for new arrivals
		FD_ZERO(sockets);
		for (int i = ift->firstIface(); i != 0; i = ift->nextIface(i)) {
			FD_SET(rtr->sock[i],sockets);
		}
		struct timeval zero; zero.tv_sec = zero.tv_usec = 0;
		int cnt = 0;
		do {
			nRdy = select(maxSockNum+1,sockets,
			       (fd_set *)NULL, (fd_set *)NULL, &zero);
		} while (nRdy < 0 && cnt++ < 10);
		if (cnt > 5) {
			cerr << "RouterInProc::receive: select failed "
			     << cnt-1 << " times\n";
		}
		if (nRdy < 0) {
			fatal("RouterInProc::receive: select failed");
		}
		if (nRdy == 0) return 0;
		cIf = 0;
	}
	while (1) { // find next ready interface
		cIf++;
		if (cIf > Forest::MAXINTF) return 0; // should never reach here
		if (ift->valid(cIf) && FD_ISSET(rtr->sock[cIf],sockets)) {
			nRdy--; break;
		}
	}
	// Now, read the packet from the interface
	int nbytes;	  	// number of bytes in received packet
	int lnk;	  	// # of link on which packet received

	pktx px;
	// if packetCache not empty, allocate a pktx from there,
	// else allocate one from ps
        if (px == 0) {
		cerr << "RouterInProc:receive: out of packets\n";
		return 0;
	}
        Packet& p = ps->getPacket(px);
        buffer_t& b = *p.buffer;

	ipa_t sIpAdr; ipp_t sPort;
	int nbytes = Np4d::recvfrom4d(rtr->sock[cIf], (void *) &b[0], 1500,
				  sIpAdr, sPort);
	if (nbytes < 0) fatal("RouterInProc::receive: error in recvfrom call");

	p.unpack();

        if (!p.hdrErrCheck()) { ps->free(px); return 0; }
	lnk = lt->lookup(sIpAdr, sPort);
	if (lnk == 0 && p.type == Forest::CONNECT
		     && p.length == Forest::OVERHEAD+2*sizeof(uint64_t)) {
		uint64_t nonce = ntohl(p.payload()[2]); nonce <<= 32;
		nonce |= ntohl(p.payload()[3]);
		lnk = lt->lookup(nonce); // check for "startup" entry
	}
        if (lnk == 0 || cIf != lt->getIface(lnk)) {
		string s;
		cerr << "RouterInProc::receive: bad packet: lnk=" << lnk << " "
		     << p.toString(s);
		cerr << "sender=(" << Np4d::ip2string(sIpAdr,s) << ","
		     << sPort << ")\n";
		ps->free(px); return 0;
	}
        
        p.inLink = lnk; p.outLink = 0;
	p.bufferLen = nbytes;
        p.tunIp = sIpAdr; p.tunPort = sPort;

        sm->cntInLink(lnk,Forest::truPktLeng(nbytes),
		      (lt->getPeerType(lnk) == Forest::ROUTER));
        return px;
}

/** Perform error checks on forest packet.
 *  @param px is a packet index
 *  @param ctx is the comtree index for p's comtree
 *  @return true if all checks pass, else false
 */
bool RouterInProc::pktCheck(pktx px, int ctx) {
	Packet& p = ps->getPacket(px);
	// check version and  length
	if (p.version != Forest::FOREST_VERSION) {
		return false;
	}
	if (p.length != p.bufferLen || p.length < Forest::HDR_LENG) {
		return false;
	}
	if (p.type == Forest::CONNECT || p.type == Forest::DISCONNECT)
	    return (p.length == Forest::OVERHEAD+2*sizeof(uint64_t));

	fAdr_t adr = p.dstAdr;
	if (!Forest::validUcastAdr(adr) && !Forest::mcastAdr(adr)) {
		return false;
	}

	int inLink = p.inLink;
	if (inLink == 0) return false;
	if (ctx != 0) {
		int cLnk = ctt->getComtLink(ctt->getComtree(ctx),inLink);
		if (cLnk == 0) {
			return false;
		}
	}

	// extra checks for packets from untrusted peers
	if (lt->getPeerType(inLink) < Forest::TRUSTED) {
		// verify that type is valid
		Forest::ptyp_t ptype = p.type;
		if (ptype != Forest::CLIENT_DATA &&
		    ptype != Forest::CONNECT && ptype != Forest::DISCONNECT &&
		    ptype != Forest::SUB_UNSUB && ptype != Forest::CLIENT_SIG)
			return false;
		// check for spoofed source address
		if (lt->getPeerAdr(inLink) != p.srcAdr) return false;
		// check that only client signalling packets on new comt
		if (ctx == 0) return ptype == Forest::CLIENT_SIG);
		// verify that header consistent with comtree constraints
		fAdr_t dest = ctt->getDest(cLnk);
		if (dest!=0 && p.dstAdr != dest && p.dstAdr != myAdr)
			return false;
		int comt = ctt->getComtree(ctx);
		if ((ptype == Forest::CONNECT || ptype == Forest::DISCONNECT) &&
		     comt != (int) Forest::CONNECT_COMT)
			return false;
		if (ptype == Forest::CLIENT_SIG &&
		    comt != (int) Forest::CLIENT_SIG_COMT)
			return false;
	} else if (ctx == 0) {
		return p.type == Forest::NET_SIG;
	}
	return true;
}

} // ends namespace
