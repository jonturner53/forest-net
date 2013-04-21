/** @file NetMgr.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "NetMgr.h"
#include "CpHandler.h"

/** usage:
 *       NetMgr extIp topoFile clientInfo finTime
 * 
 *  Command line arguments include two ip addresses for the
 *  NetMgr. The first is the IP address that a remote UI can
 *  use to connect to the NetMgr. If this is specified as 127.0.0.1
 *  the NetMgr listens on the default IP address. The second is the
 *  topology file (aka NetInfo file) that describes the network
 *  plus any pre-configured comtrees that may be required.
 *  ClientInfo is a file containing address information relating
 *  clients and routers. finTime is the number of seconds to run.
 *  If zero, the NetMgr runs forever.
 */
int main(int argc, char *argv[]) {
	uint32_t finTime;

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
	reqMap = new IdMap(TPSIZE);
	tMap = new IdMap(TPSIZE);

	booting = true;

	// read NetInfo data structure from file
	int maxNode = 100000; int maxLink = 10000;
	int maxRtr = 5000; int maxCtl = 200;
	int maxComtree = 10000;
	net = new NetInfo(maxNode, maxLink, maxRtr, maxCtl);
	comtrees = new ComtInfo(maxComtree,*net);
	ifstream fs; fs.open(topoFile);
	if (fs.fail() || !net->read(fs) || !comtrees->read(fs)) {
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
			int llnk = net->getLLnum(lnk,rtr);
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

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,4*PTHREAD_STACK_MIN);
	size_t stacksize;
	pthread_attr_getstacksize(&attr,&stacksize);
	cerr << "min stack size=" << 4*PTHREAD_STACK_MIN << endl;
	cerr << "threads in pool have stacksize=" << stacksize << endl;
	if (stacksize != 4*PTHREAD_STACK_MIN)
		fatal("init: can't set stack size");

	// setup thread pool for handling control packets
	for (int t = 1; t <= TPSIZE; t++) {
		if (!pool[t].qp.in.init() || !pool[t].qp.out.init())
			fatal("init: can't initialize thread queues\n");
		if (pthread_create(&pool[t].thid,&attr,handler,
				   (void *) &pool[t].qp) != 0)  {
                	fatal("init: can't create thread pool");
		}
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
	delete ps; delete [] pool; delete reqMap; delete tMap; delete threads;
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
		nothing2do = true;

		// check for packets
		pktx px = recvFromCons();
		if (px != 0) {
			// let handler know this is from remote console
			ps->getPacket(px).srcAdr = 0;
		} else {
			px = rcvFromForest();
		}
		if (px != 0) {
			// send pto a thread, possibly assigning one
			Packet& p = ps->getPacket(px);
			if (p.type == NET_SIG) {
				CtlPkt cp(p.payload(),
					  p.length-Forest::OVERHEAD);
				cp.unpack();
				int t;
				fAdr_t srcAdr = p.srcAdr;
				if (cp.mode == CtlPkt::REQUEST) {
					// first make sure this is not a repeat
					// of a request we're already working on
					t = threads->firstOut();
					uint64_t key = p.srcAdr;
					key <<= 32; key += cp.seqNum;
					if (reqMap->validKey(key)) {
						// in this case, we've got an
						// active thread handling this
						// request, so discard duplicate
						ps->free(px);
					} else if (t != 0) {
						threads->swap(t);
						reqMap->addPair(key,t);
						pool[t].seqNum = 0;
						pool[t].qp.in.enq(px);
					} else {
						cerr << "run: thread pool is "
							"exhausted\n";
						ps->free(px);
					}
				} else if (booting &&
					    cp.type == CtlPkt::BOOT_COMPLETE &&
					    cp.mode == CtlPkt::POS_REPLY) {
					doneBooting.insert(srcAdr);
					t = tMap->getId(cp.seqNum);
					if (t != 0) {
						tMap->dropPair(cp.seqNum);
						pool[t].seqNum = 0;
						pool[t].qp.in.enq(px);
					} else {
						ps->free(px);
					}
					if ((int) doneBooting.size() ==
					     numRouters) {
						booting = false;
						string s;
						cout << "done booting at " 
						<< Misc::nstime2string( now,s)
						<< endl;
						connect();
						usleep(1000000);
						// sleep for SPP context
						// allows time for NAT
					}
				} else if (booting &&
					    cp.type == CtlPkt::BOOT_COMPLETE &&
					    cp.mode == CtlPkt::NEG_REPLY) {
					string s;
					cerr << "router at address "
					     << Forest::fAdr2string(srcAdr,s)
					     << " failed to boot\n";
					pthread_exit(NULL);
				} else {
					// normal case of a reply
					t = tMap->getId(cp.seqNum);
					if (t != 0) {
						tMap->dropPair(cp.seqNum);
						pool[t].seqNum = 0;
						pool[t].qp.in.enq(px);
					} else {
						ps->free(px);
					}
				}
			} else {
				ps->free(px);
			}
			nothing2do = false;
		}

		// now handle packets from the thread pool
		for (int t = threads->firstIn(); t != 0;
			 t = threads->nextIn(t)) {
			if (pool[t].qp.out.empty()) continue;
			pktx px1 = pool[t].qp.out.deq();
			if (px1 == 0) { // means thread is done
				pool[t].qp.in.reset();
				reqMap->dropPair(reqMap->getKey(t));
				threads->swap(t);
				continue;
			}
			nothing2do = false;
			Packet& p1 = ps->getPacket(px1);
			CtlPkt cp1(p1.payload(),
				   p1.length-Forest::OVERHEAD);
			cp1.unpack();
			if (p1.dstAdr == 0) {
				sendToCons(px1);
			} else if (cp1.mode == CtlPkt::REQUEST) {
				// this is to catch race condition that can
				// trigger spurious CtlPkt::BOOT_COMPLETE
				if (cp1.type == CtlPkt::BOOT_COMPLETE &&
				    !booting) {
					ps->free(px1); continue;
				}
				if (cp1.seqNum == 1) {
					// means this is a repeat of a pending
					// outgoing request
					if (tMap->validId(t)) {
						cp1.seqNum = tMap->getKey(t);
					} else {
						// in this case, reply has 
						// arrived but was not yet seen
						// by thread so, suppress 
						// duplicate request
						ps->free(px1); continue;
					}
				} else {
					if (tMap->validId(t)) 
						tMap->dropPair(tMap->getKey(t));
					tMap->addPair(seqNum,t);
					cp1.seqNum = seqNum++;
				}
				cp1.pack();
				p1.payErrUpdate();
				pool[t].seqNum = cp1.seqNum;
				pool[t].ts = now + 2000000000; // 2 sec timeout
				sendToForest(px1);
			} else {
				sendToForest(px1);
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
		if (nothing2do && threads->firstIn() == 0) usleep(10000);
		sched_yield();
		now = Misc::getTimeNs();
	}
	disconnect();
	pthread_exit(NULL);
}

/** Main handler for thread.
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
	Queue& inq = ((QueuePair *) qp)->in;
	Queue& outq = ((QueuePair *) qp)->out;
	CpHandler cph(&inq, &outq, myAdr, &logger, ps);

	while (true) {
		pktx px = inq.deq();
		Packet& p = ps->getPacket(px);
		CtlPkt cp(p.payload(),p.length-Forest::OVERHEAD);
		cp.unpack();
		bool success = false;
		if (p.srcAdr == 0) {
			success = handleConsReq(px, cp, cph);
		} else {
			switch (cp.type) {
			case CtlPkt::CLIENT_CONNECT:
			case CtlPkt::CLIENT_DISCONNECT:
				success = handleConDisc(px,cp,cph);
				break;
			case CtlPkt::NEW_CLIENT:
				success = handleNewClient(px,cp,cph);
				break;
			case CtlPkt::BOOT_REQUEST:
				cph.setTunnel(p.tunIp,p.tunPort);
				success = handleBootRequest(px,cp,cph);
				cph.setTunnel(0,0);
				break;
			default:
				cph.errReply(px,cp,"invalid control packet "
						   "type for NetMgr");
				break;
			}
		}
		if (!success) {
			string s;
			cerr << "handler: operation failed\n"
			     << p.toString(s);
		}
		ps->free(px); // release p now that we're done
		outq.enq(0); // signal completion to main thread
	}
}

/** Handle a request packet from the remote console.
 *  This involves forwarding the packet to a remote router.
 *  @param px is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param return true on successful completion, else false
 */
bool handleConsReq(pktx px, CtlPkt& cp, CpHandler& cph) {
	
	// rework this using TCP connection and new text protocol
	return false;
}

/** Handle a connection/disconnection notification from a router.
 *  The request is acknowledged and then forwarded to the
 *  client manager.
 *  @param px is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param cph is the control packet handler for this thread
 *  @return true if the operation was completed successfully,
 *  otherwise false; an error reply is considered a successful
 *  completion; the operation can fail if it cannot allocate
 *  packets, or if the client manager never acknowledges the
 *  notification.
 */
bool handleConDisc(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);

	// send positive reply back to router
	CtlPkt repCp(cp.type, CtlPkt::POS_REPLY, cp.seqNum); 
	cph.sendCtlPkt(repCp,p.srcAdr);

	// now, send notification to client manager
	pktx reply;
	if (cp.type == CtlPkt::CLIENT_CONNECT)
		reply = cph.clientConnect(cliMgrAdr,cp.adr1,p.srcAdr);
	else
		reply = cph.clientDisconnect(cliMgrAdr,cp.adr1,p.srcAdr);
	if (reply == 0) return false;
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
 *  @param px is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param cph is the control packet handler for this thread
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success; the operation
 *  fails if it cannot allocate required packets, or if the
 *  router fails to respond to either of the control messages
 *  sent to it.
 */
bool handleNewClient(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	// determine which router to use
	fAdr_t rtrAdr;
	if (!findCliRtr(cp.ip1,rtrAdr)) {
		cph.errReply(px,cp,"No router assigned to client's IP");
		return true;
	}

	pktx reply = cph.addLink(rtrAdr,CLIENT,cp.ip1,cp.port1,0,0,0);
	if (reply == 0) {
		cph.errReply(px,cp,"router did not reply to add link");
		return false;
	}
	CtlPkt repCp; 
	if (!cph.getCp(reply,repCp)) {
		cph.errReply(px,cp,"router failed to allocate link");
		ps->free(reply);
		return false;
	}
	int clientLink = repCp.link;
	fAdr_t clientAdr = repCp.adr1;
	ipa_t clientRtrIp = repCp.ip1;
	ps->free(reply);

	reply = cph.modLink(rtrAdr,clientLink,net->getDefLeafRates());
	if (reply == 0) {
		cph.errReply(px,cp,"no reply from router to modify link");
		return false;
	}
	if (!cph.getCp(reply,repCp)) {
		cph.errReply(px,cp,"router could not set link rates");
		ps->free(reply);
		return false;
	}
	ps->free(reply);

	// now add the new client to the client connection comtree
	reply = cph.addComtreeLink(rtrAdr,Forest::CLIENT_CON_COMT,
				   clientLink,-1);
	if (reply == 0) {
		cph.errReply(px,cp,"no reply from router to add comtree link");
		return false;
	}
	if (!cph.getCp(reply,repCp)) {
		cph.errReply(px,cp,"router could not add client to connection "
				    "comtree");
		ps->free(reply);
		return false;
	}
	ps->free(reply);

	// Now modify comtree link rate
	int ctx = comtrees->getComtIndex(Forest::CLIENT_CON_COMT);
	reply = cph.modComtreeLink(rtrAdr,Forest::CLIENT_CON_COMT,clientLink,
				   comtrees->getDefLeafRates(ctx));
	if (reply == 0) { ps->free(reply); return false; }
	ps->free(reply);

	// now add the new client to the client signaling comtree
	reply = cph.addComtreeLink(rtrAdr,Forest::CLIENT_SIG_COMT,
				   clientLink,-1);
	if (reply == 0) {
		cph.errReply(px,cp,"no reply from router to add comtree link");
		return false;
	}
	if (!cph.getCp(reply,repCp)) {
		cph.errReply(px,cp,"router could not add client to signalling "
				   "comtree");
		ps->free(reply);
		return false;
	}
	ps->free(reply);

	// Now modify comtree link rate
	ctx = comtrees->getComtIndex(Forest::CLIENT_SIG_COMT);
	reply = cph.modComtreeLink(rtrAdr,Forest::CLIENT_SIG_COMT,clientLink,
				   comtrees->getDefLeafRates(ctx));
	if (reply == 0) { ps->free(reply); return false; }
	ps->free(reply);

	// send final reply back to client manager
	repCp.reset(CtlPkt::NEW_CLIENT,CtlPkt::POS_REPLY,cp.seqNum);
        repCp.adr1 = clientAdr; repCp.ip1 = clientRtrIp; repCp.adr2 = rtrAdr;
	cph.sendCtlPkt(repCp,p.srcAdr);
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
 *  @param px is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param cph is the control packet handler for this thread
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success; the operation
 *  fails if it cannot allocate required packets, or if the
 *  router fails to respond to either of the control messages
 *  sent to it.
 */
bool handleBootRequest(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	fAdr_t rtrAdr = p.srcAdr;
	int rtr;
	for (rtr = net->firstRouter(); rtr != 0; rtr = net->nextRouter(rtr)) {
		if (net->getNodeAdr(rtr) == rtrAdr) break;
	}
	if (rtr == 0) {
		cph.errReply(px,cp,"boot request from unknown router "
				   "rejected\n");
		logger.log("handleBootRequest: received boot request from "
			   "unknown router\n",2,p);
		return true;
	}
	// first send reply, acknowledging request and supplying
	// leaf address range
	CtlPkt repCp(CtlPkt::BOOT_REQUEST,CtlPkt::POS_REPLY,cp.seqNum);
	pair<fAdr_t,fAdr_t> leafRange; net->getLeafRange(rtr,leafRange);
        repCp.adr1 = leafRange.first;
        repCp.adr2 = leafRange.second;
	cph.sendCtlPkt(repCp,rtrAdr);

	// add/configure interfaces
	// for each interface in table, do an add iface
	for (int i = 1; i <= net->getNumIf(rtr); i++) {
		if (!net->validIf(rtr,i)) continue;
		int reply = cph.addIface(rtrAdr,i,net->getIfIpAdr(rtr,i),
				         net->getIfRates(rtr,i));
		if (reply == 0) return false;
		if (!cph.getCp(reply,repCp)) {
			cph.bootAbort(rtrAdr);
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
		int llnk = net->getLLnum(lnk,rtr);
		int iface = net->getIface(llnk,rtr);
		int peer = net->getPeer(rtr,lnk);
		ntyp_t peerType = net->getNodeType(peer);
		int plnk = net->getLLnum(lnk,peer);
		ipa_t peerIp; ipp_t peerPort;
		if (peerType == ROUTER) {
			int i = net->getIface(plnk,peer);
			peerIp = net->getIfIpAdr(peer,i);
			peerPort = Forest::ROUTER_PORT;
		} else {
			peerIp = net->getLeafIpAdr(peer);
			peerPort = 0;
		}

		// first, add the link
		int reply = cph.addLink(rtrAdr,peerType,peerIp,peerPort,
					iface,llnk,net->getNodeAdr(peer));
		if (reply == 0) return false;
		if (!cph.getCp(reply,repCp)) {
			cph.bootAbort(rtrAdr);
			ps->free(reply);
			return false;
		}
		ps->free(reply);

		// now, send modify link message, to set data rates
		reply = cph.modLink(rtrAdr,llnk,net->getLinkRates(lnk));
		if (reply == 0) return false;
		if (!cph.getCp(reply,repCp)) {
			cph.bootAbort(rtrAdr);
			ps->free(reply);
			return false;
		}
		ps->free(reply);
	}
	
	// add/configure comtrees
	// for each comtree, add it to the router, do a modify comtree
	// and a series of add comtree link ops, followed by modify comtree link
	for (int ctx = comtrees->firstComtIndex(); ctx != 0;
		 ctx = comtrees->nextComtIndex(ctx)) {
		if (!comtrees->isComtNode(ctx,rtrAdr)) continue;
		
		comt_t comt = comtrees->getComtree(ctx);

		// first step is to add comtree at router
		int reply = cph.addComtree(rtrAdr,comt);
		if (reply == 0) return false;
		if (!cph.getCp(reply,repCp)) {
			cph.bootAbort(rtrAdr);
			ps->free(reply); return false;
		}
		ps->free(reply);

		// next, add links to the comtree and set their data rates
		int plnk = comtrees->getPlink(ctx,rtrAdr);
		int parent = net->getPeer(rtr,plnk);
		for (int lnk = net->firstLinkAt(rtr); lnk != 0;
			 lnk = net->nextLinkAt(rtr,lnk)) {
			if (!comtrees->isComtLink(ctx,lnk)) continue;

			// get information about this link
			int llnk = net->getLLnum(lnk,rtr);
			int peer = net->getPeer(rtr,lnk);
			fAdr_t peerAdr = net->getNodeAdr(peer);
			bool peerCoreFlag = comtrees->isCoreNode(ctx,peerAdr);

			// first, add comtree link
			reply = cph.addComtreeLink(rtrAdr,comt,llnk,
						   peerCoreFlag);
			if (reply == 0) return false;
			if (!cph.getCp(reply,repCp)) {
				cph.bootAbort(rtrAdr);
				ps->free(reply); return false;
			}
			ps->free(reply);

			// now, set link rates
			RateSpec rs = comtrees->getLinkRates(ctx,rtrAdr);
			if (peer == parent) rs.flip();
			reply = cph.modComtreeLink(rtrAdr,comt,llnk,rs);
			if (reply == 0) return false;
			if (!cph.getCp(reply,repCp)) {
				cph.bootAbort(rtrAdr);
				ps->free(reply); return false;
			}
			ps->free(reply);
		}
		// finally, we need to modify overall comtree attributes
		reply = cph.modComtree(rtrAdr,comt,net->getLLnum(plnk,rtr),
					comtrees->isCoreNode(ctx,rtrAdr));
		if (reply == 0) return false;
		if (!cph.getCp(reply,repCp)) {
			cph.bootAbort(rtrAdr);
			ps->free(reply); return false;
		}
		ps->free(reply);
	}
	// finally, send the boot complete message to the router
	int reply = cph.bootComplete(rtrAdr);
	if (reply == 0) return false;
	if (!cph.getCp(reply,repCp)) {
		cph.bootAbort(rtrAdr);
		ps->free(reply); return false;
	}
	ps->free(reply);

	logger.log("completed boot request",0,p);
	return true;
}
	
/** Finds the router address of the router based on IP prefix
 *  @param cliIp is the ip of the client
 *  @param rtrAdr is the address of the router associated with this IP prefix
 *  @return true if there was an IP prefix found, false otherwise
 */
bool findCliRtr(ipa_t cliIp, fAdr_t& rtrAdr) {
	string cip;
	Np4d::ip2string(cliIp,cip);
	for (unsigned int i = 0; i < (unsigned int) numPrefixes; ++i) {
		string ip = prefixes[i].prefix;
		unsigned int j = 0;
		while (j < ip.size() && j < cip.size()) {
			if (ip[j] != '*' && cip[j] != ip[j]) break;
			if (ip[j] == '*' ||
			    (j+1 == ip.size() && j+1 == cip.size())) {
				rtrAdr = prefixes[i].rtrAdr;
				return true;
			}
			j++;
		}
	}
	// if no prefix for client address, select router at random
	int i = randint(0,net->getNumRouters()-1);
	for (int r = net->firstRouter(); r != 0; r = net->nextRouter(r)) {
		if (i-- == 0) {
			rtrAdr = net->getNodeAdr(r); return true;
		}
	}
	return false; // should never reach here
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
	}

	int px = ps->alloc();
	if (px == 0) return 0;
	Packet& p = ps->getPacket(px);
	
	int nbytes = Np4d::recvBuf(connSock, (char *) p.buffer,Forest::BUF_SIZ);
	if (nbytes == -1) { ps->free(px); return 0; }
	if (nbytes < Forest::HDR_LENG)
		fatal(" recvFromCons: misformatted packet from console");
	p.unpack();
	if (p.version != 1 || p.length != nbytes ||
	    (p.type != CLIENT_SIG && p.type != NET_SIG))
		fatal(" recvFromCons: misformatted packet from console");
        return px;
}

/** Write a packet to the socket for the user interface.
 *  @param px is the packet to be sent to the CLI
 */
void sendToCons(pktx px) {
	if (connSock >= 0) {
		Packet& p = ps->getPacket(px);
		p.pack();
		Np4d::sendBuf(connSock, (char *) p.buffer, p.length);
	}
	ps->free(px);
}

/** Check for next packet from the Forest network.
 *  @return next packet or 0, if no report has been received.
 */
pktx rcvFromForest() { 
	pktx px = ps->alloc();
	if (px == 0) return 0;
	Packet& p = ps->getPacket(px);
	ipa_t srcIp; ipp_t srcPort;
        int nbytes = Np4d::recvfrom4d(intSock,p.buffer,1500,srcIp,srcPort);
        if (nbytes < 0) { ps->free(px); return 0; }
	p.unpack();
	p.tunIp = srcIp; p.tunPort = srcPort;
        return px;
}

/** Send packet to Forest router.
 *  During the initialization phase, we must send directly to the target router.
 *  The handleBootRequest handler inserts the router's IP address and port in
 *  the tunIp and tunPort fields of all packets it sends.
 */
void sendToForest(int px) {
	Packet p = ps->getPacket(px);
	p.pack();
	ipa_t destIp; ipp_t destPort;
	if (booting) { destIp = p.tunIp; destPort = p.tunPort; }
	else	     { destIp = rtrIp; destPort = Forest::ROUTER_PORT; }
	int rv = Np4d::sendto4d(intSock,(void *) p.buffer,p.length,
				destIp, destPort);
	if (rv == -1) {
		fatal("sendToForest: failure in sendto");
	}
	ps->free(px);
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
void connect() {
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);

	p.length = Forest::OVERHEAD; p.type = CONNECT; p.flags = 0;
	p.comtree = Forest::CLIENT_CON_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	sendToForest(px);
}

/** Send final disconnect packet to forest router.
 */
void disconnect() {
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);

	p.length = Forest::OVERHEAD; p.type = DISCONNECT; p.flags = 0;
	p.comtree = Forest::CLIENT_CON_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	sendToForest(px);
}
