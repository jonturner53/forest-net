/** @file Substrate.cpp
 * 
 *  @author Jonathan Turner
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Substrate.h"

namespace forest {

Substrate::Substrate(fAdr_t myAdr1, ipa_t myIp1, fAdr_t rtrAdr1, ipa_t rtrIp1,
		ipp_t rtrPort1, uint64_t nonce1, int threadCount1,
		void* (*handler1)(void*), int dgPort1, int listenPort1,
		PacketStoreTs *ps1, Logger *logger1)
		: myAdr(myAdr1), myIp(myIp1), rtrAdr(rtrAdr1), rtrIp(rtrIp1),
		  rtrPort(rtrPort1), nonce(nonce1), threadCount(threadCount1),
		  handler(handler1), dgPort(dgPort1),
		  listenPort(listenPort1), ps(ps1), logger(logger1) {
	pool = new ThreadInfo[threadCount+1];
	threads = new UiSetPair(threadCount);
	inReqMap = new IdMap(threadCount);
	outReqMap = new IdMap(threadCount);

	rtrReady = false;
}

Substrate::~Substrate() {
	if (dgSock > 0) close(dgSock);
	if (listenSock > 0) close(listenSock);
	delete [] pool; delete threads; delete inReqMap; delete outReqMap;
}

bool Substrate::init() {
	// setup thread pool for handling control packets
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,4*PTHREAD_STACK_MIN);
	size_t stacksize;
	pthread_attr_getstacksize(&attr,&stacksize);
	if (stacksize != 4*PTHREAD_STACK_MIN)
		logger->log("Substrate:init: can't set stack size",4);
	for (int t = 1; t <= threadCount; t++) {
		if (!pool[t].qp.in.init() || !pool[t].qp.out.init()) {
			logger->log("Substrate::init: cannot initialize "
			   	   "thread queues",2);
			return false;
		}
		if (pthread_create(&pool[t].thid,&attr,handler,
				   (void *) &pool[t].qp) != 0) {
			logger->log("Substrate::init: cannot create "
			   	   "thread pool",2);
			return false;
		}
	}

	// setup sockets
	dgSock = Np4d::datagramSocket();
	if (dgSock < 0 || !Np4d::bind4d(dgSock,myIp,dgPort) ||
	    !Np4d::nonblock(dgSock)) {
		return false;
	}
	listenSock = Np4d::streamSocket();
	if (listenSock < 0) return false;
	return	Np4d::bind4d(listenSock,myIp,listenPort) &&
		Np4d::listen4d(listenSock) && Np4d::nonblock(listenSock);
}

/** Run forever, or until time expires.
 *  @param finTime is the number of seconds to run before halting;
 *  if zero, run forever
 */
bool Substrate::run(int finTimeSec) {
	seqNum = 1;
	now = Misc::getTimeNs();
	uint64_t finishTime = finTimeSec;
	finishTime *= 1000000000; // convert to ns

cerr << "finishTime=" << finishTime << endl;
	bool nothing2do; bool connected = false;
	while (finishTime == 0 || now <= finishTime) {
		nothing2do = true;
		if (!connected && rtrReady) {
			// this allows substrate to run before its
			// access router has booted; used by NetMgr
			if (!connect()) {
				return false;
			}
			connected = true;
		}
		// check for connection requests from remote clients
		int connSock = -1;
		if ((connSock = Np4d::accept4d(listenSock)) > 0) {
			// let handler know this is socket# for remote host
			int t = threads->firstOut();
			if (t != 0) {
				threads->swap(t);
				pool[t].qp.in.enq(-connSock);
			} else {
				logger->log("Substrate: thread pool is "
					   "exhausted",4);
			}
		} 
		// check for packets from the Forest net
		pktx px = recvFromForest();
		if (px != 0) {
			inbound(px); nothing2do = false;
		}

		// now handle outgoing packets from the thread pool
		for (int t = threads->firstIn(); t!=0; t = threads->nextIn(t)) {
			if (!pool[t].qp.out.empty()) {
				outbound(pool[t].qp.out.deq(),t);
				nothing2do = false;
			}
		}

		// check for expired timeouts
		for (int t = threads->firstIn(); t != 0;
			 t = threads->nextIn(t)) {
			if (pool[t].seqNum != 0 && pool[t].ts < now) {
				outReqMap->dropPair(pool[t].seqNum);
				pool[t].seqNum = 0;
			}
		}
		if (nothing2do && threads->firstIn() == 0) usleep(1000);
		sched_yield();
		now = Misc::getTimeNs();
	}
	if (connected) return disconnect();
	return true;
}

/** Send an incoming packet to a thread, possibly assigning new one.
 *  @param px is a packet index
 */
