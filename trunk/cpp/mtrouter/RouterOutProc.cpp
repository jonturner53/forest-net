/** @file RouterOutProc.cpp 
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RouterOutProc.h"

using namespace forest;

namespace forest {

/** Constructor for RouterOutProc.
 *  @param rp1 is pointer to main router module, giving output processor
 *  access to all router variables and data structures
 */
RouterOutProc::RouterOutProc(Router *rtr1) : rtr(rtr1) {
}

RouterOutProc::~RouterOutProc() {
}

/** Main output processing loop.
 *
 *  @param finishTime is the number of microseconds to run before stopping;
 *  if it is zero, the router runs without stopping (until killed)
 */
void RouterOutProc::run(seconds runTime) {
	bool didNothing;
	int controlCount = 20;		// used to limit overhead of control
					// packet processing
        unique_lock  ltLock( rtr->ltMtx,defer_lock);
	unique_lock cttLock(rtr->cttMtx,defer_lock);
	unique_lock  rtLock(rtr->rtMtx, defer_lock);
        lock(ltLock, cttLock, rtLock);

	nanoseconds temp = high_resolution_clock::now() - rtr->tZero;
	now = temp.count(); // time since router started running
	int64_t startTime = now;
	int64_t statsTime = now;
	int64_t finishTime = runTime.count();
	finishTime *= 1000000000; finishTime += now;
	while (runTime.count() == 0 || now < finishTime) {
		// update time
		nanoseconds temp = high_resolution_clock::now() - rtr->tZero;
		now = temp.count();

		didNothing = true;

		// process packet from transfer queue, if any
		if (!rtr->xferQ.empty()) {
			didNothing = false;
			pktx px = rtr->xferQ.deq();
			Packet& p = rtr->ps->getPacket(px);
        		lock(ltLock, cttLock, rtLock);
			int ctx = rtr->ctt->getComtIndex(p.comtree);
			if (p.destAdr != rtr->myAdr) {
				if (p.outLink == 0) {
					forward(px,ctx);
				} else if (p.outLink != 0) {
					// enqueue it on specified link
					int cLnk = rtr->ctt->getComtLink(
							p.comtree,p.outLink);
					int qid = rtr->ctt->getLinkQ(ctx,cLnk)
					if (!rtr->qm->enq(px,qid,now))
						rtr->ps->free(px);
				}
			} else if (p.flags & Forest::ACK_FLAG) {
				// find and remove matching request
				int64_t seqNum =htohl((p.payload())[0]);
				seqNum <<= 32;
				seqNum |= ntohl((p.payload())[1]);
				int rx = rptr->deleteMatch(seqNum);
				if (rx != 0) ps->free(rx);
			} else if (p.type == Forest::SUB_UNSUB) {
				subUnsub(px,ctx);
			} else if (p.type == Forest::RTE_REPLY) {
				handleRteReply(px,ctx);
			} else if (p.type == Forest::CONNECT ||
				   p.type == Forest::DISCONNECT) {
				handleConnDisc(px);
			} else {
				ps->free(px);
			}
			// unlock everything
        		ltLock.unlock();
			cttLock.unlock();
			rtLock.unlock();
		}

		// output processing
		int lnk;
		while ((px = rtr->qm->deq(lnk, now)) != 0) {
			didNothing = false;
			pktLog->log(px,lnk,true,now);
			send(px,lnk);
		}

		// every 300 ms, update statistics and check for un-acked
		// in-band control packets
		if (now > statsTime + 300*1000000) {
			sm->record(now); statsTime = now; resendControl();
			didNothing = false;
		}

		// if did nothing on that pass, sleep for a millisecond.
		if (didNothing) 
			this_thread::sleep_for(chrono::milliseconds(1));
	}

	// write out recorded events
	pktLog->write(cout);
	cout << endl;
	cout << sm->iPktCnt(0) << " packets received, "
	     <<	sm->oPktCnt(0) << " packets sent\n";
	cout << sm->iPktCnt(-1) << " from routers,    "
	     << sm->oPktCnt(-1) << " to routers\n";
	cout << sm->iPktCnt(-2) << " from clients,    "
	     << sm->oPktCnt(-2) << " to clients\n";
}

/** Lookup routing entry and forward packet accordingly.
 *  @param px is a packet number for a CLIENT_DATA packet
 *  @param ctx is the comtree table index for the comtree in p's header
 */
