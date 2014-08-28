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
	ift = rtr->ift; lt = rtr->lt;
	ctt = rtr->ctt; rt = rtr->rt;
	ps = rtr->ps; qm = rtr->qm;
	pktLog = rtr->pktLog;

	nRdy = 0; 
	sockets = new fd_set;

	// setup thread pool
	tpool = new ThreadInfo[numThreads+1];
	for (int i = 1; i <= numThreads; i++) {
		tpool[i].q.resize(100);
		tpool[i].rc = RouterControl(rtr, i, &(tpool[i].q), &retQ);
		tpool[i].thred = thread(RouterControl::start,&tpool[i].rc);
	}

	comtSet = new HashSet<comt_t,Hash::u32>(numThreads,false);
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
void RouterInProc::start(RouterInProc *self) { self->run(); }

/** Main input processing loop.
 */
void RouterInProc::run() {
	nanoseconds temp = high_resolution_clock::now() - rtr->tZero;
	now = temp.count();

	if (rtr->booting) {
		if (!bootRouter()) {
			Util::fatal("RouterInProc::run: could not complete "
				    "remote boot");
		}
		if (!rtr->setup()) {
			cerr << "RouterCore::handleControlReply: "
				"setup failed after completion of boot "
				"phase\n";
			perror("");
			pktLog->write(cout);
			exit(1);
		}
		close(bootSock);
		rtr->booting = false;
	}

	temp = high_resolution_clock::now() - rtr->tZero;
	now = temp.count();
	int64_t finishTime = now + nanoseconds(rtr->runLength).count();
	while (finishTime == 0 || now < finishTime) {
		temp = high_resolution_clock::now() - rtr->tZero;
		now = temp.count();

		int px = repH->expired(now);
		if (px != 0) ps->free(px); 

		if (!mainline()) {
			this_thread::sleep_for(milliseconds(1));
		}
	}
}

/** Send a boot request and then process configuration packets from NetMgr.
 */
bool RouterInProc::bootRouter() {
	if (!bootStart()) {
		cerr <<	"RouterInProc::bootRouter: unable to initiate "
		    	"boot process\n";
		return false;
	}
	while (true) {
		nanoseconds temp = high_resolution_clock::now() - rtr->tZero;
		now = temp.count();

		// check for old entries in RepeatHandler and discard
		int px = repH->expired(now);
		if (px != 0) ps->free(px);

		// first check for arriving packet from NetMgr
		px = bootReceive();
		if (px != 0) {
			// bootReceive verifies packets are from netMgr
			Packet& p = ps->getPacket(px);
			p.rcvSeqNum = ++rcvSeqNum;
			CtlPkt cp(p);
			if (cp.type == CtlPkt::BOOT_COMPLETE &&
			    cp.mode == CtlPkt::REQUEST) {
				cp.mode = CtlPkt::POS_REPLY;
				cp.fmtBase(); 
				bootSend(px);
				return true; // successful boot
			} else if (cp.type == CtlPkt::BOOT_ABORT &&
				   cp.mode == CtlPkt::REQUEST) {
				cp.mode = CtlPkt::POS_REPLY; cp.fmtBase(); 
				bootSend(px);
				cerr << "RouterInProc::bootRouter: remote "
				    	"boot aborted by NetMgr\n";
				return false;
			}
			// handle signalling packets from NetMgr
			if (cp.mode != CtlPkt::REQUEST) {
				// may be extra reply to boot request, ignore
				ps->free(px); continue;
			}
			// typical case of request from NetMgr
			pktx sx;
			if ((sx = repH->find(p.srcAdr,cp.seqNum)) != 0) {
				// repeat of a request we've already received
				ps->free(px);
				Packet& saved = ps->getPacket(sx);
				CtlPkt scp(saved);
				if (scp.mode != CtlPkt::REQUEST) {
					// already replied to this request,
					// reply again
					pktx cx = ps->clone(sx);
					bootSend(cx);
				}
				// working on request, have not yet replied
				continue;
			}
			// new request packet
			// assign worker thread
			int thx = freeThreads.first();
			if (thx == 0) {
				// this should never happen
				cerr << "RouterInProc::bootRouter: "
					"ran out of threads while "
					"booting\n";
				return false;
			}
			freeThreads.removeFirst();
			tpool[thx].rcvSeqNum = p.rcvSeqNum;
			// save a copy, so we can detect repeats
			pktx cx = ps->clone(px);
			pktx ox = repH->saveReq(cx,p.srcAdr,
						cp.seqNum,now);
			if (ox != 0) { // old entry removed to make room
				ps->free(ox);
			}
			// and send original to the thread
			tpool[thx].q.enq(px);
			continue;
		}

		// check for and process outgoing packets
		if (retQ.empty()) { 
			// nothing happening right now, take a nap
			this_thread::sleep_for(milliseconds(10));
			continue;
		}
		// process outgoing packet from RouterControl
		// note: can only be a reply to NetMgr request
		pair<int,int> retp = retQ.deq();
	
		int thx = retp.first; 	// index of sending thread
		px = retp.second; 	// packet index of outgoing packet
		
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
				freeThreads.addFirst(thx);
			}
			ps->free(px); return true;
		}
		CtlPkt cp(p);
		// make copy, send original, save copy in repeat handler
		// recycle request that was stored in repeat handler
		pktx cx = ps->clone(px);
		bootSend(px);
		int sx = repH->saveRep(cx, p.dstAdr, cp.seqNum);
		if (sx != 0) ps->free(sx);
	}
}

