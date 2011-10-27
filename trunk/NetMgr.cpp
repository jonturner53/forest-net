/** @file NetMgr.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "NetMgr.h"

/** usage:
 *       NetMgr extIp topoFile clientInfo finTime
 * 
 *  Command line arguments include two ip addresses for the
 *  NetMgr. The first is the IP address that a remote UI can
 *  use to connect to the NetMgr. If this is specified as 127.0.0.1
 *  the NetMgr listens on the default IP address. The second is the
 *  topology file (aka NetInfo file) that describes the network.
 *  ClientInfo is a file containing address information relating
 *  clients and routers. finTime is the number of seconds to run.
 *  If zero, the NetMgr runs forever.
 */
main(int argc, char *argv[]) {
	int comt; uint32_t finTime;

	if (argc != 5 ||
  	    (extIp  = Np4d::ipAddress(argv[1])) == 0 ||
	     sscanf(argv[4],"%d", &finTime) != 1)
		fatal("usage: NetMgr extIp topoFile clientInfo finTime");
	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress(); 
	if (extIp == 0) fatal("can't retrieve default IP address");

	if (!init(argv[2])) {
		fatal("NetMgr: initialization failure");
	}
	if (!readPrefixInfo(argv[3]))
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
bool init(const char *topoFile) {
	Misc::getTimeNs(); // initialize time reference
	int nPkts = 10000;
	ps = new PacketStoreTs(nPkts+1);

	pool = new ThreadInfo[TPSIZE+1];
	threads = new UiSetPair(TPSIZE);
	tMap = new IdMap(TPSIZE);

	booting = true;

	// read NetInfo data structure from file
	int maxNode = 100000; int maxLink = 10000;
	int maxRtr = 5000; int maxCtl = 200;
	int maxComtree = 10000;
	net = new NetInfo(maxNode, maxLink, maxRtr, maxCtl, maxComtree);
	ifstream fs; fs.open(topoFile);
	if (fs.fail() || !net->read(fs)) {
		cerr << "NetMgr::init: could not read topology file, or error "
		      	"in topology file\n";
		return false;
	}
	fs.close();

	// find node information for netMgr and cliMgr and initialize
	// associated data
	myAdr = 0; cliMgrAdr = 0;
	for (int c = net->firstController(); c != 0;
		 c = net->nextController(c)) {
		string s;
		if (net->getNodeName(c,s).compare("netMgr") == 0) {
			intIp = net->getLeafIpAdr(c);
			myAdr = net->getNodeAdr(c);
			int lnk = net->firstLinkAt(c);
			int rtr = net->getPeer(c,lnk);
			int llnk = net->getLocLink(lnk,rtr);
			int iface = net->getIface(llnk,rtr);
			if (iface == 0) {
				cerr << "NetMgr:init: can't find ip address "
				     << "of access router\n";
			}
			rtrIp = net->getIfIpAdr(rtr,iface);
			rtrAdr = net->getNodeAdr(rtr);
		} else if (net->getNodeName(c,s).compare("cliMgr") == 0) {
			cliMgrAdr = net->getNodeAdr(c);
		}
	}
	if (myAdr == 0 || cliMgrAdr == 0) {
		cerr << "could not find netMgr or cliMgr in topology file\n";
		return false;
	}

	// setup thread pool for handling control packets
	for (int t = 1; t <= TPSIZE; t++) {
		if (!pool[t].qp.in.init() || !pool[t].qp.out.init())
			fatal("init: can't initialize thread queues\n");
		if (pthread_create(&pool[t].thid,NULL,handler,
				   (void *) &pool[t].qp) != 0) 
                	fatal("init: can't create thread pool");
	}
	
	// setup sockets
	intSock = Np4d::datagramSocket();
	if (intSock < 0 || 
	    !Np4d::bind4d(intSock,intIp,30122) ||
	    !Np4d::nonblock(intSock))
		return false;

	// setup external TCP socket for use by remote GUI
	extSock = Np4d::streamSocket();
	if (extSock < 0) return false;
	connSock = -1; // means no one connected yet
	return	Np4d::bind4d(extSock,extIp,Forest::NM_PORT) &&
		Np4d::listen4d(extSock) && 
		Np4d::nonblock(extSock);
}

void cleanup() {
	cout.flush(); cerr.flush();
	if (extSock > 0) close(extSock);
	if (intSock > 0) close(intSock);
	delete ps; delete [] pool; delete tMap; delete threads;
	delete net;
}

/** Run the NetMgr.
 *  @param finTime is the number of seconds to run before halting;
 *  if zero, run forever
 */
void* run(void* finTimeSec) {
	int numRouters = net->getNumRouters();
	set<int> doneBooting; // set of routers that have finished booting

	uint64_t seqNum = 1;
	uint64_t now = Misc::getTimeNs();
	uint64_t finishTime = *((uint32_t *) finTimeSec);
	finishTime *= 1000000000; // convert to ns

	bool nothing2do;
	while (finishTime == 0 || now <= finishTime) {
		bool nothing2do = true;

		// check for packets
		int p = recvFromCons();
		if (p != 0) {
			// let handler know this is from remote console
			ps->getHeader(p).setSrcAdr(0);
		} else {
			p = rcvFromForest();
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
						threads->swap(t);
						pool[t].seqNum = 0;
						pool[t].qp.in.enq(p);
					} else {
						cerr << "run: thread "
							"pool is exhausted\n";
						ps->free(p);
					}
				} else {
					fAdr_t srcAdr = h.getSrcAdr();
					if (booting &&
					    cp.getCpType() == BOOT_COMPLETE &&
					    cp.getRrType() == POS_REPLY) {
						doneBooting.insert(srcAdr);
						if (doneBooting.size() ==
						                  numRouters) {
							booting = false;

							string s;
							cout << "done booting "
							<< "at " //<< now
							<< Misc::nstime2string(
									  now,s)
							<< endl;

							connect();
							usleep(1000000);
							// sleep for SPP context
							// allows time for NAT
						}
					} else if (booting &&
					    cp.getCpType() == BOOT_COMPLETE &&
					    cp.getRrType() == NEG_REPLY) {
						string s;
						cerr << "router at address "
						<< Forest::fAdr2string(srcAdr,s)
						<< " failed to boot\n";
						pthread_exit(NULL);
					} 
					t = tMap->getId(cp.getSeqNum());
					if (t != 0) {
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
			if (p1 == 0) { // means thread is done
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
				sendToForest(p1);
			} else {
				sendToForest(p1);
			}
		}

		// check for expired timeouts
		for (int t = threads->firstIn(); t != 0;
			 t = threads->nextIn(t)) {
			if (pool[t].seqNum != 0 && pool[t].ts < now) {
				tMap->dropPair(pool[t].seqNum);
				pool[t].seqNum = 0;
			}
		}
		if (nothing2do && threads->firstIn() == 0) usleep(1000);
		sched_yield();
		now = Misc::getTimeNs();
	}
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
		int p = inQ.deq();
		PacketHeader& h = ps->getHeader(p);
		CtlPkt cp; 
		cp.unpack(ps->getPayload(p),h.getLength()-Forest::OVERHEAD);
		bool success;
		if (h.getSrcAdr() == 0) {
			success = handleConsReq(p, cp, inQ, outQ);
		} else {
			switch (cp.getCpType()) {
			case CLIENT_CONNECT:
			case CLIENT_DISCONNECT:
				success = handleConDisc(p,cp,inQ,outQ);
				break;
			case NEW_CLIENT:
				success = handleNewClient(p,cp,inQ,outQ);
				break;
			case BOOT_REQUEST:
				success = handleBootRequest(p,cp,inQ,outQ);
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
	int reply = sendAndWait(p,cp,inQ,outQ);
	if (reply != 0) {
		PacketHeader& h = ps->getHeader(reply);
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

	// send positive reply back to router
	CtlPkt repCp(cp.getCpType(), POS_REPLY, cp.getSeqNum()); 
	sendCtlPkt(repCp,h.getSrcAdr(),inQ,outQ);

	// now, send notification to client manager
	CtlPkt reqCp(cp.getCpType(), REQUEST, 0);
	reqCp.setAttr(CLIENT_ADR,cp.getAttr(CLIENT_ADR));
	reqCp.setAttr(RTR_ADR,h.getSrcAdr());
	int reply = sendCtlPkt(reqCp,cliMgrAdr,inQ,outQ);
	if (reply == 0) {
		cerr << "handleConDisc: no reply from client manager\n";
		cerr.flush();
		errReply(p,cp,outQ,"client manager never replied");
		return false;
	}
	repCp.reset();
	repCp.unpack(ps->getPayload(reply),
		     (ps->getHeader(reply)).getLength()-Forest::OVERHEAD);
	if (repCp.getRrType() == NEG_REPLY) {
		errReply(p,cp,outQ,"negative reply from client manager");
		cerr << "handleConDisc: negative reply from client manager";
			"manager\n";
		ps->free(reply);
		return false;
	}
	ps->free(reply);
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
	if (!cp.isSet(CLIENT_IP) || !cp.isSet(CLIENT_PORT)) {
		errReply(p,cp,outQ,"client IP or port attribute is missing");
		return true;
	}
	// determine which router to use
	fAdr_t rtrAdr;
	if (!findCliRtr(cp.getAttr(CLIENT_IP),rtrAdr)) {
		errReply(p,cp,outQ,"No router assigned to client's IP");
		return true;
	}

	// send add link packet to router and extract info from reply
	CtlPkt reqCp(ADD_LINK,REQUEST,0);
	reqCp.setAttr(PEER_IP,cp.getAttr(CLIENT_IP));
        reqCp.setAttr(PEER_PORT,cp.getAttr(CLIENT_PORT));
        reqCp.setAttr(PEER_TYPE,CLIENT);
	int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	if (reply == 0) {
		errReply(p,cp,outQ,"router did not reply to add link");
		cerr << "handleNewClient: no reply from router to add link\n";
		return false;
	}
	CtlPkt repCp;
	repCp.unpack(ps->getPayload(reply),
		     (ps->getHeader(reply)).getLength()-Forest::OVERHEAD);
	if (repCp.getRrType() == NEG_REPLY) {
		errReply(p,cp,outQ,"router failed to allocate link");
		cerr << "handleNewClient: router failed to allocate link\n";
		ps->free(reply);
		return false;
	}
	int clientLink = repCp.getAttr(LINK_NUM);

	fAdr_t clientAdr = repCp.getAttr(PEER_ADR);
	ipa_t clientRtrIp = repCp.getAttr(RTR_IP);
	ps->free(reply);

	// now set rates on new link - for now, 500 Kb/s and 250 p/s
	reqCp.reset(MOD_LINK,REQUEST,0);
	reqCp.setAttr(LINK_NUM,clientLink);
	reqCp.setAttr(BIT_RATE,500); // default value of 500 Kb/s
	reqCp.setAttr(PKT_RATE,250); // 250 p/s
	reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	if (reply == 0) {
		errReply(p,cp,outQ,"no reply from router to modify link");
		cerr << "handleNewClient: no reply from router to modify"
			"link\n";
		return false;
	}
	repCp.reset();
	repCp.unpack(ps->getPayload(reply),
		     (ps->getHeader(reply)).getLength() - Forest::OVERHEAD);
	if (repCp.getRrType() == NEG_REPLY) {
		errReply(p,cp,outQ,"router could not set link rates");
		cerr << "handleNewClient: router could not add set link rates "
		   	" for new client link\n";
		ps->free(reply);
		return false;
	}
	ps->free(reply);

	// now add the new client to the user signalling comtree
	reqCp.reset(ADD_COMTREE_LINK,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,1);
	reqCp.setAttr(LINK_NUM,clientLink);
	reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	if (reply == 0) {
		errReply(p,cp,outQ,"no reply from router to add comtree link");
		cerr << "handleNewClient: no reply from router to add comtree "
			"link\n";
		return false;
	}
	repCp.reset();
	repCp.unpack(ps->getPayload(reply),
		     (ps->getHeader(reply)).getLength() - Forest::OVERHEAD);
	if (repCp.getRrType() == NEG_REPLY) {
		errReply(p,cp,outQ,"router could not add client to signalling "
				   "comtree");
		cerr << "handleNewClient: router could not add client to "
			"signalling comtree";
		ps->free(reply);
		return false;
	}
	ps->free(reply);

	// send final reply back to client manager
	repCp.reset(NEW_CLIENT,POS_REPLY,cp.getSeqNum());
        repCp.setAttr(CLIENT_ADR,clientAdr);
        repCp.setAttr(RTR_IP,clientRtrIp);
        repCp.setAttr(RTR_ADR,rtrAdr);
	sendCtlPkt(repCp,h.getSrcAdr(),inQ,outQ);
	return true;
}

/** Handle a boot request from a router.
 *  This requires sending a series of control packets to the router
 *  to configure its interface table, its link table and its comtree
 *  table. When configuration is complete, we send a boot complete
 *  message to the router.
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
bool handleBootRequest(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
	PacketHeader& h = ps->getHeader(p);
	fAdr_t rtrAdr = h.getSrcAdr();
	ipa_t rtrIp = h.getTunSrcIp();
	ipa_t rtrPort = h.getTunSrcPort();
	int rtr;
	for (rtr = net->firstRouter(); rtr != 0; rtr = net->nextRouter(rtr)) {
		if (net->getNodeAdr(rtr) == rtrAdr) break;
	}
	if (rtr == 0) {
		errReply(p,cp,outQ,"boot request from unknown router "
				   "rejected\n");
		cerr << "handleBootRequest: received boot request from unknown "
			"router\n";
		return true;
	}
	// first send reply, acknowledging request and supplying
	// leaf address range
	CtlPkt repCp(BOOT_REQUEST,POS_REPLY,cp.getSeqNum());
        repCp.setAttr(FIRST_LEAF_ADR,net->getFirstLeafAdr(rtr));
        repCp.setAttr(LAST_LEAF_ADR,net->getLastLeafAdr(rtr));
	sendCtlPkt(repCp,rtrAdr,rtrIp,rtrPort,inQ,outQ);

	// add/configure interfaces
	// for each interface in table, do an add iface
	for (int i = 1; i <= net->getNumIf(rtr); i++) {
		if (!net->validIf(rtr,i)) continue;
		CtlPkt reqCp(ADD_IFACE,REQUEST,0);
		reqCp.setAttr(IFACE_NUM,i);
		reqCp.setAttr(LOCAL_IP,net->getIfIpAdr(rtr,i));
		reqCp.setAttr(MAX_BIT_RATE,net->getIfBitRate(rtr,i));
		reqCp.setAttr(MAX_PKT_RATE,net->getIfPktRate(rtr,i));
		int reply = sendCtlPkt(reqCp,rtrAdr,rtrIp,rtrPort,inQ,outQ);
		if (reply == 0) {
			cerr << "handleBootRequest: no reply from router to "
				"add interface message for "
				"interface" << i << "\n";
			ps->free(reply);
			return false;
		}
		CtlPkt repCp;
		repCp.unpack(ps->getPayload(reply),
			 (ps->getHeader(reply)).getLength() - Forest::OVERHEAD);
		if (repCp.getRrType() == NEG_REPLY) {
			CtlPkt acp(BOOT_ABORT,REQUEST,0);
			int aRep = sendCtlPkt(acp,rtrAdr,rtrIp,rtrPort,
						inQ,outQ);
			if (aRep != 0) ps->free(aRep); // ignore reply
			cerr << "handleBootRequest: router could not add "
				"interface";
			ps->free(reply);
			return false;
		}
		ps->free(reply);
	}

	// add/configure links
	// for each link, identify its interface and do an add link,
	// followed by a modify link to set attributes
	for (int lnk = net->firstLinkAt(rtr); lnk != 0;
		 lnk = net->nextLinkAt(rtr,lnk)) {
		int llnk = net->getLocLink(lnk,rtr);
		int iface = net->getIface(llnk,rtr);
		int peer = net->getPeer(rtr,lnk);
		int plnk = net->getLocLink(lnk,peer);
		ipa_t peerIp; ipp_t peerPort;
		if (net->getNodeType(peer) == ROUTER) {
			int i = net->getIface(plnk,peer);
			peerIp = net->getIfIpAdr(peer,i);
			peerPort = Forest::ROUTER_PORT;
		} else {
			peerIp = net->getLeafIpAdr(peer);
			peerPort = 0;
		}
		CtlPkt reqCp(ADD_LINK,REQUEST,0);
		reqCp.setAttr(LINK_NUM,llnk);
		reqCp.setAttr(IFACE_NUM,iface);
		reqCp.setAttr(PEER_TYPE,net->getNodeType(peer));
		reqCp.setAttr(PEER_IP,peerIp);
		reqCp.setAttr(PEER_PORT,peerPort);
		reqCp.setAttr(PEER_ADR,net->getNodeAdr(peer));
		int reply = sendCtlPkt(reqCp,rtrAdr,rtrIp,rtrPort,inQ,outQ);
		if (reply == 0) {
			cerr << "handleBootRequest: no reply from router "
			     << rtr << " to add link message for "
				"local link" << llnk << "\n";
			ps->free(reply);
			return false;
		}
		CtlPkt repCp;
		repCp.unpack(ps->getPayload(reply),
			 (ps->getHeader(reply)).getLength() - Forest::OVERHEAD);
		if (repCp.getRrType() == NEG_REPLY) {
			CtlPkt acp(BOOT_ABORT,REQUEST,0);
			int aRep = sendCtlPkt(acp,rtrAdr,rtrIp,rtrPort,
						inQ,outQ);
			if (aRep != 0) ps->free(aRep); // ignore reply
			cerr << "handleBootRequest: router " << rtr << " could "
				"not add local link " << llnk << endl;
			ps->free(reply);
			return false;
		}
		ps->free(reply);

		// now, send modify link message, to set data rates
		reqCp.reset(MOD_LINK,REQUEST,0);
		reqCp.setAttr(LINK_NUM,llnk);
		reqCp.setAttr(BIT_RATE,net->getLinkBitRate(lnk));
		reqCp.setAttr(PKT_RATE,net->getLinkPktRate(lnk));
		reply = sendCtlPkt(reqCp,rtrAdr,rtrIp,rtrPort,inQ,outQ);
		if (reply == 0) {
			cerr << "handleBootRequest: no reply from router "
			     << rtr << " to modify link message for local link"
			     << llnk << "\n";
			ps->free(reply);
			return false;
		}
		repCp.reset();
		repCp.unpack(ps->getPayload(reply),
			 (ps->getHeader(reply)).getLength() - Forest::OVERHEAD);
		if (repCp.getRrType() == NEG_REPLY) {
			CtlPkt acp(BOOT_ABORT,REQUEST,0);
			int aRep = sendCtlPkt(acp,rtrAdr,rtrIp,rtrPort,
						inQ,outQ);
			if (aRep != 0) ps->free(aRep); // ignore reply
			cerr << "handleBootRequest: router " << rtr << " could "
				"not set link rates for link " << llnk << endl;
			ps->free(reply);
			return false;
		}
		ps->free(reply);
	}
	
	// add/configure comtrees
	// for each comtree, add it to the router, do a modify comtree
	// and a series of add comtree link ops, followed by modify comtree link
	for (int ctx = net->firstComtIndex(); ctx != 0;
		 ctx = net->nextComtIndex(ctx)) {
		if (!net->isComtNode(ctx,rtr)) continue;
		
		comt_t comt = net->getComtree(ctx);

		// first step is to add comtree at router
		CtlPkt reqCp(ADD_COMTREE,REQUEST,0);
		reqCp.setAttr(COMTREE_NUM,comt);
		int reply = sendCtlPkt(reqCp,rtrAdr,rtrIp,rtrPort,
					inQ,outQ);
		if (reply == 0) {
			cerr << "handleBootRequest: no reply from router "
			     << rtr << " to add comtree message for comtree "
			     << comt << "\n";
			ps->free(reply);
			return false;
		}
		CtlPkt repCp;
		repCp.unpack(ps->getPayload(reply),
			 (ps->getHeader(reply)).getLength() - Forest::OVERHEAD);
		if (repCp.getRrType() == NEG_REPLY) {
			CtlPkt acp(BOOT_ABORT,REQUEST,0);
			int aRep = sendCtlPkt(acp,rtrAdr,rtrIp,rtrPort,
						inQ,outQ);
			if (aRep != 0) ps->free(aRep); // ignore reply
			cerr << "handleBootRequest: router " << rtr << " could "
				"not add comtree " << comt << endl;
			ps->free(reply);
			return false;
		}
		ps->free(reply);

		int plnk = net->getComtPlink(ctx,rtr);
		int parent = net->getPeer(rtr,plnk);
		// next, add links to the comtree and set their data rates
		for (int lnk = net->firstComtLink(ctx); lnk != 0;
			 lnk = net->nextComtLink(lnk,ctx)) {
			if (net->getLinkL(lnk) != rtr &&
			    net->getLinkR(lnk) != rtr)
				continue;

			// get information about this link
			int llnk = net->getLocLink(lnk,rtr);
			int peer = net->getPeer(rtr,lnk);
			ntyp_t peerType = net->getNodeType(peer);
			bool peerCoreFlag = net->isComtCoreNode(ctx,peer);

			// first, add comtree link
			CtlPkt reqCp(ADD_COMTREE_LINK,REQUEST,0);
			reqCp.setAttr(COMTREE_NUM,comt);
			reqCp.setAttr(LINK_NUM,llnk);
			reqCp.setAttr(PEER_CORE_FLAG,peerCoreFlag);
			int reply = sendCtlPkt(reqCp,rtrAdr,rtrIp,rtrPort,
							inQ,outQ);
			if (reply == 0) {
				cerr << "handleBootRequest: no reply from "
				     << "router " << rtr << " to add comtree "
				     << "link message for comtree " << comt
				     << " link " << llnk << "\n";
				ps->free(reply);
				return false;
			}
			CtlPkt repCp;
			repCp.unpack(ps->getPayload(reply),
				 	(ps->getHeader(reply)).getLength()
					- Forest::OVERHEAD);
			if (repCp.getRrType() == NEG_REPLY) {
				CtlPkt acp(BOOT_ABORT,REQUEST,0);
				int aRep = sendCtlPkt(acp,rtrAdr,rtrIp,rtrPort,
							inQ,outQ);
				if (aRep != 0) ps->free(aRep); // ignore reply
				cerr << "handleBootRequest: router "
				     << rtr << " could not add comtree link for"
				     << " comtree " << comt << " link "
				     << llnk << "\n";
				ps->free(reply);
				return false;
			}
			ps->free(reply);

			// then, set the data rates for the comtree link
			int brIn, brOut, prIn, prOut;
			if (peer == parent) {
				brIn  = net->getComtBrDown(ctx);
				brOut = net->getComtBrUp(ctx);
				prIn  = net->getComtPrDown(ctx);
				prOut = net->getComtPrUp(ctx);
			} else if (peerType == ROUTER) {
				brIn  = net->getComtBrUp(ctx);
				brOut = net->getComtBrDown(ctx);
				prIn  = net->getComtPrUp(ctx);
				prOut = net->getComtPrDown(ctx);
			} else {
				brIn  = net->getComtLeafBrUp(ctx);
				brOut = net->getComtLeafBrDown(ctx);
				prIn  = net->getComtLeafPrUp(ctx);
				prOut = net->getComtLeafPrDown(ctx);
			}
			reqCp.reset(MOD_COMTREE_LINK,REQUEST,0);
			reqCp.setAttr(COMTREE_NUM,comt);
			reqCp.setAttr(LINK_NUM,llnk);
			reqCp.setAttr(BIT_RATE_IN,brIn);
			reqCp.setAttr(BIT_RATE_OUT,brOut);
			reqCp.setAttr(PKT_RATE_IN,prIn);
			reqCp.setAttr(PKT_RATE_OUT,prOut);
			reply = sendCtlPkt(reqCp,rtrAdr,rtrIp,rtrPort,inQ,outQ);
			if (reply == 0) {
				cerr << "handleBootRequest: no reply from "
				     << "router " << rtr << " to modify "
				     << "comtree link message for comtree "
				     << comt << " link " << llnk << "\n";
				ps->free(reply);
				return false;
			}
			repCp.reset();
			repCp.unpack(ps->getPayload(reply),
				 	(ps->getHeader(reply)).getLength()
					 - Forest::OVERHEAD);
			if (repCp.getRrType() == NEG_REPLY) {
				CtlPkt acp(BOOT_ABORT,REQUEST,0);
				int aRep = sendCtlPkt(acp,rtrAdr,rtrIp,rtrPort,
							inQ,outQ);
				if (aRep != 0) ps->free(aRep); // ignore reply
				cerr << "handleBootRequest: router " << rtr
				     << " could not set comtree link rates for "
				     << "comtree " << comt << " link "
				     << llnk << "\n";
				ps->free(reply);
				return false;
			}
			ps->free(reply);
		}

		// finally, we need to modify overall comtree attributes
		reqCp.reset(MOD_COMTREE,REQUEST,0);
		reqCp.setAttr(COMTREE_NUM,comt);
		reqCp.setAttr(CORE_FLAG,net->isComtCoreNode(ctx,rtr));
		reqCp.setAttr(PARENT_LINK,net->getLocLink(plnk,rtr));
		reply = sendCtlPkt(reqCp,rtrAdr,rtrIp,rtrPort,inQ,outQ);
		if (reply == 0) {
			cerr << "handleBootRequest: no reply from router "
			     << rtr << " to modify comtree message for comtree "
			     << comt << "\n";
			ps->free(reply);
			return false;
		}
		repCp.reset();
		repCp.unpack(ps->getPayload(reply),
			 (ps->getHeader(reply)).getLength() - Forest::OVERHEAD);
		if (repCp.getRrType() == NEG_REPLY) {
			CtlPkt acp(BOOT_ABORT,REQUEST,0);
			int aRep = sendCtlPkt(acp,rtrAdr,rtrIp,rtrPort,
						inQ,outQ);
			if (aRep != 0) ps->free(aRep); // ignore reply
			cerr << "handleBootRequest: router " << rtr << " could "
				"not add comtree " << comt << endl;
			ps->free(reply);
			return false;
		}
		ps->free(reply);
	}
	// finally, send the boot complete message to the router
	CtlPkt reqCp(BOOT_COMPLETE,REQUEST,0);
	int reply = sendCtlPkt(reqCp,rtrAdr,rtrIp,rtrPort,inQ,outQ);
	if (reply == 0) {
		cerr << "handleBootRequest: no reply from router " << rtr <<
			" to boot complete message\n";
		ps->free(reply);
		return false;
	}
	repCp.reset();
	repCp.unpack(ps->getPayload(reply),
		 (ps->getHeader(reply)).getLength() - Forest::OVERHEAD);
	if (repCp.getRrType() == NEG_REPLY) {
		cerr << "handleBootRequest: router " << rtr <<
			" sent negative reply to boot complete message\n";
		ps->free(reply);
		return false;
	}
	ps->free(reply);
	return true;
}
	

/** Send a control packet back through the main thread.
 *  The control packet object is assumed to be already initialized.
 *  If it is a reply packet, it is packed into a packet object whose index is
 *  then placed in the outQ. If it is a request packet, it is sent similarly
 *  and then the method waits for a reply. If the reply is not received
 *  after one second, it tries again. After three attempts, it gives up.
 *  @param cp is the pre-formatted control packet
 *  @param dest is the destination address to which it is to be sent
 *  @param inQ is the thread's input queue from the main thread
 *  @param outQ is the thread's output queue back to the main thread
 *  @return the packet number of the reply, when there is one, and 0
 *  on on error, or if there is no reply.
 */
int sendCtlPkt(CtlPkt& cp, fAdr_t dest, ipa_t destIp, ipp_t destPort,
		Queue& inQ, Queue& outQ) {
	int p = ps->alloc();
	if (p == 0) {
		cerr << "sendCtlPkt: no packets left in packet store\n";
		return 0;
	}
	int plen = cp.pack(ps->getPayload(p));
	if (plen == 0) {
		cerr << "sendCtlPkt: packing error for packet:\n";
		cp.write(cerr);
		ps->free(p);
		return 0;
	}
	PacketHeader& h = ps->getHeader(p);
	h.setLength(plen + Forest::OVERHEAD);
	h.setPtype(NET_SIG);
	h.setFlags(0);
	h.setComtree(Forest::NET_SIG_COMT);
	h.setDstAdr(dest); h.setSrcAdr(myAdr);
	h.setTunSrcIp(destIp); h.setTunSrcPort(destPort);
	h.pack(ps->getBuffer(p));

	if (cp.getRrType() != REQUEST) {
		outQ.enq(p); return 0;
	}
	int reply = sendAndWait(p,cp,inQ,outQ);
	ps->free(p);
	return reply;
}

int sendCtlPkt(CtlPkt& cp, fAdr_t dest, Queue& inQ, Queue& outQ) {
	return sendCtlPkt(cp, dest, 0, 0, inQ, outQ);
}


/** Send a control request packet multiple times before giving up.
 *  This method makes a copy of the original and sends the copy
 *  back to the main thread. If no reply is received after 1 second,
 *  it tries again; it makes a total of three attempts before giving up.
 *  @param p is the packet number of the packet to be sent; the header
 *  for p is assumed to be unpacked
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return the packet number for the reply packet, or 0 if there
 *  was an error or no reply
 */
int sendAndWait(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
	PacketHeader& h = ps->getHeader(p);
	h.setSrcAdr(myAdr); ps->pack(p);

	// make copy of packet and send the copy
	int copy = ps->fullCopy(p);
	if (copy == 0) {
		cerr << "sendAndWait: no packets left in packet store\n";
		return 0;
	}
	// copy the tunnel ip and port numbers also
	PacketHeader& hc = ps->getHeader(copy);
	hc.setTunSrcIp(h.getTunSrcIp());
	hc.setTunSrcPort(h.getTunSrcPort());

//cerr << "sendAndWait: sending request for thread " << pthread_self() << "\n";
//h.write(cerr,ps->getBuffer(p));
	outQ.enq(copy);

	for (int i = 1; i < 3; i++) {
		int reply = inQ.deq(1000000000); // 1 sec timeout
		if (reply == 0) { // timeout
//cerr << "sendAndWait: timeout\n";
			// no reply, make new copy and send
			int retry = ps->fullCopy(p);
			if (retry == 0) {
				cerr << "sendAndWait: no packets "
					"left in packet store\n";
				return 0;
			}
			outQ.enq(retry);
		} else {
//cerr << "sendAndWait: got reply\n";
			return reply;
		}
	}
	return 0;
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
//cerr << "connection socket established, connSock=" << connSock << endl;
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
	if (connSock >= 0) {
		buffer_t& buf = ps->getBuffer(p);
		int length = ps->getHeader(p).getLength();
		ps->pack(p);
		Np4d::sendBuf(connSock, (char *) &buf, length);
	}
	ps->free(p);
}

/** Check for next packet from the Forest network.
 *  @return next packet or 0, if no report has been received.
 */
int rcvFromForest() { 
	int p = ps->alloc();
	if (p == 0) return 0;
        buffer_t& b = ps->getBuffer(p);
	ipa_t srcIp; ipp_t srcPort;
        int nbytes = Np4d::recvfrom4d(intSock,&b[0],1500,srcIp,srcPort);
        if (nbytes < 0) { ps->free(p); return 0; }
	ps->unpack(p);
	PacketHeader& h = ps->getHeader(p);
	h.setTunSrcIp(srcIp); h.setTunSrcPort(srcPort);
        return p;
}

/** Send packet to Forest router.
 *  During the initialization phase, we must send directly to the target router.
 *  The handleBootRequest handler inserts the router's IP address and port in
 *  the tunSrcIp and tunSrcPort fields of all packets it sends.
 */
void sendToForest(int p) {
	buffer_t& buf = ps->getBuffer(p);
	PacketHeader h = ps->getHeader(p);
	int leng = h.getLength();
	ps->pack(p);
	ipa_t destIp; ipp_t destPort;
	if (booting) { destIp = h.getTunSrcIp(); destPort = h.getTunSrcPort(); }
	else	     { destIp = rtrIp; destPort = Forest::ROUTER_PORT; }
	int rv = Np4d::sendto4d(intSock,(void *) &buf,leng, destIp, destPort);
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
