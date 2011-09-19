/** @file NetMgr.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "NetMgr.h"

/** usage:
 *       NetMgr extIp intIp rtrIp myAdr rtrAdr finTime cliMgrAdr clientInfo
 * 
 *  Command line arguments include two ip addresses for the
 *  NetMgr. The first is the IP address that a remote UI can
 *  use to connect to the NetMgr. If this is specified as 127.0.0.1
 *  the NetMgr listens on the default IP address. The second is the
 *  IP address used by the NetMgr within the Forest overlay. RtrIp
 *  is the router's IP address, myAdr is the NetMgr's Forest
 *  address, rtrAdr is the Forest address of the router.
 *  CliMgrAdr is the Forest address of the client manager.
 *  ClientInfo is a file containing address information relating
 *  clients and routers. This is a temporary expedient.
 */


main(int argc, char *argv[]) {
	int comt; uint32_t finTime;

	if (argc != 9 ||
  	    (extIp  = Np4d::ipAddress(argv[1])) == 0 ||
  	    (intIp  = Np4d::ipAddress(argv[2])) == 0 ||
	    (rtrIp  = Np4d::ipAddress(argv[3])) == 0 ||
	    (myAdr  = Forest::forestAdr(argv[4])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[5])) == 0 ||
	    (cliMgrAdr = Forest::forestAdr(argv[6])) == 0 ||
	     sscanf(argv[7],"%d", &finTime) != 1)
		fatal("usage: NetMgr extIp intIp rtrIpAdr myAdr rtrAdr "
		      "clientInfo");

	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress(); 
	if (extIp == 0) fatal("can't retrieve default IP address");

	if (!init()) {
		fatal("NetMgr: initialization failure");
	}
	if (!readPrefixInfo(argv[8]))
		fatal("can't read prefix address info");
	//run(finTime);
	pthread_t run_thread;
	if (pthread_create(&run_thread,NULL,run,(void *) &finTime) != 0) {
		fatal("can't create run thread\n");
	}
	pthread_join(run_thread,NULL);
	cleanup();
	exit(0);
}


/** Initialization of NetMgr.
 *  Involves setting up thread pool to handle control packets
 *  and setting up sockets.
 *  @return true on success, false on failure
 */
bool init() {
	int nPkts = 10000;
	ps = new PacketStoreTs(nPkts+1);

	pool = new ThreadPool[TPSIZE+1];
	threads = new UiSetPair(TPSIZE);
	tMap = new IdMap(TPSIZE);

	// read NetInfo data structure from file - defer for now

	// for each router, add send control packets to
	// add interfaces, links, comtrees and comtree links 
	// - defer for now

	// setup thread pool for handling control packets
	for (int t = 1; t <= TPSIZE; t++) {
		if (!pool[t].qp.in.init() || !pool[t].qp.out.init())
			fatal("init: can't initialize thread queues\n");
		if (pthread_create(&pool[t].th,NULL,handler,
				   (void *) &pool[t].qp) != 0) 
                	fatal("init: can't create thread pool");
	}
	
	// setup sockets
	intSock = Np4d::datagramSocket();
	if (intSock < 0 || 
	    !Np4d::bind4d(intSock,intIp,0) ||
	    !Np4d::nonblock(intSock))
		return false;

	connect(); 		// send initial connect packet
	usleep(1000000);	// 1 second delay provided for use in SPP
				// delay gives SPP linecard the time it needs
				// to setup NAT filters before second packet

	// setup external TCP socket for use by remote GUI
	extSock = Np4d::streamSocket();
	if (extSock < 0) return false;
	return	Np4d::bind4d(extSock,extIp,NM_PORT) &&
		Np4d::listen4d(extSock) && 
		Np4d::nonblock(extSock);
}

void cleanup() {
	cout.flush(); cerr.flush();
	if (extSock > 0) close(extSock);
	if (intSock > 0) close(intSock);
	delete ps; delete [] pool; delete tMap; delete threads;
}