void RouterOutProc::forward(pktx px, int ctx) {
	Packet& p = rtr->ps->getPacket(px);
	int rtx = rtr->rt->getRtx(p.comtree,p.dstAdr);
	if (rtx != 0) { // valid route case
		// reply to route request
		if ((p.flags & Forest::RTE_REQ)) {
			sendRteReply(px,ctx);
			p.flags = (p.flags & (~Forest::RTE_REQ));
			p.pack();
			p.hdrErrUpdate();
		}
		if (Forest::validUcastAdr(p.dstAdr)) {
			int rcLnk = rtr->rt->getClnkNum(p.comtree, p.inLink);
			int qid = rtr->ctt->getLinkQ(ctx,rcLnk);
			if (lnk == p.inLink || !rtr->qm->enq(px,qid,now)) {
				rtr->ps->free(px);
			}
			return;
		}
		// multicast data packet
		multiSend(px,ctx,rtx);
		return;
	}
	// no valid route
	if (Forest::validUcastAdr(p.dstAdr)) {
		if (rtr->firstLeafAdr <= p.dstAdr &&
		    p.dstAdr <= rtr->lastLeafAdr)) {
			// send error packet to client
			p.type = Forest::UNKNOWN_DEST;
			(p.payload())[0] = htonl(p.dstAdr);
			p.dstAdr = p.srcAdr;
			p.srcAdr = rtr->myAdr;
			p.length = Forest::OVERHEAD + sizeof(fAdr_t);
			p.pack(); p.hdrErrUpdate(); p.payErrUpdate();
			int cLnk = rtr->ctt->getClnkNum(ctx,p.inLink);
			int qid = rtr->ctt->getLinkQ(ctx,cLnk);
			if (!rtr->qm->enq(px,qid,now)) rtr->ps->free(px);
			return;
		}
		// send to neighboring routers in comtree
		p.flags = Forest::RTE_REQ;
		p.pack(); p.hdrErrUpdate();
	}
	multiSend(px,ctx,rtx);
	return;
}

/** Forward multiple copies of a packet.
 *  There are two contexts in which this method is called.
 *  The most common case is to forward a CLIENT_DATA packet.
 *  The other case is to forward a control packet that originates
 *  from this router. In this case the inLink field of the packet's
 *  header should be zero.
 *  @param px is the number of a multi-destination packet
 *  @param ctx is the comtree index for the comtree in p's header
 *  @param rtx is the route index for p, or 0 if there is no route
 */
void RouterOutProc::multiSend(pktx px, int ctx, int rtx) {
	int qvec[nLnks]; int n = 0;
	Packet& p = rtr->ps->getPacket(px);

	int inLink = p.inLink;
	if (Forest::validUcastAdr(p.dstAdr)) {
		// flooding a unicast packet to neighboring routers
		int myZip = Forest::zipCode(myAdr);
		int pZip = Forest::zipCode(p.dstAdr);
		for (int rcLnk = rtr->ctt->firstRtrLink(ctx); rcLnk != 0;
			 rcLnk = rtr->ctt->nextRtrLink(ctx,rcLnk)) {
			int lnk = rtr->ctt->getLink(ctx,rcLnk);
			int peerZip = Forest::zipCode(rtr->lt->getPeerAdr(lnk));
			if (pZip == myZip && peerZip != myZip) continue;
			if (lnk == inLink) continue;
			qvec[n++] = rtr->ctt->getLinkQ(ctx,rcLnk);
		}
	} else { 
		// forwarding a multicast packet
		// first identify neighboring core routers to get copies
		int pLink = rtr->ctt->getPlink(ctx);	
		for (int rcLnk = rtr->ctt->firstCoreLink(ctx); rcLnk != 0;
			 rcLnk = rtr->ctt->nextCoreLink(ctx,rcLnk)) {
			int lnk = rtr->ctt->getLink(ctx,rcLnk);
			if (lnk == inLink || lnk == pLink) continue;
			qvec[n++] = rtr->ctt->getLinkQ(ctx,rcLnk);
		}
		// now copy for parent
		if (pLink != 0 && pLink != inLink) {
			qvec[n++] = rtr->ctt->getLinkQ(ctt->getPCLink(ctx));
		}
		// now, copies for subscribers if any
		if (rtx != 0) {
			for (int rcLnk = rtr->rt->firstComtLink(rtx); rcLnk!=0;
				 rcLnk = rtr->rt->nextComtLink(rtx)) {
				int lnk = rtr->ctt->getLink(rcLnk);
				if (lnk == inLink) continue;
				qvec[n++] = rtr->ctt->getLinkQ(rcLnk);
			}
		}
	}

	if (n == 0) { rtr->ps->free(px); return; }

	// make copies and queue them
        pktx px1 = px;
        for (int i = 0; i < n-1; i++) { // process first n-1 copies
                if (rtr->qm->enq(px1,qvec[i],now)) {
			px1 = rtr->ps->clone(px);
		}
        }
        // process last copy
        if (!rtr->qm->enq(px1,qvec[n-1],now)) {
		rtr->ps->free(px1);
	}
}