/** Send boot request to NetMgr and wait for reply.
 *  Repeat request several times if necessary.
 */
bool RouterInProc::bootStart() {
	// open datagram socket
	bootSock = Np4d::datagramSocket();
	if (bootSock < 0) {
		cerr << "RouterInProc::bootStart: socket call failed\n";
		return false;
	}
	// bind it to the bootIp
	if (!Np4d::bind4d(bootSock, rtr->bootIp, 0)) {
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
	CtlPkt cp(p);
	cp.fmtBootRouter(rtr->nextSeqNum());

	for (int i = 0; i <= 3; i++) {
		bootSend(px);
		for (int j = 0; j < 9; j++) {
			this_thread::sleep_for(seconds(1));
			int rx = bootReceive();
			if (rx == 0) continue;
			Packet& reply = ps->getPacket(rx);
			CtlPkt cpr(reply);
			if (cpr.type == CtlPkt::BOOT_ROUTER &&
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
	if (sIpAdr != rtr->nmIp || sPort != Forest::NM_PORT) {
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
				    rtr->nmIp, Forest::NM_PORT);
	} while (rv == -1 && errno == EAGAIN && lim++ < 10);
	if (rv == -1) {
		Util::fatal("RouterInProc:: send: failure in sendto");
	}
	ps->free(px);
	return;
}

/** Check for a incoming and outgoings packets and process them. 
 *  @return true if a packet was processed, false if nothing happening
 */
bool RouterInProc::mainline() {
	pktx px;
	unique_lock<mutex> iftLock(rtr->iftMtx,defer_lock);
	unique_lock<mutex>  ltLock(rtr->ltMtx,defer_lock);
	unique_lock<mutex> cttLock(rtr->cttMtx,defer_lock);
	unique_lock<mutex>  rtLock(rtr->rtMtx,defer_lock);
	lock(iftLock,ltLock);

	px = receive();
	if (px != 0) {
		Packet& p = ps->getPacket(px);
		p.outQueue = 0;
		((uint32_t*) p.buffer)[1500] = 0; // clear multicast qids
		p.rcvSeqNum = ++rcvSeqNum;
		pktLog->log(px,p.inLink,false,now);
		lock(cttLock, rtLock);
		int ctx = ctt->getComtIndex(p.comtree);
		if (!pktCheck(px,ctx)) {
			ps->free(px); return true;
		}
		iftLock.unlock(); ltLock.unlock();
		if (p.dstAdr != rtr->myAdr) {
			forward(px,ctx); return true;
		}
		// otherwise must be some kind of control packet
		handleControl(px,ctx);
		return true;
	}
	// check for packet from RouterControl
	if (retQ.empty()) { 
		// check for overdue packet and resend
		pair<int,int> pp = rptr->overdue(now);
		if (pp.first == 0) return false;
		if (pp.first > 0) {
			pktx cx = ps->clone(pp.first);
			if (cx == 0) return true;
			lock(cttLock, rtLock);
			int ctx = ctt->getComtIndex(ps->getPacket(cx).comtree);
			forward(cx,ctx); return true;
		} 
		// no more retries, return to thread
		// with a NO_REPLY mode
		int px = -pp.first;
		Packet& p = ps->getPacket(px);
		CtlPkt cp(p); cp.mode = CtlPkt::NO_REPLY; cp.fmtBase();
		p.length = Forest::OVERHEAD + cp.paylen;
		p.pack();
		tpool[pp.second].q.enq(px);
		return true;
	}
	// process outgoing packet from RouterControl
	pair<int,int> retp = retQ.deq();

	int thx = retp.first; 	// index of sending thread
	px = retp.second; 	// packet index of outgoing packet
	
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
			freeThreads.addFirst(thx);
		}
		ps->free(px); return true;
	}
	if (p.type != Forest::CLIENT_SIG || p.type != Forest::NET_SIG) {
		rtr->xferQ.enq(px); return true;
	}
	CtlPkt cp(p);
	if (cp.mode == CtlPkt::REQUEST) {
		// assign sequence number to packet
		cp.seqNum = rtr->nextSeqNum();
		cp.updateSeqNum();
		// make and save copy in repeat handler, send original
		pktx cx = ps->clone(px);
		lock(cttLock, rtLock);
		int ctx = ctt->getComtIndex(p.comtree);
		if (ctx == 0) { ps->free(px); return true; }
		forward(px,ctx);
		rptr->saveReq(cx, cp.seqNum, now, thx);
		return true;
	}
	// it's a reply, make copy, send original, save copy in repeat handler
	// and recycle corresponding request that was stored in repeat handler
	pktx cx = ps->clone(px);
	lock(cttLock, rtLock);
	int ctx = ctt->getComtIndex(p.comtree);
	if (ctx == 0) { ps->free(px); return true; }
	forward(px,ctx);
	int sx = repH->saveRep(cx, p.dstAdr, cp.seqNum);
	if (sx != 0) ps->free(sx);
	return true;
}

/** Handle a received control packet.
 *  Caller is assumed to hold the comtree table and route table locks.
 *  @param px is the control packet index
 *  @param ctx is the comtree index of the packet's comtree
 */
void RouterInProc::handleControl(pktx px, int ctx) {
	Packet& p = ps->getPacket(px);
	if (p.flags & Forest::ACK_FLAG) {
		// find and remove matching request
		int64_t seqNum = Np4d::unpack64(p.payload());
		pair<int,int> pp = rptr->deleteMatch(seqNum);
		if (pp.first != 0) ps->free(pp.first);
		ps->free(px); return;
	}
	if (p.type == Forest::SUB_UNSUB) {
		subUnsub(px,ctx); return;
	}
	if (p.type == Forest::RTE_REPLY) {
		handleRteReply(px,ctx); return;
	}
	if (p.type == Forest::CONNECT || p.type == Forest::DISCONNECT) {
		handleConnDisc(px); return;
	}
	if (p.type != Forest::NET_SIG && p.type != Forest::CLIENT_SIG) {
		ps->free(px); return;
	}
	// handle signalling packets
	CtlPkt cp(p);
	if (cp.mode != CtlPkt::REQUEST) {
		// reply to a request sent earlier
		pair<int,int> pp =rptr->deleteMatch(cp.seqNum);
		if (pp.first == 0) { // no matching request
			ps->free(px); return;
		}
		ps->free(pp.first); // free saved copy of request
		// pass reply to responsible thread; remember rcvSeqNum
		tpool[pp.second].rcvSeqNum = p.rcvSeqNum;
		tpool[pp.second].q.enq(px);
		return;
	}
	pktx sx;
	if ((sx = repH->find(p.srcAdr,cp.seqNum)) != 0) {
		// repeat of a request we've already received
		ps->free(px);
		Packet& saved = ps->getPacket(sx);
		CtlPkt scp(saved);
		if (scp.mode != CtlPkt::REQUEST) {
			// already replied to this request, reply again
			pktx cx = ps->clone(sx);
			forward(cx,ctx);
		}
		return;
	}
	// new request packet
	if (Forest::isSigComt(p.comtree)) {
		// so not a comtree control packet
		// assign worker thread
		int thx = freeThreads.first();
		if (thx == 0) {
			// no threads available to handle it
			// return negative reply
			cp.fmtError("to busy to handle request, "
				    "retry later");
			p.dstAdr = p.srcAdr;
			p.srcAdr = rtr->myAdr;
			p.pack();
			forward(px,ctx);
			return;
		}
		freeThreads.removeFirst();
		tpool[thx].rcvSeqNum = p.rcvSeqNum;
		// save a copy, so we can detect repeats
		pktx cx = ps->clone(px);
		pktx ox = repH->saveReq(cx,p.srcAdr,cp.seqNum,now);
		if (ox != 0) { // old entry was removed to make room
			ps->free(ox);
		}
		// and send original to the thread
		tpool[thx].q.enq(px);
		return;
	}
	// request for changing comtree
	int thx = comtSet->find(p.comtree);
	if (thx == 0) {
		// no thread assigned to this comtree yet
		// assign a worker thread
		thx = freeThreads.first();
		if (thx == 0) {
			cp.fmtError("too busy to handle request,"
				    " retry later");
			p.dstAdr = p.srcAdr;
			p.srcAdr = rtr->myAdr;
			p.pack();
			forward(px,ctx);
			return;
		}
		freeThreads.removeFirst();
		comtSet->insert(p.comtree,thx);
	}
	tpool[thx].rcvSeqNum = p.rcvSeqNum;
	tpool[thx].q.enq(px);
	return;
}


/** Lookup routing entry and forward packet accordingly.
 *  Calling program is assumed to hold the comtree table and route table locks.
 *  @param px is a packet number for a CLIENT_DATA packet
 *  @param ctx is the comtree table index for the comtree in p's header
 */
void RouterInProc::forward(pktx px, int ctx) {
	Packet& p = ps->getPacket(px);
	p.outQueue = 0;
	int rtx = rt->getRtx(p.comtree,p.dstAdr);
	if (rtx != 0) { // valid route case
		if ((p.flags & Forest::RTE_REQ)) {
			// reply to route request
			sendRteReply(px,ctx);
			p.flags = (p.flags & (~Forest::RTE_REQ));
			p.pack(); p.hdrErrUpdate();
		}
		if (Forest::validUcastAdr(p.dstAdr)) {
			int rcLnk = rt->getClnk(rtx,rt->firstClx(rtx));
			if (ctt->getLink(ctx,rcLnk) == p.inLink) {
				ps->free(px);
			} else {
				p.outQueue = ctt->getClnkQ(ctx,rcLnk);
				rtr->xferQ.enq(px);
			}
			return;
		}
		// multicast data packet
		multiForward(px,ctx,rtx);
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
			p.outQueue = ctt->getLinkQ(ctx,p.inLink);
			rtr->xferQ.enq(px);
			return;
		}
		// send to neighboring routers in comtree
		p.flags = Forest::RTE_REQ;
		p.pack(); p.hdrErrUpdate();
	}
	multiForward(px,ctx,rtx);
	return;
}