/** Run the NetMgr.
 *  @param finTime is the number of seconds to run before halting;
 *  if zero, run forever
 */
void* run(void* finTimeP) {
	bool nothing2do;
	uint64_t seqNum = 1;
	uint64_t now = Misc::getTimeNs();
	uint64_t finishTime = *((uint32_t *) finTimeP);
	finishTime *= 1000000000; // convert to ns
	connSock = -1;
cerr << "run: entering main loop\n"; cerr.flush();
	while (finishTime == 0 || now <= finishTime) {
		bool nothing2do = true;

		// check for packets
		int p = recvFromCons();
		if (p != 0) {
			// let handler know this is from remote console
			ps->getHeader(p).setSrcAdr(0);
cerr << "run: got packet from console\n"; cerr.flush();
ps->getHeader(p).write(cerr,ps->getBuffer(p));
		} else {
			p = rcvFromForest();
if (p != 0) {
cerr << "run: got packet from forest\n"; cerr.flush();
ps->getHeader(p).write(cerr,ps->getBuffer(p));
}
		}
		if (p != 0) {
			// send p to a thread, possibly assigning one
			PacketHeader& h = ps->getHeader(p);
			if (h.getPtype() == NET_SIG) {
				CtlPkt cp;
				cp.unpack(ps->getPayload(p),
					  h.getLength()-Forest::OVERHEAD);
				int t;
				if (cp.getRrType() == REQUEST) {
					t = threads->firstOut();
					if (t != 0) {
cerr << "run: sending packet to thread " << pool[t].th << "\n"; cerr.flush();
						threads->swap(t);
						pool[t].seqNum = 0;
						pool[t].qp.in.enq(p);
					} else {
						cerr << "run: thread "
							"pool is exhausted\n";
						ps->free(p);
					}
				} else {
					t = tMap->getId(cp.getSeqNum());
					if (t != 0) {
cerr << "run: sending packet to thread " << pool[t].th << "\n"; cerr.flush();
						tMap->dropPair(cp.getSeqNum());
						pool[t].seqNum = 0;
						pool[t].qp.in.enq(p);
					} else {
						ps->free(p);
					}
				}
			} else {
				ps->free(p);
			}
			nothing2do = false;
		}

		// now handle packets from the thread pool
		for (int t = threads->firstIn(); t != 0;
			 t = threads->nextIn(t)) {
			if (pool[t].qp.out.empty()) continue;
			int p1 = pool[t].qp.out.deq();
cerr << "read packet " << p1 << " from thread " << pool[t].th << "'s queue\n";
			if (p1 == 0) { // means thread is done
cerr << "run: thread " << pool[t].th << " reports done\n"; cerr.flush();
				pool[t].qp.in.reset();
				threads->swap(t);
				continue;
			}
			nothing2do = false;
			PacketHeader& h1 = ps->getHeader(p1);
			CtlPkt cp1;
			cp1.unpack(ps->getPayload(p1),
				   h1.getLength()-Forest::OVERHEAD);
			if (h1.getDstAdr() == 0) {
cerr << "run: sending reply back to console\n";
h1.write(cerr,ps->getBuffer(p));
cerr.flush();
				sendToCons(p1);
			} else if (cp1.getRrType() == REQUEST) {
				if (tMap->validId(t)) 
					tMap->dropPair(tMap->getKey(t));
				tMap->addPair(seqNum,t);
				cp1.setSeqNum(seqNum);
				cp1.pack(ps->getPayload(p1));
				h1.payErrUpdate(ps->getBuffer(p1));
				pool[t].seqNum = seqNum;
				pool[t].ts = now + 2000000000; // 2 sec timeout
				seqNum++;
cerr << "run: sending request to forest for thread " << pool[t].th << "\n";
h1.write(cerr,ps->getBuffer(p1));
cerr.flush();
				sendToForest(p1);
			} else {
cerr << "run: sending reply to forest for thread " << pool[t].th << "\n";
h1.write(cerr,ps->getBuffer(p1));
cerr.flush();
				sendToForest(p1);
			}
		}

		// check for expired timeouts
		for (int t = threads->firstIn(); t != 0;
			 t = threads->nextIn(t)) {
			if (pool[t].seqNum != 0 && pool[t].ts < now) {
cerr << "run: timeout on request sent to router for thread " << pool[t].th << "\n";
				tMap->dropPair(pool[t].seqNum);
				pool[t].seqNum = 0;
			}
		}
		sched_yield();
usleep(50000); // to reduce debugging output
/*
		if (nothing2do) {
cerr << "main thread going to sleep\n";
			usleep(200000); // cut to 1 ms after debug
cerr << "main thread waking up\n";
		}
*/
		now = Misc::getTimeNs();
	}
cerr << "exiting main loop\n"; cerr.flush();
	disconnect();
}

