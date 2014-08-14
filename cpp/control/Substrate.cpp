/** @file Substrate.cpp
 * 
 *  @author Jonathan Turner
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Substrate.h"

namespace forest {

Substrate::Substrate(int threadCount1, Controller *ctlr1,
		     PacketStore *ps1, Logger *logger1) {
	threadCount = threadCount1;
	ctlr = ctlr1; ps = ps1; logger = logger1;

	thredIdx = new ListPair(threadCount);
	rptr = new Repeater(threadCount);
	repH = new RepeatHandler(20*threadCount);

	rtrReady = false;
	dgSock = listenSock = -1;
}

Substrate::~Substrate() {
	if (dgSock > 0) close(dgSock);
	if (listenSock > 0) close(listenSock);
	delete thredIdx; delete rptr; delete repH;
}

bool Substrate::init(fAdr_t myAdr1, ipa_t myIp1, fAdr_t rtrAdr1,
		     ipa_t rtrIp1, ipp_t rtrPort1, uint64_t nonce1,
		     int dgPort1, int listenPort1, int finTime1) {
	myAdr = myAdr1; myIp = myIp1;
	rtrAdr = rtrAdr1; rtrIp = rtrIp1; rtrPort = rtrPort1;
	nonce = nonce1; dgPort = dgPort1; listenPort = listenPort1;
	finTime = finTime1;
	
	// setup sockets
	dgSock = Np4d::datagramSocket();
	if (dgSock < 0 || !Np4d::bind4d(dgSock,myIp,dgPort) ||
	    !Np4d::nonblock(dgSock)) {
		return false;
	}
	listenSock = Np4d::streamSocket();
	if (listenSock < 0) return false;
	return	Np4d::bind4d(listenSock,INADDR_ANY,listenPort) &&
		Np4d::listen4d(listenSock) && Np4d::nonblock(listenSock);
}

/** Run forever, or until time expires.
 */
bool Substrate::run() {
	seqNum = 1;
	tZero = high_resolution_clock::now();
	now = 0;
	nanoseconds temp;

	uint64_t finishTime = finTime;
	finishTime *= 1000000000; // convert to ns

	bool nothing2do; bool connected = false;
	while (finishTime == 0 || now <= finishTime) {
		temp = high_resolution_clock::now() - tZero;
		now = temp.count();

		nothing2do = true;
		if (!connected && rtrReady) {
			// this allows substrate to run before its
			// access router has booted; used by NetMgr
			if (!connect()) return false;
			connected = true;
		}
		// check for connection requests from remote hosts
		int connSock = -1;
		if ((connSock = Np4d::accept4d(listenSock)) > 0) {
			// let handler know this is socket# for remote host
			int thx = thredIdx->firstOut();
			if (thx != 0) {
				thredIdx->swap(thx);
				pool[t].inq.enq(-connSock);
			} else {
				logger->log("Substrate: thread pool is "
					   "exhausted",4);
			}
		} 
		// handle incoming packets
		pktx px = recvFromForest();
		if (px != 0) {
			inbound(px); nothing2do = false;
		}

		// and outgoing packets
		if (!retQ.empty()) {
			pair<int,int> pp = retQ.deq();
			outbound(pp.first,pp.second);
			nothing2do = false;
		}

		if (nothing2do) {
			// check for expired entries in repH & remove oldest
			int ox = repH.expired(now);
			if (ox != 0) {
				ps->free(ox); didNothing = false;
			}
			nothing2do = false;
		}
		if (nothing2do && thredIdx->firstIn() == 0)
			this_thread::sleep_for(milliseconds(1));
	}
	if (connected) return disconnect();
	return true;
}

/** Send an incoming packet to a thread, possibly assigning new one.
 *  @param px is a packet index
 */
void Substrate::inbound(pktx px) {
	Packet& p = ps->getPacket(px);
	if (p.type != Forest::CLIENT_SIG && p.type != Forest::NET_SIG) {
		ps->free(px); return;
	}
	CtlPkt cp(p);
	if (cp.mode == CtlPkt::REQUEST) {
		if ((pktx sx = repH->find(p.srcAdr,cp.seqNum)) != 0) {
			// repeat of a request we've already received
			ps->free(px);
			Packet& saved = ps->getPacket(sx);
			CtlPkt scp(saved);
			if (scp.mode != CtlPkt::REQUEST) {
				// already replied, reply again
				pktx cx = ps->clone(sx);
				if (cx != 0) sendToForest(cx);
			}
			return;
		}
		// new request, save a copy
		pktx cx = ps->clone(px);
		pktx ox = repH.saveReq(cx,p.srcAdr,cp.seqNum,now);
		if (ox != 0) { // old entry was removed to make room
			ps->free(ox);
		}

		// assign a thread to handle it
		int thx = thredIdx->firstOut();
		if (thx != 0) { // assign thread
			thredIdx->swap(thx);
			pool[thx].inQ.enq(px);
		} else {
			logger->log("Substrate: thread pool is "
				    "exhausted",4);
		}
		return;
	}
	// reply; drop matching request
	pair<int,int> pp = rptr.deleteMatch(p.srcAdr,cp.seqNum);
	if (pp.first == 0) { // no matching request
		ps->free(px); return;
	}
	ps->free(pp->first); // free saved copy of request
	// forward reply to the thread that sent the request
	if (pp.second != 0) pool[pp.second].inq.enq(px);
	return;
}