/** Setup to forestf multiple copies of a packet.
 *  Copies outgoing queue identifiers to packet's buffer, in otherwise
 *  unused space at end of buffer. This information is used in 
 *  RouterOutProc where copies of packets are created and queued. 
 *  The end of the list of queue id list is identified by a zero value.
 *  @param px is the number of a multi-destination packet
 *  @param ctx is the comtree index for the comtree in p's header
 *  @param rtx is the route index for p, or 0 if there is no route
 */
void RouterInProc::multiForward(pktx px, int ctx, int rtx) {
	Packet& p = ps->getPacket(px);
	int next = 1500;	// offset in p.buffer where qids are stored

	int inLink = p.inLink;
	uint32_t* buf = (uint32_t*) p.buffer;
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
			if (next < 1500+MAXFANOUT)
				buf[next++] = ctt->getClnkQ(ctx,rcLnk);
		}
	} else { 
		// forwarding a multicast packet
		// first identify neighboring core routers to get copies
		int pLink = ctt->getPlink(ctx);	
		for (int rcLnk = ctt->firstCoreLink(ctx); rcLnk != 0;
			 rcLnk = ctt->nextCoreLink(ctx,rcLnk)) {
			int lnk = ctt->getLink(ctx,rcLnk);
			if (lnk == inLink || lnk == pLink) continue;
			if (next < 1500+MAXFANOUT)
				buf[next++] = ctt->getClnkQ(ctx,rcLnk);
		}
		// now copy for parent
		if (pLink != 0 && pLink != inLink) {
			if (next < 1500+MAXFANOUT)
				buf[next++] = ctt->getClnkQ(ctx,
							ctt->getPClnk(ctx));
		}
		// now, copies for subscribers if any
		if (rtx != 0) {
			for (int clx = rt->firstClx(rtx); clx != 0;
				 clx = rt->nextClx(rtx, clx)) {
				int rcLnk = rt->getClnk(rtx,clx);
				int lnk = ctt->getLink(ctx,rcLnk);
				if (lnk == inLink) continue;
				if (next < 1500+MAXFANOUT)
					buf[next++] = ctt->getClnkQ(ctx,rcLnk);
			}
		}
	}
	buf[next] = 0;
	rtr->xferQ.enq(px); // place packet in transfer queue
}