/** Control packet handler.
 *  This method is run as a separate thread and does not terminate
 *  until the process does. It communicates with the main thread
 *  through a pair of queues, passing packet numbers back and forth
 *  across the thread boundary. When a packet number is passed to
 *  a handler, the handler "owns" the corresponding packet and can
 *  read/write it without locking. The handler is required free any
 *  packets that it no longer needs to the packet store.
 *  When the handler has completed the requested operation, it sends
 *  a 0 value back to the main thread, signalling that it is available
 *  to handle another task.
 *  @param qp is a pair of queues; on is the input queue to the
 *  handler, the other is its output queue
 */
void* handler(void *qp) {
	Queue& inQ  = ((QueuePair *) qp)->in;
	Queue& outQ = ((QueuePair *) qp)->out;

	while (true) {
cerr << "handler " << pthread_self() << " entering deq\n";
		int p = inQ.deq();
cerr << "handler " << pthread_self() << " got packet\n";
		PacketHeader& h = ps->getHeader(p);
		CtlPkt cp;
		cp.unpack(ps->getPayload(p),h.getLength()-Forest::OVERHEAD);
		bool success;
		if (h.getSrcAdr() == 0) {
			success = handleConsReq(p, cp, inQ, outQ);
		} else {
			switch (cp.getCpType()) {
			case NEW_CLIENT:
				success = handleNewClient(p,cp,inQ,outQ);
				break;
			case CLIENT_CONNECT:
			case CLIENT_DISCONNECT:
				success = handleConDisc(p,cp,inQ,outQ);
				break;
			default:
				errReply(p,cp,outQ,"invalid control packet "
						   "type for NetMgr");
				break;
			}
		}
		if (!success) {
			cerr << "handler: operation failed\n";
			h.write(cerr,ps->getBuffer(p));
		}
		ps->free(p); // release p now that we're done
cerr << "handler " << pthread_self() << " finishing\n";
		outQ.enq(0); // signal completion to main thread
	}
}

/** Handle a request packet from the remote console.
 *  This involves forwarding the packet to a remote router.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @param return true on successful completion, else false
 */
bool handleConsReq(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
cerr << "handleConsReq: sending console request to Forest " << pthread_self() << endl;
	int reply = sendAndWait(p,cp,inQ,outQ);
if (reply == 0)
cerr << "handlleConsReq: got no reply\n";
	if (reply != 0) {
		PacketHeader& h = ps->getHeader(reply);
cerr << "handleConsReq: got reply" << endl;
h.write(cerr,ps->getBuffer(p));
		// use 0 destination address to tell main thread to send
		// this packet to remote console
		h.setDstAdr(0); ps->pack(reply);
		outQ.enq(reply);
		return true;
	}
	return false;
}

/** Handle a connection/disconnection notification from a router.
 *  The request is acknowledged and then forwarded to the
 *  client manager.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true if the operation was completed successfully,
 *  otherwise false; an error reply is considered a successful
 *  completion; the operation can fail if it cannot allocate
 *  packets, or if the client manager never acknowledges the
 *  notification.
 */