void Substrate::inbound(pktx px) {
	Packet& p = ps->getPacket(px);
	if (p.type == Forest::CLIENT_SIG || p.type == Forest::NET_SIG) {
		CtlPkt cp(p);
		if (cp.mode == CtlPkt::REQUEST) {
			int t = threads->firstOut();
			uint64_t key = p.srcAdr;
			key <<= 32; key += cp.seqNum;
			if (inReqMap->validKey(key)) {
				// in this case, we've got an active thread 
				// handling this request, so ignore duplicate
			} else if (t != 0) { // assign thread
				threads->swap(t);
				inReqMap->addPair(key,t);
				pool[t].seqNum = 0;
				pool[t].qp.in.enq(px);
				return;
			} else {
				logger->log("Substrate: thread pool is "
					   "exhausted",4);
			}
		} else {
			// replies are returned to the thread that
			// sent the request - outReqMap contains thread index
			int t = outReqMap->getId(cp.seqNum);
			if (t != 0) {
				outReqMap->dropPair(cp.seqNum);
				pool[t].seqNum = 0;
				pool[t].qp.in.enq(px);
				return;
			}
		}
	}
	ps->free(px);
}

/** Handle an outbound packet from one of the worker threads.
 *  @param px is a packet index
 *  @param t is the thread index (in the tread pool)
 */
void Substrate::outbound(pktx px, int t) {
	if (px == 0) { // means worker thread completed task
		inReqMap->dropPair(inReqMap->getKey(t));
		pool[t].qp.in.reset();
		threads->swap(t);
		return;
	}
	Packet& p = ps->getPacket(px);
	CtlPkt cp(p.payload(),p.length-Forest::OVERHEAD);
	cp.unpack();
	if (cp.mode != CtlPkt::REQUEST) {
		// just send it and return
		sendToForest(px); return;
	}
	if (cp.seqNum == 1) {
		// means this is a repeat of a pending
		// outgoing request
		if (outReqMap->validId(t)) {
			cp.seqNum = outReqMap->getKey(t);
		} else {
			// in this case, reply has arrived but was not yet
			// seen by thread so, suppress duplicate request
			ps->free(px); return;
		}
	} else {
		// first time for this request, assign seq number and
		// remember it so we can route reply to correct thread
		if (outReqMap->validId(t))
			outReqMap->dropPair(outReqMap->getKey(t));
		outReqMap->addPair(seqNum,t);
		pool[t].seqNum = seqNum;
		cp.seqNum = seqNum++;
	}
	cp.pack();
	p.payErrUpdate();
	// timeout used to purge old entries
	pool[t].ts = now + 2000000000; // 2 sec timeout
	seqNum++;
	sendToForest(px);
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
	if (nbytes < 0) { ps->free(px); return 0; }
	p.unpack();
	p.tunIp = srcIp; p.tunPort = srcPort;
string s;
cerr << "got packet from " << Np4d::ip2string(srcIp,s) << " " << srcPort << endl;
cerr << p.toString(s) << endl;

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
string s;
cerr << "substrate sending to " << Np4d::ip2string(ip,s) << " " << port << endl;
cerr << p.toString(s) << endl;
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

	p.payload()[0] = htonl((uint32_t) (nonce >> 32));
	p.payload()[1] = htonl((uint32_t) (nonce & 0xffffffff));
	p.length = Forest::OVERHEAD + 8; p.type = Forest::CONNECT; p.flags = 0;
	p.comtree = Forest::CONNECT_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	int resendTime = Misc::getTime();
	int resendCount = 1;
	while (true) {
		int now = Misc::getTime();
		if (now > resendTime) {
			if (resendCount > 3) { ps->free(px); return false; }
			sendToForest(px);
			resendTime += 1000000;
			resendCount++;
		}
		int rx = recvFromForest();
		if (rx == 0) { usleep(100000); continue; }
		Packet& reply = ps->getPacket(rx);
		bool status =  (reply.type == Forest::CONNECT &&
		    		reply.flags == Forest::ACK_FLAG);
		ps->free(px); ps->free(rx);
		return status;
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

	int resendTime = Misc::getTime();
	int resendCount = 1;
	while (true) {
		int now = Misc::getTime();
		if (now > resendTime) {
			if (resendCount > 3) { ps->free(px); return false; }
			sendToForest(px);
			resendTime += 1000000;
			resendCount++;
		}
		int rx = recvFromForest();
		if (rx == 0) { usleep(100000); continue; }
		Packet& reply = ps->getPacket(rx);
		bool status =  (reply.type == Forest::DISCONNECT &&
		    		reply.flags == Forest::ACK_FLAG);
		ps->free(px); ps->free(rx);
		return status;
	}
}

} // ends namespace