/** Send route reply back towards p's source.
 *  The reply is sent on the link on which p was received and
 *  is addressed to p's original sender.
 *  @param px is a packet number
 *  @param ctx is the comtree index for p's comtree
 */
void RouterInProc::sendRteReply(pktx px, int ctx) {
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

	p.outQueue = ctt->getLinkQ(ctx,p.inLink);
	rtr->xferQ.enq(px);
}

/** Handle a route reply packet.
 *  Adds a route to the destination of the original packet that
 *  triggered the route reply, if no route is currently defined.
 *  If there is no route to the destination address in the packet,
 *  the packet is flooded to neighboring routers.
 *  If there is a route to the destination, it is forwarded along
 *  that route, so long as the next hop is another router.
 */
void RouterInProc::handleRteReply(pktx px, int ctx) {

// need lock??
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
		multiForward(px,ctx,rtx);
		return;
	}
	int dcLnk = rt->getClnk(rtx,rt->firstClx(rtx));

	int lnk = ctt->getLink(ctx,dcLnk);
	if (lt->getEntry(lnk).peerType == Forest::ROUTER) {
		p.outQueue = ctt->getClnkQ(ctx,dcLnk);
		rtr->xferQ.enq(px);
	} else {
		ps->free(px);
	}
	return;
}