bool handleConDisc(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
	PacketHeader& h = ps->getHeader(p);
	if (!cp.isSet(CLIENT_ADR)) {
		errReply(p,cp,outQ,"missing required attribute");
		return true;
	}
	fAdr_t clientAdr = cp.getAttr(CLIENT_ADR);
	fAdr_t rtrAdr = h.getSrcAdr();

	// send positive reply back to router
	int p1 = ps->alloc();
	if (p1 == 0) { return false; }
	CtlPkt cp1; 
	cp1.setCpType(cp.getCpType());
	cp1.setRrType(POS_REPLY);
	cp1.setSeqNum(cp.getSeqNum());
	int plen = cp1.pack(ps->getPayload(p1));

	PacketHeader& h1 = ps->getHeader(p1);
	h1.setLength(Forest::OVERHEAD + plen);
	h1.setComtree(100);
	h1.setDstAdr(rtrAdr);
	h1.setSrcAdr(myAdr);
	h1.pack(ps->getBuffer(p1));

	outQ.enq(p1);

	// now, send notification to client manager
	int p2 = ps->alloc();
	if (p2 == 0) { return false; }
	CtlPkt cp2; 
	cp2.setCpType(cp.getCpType());
	cp2.setRrType(REQUEST);
	cp2.setAttr(CLIENT_ADR,clientAdr);
	cp2.setAttr(RTR_ADR,rtrAdr);
	plen = cp2.pack(ps->getPayload(p2));

	PacketHeader& h2 = ps->getHeader(p2);
	h2.setLength(Forest::OVERHEAD + plen);
	h2.setComtree(100);
	h2.setDstAdr(cliMgrAdr);
	h2.setSrcAdr(myAdr);
	h2.pack(ps->getBuffer(p2));

	int reply = sendAndWait(p2,cp2,inQ,outQ);
	if (reply == -1) {
		ps->free(p2);
		cerr << "handleRtrReq: no reply from client manager\n";
		return false;
	}
	PacketHeader& hr = ps->getHeader(reply);
	CtlPkt cpr;
	cpr.unpack(ps->getPayload(reply),hr.getLength()-Forest::OVERHEAD);
	if (cpr.getRrType() == NEG_REPLY) {
		cerr << "handleRtrReq: negative reply from client ";
			"manager\n";
		ps->free(p2); ps->free(reply);
		return false;
	}
	ps->free(p2); ps->free(reply);
	return true;
}

/** Handle a new client request.
 *  This requires selecting a router, sending an add_link message
 *  to the router, and after getting a response, sending an
 *  add_comtree_link message to the router, in order to add
 *  the new client to the client signalling comtree.
 *  Once that is all done, a reply is sent to the original request.
 *  The operation can fail for a variety of reasons, at different
 *  points in the process. Any failure causes a negative reply
 *  to be sent back to the original requestor.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success; the operation
 *  fails if it cannot allocate required packets, or if the
 *  router fails to respond to either of the control messages
 *  sent to it.
 */