/** Send route reply back towards p's source.
 *  The reply is sent on the link on which p was received and
 *  is addressed to p's original sender.
 *  @param px is a packet number
 *  @param ctx is the comtree index for p's comtree
 */
void RouterOutProc::sendRteReply(pktx px, int ctx) {
	Packet& p = rtr->ps->getPacket(px);

	pktx px1 = rtr->ps->alloc();
	Packet& p1 = rtr->ps->getPacket(px1);
	p1.length = Forest::OVERHEAD + sizeof(fAdr_t);
	p1.type = Forest::RTE_REPLY;
	p1.flags = 0;
	p1.comtree = p.comtree;
	p1.srcAdr = myAdr;
	p1.dstAdr = p.srcAdr;

	p1.pack();
	(p1.payload())[0] = htonl(p.dstAdr);
	p1.hdrErrUpdate(); p.payErrUpdate();

	int cLnk = rtr->ctt->getComtLink(rtr->ctt->getComtree(ctx),p.inLink);
	if (!rtr->qm->enq(px1,rtr->ctt->getLinkQ(cLnk),now)) rtr->ps->free(px1);
}

/** Handle a route reply packet.
 *  Adds a route to the destination of the original packet that
 *  triggered the route reply, if no route is currently defined.
 *  If there is no route to the destination address in the packet,
 *  the packet is flooded to neighboring routers.
 *  If there is a route to the destination, it is forwarded along
 *  that route, so long as the next hop is another router.
 */
void RouterOutProc::handleRteReply(pktx px, int ctx) {
	Packet& p = rtr->ps->getPacket(px);
	int rtx = rtr->rt->getRteIndex(p.comtree, p.dstAdr);
	int cLnk = rtr->ctt->getComtLink(ctt->getComtree(ctx),p.inLink);
	if ((p.flags & Forest::RTE_REQ) && rtx != 0)
		sendRteReply(px,ctx);
	int adr = ntohl((p.payload())[0]);
	if (Forest::validUcastAdr(adr) &&
	    rtr->rt->getRteIndex(p.comtree,adr) == 0) {
		rtr->rt->addEntry(p.comtree,adr,cLnk); 
	}
	if (rtx == 0) {
		// send to neighboring routers in comtree
		p.flags = Forest::RTE_REQ;
		p.pack(); p.hdrErrUpdate();
		multiSend(px,ctx,rtx);
		return;
	}
	int dcLnk = rtr->rt->getLink(rtx); int dLnk = rtr->ctt->getLink(dcLnk);
	if (rtr->lt->getPeerType(dLnk) != Forest::ROUTER ||
	    !rtr->qm->enq(px,dLnk,now))
		rtr->ps->free(px);
	return;
}

/** Perform subscription processing on a packet.
 *  The packet contains two lists of multicast addresses,
 *  each preceded by its length. The combined list lengths
 *  is limited to 350.
 *  @param px is a packet number
 *  @param ctx is the comtree index for p's comtree
 */
