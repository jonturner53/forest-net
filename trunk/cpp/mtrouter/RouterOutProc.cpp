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
	ift = rtr->ift; lt = rtr->lt;
	ctt = rtr->ctt; rt = rtr->rt;
	ps = rtr->ps; qm = rtr->qm;
	sm = rtr->sm; pktLog = rtr->pktLog;
	rptr = new Repeater(1000);
}

RouterOutProc::~RouterOutProc() {
	delete rptr;
}

/** Start input processor.
 *  Start() is a static method used to initiate a separate thread.
 */
void RouterOutProc::start(RouterOutProc *self) { self->run(); }

/** Main output processing loop.
 *
 *  @param finishTime is the number of microseconds to run before stopping;
 *  if it is zero, the router runs without stopping (until killed)
 */
void RouterOutProc::run() {
        unique_lock<mutex>  ltLock( rtr->ltMtx,defer_lock);
	unique_lock<mutex> cttLock(rtr->cttMtx,defer_lock);
	unique_lock<mutex>  rtLock(rtr->rtMtx, defer_lock);

	nanoseconds temp = high_resolution_clock::now() - rtr->tZero;
	now = temp.count(); // time since router started running
	int64_t statsTime = now;
	int64_t runTime = nanoseconds(rtr->runLength).count();
	int64_t finishTime = now + runTime;
	while (runTime == 0 || now < finishTime) {
		// update time
		nanoseconds temp = high_resolution_clock::now() - rtr->tZero;
		now = temp.count();

		bool didNothing = true;

		pktx px;
		// process packet from transfer queue, if any
		if (!rtr->xferQ.empty()) {
			didNothing = false;
			px = rtr->xferQ.deq();
			Packet& p = ps->getPacket(px);
        		lock(ltLock, cttLock, rtLock);
			int ctx = ctt->getComtIndex(p.comtree);
			if (p.dstAdr != rtr->myAdr) {
				if (p.outLink == 0) {
					forward(px,ctx);
				} else if (p.outLink != 0) {
					// enqueue it on specified link
					int qid;
					qid = ctt->getLinkQ(ctx,p.outLink);
					if (!qm->enq(px,qid,now))
						ps->free(px);
				}
			} else if (p.flags & Forest::ACK_FLAG) {
				// find and remove matching request
				int64_t seqNum = Np4d::unpack64(p.payload());
				pair<int,int> pp = rptr->deleteMatch(seqNum);
				if (pp.first != 0) ps->free(pp.first);
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
		while ((px = qm->deq(lnk, now)) != 0) {
			didNothing = false;
			pktLog->log(px,lnk,true,now);
			Packet& p = ps->getPacket(px);
			if (p.srcAdr == rtr->myAdr && 
			    (p.type == Forest::CONNECT ||
			     p.type == Forest::DISCONNECT ||
			     p.type == Forest::SUB_UNSUB)) {
				int cx = ps->clone(px);
				if (cx != 0) {
					uint64_t seqNum =
						Np4d::unpack64(p.payload());
					rptr->saveReq(cx, seqNum, now);
				};
			}
			send(px);
		}

		// every 300 ms, update statistics and check for un-acked
		// in-band control packets
		if (now > statsTime + 300*1000000) {
			sm->record(now); statsTime = now; didNothing = false;
			pair<int,int> pp = rptr->overdue(now);
			if (pp.first > 0) { // resend
				pktx cx = ps->clone(pp.first);
				if (cx != 0) {
					Packet& c = ps->getPacket(cx);
					pktLog->log(cx,c.outLink,true,now);
					send(cx);
				}
			} else if (pp.first < 0) {
				// no more retries, discard
				ps->free(-pp.first);
			}
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
	Packet& p = ps->getPacket(px);
	int rtx = rt->getRtx(p.comtree,p.dstAdr);
	if (rtx != 0) { // valid route case
		// reply to route request
		if ((p.flags & Forest::RTE_REQ)) {
			sendRteReply(px,ctx);
			p.flags = (p.flags & (~Forest::RTE_REQ));
			p.pack(); p.hdrErrUpdate();
		}
		if (Forest::validUcastAdr(p.dstAdr)) {
			int rcLnk = rt->firstComtLink(rtx);
			int qid = ctt->getClnkQ(ctx,rcLnk);
			p.outLink = qm->getLink(qid);
			if (p.outLink == p.inLink || !qm->enq(px,qid,now)) {
				ps->free(px);
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
		    p.dstAdr <= rtr->lastLeafAdr) {
			// send error packet to client
			p.type = Forest::UNKNOWN_DEST;
			(p.payload())[0] = htonl(p.dstAdr);
			p.dstAdr = p.srcAdr;
			p.srcAdr = rtr->myAdr;
			p.length = Forest::OVERHEAD + sizeof(fAdr_t);
			p.pack(); p.hdrErrUpdate(); p.payErrUpdate();
			int qid = ctt->getLinkQ(ctx,p.inLink);
			p.outLink = p.inLink;
			if (!qm->enq(px,qid,now)) ps->free(px);
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
	int qvec[lt->maxLink()]; int n = 0;
	Packet& p = ps->getPacket(px);

	int inLink = p.inLink;
	if (Forest::validUcastAdr(p.dstAdr)) {
		// flooding a unicast packet to neighboring routers
		int myZip = Forest::zipCode(rtr->myAdr);
		int pZip = Forest::zipCode(p.dstAdr);
		for (int rcLnk = ctt->firstRtrLink(ctx); rcLnk != 0;
			 rcLnk = ctt->nextRtrLink(ctx,rcLnk)) {
			int lnk = ctt->getLink(ctx,rcLnk);
			int peerZip =Forest::zipCode(lt->getEntry(lnk).peerAdr);
			if (pZip == myZip && peerZip != myZip) continue;
			if (lnk == inLink) continue;
			qvec[n++] = ctt->getClnkQ(ctx,rcLnk);
		}
	} else { 
		// forwarding a multicast packet
		// first identify neighboring core routers to get copies
		int pLink = ctt->getPlink(ctx);	
		for (int rcLnk = ctt->firstCoreLink(ctx); rcLnk != 0;
			 rcLnk = ctt->nextCoreLink(ctx,rcLnk)) {
			int lnk = ctt->getLink(ctx,rcLnk);
			if (lnk == inLink || lnk == pLink) continue;
			qvec[n++] = ctt->getClnkQ(ctx,rcLnk);
		}
		// now copy for parent
		if (pLink != 0 && pLink != inLink) {
			qvec[n++] = ctt->getClnkQ(ctx,ctt->getPClnk(ctx));
		}
		// now, copies for subscribers if any
		if (rtx != 0) {
			for (int rcLnk = rt->firstComtLink(rtx); rcLnk!=0;
				 rcLnk = rt->nextComtLink(rtx, rcLnk)) {
				int lnk = ctt->getLink(ctx,rcLnk);
				if (lnk == inLink) continue;
				qvec[n++] = ctt->getClnkQ(ctx,rcLnk);
			}
		}
	}

	if (n == 0) { ps->free(px); return; }

	// make copies and queue them
        pktx px1 = px;
        for (int i = 0; i < n-1; i++) { // process first n-1 copies
		ps->getPacket(px1).outLink = qm->getLink(qvec[i]);
                if (qm->enq(px1,qvec[i],now)) {
			px1 = ps->clone(px);
		}
        }
        // process last copy
	ps->getPacket(px1).outLink = qm->getLink(qvec[n-1]);
        if (!qm->enq(px1,qvec[n-1],now)) {
		ps->free(px1);
	}
}

/** Send route reply back towards p's source.
 *  The reply is sent on the link on which p was received and
 *  is addressed to p's original sender.
 *  @param px is a packet number
 *  @param ctx is the comtree index for p's comtree
 */
void RouterOutProc::sendRteReply(pktx px, int ctx) {
	Packet& p = ps->getPacket(px);

	pktx px1 = ps->alloc();
	if (px1 == 0) return;
	Packet& p1 = ps->getPacket(px1);
	p1.length = Forest::OVERHEAD + sizeof(fAdr_t);
	p1.type = Forest::RTE_REPLY;
	p1.flags = 0;
	p1.comtree = p.comtree;
	p1.srcAdr = rtr->myAdr;
	p1.dstAdr = p.srcAdr;

	p1.pack();
	(p1.payload())[0] = htonl(p.dstAdr);
	p1.hdrErrUpdate(); p.payErrUpdate();

	int qid = ctt->getLinkQ(ctx,p.inLink);
	p.outLink = p.inLink;
	if (!qm->enq(px1,qid,now)) ps->free(px1);
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
	Packet& p = ps->getPacket(px);
	int rtx = rt->getRtx(p.comtree, p.dstAdr);
	int cLnk = ctt->getClnkNum(ctt->getComtree(ctx),p.inLink);
	if ((p.flags & Forest::RTE_REQ) && rtx != 0)
		sendRteReply(px,ctx);
	int adr = ntohl((p.payload())[0]);
	if (Forest::validUcastAdr(adr) &&
	    rt->getRtx(p.comtree,adr) == 0) {
		rt->addRoute(p.comtree,adr,cLnk); 
	}
	if (rtx == 0) {
		// send to neighboring routers in comtree
		p.flags = Forest::RTE_REQ;
		p.pack(); p.hdrErrUpdate();
		multiSend(px,ctx,rtx);
		return;
	}
	int dcLnk = rt->firstComtLink(rtx);
	int qid = ctt->getClnkQ(ctx,dcLnk);
	p.outLink = qm->getLink(qid);
	if (lt->getEntry(p.outLink).peerType != Forest::ROUTER ||
	    !qm->enq(px,qid,now))
		ps->free(px);
	return;
}

/** Convert a packet to an ack or nack and queue it.
 *  @param px is the packet index for the packet
 *  @param ctx is the comtree index for the comtree px belongs to
 *  @param ackNack is true for a positive ack, false for a negative ack
 */
void RouterOutProc::returnAck(pktx px, int ctx, bool ackNack) {
	Packet& p = ps->getPacket(px);
	p.dstAdr = p.srcAdr; p.srcAdr = rtr->myAdr;
	p.flags |= (ackNack ? Forest::ACK_FLAG : Forest::NACK_FLAG);
	p.outLink = p.inLink;
	p.pack(); p.hdrErrUpdate();
	int qid = ctt->getLinkQ(ctx,p.outLink);
	if (!qm->enq(px,qid,now)) ps->free(px);
}

/** Perform subscription processing on a packet.
 *  The packet contains two lists of multicast addresses,
 *  each preceded by its length. The combined list lengths
 *  is limited to 350.
 *  @param px is a packet number
 *  @param ctx is the comtree index for p's comtree
 */
void RouterOutProc::subUnsub(pktx px, int ctx) {
	Packet& p = ps->getPacket(px);
	uint32_t *pp = p.payload();

	// add/remove branches from routes
	// if non-core node, also propagate requests upward as appropriate
	int comt = ctt->getComtree(ctx);
	int inLink = p.inLink;
	int cLnk = ctt->getClnkNum(comt,inLink);

	// ignore subscriptions from the parent or core neighbors
	if (inLink == ctt->getPlink(ctx) ||
	    ctt->isCoreLink(ctx,cLnk)) {
		returnAck(px,ctx,false); // negative ack
		return;
	}

	// extract and check addcnt and dropcnt
	uint32_t addcnt = ntohl(pp[2]);
	if (Forest::OVERHEAD + (addcnt + 4)*4 > p.length) {
		returnAck(px,ctx,false); // negative ack
		return;
	}
	uint32_t dropcnt = ntohl(pp[addcnt+3]);
	if (Forest::OVERHEAD + (addcnt + dropcnt + 4)*4 > p.length) {
		returnAck(px,ctx,false); // negative ack
		return;
	}

	// make copy to be used for ack
	pktx cx = ps->fullCopy(px);

	// add subscriptions
	bool propagate = false;
	int rtx; fAdr_t addr;
	for (int i = 3; i <= addcnt + 2; i++) {
		addr = ntohl(pp[i]);
		if (!Forest::mcastAdr(addr)) continue;  // ignore unicast or 0
		rtx = rt->getRtx(comt,addr);
		if (rtx == 0) { 
			rtx = rt->addRoute(comt,addr,cLnk);
			propagate = true;
		} else if (!rt->isLink(rtx,cLnk)) {
			rt->addLink(rtx,cLnk);
			pp[i] = 0; // so, parent will ignore
		}
	}
	// remove subscriptions
	for (int i = addcnt + 4; i <= addcnt + dropcnt + 3; i++) {
		addr = ntohl(pp[i]);
		if (!Forest::mcastAdr(addr)) continue; // ignore unicast or 0
		rtx = rt->getRtx(comt,addr);
		if (rtx == 0) continue;
		rt->removeLink(rtx,cLnk);
		if (rt->noLinks(rtx)) {
			rt->removeRoute(rtx);
			propagate = true;
		} else {
			pp[i] = 0;
		}		
	}
	// propagate subscription packet to parent if not a core node
	if (propagate && !ctt->inCore(ctx) && ctt->getPlink(ctx) != 0) {
		Np4d::pack64(rtr->nextSeqNum(),pp);
		p.srcAdr = rtr->myAdr;
		p.outLink = ctt->getPlink(ctx);
		p.dstAdr = lt->getEntry(p.outLink).peerAdr;
		int qid = ctt->getLinkQ(ctx,p.outLink);
		if (!qm->enq(px,qid,now)) ps->free(px);
	} else {
		ps->free(px);
	}
	// send ack back to sender
	// note that we send ack before getting ack from sender
	// this is by design
	returnAck(cx,ctx,true);
	return;
}

/** Handle a CONNECT or DISCONNECT packet.
 *  @param px is the packet number of the packet to be handled.
 */
void RouterOutProc::handleConnDisc(pktx px) {
	Packet& p = ps->getPacket(px);
	int inLnk = p.inLink;
	int ctx = ctt->getComtIndex(p.comtree);

	LinkTable::Entry& lte = lt->getEntry(inLnk);
	if (p.srcAdr != lte.peerAdr ||
	    p.length != Forest::OVERHEAD + 8) {
		returnAck(px,ctx,false); return;
	}

	uint64_t nonce = Np4d::unpack64(p.payload()+2);
	if (nonce != lte.nonce) {
		returnAck(px,ctx,false); return;
	}
	if (p.type == Forest::CONNECT) {
		if (lte.isConnected &&
		    !lt->revertEntry(inLnk)) {
			returnAck(px,ctx,false); return;
		}
		if (!lt->remapEntry(inLnk,p.tunIp,p.tunPort)) {
			returnAck(px,ctx,false); return;
		}
		lte.isConnected = true;
		if (rtr->nmAdr != 0 && lte.peerType==Forest::CLIENT) {
			pktx rx = ps->alloc();
			if (rx == 0) { returnAck(px,ctx,false); return; }
			Packet& rep = ps->getPacket(rx);
			CtlPkt cp(rep);
			cp.fmtClientConnect(p.srcAdr, rtr->myAdr);
			p.type = Forest::NET_SIG; p.flags = 0;
			p.length = Forest::OVERHEAD + cp.paylen;
			p.srcAdr = rtr->myAdr; p.dstAdr = rtr->nmAdr;
			p.comtree = Forest::NET_SIG_COMT;
			p.pack(); p.payErrUpdate(); p.hdrErrUpdate();
			forward(rx, ctt->getComtIndex(p.comtree));
		}
	} else if (p.type == Forest::DISCONNECT) {
		lte.isConnected = false;
		lt->revertEntry(inLnk);
		if (rtr->nmAdr != 0 && lte.peerType == Forest::CLIENT) {
			pktx rx = ps->alloc();
			if (rx == 0) { returnAck(px,ctx,false); return; }
			Packet& rep = ps->getPacket(rx);
			CtlPkt cp(rep);
			cp.fmtClientDisconnect(p.srcAdr, rtr->myAdr);
			p.type = Forest::NET_SIG; p.flags = 0;
			p.length = Forest::OVERHEAD + cp.paylen;
			p.srcAdr = rtr->myAdr; p.dstAdr = rtr->nmAdr;
			p.comtree = Forest::NET_SIG_COMT;
			p.pack(); p.payErrUpdate(); p.hdrErrUpdate();
			forward(rx, ctt->getComtIndex(p.comtree));
		}
	}
	// send ack back to sender
	returnAck(px,ctx,true);
	return;
}

/** Send packet on specified link and recycle storage. */
void RouterOutProc::send(pktx px) {
	Packet p = ps->getPacket(px);
	LinkTable::Entry& lte = lt->getEntry(p.outLink);
	ipa_t farIp = lte.peerIp;
	ipp_t farPort = lte.peerPort;
	if (farIp == 0 || farPort == 0) { ps->free(px); return; }

	int rv, lim = 0;
	do {
		rv = Np4d::sendto4d(rtr->sock[lte.iface],
			(void *) p.buffer, p.length,
			farIp, farPort);
	} while (rv == -1 && errno == EAGAIN && lim++ < 10);
	if (rv == -1) {
		cerr << "RouterOutProc:: send: failure in sendto (errno="
		     << errno << ")\n";
		exit(1);
	}
	sm->cntOutLink(p.outLink,Forest::truPktLeng(p.length),
		       (lte.peerType == Forest::ROUTER));
	ps->free(px);
}

} // ends namespace