bool handleNewClient(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
	PacketHeader& h = ps->getHeader(p);
	fAdr_t requestorAdr = h.getSrcAdr();

	// determine which router to use
	if (!cp.isSet(CLIENT_IP)) {
		errReply(p,cp,outQ,"CLIENT_IP attribute is missing");
		return true;
	}
	ipa_t clientIp = cp.getAttr(CLIENT_IP);
	ipp_t clientPort = cp.getAttr(CLIENT_PORT);
	fAdr_t rtrAdr;
	if (!findCliRtr(clientIp,rtrAdr)) {
		errReply(p,cp,outQ,"No router assigned to client's IP");
		return true;
	}

	// send add link packet to router and extract info from reply
	int p1 = ps->alloc();
	if (p1 == 0) return false; 
	CtlPkt cp1;
	cp1.setRrType(REQUEST);
	cp1.setCpType(ADD_LINK);
	cp1.setAttr(PEER_IP,clientIp);
	cp1.setAttr(PEER_PORT,clientPort);
	cp1.setAttr(PEER_TYPE,CLIENT);
	int plen = cp1.pack(ps->getPayload(p1));

	PacketHeader& h1 = ps->getHeader(p1);
	h1.setPtype(NET_SIG);
	h1.setLength(plen + Forest::OVERHEAD);
	h1.setFlags(0);
	h1.setDstAdr(rtrAdr);
	h1.setSrcAdr(myAdr);
	h1.setComtree(100);
	h1.pack(ps->getBuffer(p1));

	int reply = sendAndWait(p1,cp1,inQ,outQ);
	if (reply == -1) {
		ps->free(p1);
		cerr << "handleCMReq: no reply from router\n";
		errReply(p,cp,outQ,"no reply from router");
		return true;
	}
	PacketHeader& hr = ps->getHeader(reply);
	CtlPkt cpr;
	cpr.unpack(ps->getPayload(reply),hr.getLength()-Forest::OVERHEAD);
	if (cpr.getRrType() == NEG_REPLY) {
		errReply(p,cp,outQ,"Router failed to allocate link");
		ps->free(reply);
		return true;
	}
	int clientLink = cpr.getAttr(LINK_NUM);
	fAdr_t clientAdr = cpr.getAttr(PEER_ADR);
	ipa_t clientRtrIp = cpr.getAttr(RTR_IP);
	ps->free(reply);

	// now set rates on new link - for now, just 5*(min rates) - re-using p1
	cp1.reset();
	cp1.setRrType(REQUEST);
	cp1.setCpType(MOD_LINK);
	cp1.setAttr(LINK_NUM,clientLink);
	cp1.setAttr(BIT_RATE,5*Forest::MINBITRATE);
	cp1.setAttr(PKT_RATE,5*Forest::MINPKTRATE);
	plen = cp1.pack(ps->getPayload(p1));

	h1.setPtype(NET_SIG);
	h1.setLength(plen + Forest::OVERHEAD);
	h1.setFlags(0);
	h1.setDstAdr(rtrAdr);
	h1.setSrcAdr(myAdr);
	h1.setComtree(100);
	h1.pack(ps->getBuffer(p1));

	int reply1 = sendAndWait(p1,cp1,inQ,outQ);
	if (reply1 == -1) {
		ps->free(p1);
		cerr << "handleCMReq: no reply from router\n";
		errReply(p,cp,outQ,"no reply from router");
		return true;
	}
	PacketHeader& hr1 = ps->getHeader(reply1);
	CtlPkt cpr1;
	cpr1.unpack(ps->getPayload(reply1),hr1.getLength() - Forest::OVERHEAD);
	if (cpr1.getRrType() == NEG_REPLY) {
		errReply(p,cp,outQ,"Router could not add set link rates "
				   " for new client link");
		ps->free(p1);
		ps->free(reply1);
		return true;
	}

	// now add the new client to the user signalling comtree, re-using p1
	cp1.reset();
	cp1.setRrType(REQUEST);
	cp1.setCpType(ADD_COMTREE_LINK);
	cp1.setAttr(COMTREE_NUM,1);
	cp1.setAttr(LINK_NUM,clientLink);
	plen = cp1.pack(ps->getPayload(p1));

	h1.setPtype(NET_SIG);
	h1.setLength(plen + Forest::OVERHEAD);
	h1.setFlags(0);
	h1.setDstAdr(rtrAdr);
	h1.setSrcAdr(myAdr);
	h1.setComtree(100);
	h1.pack(ps->getBuffer(p1));

	int reply2 = sendAndWait(p1,cp1,inQ,outQ);
	ps->free(p1); // now, done with p1
	if (reply2 == -1) {
		cerr << "handleCMReq: no reply from router\n";
		errReply(p,cp,outQ,"no reply from router");
		return true;
	}
	PacketHeader& hr2 = ps->getHeader(reply2);
	CtlPkt cpr2;
	cpr2.unpack(ps->getPayload(reply2),hr2.getLength() - Forest::OVERHEAD);
	if (cpr2.getRrType() == NEG_REPLY) {
		errReply(p,cp,outQ,"Router could not add client to signalling"
				   " comtree");
		ps->free(reply2);
		return true;
	}

	// re-use reply2 to respond back to client manager
	cpr2.reset();
	cpr2.setCpType(NEW_CLIENT);
	cpr2.setSeqNum(cp.getSeqNum());
	cpr2.setRrType(POS_REPLY);
	cpr2.setAttr(COMTREE_NUM,1);
	cpr2.setAttr(CLIENT_ADR,clientAdr);
	cpr2.setAttr(RTR_IP,clientRtrIp);
	cpr2.setAttr(RTR_ADR,rtrAdr);
	plen = cpr2.pack(ps->getPayload(reply2));

	hr2.setPtype(NET_SIG);
	hr2.setLength(plen + Forest::OVERHEAD);
	hr2.setFlags(0);
	hr2.setDstAdr(requestorAdr);
	hr2.setSrcAdr(myAdr);
	hr2.setComtree(100);
	hr2.pack(ps->getBuffer(reply2));

	outQ.enq(reply2);
	return true;
}