/** Convert a packet to an ack or nack and queue it.
 *  @param px is the packet index for the packet
 *  @param ctx is the comtree index for the comtree px belongs to
 *  @param ackNack is true for a positive ack, false for a negative ack
 */
void RouterInProc::returnAck(pktx px, int ctx, bool ackNack) {
	Packet& p = ps->getPacket(px);
	p.dstAdr = p.srcAdr; p.srcAdr = rtr->myAdr;
	p.flags |= (ackNack ? Forest::ACK_FLAG : Forest::NACK_FLAG);
	p.pack(); p.hdrErrUpdate();
	p.outQueue = ctt->getLinkQ(ctx,p.inLink);
	rtr->xferQ.enq(px);
}

/** Perform subscription processing on a packet.
 *  The packet contains two lists of multicast addresses,
 *  each preceded by its length. The combined list lengths
 *  is limited to 350.
 *  @param px is a packet number
 *  @param ctx is the comtree index for p's comtree
 */
void RouterInProc::subUnsub(pktx px, int ctx) {
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
		int lnk = ctt->getPlink(ctx);
		p.dstAdr = lt->getEntry(lnk).peerAdr;
		p.outQueue = ctt->getLinkQ(ctx,lnk);
		uint64_t seqNum = rtr->nextSeqNum();
		Np4d::pack64(seqNum, p.payload());
		p.hdrErrUpdate(); p.payErrUpdate();
		int cx = ps->clone(px);
		if (cx != 0) {
			rptr->saveReq(cx, seqNum, now);
		};
		rtr->xferQ.enq(px);
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
void RouterInProc::handleConnDisc(pktx px) {
	Packet& p = ps->getPacket(px);
	int inLnk = p.inLink;
	int ctx = ctt->getComtIndex(p.comtree);

	LinkTable::Entry& lte = lt->getEntry(inLnk);
	if (p.srcAdr != lte.peerAdr ||
	    p.length != Forest::OVERHEAD + 2*sizeof(uint64_t)) {
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
		if (!lte.isConnected &&
		    !lt->remapEntry(inLnk,p.tunIp,p.tunPort)) {
			returnAck(px,ctx,false); return;
		}
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
			nRdy = select(rtr->maxSockNum+1,sockets,
			       (fd_set *)NULL, (fd_set *)NULL, &zero);
		} while (nRdy < 0 && cnt++ < 10);
		if (cnt > 5) {
			cerr << "RouterInProc::receive: select failed "
			     << cnt-1 << " times\n";
		}
		if (nRdy < 0) {
			Util::fatal("RouterInProc::receive: select failed");
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
	pktx px = ps->alloc();
	if (px == 0) {
		cerr << "RouterInProc:receive: out of packets\n";
		return 0;
	}
	Packet& p = ps->getPacket(px);
	buffer_t& b = *p.buffer;

	ipa_t sIpAdr; ipp_t sPort;
	int nbytes = Np4d::recvfrom4d(rtr->sock[cIf], (void *) &b[0], 1500,
				  sIpAdr, sPort);
	if (nbytes < 0) 
		Util::fatal("RouterInProc::receive: error in recvfrom call");

	p.unpack();

	if (!p.hdrErrCheck()) { ps->free(px); return 0; }
	int lnk = lt->lookup(sIpAdr, sPort);
	if (lnk == 0 && p.type == Forest::CONNECT
		     && p.length == Forest::OVERHEAD+2*sizeof(uint64_t)) {
		uint64_t nonce = Np4d::unpack64(&(p.payload()[2]));
		lnk = lt->lookup(nonce); // check for "startup" entry
	}
	if (lnk == 0 || cIf != lt->getEntry(lnk).iface) {
		cerr << "RouterInProc::receive: bad packet: lnk=" << lnk << " "
		     << p.toString();
		cerr << "sender=(" << Np4d::ip2string(sIpAdr) << ","
		     << sPort << ")\n";
		ps->free(px); return 0;
	}
	
	p.inLink = lnk;
	p.bufferLen = nbytes;
	p.tunIp = sIpAdr; p.tunPort = sPort;

	lt->countIncoming(lnk,Forest::truPktLeng(nbytes));
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
	int cLnk;
	if (ctx != 0) {
		cLnk = ctt->getClnkNum(ctt->getComtree(ctx),inLink);
		if (cLnk == 0) return false;
	}

	// extra checks for packets from untrusted peers
	LinkTable::Entry& lte = lt->getEntry(inLink);
	if (lte.peerType < Forest::TRUSTED) {
		// verify that type is valid
		Forest::ptyp_t ptype = p.type;
		if (ptype != Forest::CLIENT_DATA &&
		    ptype != Forest::CONNECT && ptype != Forest::DISCONNECT &&
		    ptype != Forest::SUB_UNSUB && ptype != Forest::CLIENT_SIG)
			return false;
		// check for spoofed source address
		if (lte.peerAdr != p.srcAdr) return false;
		// check that only client signalling packets on new comt
		if (ctx == 0) return ptype == Forest::CLIENT_SIG;
		// verify that header consistent with comtree constraints
		fAdr_t dest = ctt->getDest(ctx, cLnk);
		if (dest!=0 && p.dstAdr != dest && p.dstAdr != rtr->myAdr)
			return false;
		int comt = ctt->getComtree(ctx);
		if ((ptype == Forest::CONNECT || ptype == Forest::DISCONNECT) &&
		     comt != (int) Forest::NABOR_COMT)
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