/** Handle an outbound packet from one of the worker threads.
 *  @param px is a packet index
 *  @param thx is the thread index (in the tread pool)
 */
void Substrate::outbound(pktx px, int thx) {
	if (px == 0) { // means worker thread completed task
		pool[thx].inq.reset();
		thredIdx->swap(thx);
		return;
	}
	Packet& p = ps->getPacket(px);
	CtlPkt cp(p);
	if (cp.mode != CtlPkt::REQUEST) {
		// save reply in repH, and free saved copy of request
		pktx cx = ps->clone(px);
		if (cx != 0) {
			pktx sx = saveRep(cx, p.dstAdr, cp.seqNum);
			if (sx != 0) ps->free(sx);
		}
		// and send reply
		sendToForest(px); return;
	}
	// request packet; remember it so we can route reply to correct thread
	int cx = ps->clone(px);
	if (cx == 0) {
		// to make this robust, return NO_REPLY to thread
		return;
	}
	// assign sequence number to outgoing request, send it and save copy
	cp.seqNum = seqNum++;
	cp.pack(); p.payErrUpdate();
	sendToForest(px);
	rptr->saveReq(cx, cp.seqNum, now, thx);
}

/** Check for next packet from the Forest network.
 *  The sender's IP and port are placed in the packet's tunnel fields.
 *  @return next packet or 0, if no report has been received.
 */
pktx Substrate::recvFromForest() { 
	pktx px = ps->alloc();
	if (px == 0) return 0;
	Packet& p = ps->getPacket(px);

	ipa_t srcIp; ipp_t srcPort;
	int nbytes = Np4d::recvfrom4d(dgSock,p.buffer,1500,srcIp,srcPort);
	if (nbytes < Forest::OVERHEAD || !p.unpack()) {
		ps->free(px); return 0;
	}
	p.tunIp = srcIp; p.tunPort = srcPort;
/*
cerr << "got packet from " << Np4d::ip2string(srcIp) << " " << srcPort << endl;
cerr << p.toString() << endl;
*/

	return px;
}

/** Send packet to Forest network.
 *  If the packet has a zero destination address, it is sent to the
 *  (ip,port) specified in the packet's tunnel fields. Otherwise,
 *  it is sent to the router.
 */
void Substrate::sendToForest(pktx px) {
	Packet p = ps->getPacket(px); p.pack();
	ipa_t ip; ipp_t port;
	if (p.dstAdr == 0) { // send to tunnel, not router
		ip = p.tunIp; port = p.tunPort;
	} else {
		ip = rtrIp; port = rtrPort;
	}
/*
cerr << "substrate sending to " << Np4d::ip2string(ip) << " " << port << endl;
cerr << p.toString() << endl;
*/
	if (port == 0) fatal("Substrate::sendToForest: zero port number");
	int rv = Np4d::sendto4d(dgSock,(void *) p.buffer, p.length,ip,port);
	if (rv == -1) fatal("Substrate::sendToForest: failure in sendto");
	ps->free(px);
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
bool Substrate::connect() {
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);

	Forest::pack64(seqNum++, p.payload());
	Forest::pack64(nonce, p.payload()+2);

	p.length = Forest::OVERHEAD + 2*sizeof(int64_t);
	p.type = Forest::CONNECT; p.flags = 0;
	p.comtree = Forest::CONNECT_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	nanoseconds temp;
	temp = high_resolution_clock::now() - tZero;
	int64_t resendTime = temp.count();
	int resendCount = 1;
	while (true) {
		temp = high_resolution_clock::now() - tZero;
		int64_t now = temp.count();
		if (now > resendTime) {
			if (resendCount > 3) { ps->free(px); return false; }
			sendToForest(px);
			resendTime += 1000000000;
			resendCount++;
		}
		this_thread::sleep_for(milliseconds(10));
		int rx = recvFromForest();
		if (rx != 0) {
			Packet& reply = ps->getPacket(rx);
			if (reply.type == Forest::CONNECT &&
			    reply.flags == Forest::ACK_FLAG) {
				ps->free(px); ps->free(rx);
				return true;
			}
		}
	}
}


/** Send final disconnect packet to forest router.
 */
bool Substrate::disconnect() {
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);

	p.payload()[0] = htonl((uint32_t) (nonce >> 32));
	p.payload()[1] = htonl((uint32_t) (nonce & 0xffffffff));
	p.length = Forest::OVERHEAD + 8; p.type = Forest::DISCONNECT;
	p.flags = 0; p.comtree = Forest::CONNECT_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	nanoseconds temp;
	temp = high_resolution_clock::now() - tZero;
	int64_t resendTime = temp.count();
	int resendCount = 1;
	while (true) {
		temp = high_resolution_clock::now() - tZero;
		int64_t now = temp.count();
		if (now > resendTime) {
			if (resendCount > 3) { ps->free(px); return false; }
			sendToForest(px);
			resendTime += 1000000000;
			resendCount++;
		}
		this_thread::sleep_for(milliseconds(100));
		int rx = recvFromForest();
		if (rx != 0) {
			Packet& reply = ps->getPacket(rx);
			if (reply.type == Forest::DISCONNECT &&
			    reply.flags == Forest::ACK_FLAG) {
				ps->free(px); ps->free(rx);
				return true;
			}
		}
	}
}

} // ends namespace