/** Send a control packet multiple times before giving up.
 *  This method makes a copy of the original and sends the copy
 *  back to the main thread. If no reply is received after 1 second,
 *  it tries again; it makes a total of three attempts before giving up.
 *  @param p is the packet number of the packet to be sent; the header
 *  for p is assumed to be unpacked
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return the packet number for the reply packet, or -1 if there
 *  was no reply
 */
int sendAndWait(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
	PacketHeader& h = ps->getHeader(p);
	buffer_t& b = ps->getBuffer(p);
	int nwords = (h.getLength() + 3)/4;
	h.setSrcAdr(myAdr); ps->pack(p);

	// make copy of packet and send the copy
	int copy = ps->fullCopy(p);
	if (copy == 0) return false;
	buffer_t& bc = ps->getBuffer(copy);
	for (int j = 0; j < nwords; j++) bc[j] = b[j];

cerr << "sendAndWait: sending request for thread " << pthread_self() << "\n";
h.write(cerr,ps->getBuffer(p));
	outQ.enq(copy);

	for (int i = 1; i < 3; i++) {
		int reply = inQ.deq(1000000000); // 1 sec timeout
		if (reply == -1) { // timeout
cerr << "sendAndWait: timeout\n";
			// no reply, make new copy and send
			int copy = ps->alloc();
			if (copy == 0) {
				cerr << "sendAndWait: no packets "
					"left in packet store\n";
				return -1;
			}
			buffer_t& bc = ps->getBuffer(copy);
			for (int j = 0; j < nwords; j++) bc[j] = b[j];
			outQ.enq(copy);
		} else {
cerr << "sendAndWait: got reply\n";
			return reply;
		}
	}
	return -1;
}


/** Build and send error reply packet for p.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param outQ is the queue going back to the main thread
 *  @param msg is the error message to be sent.
 */
void errReply(int p, CtlPkt& cp, Queue& outQ, const char* msg) {
	PacketHeader& h = ps->getHeader(p);

	int p1 = ps->fullCopy(p);
	PacketHeader& h1 = ps->getHeader(p1);
	CtlPkt cp1;
	cp1.unpack(ps->getPayload(p1),h1.getLength()-Forest::OVERHEAD);

	cp1.setRrType(NEG_REPLY);
	cp1.setErrMsg(msg);
	int plen = cp1.pack(ps->getPayload(p1));

	h1.setLength(Forest::OVERHEAD + plen);
	h1.setDstAdr(h.getSrcAdr());
	h1.setSrcAdr(myAdr);
	h1.pack(ps->getBuffer(p1));

	outQ.enq(p1);
}