void RouterOutProc::subUnsub(pktx px, int ctx) {
	Packet& p = rtr->ps->getPacket(px);
	uint32_t *pp = p.payload();

	// add/remove branches from routes
	// if non-core node, also propagate requests upward as appropriate
	int comt = rtr->ctt->getComtree(ctx);
	int inLink = p.inLink;
	int cLnk = rtr->ctt->getClnkNum(comt,inLink);

	// ignore subscriptions from the parent or core neighbors
	if (inLink == rtr->ctt->getPlink(ctx) ||
	    rtr->ctt->isCoreLink(ctx,cLnk)) {
		rtr->ps->free(px); return;
	}

	// make copy to be used for ack
	pktx cx = rtr->ps->fullCopy(px);
	Packet& copy = rtr->ps->getPacket(cx);

	bool propagate = false;
	int rtx; fAdr_t addr;
	
	// add subscriptions
	int addcnt = ntohl(pp[2]);
	if (addcnt < 0 || addcnt > 350 ||
	    Forest::OVERHEAD + (addcnt + 4)*4 > p.length) {
		rtr->ps->free(px); rtr->ps->free(cx); return;
	}
	for (int i = 3; i <= addcnt + 2; i++) {
		addr = ntohl(pp[i]);
		if (!Forest::mcastAdr(addr)) continue;  // ignore unicast or 0
		rtx = rtr->rt->getRteIndex(comt,addr);
		if (rtx == 0) { 
			rtx = rtr->rt->addRoute(comt,addr,cLnk);
			propagate = true;
		} else if (!rt->isLink(rtx,cLnk)) {
			rtr->rt->addLink(rtx,cLnk);
			pp[i] = 0; // so, parent will ignore
		}
	}
	// remove subscriptions
	int dropcnt = ntohl(pp[addcnt+3]);
	if (dropcnt < 0 || addcnt + dropcnt > 350 ||
	    Forest::OVERHEAD + (addcnt + dropcnt + 4)*4 > p.length) {
		rtr->ps->free(px); rtr->ps->free(cx); return;
	}
	for (int i = addcnt + 4; i <= addcnt + dropcnt + 3; i++) {
		addr = ntohl(pp[i]);
		if (!Forest::mcastAdr(addr)) continue; // ignore unicast or 0
		rtx = rtr->rt->getRtx(comt,addr);
		if (rtx == 0) continue;
		rtr->rt->removeLink(rtx,cLnk);
		if (rtr->rt->noLinks(rtx)) {
			rtr->rt->removeRoute(rtx);
			propagate = true;
		} else {
			pp[i] = 0;
		}		
	}
	// propagate subscription packet to parent if not a core node
	if (propagate && !ctt->inCore(ctx) && rtr->ctt->getPlink(ctx) != 0) {
		pp[0] = htonl((uint32_t) (seqNum >> 32));
		pp[1] = htonl((uint32_t) (seqNum & 0xffffffff));
		p.srcAdr = myAdr;
		p.dstAdr = rtr->lt->getPeerAdr(rtr->ctt->getPlink(ctx));
// change
		sendControl(px,seqNum++,ctt->getPlink(ctx));
	} else {
		rtr->ps->free(px);
	}
	// send ack back to sender
	// note that we send ack before getting ack from sender
	// this is by design
	copy.flags |= Forest::ACK_FLAG;
	copy.dstAdr = copy.srcAdr; copy.srcAdr = myAdr;
	copy.pack();
	int qid = rtr->ctt->getLinkQ(cLnk);
        if (!rtr->qm->enq(cx,qid,now)) rtr->ps->free(cx);	
	return;
}

/** Handle a CONNECT or DISCONNECT packet.
 *  @param px is the packet number of the packet to be handled.
 */
void RouterOutProc::handleConnDisc(pktx px) {
	Packet& p = rtr->ps->getPacket(px);
	int inLnk = p.inLink;

	if (p.srcAdr != rtr->lt->getPeerAdr(inLnk) ||
	    p.length != Forest::OVERHEAD + 8) {
		rtr->ps->free(px); return;
	}

	uint64_t nonce = ntohl(p.payload()[0]);
	nonce <<= 32; nonce |= ntohl(p.payload()[1]);
	if (nonce != rtr->lt->getNonce(inLnk)) { rtr->ps->free(px); return; }
	if (p.type == Forest::CONNECT) {
		if (rtr->lt->isConnected(inLnk) &&
		    !rtr->lt->revertEntry(inLnk)) {
			rtr->ps->free(px); return;
		}
		if (!rtr->lt->remapEntry(inLnk,p.tunIp,p.tunPort)) {
			rtr->ps->free(px); return;
		}
		lt->setConnectStatus(inLnk,true);
		if (nmAdr != 0 && rtr->lt->getPeerType(inLnk)==Forest::CLIENT) {
			CtlPkt cp(CtlPkt::CLIENT_CONNECT,CtlPkt::REQUEST,0);
			cp.adr1 = p.srcAdr; cp.adr2 = myAdr;
			sendCpReq(cp,nmAdr);
		}
	} else if (p.type == Forest::DISCONNECT) {
		rtr->lt->setConnectStatus(inLnk,false);
		rtr->lt->revertEntry(inLnk);
		if (nmAdr != 0 && rtr->lt->getPeerType(inLnk)==Forest::CLIENT) {
			dropLink(inLnk);
			CtlPkt cp(CtlPkt::CLIENT_DISCONNECT,CtlPkt::REQUEST,0);
			cp.adr1 = p.srcAdr; cp.adr2 = myAdr;
// todo
			sendCpReq(cp,nmAdr);
		}
	}
	// send ack back to sender
	p.flags |= Forest::ACK_FLAG; p.dstAdr = p.srcAdr; p.srcAdr = myAdr;
	p.pack();
	pktLog->log(px,inLnk,true,now); send(px,inLnk);
	return;


/** Send a connect packet to a peer router.
 *  @param lnk is link number of the link on which we want to connect.
 *  @param type is CONNECT or DISCONNECT
 */
void RouterOutProc::sendConnDisc(int lnk, Forest::ptyp_t type) {
	pktx px = rtr->ps->alloc();
	Packet& p = rtr->ps->getPacket(px);

	uint64_t nonce = rtr->lt->getNonce(lnk);
	p.length = Forest::OVERHEAD + 8; p.type = type; p.flags = 0;
	p.comtree = Forest::CONNECT_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtr->lt->getPeerAdr(lnk);
	p.payload()[2] = htonl((uint32_t) (nonce >> 32));
	p.payload()[3] = htonl((uint32_t) (nonce & 0xffffffff));

	sendControl(px,nonce,lnk);
}

/** Send a control packet.
 */
bool RouterOutProc::sendControl(pktx px, int lnk) {
	Packet& p = rtr->ps->getPacket(px);
	uint64_t = rtr->nextSeqNum();
	p.payload()[0] = htonl((uint32_t) (seqNum >> 32));
	p.payload()[1] = htonl((uint32_t) (seqNum & 0xffffffff));
	p.pack();

	// now, make copy of packet and save it in pending
	pktx cx = rtr->ps->clone(px);
	if (cx == 0) {
		cerr << "RouterOutProc::sendControl: no packets left in packet "
			"store\n";
		return false;
	}

	// save a record of the packet in pending map
	int x = resendMap->insert(pid,px);

	// send the packet
// need to review this
// another issue, when adding/removing a link, we need to
/ send a conn/disc packet
	comt_t comt = p.comtree;
	if (rtr->booting) {
		pktLog->log(cx,lnk,true,now); bootSend(cx,0);
	} else if (lnk != 0) {
		int ctx; int clnk; int qid;
		if ((ctx = rtr->ctt->getComtIndex(comt)) == 0 ||
		    (clnk = rtr->ctt->getClnkNum(comt,lnk)) == 0 ||
                    (qid = rtr->ctt->getLinkQ(clnk)) == 0 ||
                    (!rtr->qm->enq(cx,qid,now))) {
			rtr->ps->free(cx);
		}
	} else {
		forward(cx,rtr->ctt->getComtIndex(comt));
	}
	return true;
}

/** Retransmit any pending control packets that have timed out.
 *  This method checks the map of pending requests and whenever
 *  it finds a packet that has been waiting for an acknowledgement
 *  for more than a second, it retransmits it or gives up, 
 *  if it has already made three attempts.
 */
void RouterOutProc::resendControl() {
	map<uint64_t,ControlInfo>::iterator pp;
	list<uint64_t> dropList;
	for (pp = pending->begin(); pp != pending->end(); pp++) {
		if (now < pp->second.timestamp + 1000000000) continue;
		pktx px = pp->second.px;
		Packet& p = rtr->ps->getPacket(px);
		string s;
		if (pp->second.nSent >= 3) { // give up on this packet
			cerr << "RouterOutProc::resendControl: received no "
				"reply to control packet after 3 attempts\n"
			     << p.toString(s);
			rtr->ps->free(px); dropList.push_front(pp->first);
			continue;
		}
		// make copy of packet and send the copy
		pp->second.timestamp = now;
		pp->second.nSent++;
		pktx cx = rtr->ps->fullCopy(px);
		if (cx == 0) {
			cerr << "RouterOutProc::resendControl: no packets left in "
				"packet store\n";
			break;
		}
		int lnk = pp->second.lnk; comt_t comt = p.comtree;
		if (booting) {
			pktLog->log(cx,lnk,true,now); iop->send(cx,lnk);
		} else if (lnk != 0) {
			int ctx; int clnk; int qid;
			if ((ctx = rtr->ctt->getComtIndex(comt)) == 0 ||
			    (clnk = rtr->ctt->getComtLink(comt,lnk)) == 0 ||
	                    (qid = rtr->ctt->getLinkQ(clnk)) == 0 ||
	                    (!rtr->qm->enq(cx,qid,now)))
				rtr->ps->free(cx);
		} else {
			forward(cx,ctt->getComtIndex(comt));
		}
	}
	// remove expired entries from pending list
	for (uint64_t pid : dropList) pending->erase(pid);
}

/** Handle incoming replies to control packets.
 *  The reply is checked against the map of pending control packets,
 *  and if a match is found, the entry is removed from the map
 *  and the storage for the original control packet is freed.
 *  Currently, the only action on a reply is to print an error message
 *  to the log if we receive a negative reply.
 *  @param rx is the packet index of the reply packet
 */
void RouterOutProc::handleControlReply(pktx rx) {
	Packet& reply = rtr->ps->getPacket(rx);
	uint64_t pid;

	CtlPkt cpr;
	if (reply.type == Forest::CONNECT || reply.type == Forest::DISCONNECT ||
	    reply.type == Forest::SUB_UNSUB) {
		pid  = ntohl(reply.payload()[0]); pid <<= 32;
		pid |= ntohl(reply.payload()[1]);
	} else if (reply.type == Forest::NET_SIG) {
		cpr.reset(reply); pid = cpr.seqNum;
	} else {
		string s;
		cerr << "RouterOutProc::handleControlReply: unexpected reply "
		     << reply.toString(s); 
		rtr->ps->free(rx); return;
	}

	map<uint64_t,ControlInfo>::iterator pp = pending->find(pid);
	if (pp == pending->end()) {
		// this is a reply to a request we never sent, or
		// possibly, a reply to a request we gave up on
		string s;
		cerr << "RouterOutProc::handleControlReply: unexpected reply "
		     << reply.toString(s); 
		rtr->ps->free(rx); return;
	}
	// handle signalling packets
	if (reply.type == Forest::NET_SIG) {
		if (cpr.mode == CtlPkt::NEG_REPLY) {
			string s;
			cerr << "RouterOutProc::handleControlReply: got negative "
			        "reply to "
			     << rtr->ps->getPacket(pp->second.px).toString(s); 
			cerr << "reply=" << reply.toString(s);
		} else if (cpr.type == CtlPkt::BOOT_ROUTER) {
			if (booting && !setup()) {
				cerr << "RouterOutProc::handleControlReply: "
					"setup failed after completion of boot "
					"phase\n";
				perror("");
				pktLog->write(cout);
				exit(1);
			}
			iop->closeBootSock();
			booting = false;
		}
	}

	// free both packets and erase pending entry
	rtr->ps->free(pp->second.px); rtr->ps->free(rx); pending->erase(pp);
}

/** Send packet back to sender.
 *  Update the length, flip the addresses and pack the buffer.
 *  @param px is the packet number
 *  @param cp is a control packet to be packed
 */
void RouterOutProc::returnToSender(pktx px, CtlPkt& cp) {
	Packet& p = rtr->ps->getPacket(px);
	cp.payload = p.payload(); 
	int paylen = cp.pack();
	if (paylen == 0) {
		cerr << "RouterOutProc::returnToSender: control packet formatting "
			"error, zero payload length\n";
		rtr->ps->free(px);
	}
	p.length = (Packet::OVERHEAD + paylen);
	p.flags = 0;
	p.dstAdr = p.srcAdr;
	p.srcAdr = myAdr;
	p.pack();

	if (booting) {
		pktLog->log(px,0,true,now);
		iop->send(px,0);
		return;
	}

	int cLnk = rtr->ctt->getComtLink(p.comtree,p.inLink);
	int qn = rtr->ctt->getLinkQ(cLnk);
	if (!rtr->qm->enq(px,qn,now)) { rtr->ps->free(px); }
}

} // ends namespace