/** Finds the router address of the router based on IP prefix
 *  @param cliIp is the ip of the client
 *  @param rtrAdr is the address of the router associated with this IP prefix
 *  @return true if there was an IP prefix found, false otherwise
 */
bool findCliRtr(ipa_t cliIp, fAdr_t& rtrAdr) {
	string cip;
	Np4d::addIp2string(cip,cliIp);
	for(size_t i = 0; i < numPrefixes; ++i) {
		string ip = prefixes[i].prefix;
		for(size_t j = 0; j < ip.size(); ++j) {
			if(ip[j]=='*') {
				rtrAdr = prefixes[i].rtrAdr;
				return true;
			} else if(cip[j]!=ip[j]) {
				break;
			}
		}
	}
	return false;
}

/** Reads the prefix file
 *  @param filename is the name of the prefix file
 *  @return true on success, false otherwise
 */
bool readPrefixInfo(char filename[]) {
	ifstream ifs; ifs.open(filename);
	if(ifs.fail()) return false;
	Misc::skipBlank(ifs);
	int i = 0;
	while(!ifs.eof()) {
		string pfix; fAdr_t rtrAdr;
		ifs >> pfix;
		if(!Forest::readForestAdr(ifs,rtrAdr))
			break;
		prefixes[i].prefix = pfix;
		prefixes[i].rtrAdr = rtrAdr;
		Misc::skipBlank(ifs);
		i++;
	}
	numPrefixes = i;
	cout << "read address info for " << numPrefixes << " prefixes" << endl;
	return true;
}

/** Check for next packet from the remote console.
 *  @return a packet number with a formatted control packet on success,
 *  0 on failure
 */
int recvFromCons() { 
	if (connSock < 0) {
		connSock = Np4d::accept4d(extSock);
		if (connSock < 0) return 0;
		if (!Np4d::nonblock(connSock))
			fatal("can't make connection socket nonblocking");
cerr << "connection socket established, connSock=" << connSock << endl;
	}

	int p = ps->alloc();
	if (p == 0) return 0;
	PacketHeader& h = ps->getHeader(p);
	buffer_t& b = ps->getBuffer(p);
	
	int nbytes = Np4d::recvBuf(connSock, (char *) &b, Forest::BUF_SIZ);
	if (nbytes == -1) { ps->free(p); return 0; }
	if (nbytes < Forest::HDR_LENG)
		fatal(" recvFromCons: misformatted packet from console");
	h.unpack(b);
	if (h.getVersion() != 1 || h.getLength() != nbytes ||
	    (h.getPtype() != CLIENT_SIG && h.getPtype() != NET_SIG))
		fatal(" recvFromCons: misformatted packet from console");
        return p;
}

/** Write a packet to the socket for the user interface.
 *  @param p is the packet to be sent to the CLI
 */
void sendToCons(int p) {
	buffer_t& buf = ps->getBuffer(p);
	int length = ps->getHeader(p).getLength();
	ps->pack(p);
	Np4d::sendBuf(connSock, (char *) &buf, length);
	ps->free(p);
}

/** Check for next packet from the Forest network.
 *  @return next packet or 0, if no report has been received.
 */
int rcvFromForest() { 
	int p = ps->alloc();
	if (p == 0) return 0;
        buffer_t& b = ps->getBuffer(p);

        int nbytes = Np4d::recv4d(intSock,&b[0],1500);
        if (nbytes < 0) { ps->free(p); return 0; }
	ps->unpack(p);

	PacketHeader& h = ps->getHeader(p);

        return p;
}

/** Send packet to Forest router.
 */
void sendToForest(int p) {
	buffer_t& buf = ps->getBuffer(p);
	int leng = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(intSock,(void *) &buf,leng,
		    	      	rtrIp,Forest::ROUTER_PORT);
	if (rv == -1) fatal("sendToForest: failure in sendto");
	ps->free(p);
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
void connect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	sendToForest(p);
}

/** Send final disconnect packet to forest router.
 */
void disconnect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	sendToForest(p);
}
