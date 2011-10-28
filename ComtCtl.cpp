/** @file ComtCtl.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ComtCtl.h"

/** usage:
 *       ComtCtl intIp extIp topoFile firstComt lastComt finTime
 * 
 *  Command line arguments include two IP addresses. The first is
 *  an internal IP addresses used for communication with an access
 *  router. The second is an external IP address that can
 *  be used by a remote display program to retrieve information
 *  about comtrees. The next argument is the name of a topololgy file
 *  (aka NetInfo file) that describes the network and any
 *  pre-configured comtrees. The next two arguments identify
 *  the first and last comtree controlled by this comtree controller.
 *  FinTime is the number of seconds to run.
 *  If zero, ComtCtl runs forever.
 */
main(int argc, char *argv[]) {
	int comt; uint32_t finTime;

	if (argc != 7 ||
  	    (intIp  = Np4d::ipAddress(argv[1])) == 0 ||
  	    (extIp  = Np4d::ipAddress(argv[2])) == 0 ||
	    (sscanf(argv[4],"%d", &firstComt)) != 1 ||
	    (sscanf(argv[5],"%d", &lastComt)) != 1 ||
	     sscanf(argv[6],"%d", &finTime) != 1)
		fatal("usage: ComtCtl intIp extIp topoFile clientInfo finTime");
	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress(); 
	if (extIp == 0) fatal("can't retrieve default IP address");

	if (!init(argv[3])) {
		fatal("ComtCtl: initialization failure");
	}
	pthread_t run_thread;
	if (pthread_create(&run_thread,NULL,run,(void *) &finTime) != 0) {
		fatal("can't create run thread\n");
	}
	pthread_join(run_thread,NULL);
	cleanup();
	exit(0);
}


/** Initialization of ComtCtl.
 *  Involves setting up thread pool to handle client signalling packets
 *  and setting up sockets.
 *  @return true on success, false on failure
 */
bool init(const char *topoFile) {
	int nPkts = 10000;
	ps = new PacketStoreTs(nPkts+1);

	pool = new ThreadInfo[TPSIZE+1];
	threads = new UiSetPair(TPSIZE);
	tMap = new IdMap(TPSIZE);
	if (firstComt < 1 || lastComt < 1 || firstComt > lastComt) {
		cerr << "init: invalid comtree range\n";
		return false;
	}

	// read NetInfo data structure from file
	int maxNode = 5000; int maxLink = 10000;
	int maxRtr = 4500; int maxCtl = 400;
	maxComtree = 100000; // declared in header file
	net = new NetInfo(maxNode, maxLink, maxRtr, maxCtl, maxComtree);
	ifstream fs; fs.open(topoFile);
	if (fs.fail() || !net->read(fs)) {
		cerr << "ComtCtl::init: could not read topology file, or error "
		      	"in topology file";
		return false;
	}
	fs.close();

	comtSet = new UiSetPair((lastComt-firstComt)+1);
	// Mark all pre-configured comtrees in the range this controller
	// is responsible for as in-use".
	for (int ctx = net->firstComtIndex(); ctx != 0;
		 ctx = net->nextComtIndex(ctx)) {
		int comt = net->getComtree(ctx);
		if (firstComt <= comt && comt <= lastComt) {
			comtSet->swap((comt-firstComt)+1);
		}
	}

	// find node information for comtree controller in topo file
	myAdr = 0;
	for (int c = net->firstController(); c != 0;
		 c = net->nextController(c)) {
		string s;
		if (net->getNodeName(c,s).compare("comtCtl") == 0) {
			intIp = net->getLeafIpAdr(c);
			myAdr = net->getNodeAdr(c);
			int lnk = net->firstLinkAt(c);
			int rtr = net->getPeer(c,lnk);
			int llnk = net->getLocLink(lnk,rtr);
			int iface = net->getIface(llnk,rtr);
			if (iface == 0) {
                                cerr << "ComtCtl:init: comtCtl access link not "
			                "bound to any interface at my router\n";
                        }
			rtrIp = net->getIfIpAdr(rtr,iface);
			rtrAdr = net->getNodeAdr(rtr);
		}
	}
	if (myAdr == 0) {
		cerr << "could not find comtCtl in topology file\n";
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

	// initialize locks used to control access to comtrees
	// and available link rates
	if (pthread_mutex_init(&allComtLock,NULL) != 0) {
		cerr << "init: could not initialize all comtree lock\n";
		return false;
	}
	if (pthread_mutex_init(&rateLock,NULL) != 0) {
		cerr << "init: could not initialize rate lock\n";
		return false;
	}
	comtLock = new pthread_mutex_t[maxComtree+1];
	for (int i = 1; i <= maxComtree; i++) {
		if (pthread_mutex_init(&comtLock[i],NULL) != 0) {
			cerr << "init: could not initialize per comtree lock\n";
			return false;
		}
	}
	
	// setup sockets
	intSock = Np4d::datagramSocket();
	if (intSock < 0 || 
	    !Np4d::bind4d(intSock,intIp,0) ||
	    !Np4d::nonblock(intSock))
		return false;

	// setup external TCP socket for use by remote display program
	extSock = Np4d::streamSocket();
	if (extSock < 0) return false;
	return	Np4d::bind4d(extSock,extIp,Forest::NM_PORT) &&
		Np4d::listen4d(extSock) && 
		Np4d::nonblock(extSock);
}

void cleanup() {
	cout.flush(); cerr.flush();
	if (extSock > 0) close(extSock);
	if (intSock > 0) close(intSock);
	delete ps; delete [] pool; delete tMap; delete threads;

	pthread_mutex_destroy(&allComtLock);
	pthread_mutex_destroy(&rateLock);
	for (int i = 1; i <= maxComtree; i++) {
		pthread_mutex_destroy(&comtLock[i]);
	}
	delete net;
	delete [] comtLock;
}

/** Run the ComtCtl.
 *  @param finTime is the number of seconds to run before halting;
 *  if zero, run forever
 */
void* run(void* finTimeSec) {
	uint64_t seqNum = 1;
	uint64_t now = Misc::getTimeNs();
	uint64_t finishTime = *((uint32_t *) finTimeSec);
	finishTime *= 1000000000; // convert to ns

	connect();
	usleep(1000000);	// sleep needed when running on SPP nodes

	bool nothing2do;
	while (finishTime == 0 || now <= finishTime) {
		bool nothing2do = true;

		// check for packets
		int p = 0; int connSock = -1;
		if ((connSock = Np4d::accept4d(extSock)) > 0) {
			// let handler know this is socket# for remote host
			int t = threads->firstOut();
			if (t != 0) {
				threads->swap(t);
				pool[t].seqNum = 0;
				pool[t].qp.in.enq(-connSock);
			} else
				cerr << "run: thread pool is exhausted\n";
		} else {
			p = rcvFromForest();
		}
		if (p != 0) {
			// send p to a thread, possibly assigning one
			PacketHeader& h = ps->getHeader(p);
			if (h.getPtype() == CLIENT_SIG || 
			    h.getPtype() == NET_SIG) {
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
			if (cp1.getRrType() == REQUEST) {
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
 *  read/write it without locking. The handler is required to free any
 *  packets that it no longer needs to the packet store.
 *  When the handler has completed the requested operation, it sends
 *  a 0 value back to the main thread, signalling that it is available
 *  to handle another task.
 *  If the main thread passes a negative number through the queue,
 *  it is interpreted as a negated socket number, for an accepted
 *  stream socket to a remote display application.
 *  @param qp is a pair of queues; one is the input queue to the
 *  handler, the other is its output queue
 */
void* handler(void *qp) {
	Queue& inQ  = ((QueuePair *) qp)->in;
	Queue& outQ = ((QueuePair *) qp)->out;

	while (true) {
		int p = inQ.deq();
		bool success;
		if (p < 0) {
			// then p is really a negated socket number
			// for a remote comtree display
			// add handler for this later
		} else {
			PacketHeader& h = ps->getHeader(p);
			CtlPkt cp; 
			cp.unpack(ps->getPayload(p),
				  h.getLength()-Forest::OVERHEAD);
			switch (cp.getCpType()) {
			case CLIENT_ADD_COMTREE:
				success = handleAddComtReq(p,cp,inQ,outQ);
				break;
			case CLIENT_DROP_COMTREE:
				success = handleDropComtReq(p,cp,inQ,outQ);
				break;
			case CLIENT_JOIN_COMTREE:
				success = handleJoinComtReq(p,cp,inQ,outQ);
				break;
			case CLIENT_LEAVE_COMTREE:
				success = handleLeaveComtReq(p,cp,inQ,outQ);
				break;
			default:
				errReply(p,cp,outQ,"invalid control packet "
						   "type for ComtCtl");
				break;
			}
			if (!success) {
				cerr << "handler: operation failed\n";
				h.write(cerr,ps->getBuffer(p));
			}
		}
		ps->free(p); // release p now that we're done
		outQ.enq(0); // signal completion to main thread
	}
}

/** Handle an add comtree request.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true if the operation was completed successfully,
 *  otherwise false; an error reply is considered a successful
 *  completion; the operation can fail if it cannot allocate
 *  packets, or if one of the routers that must be configured
 *  fails to respond.
 */
bool handleAddComtReq(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
// Hmmm. This method can produce orphan comtrees.
// If a comtree is added, but the reply to the user is lost,
// the user will try again, leading to an additional comtree
// for that user. Could alter the protocol to deal with this.
// We reply immediately to handleAddComtReq, but send completion
// message when done, for which we require an ack. If we don't
// get it, we tear down. Ugly. Alternatively, request could
// contain a comtree id, which we store with the comtree and
// if a specific user requests two comtrees with the same id,
// we assume they are the same.
	PacketHeader& h = ps->getHeader(p);
	if (!cp.isSet(ROOT_ZIP)) {
		errReply(p,cp,outQ,"missing required attribute");
		return true;
	}
	int rootZip = cp.getAttr(ROOT_ZIP);

	pthread_mutex_lock(&allComtLock);
	int comt = comtSet->firstOut();
	if (comt == 0) {
		pthread_mutex_unlock(&allComtLock);
		errReply(p,cp,outQ,"no comtrees available to satisfy request");
		return true;
	}
	comtSet->swap(comt);
	comt += (firstComt - 1); // convert comtSet index to actual comtree #
	if (!net->addComtree(comt)) {
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		errReply(p,cp,outQ,"internal error prevents adding new "
				   "comtree");
		cerr << "handleAddComt: addComtree() failed due to program "
			"error\n";
		return false;
	}
	int ctx = net->lookupComtree(comt);
	pthread_mutex_lock(&comtLock[ctx]);
	pthread_mutex_unlock(&allComtLock);

	// find router in specified zipCode
	// if more than one, choose randomly (for now)
	// ultimately, we may want to select the one with the
	// most available capacity on its router-to-router links
	vector<int> matches(100,0); int cnt = 0;
	for (int rtr = net->firstRouter(); rtr != 0;
		 rtr = net->nextRouter(rtr)) {
		if (Forest::zipCode(net->getNodeAdr(rtr)) == rootZip) {
			matches[cnt++] = rtr;
		}
	}
	if (cnt == 0) {
		pthread_mutex_lock(&allComtLock);
		net->removeComtree(ctx);
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"network contains no router with specified "
				   "zip code");
		return true;
	}
	int rootRtr = matches[randint(0,cnt-1)];
	fAdr_t rootAdr = net->getNodeAdr(rootRtr);
	
	// configure root router to add comtree
	CtlPkt reqCp(ADD_COMTREE, REQUEST, 0);
	reqCp.setAttr(COMTREE_NUM,comt);
	int reply = sendCtlPkt(reqCp,rootAdr,inQ,outQ);
	CtlPkt repCp; string s1;
	string noRstr = "handleAddComt: add comtree request to "
			+ net->getNodeName(rootRtr,s1);
	string negRstr = noRstr;
	bool success = handleReply(reply,repCp,noRstr,negRstr);
	if (reply == 0) {
		pthread_mutex_lock(&allComtLock);
		net->removeComtree(ctx);
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"root router never replied");
		return false;
	} else if (!success) {
		pthread_mutex_lock(&allComtLock);
		net->removeComtree(ctx);
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"root router could not add comtree");
		return false;
	}
	
	// modify comtree to set global attributes
	reqCp.reset(MOD_COMTREE, REQUEST, 0);
	reqCp.setAttr(COMTREE_NUM,comt);
	reqCp.setAttr(CORE_FLAG,true);
	reqCp.setAttr(PARENT_LINK,0);
	reply = sendCtlPkt(reqCp,rootAdr,inQ,outQ);
	noRstr = "handleAddComt: mod comtree request to "
		 + net->getNodeName(rootRtr,s1);
	negRstr = noRstr;
	success = handleReply(reply,repCp,noRstr,negRstr);
	if (reply == 0) {
		pthread_mutex_lock(&allComtLock);
		net->removeComtree(ctx);
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"root router never replied");
		return false;
	} else if (!success) {
		pthread_mutex_lock(&allComtLock);
		net->removeComtree(ctx);
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		// arguably, should send message to router to drop comtree
		// at this point, but since a negative reply shouldn't happen,
		// there's no reason to think that the drop comtree would
		// succeed
		errReply(p,cp,outQ,"root router could not modify comtree");
		return false;
	}

	// now update local data structure to reflect addition
	net->addComtNode(ctx,rootRtr);
	net->addComtCoreNode(ctx,rootRtr);
	net->setComtRoot(ctx,rootRtr);
	fAdr_t cliAdr = h.getSrcAdr();
	net->setComtOwner(ctx,cliAdr);
	pthread_mutex_unlock(&comtLock[ctx]);

	// send positive reply back to client
	repCp.reset(cp.getCpType(), POS_REPLY, cp.getSeqNum()); 
	sendCtlPkt(repCp,h.getSrcAdr(),inQ,outQ);

	return true;
}

/** Handle a drop comtree request.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true if the operation was completed successfully,
 *  otherwise false; an error reply is considered a successful
 *  completion; the operation can fail if it cannot allocate
 *  packets, or if one of the routers that must be configured
 *  fails to respond.
 */
bool handleDropComtReq(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
	PacketHeader& h = ps->getHeader(p);
	if (!cp.isSet(COMTREE_NUM)) {
		errReply(p,cp,outQ,"missing required attribute");
		return true;
	}
	int comt = cp.getAttr(COMTREE_NUM);
	fAdr_t cliAdr = h.getSrcAdr();
	
	pthread_mutex_lock(&allComtLock);
	int ctx = net->lookupComtree(comt);
	pthread_mutex_unlock(&allComtLock);
	if (ctx == 0) { // treat this case as a success
		CtlPkt repCp(cp.getCpType(),POS_REPLY,cp.getSeqNum());
		sendCtlPkt(cp,cliAdr,inQ,outQ);
		return true;
	}
	if (cliAdr != net->getComtOwner(ctx)) {
		errReply(p,cp,outQ,"only the owner can drop a comtree");
		return true;
	}
	pthread_mutex_lock(&comtLock[ctx]);
	
	// now, find all leaf routers in comtree by identifying nodes
	// that appear in only one link
	map<int,int> rtrCnt;	
	for (int lnk = net->firstComtLink(ctx); lnk != 0; 
		 lnk = net->nextComtLink(ctx,lnk)) {
		int left  = net->getLocLinkL(lnk);
		int right = net->getLocLinkR(lnk);
		if (rtrCnt.find(left) == rtrCnt.end()) 	rtrCnt[left] = 1;
		else 					rtrCnt[left] += 1;
		if (rtrCnt.find(right) == rtrCnt.end())	rtrCnt[right] = 1;
		else 					rtrCnt[right] += 1;
	}

	// now drop all paths starting at leaf nodes and release resources
	map<int,int>::iterator rcp;
	for (rcp = rtrCnt.begin(); rcp != rtrCnt.end(); rcp++) {
		if (rcp->second != 1) continue;
		int rtr = rcp->first; fAdr_t rtrAdr = net->getNodeAdr(rtr);
		dropPath(rtr,ctx,inQ,outQ);
		pthread_mutex_lock(&rateLock);
		releasePath(rtr,ctx);
		pthread_mutex_unlock(&rateLock);
	}

	// finally remove comtree at root router
	int root = net->getComtRoot(ctx);
	fAdr_t rootAdr = net->getNodeAdr(root);

	pthread_mutex_lock(&allComtLock);
	pthread_mutex_unlock(&comtLock[ctx]);
	net->removeComtree(ctx);
	comtSet->swap(comt - (firstComt-1));
	pthread_mutex_unlock(&allComtLock);

	CtlPkt reqCp(DROP_COMTREE, REQUEST, 0);
	reqCp.setAttr(COMTREE_NUM,comt);
	int reply = sendCtlPkt(reqCp,rootAdr,inQ,outQ);
	CtlPkt repCp; string s1;
	string noRstr = "handleDropComt: drop comtree request to "
			+ net->getNodeName(root,s1);
	string negRstr = noRstr;
	bool success = handleReply(reply,repCp,noRstr,negRstr);
	if (reply == 0) {
		errReply(p,cp,outQ,"root router never replied");
		return false;
	} else if (!success) {
		errReply(p,cp,outQ,"root router could not drop comtree");
		return false;
	}
	
	// send positive reply to client
	repCp.reset(cp.getCpType(),POS_REPLY,cp.getSeqNum());
	sendCtlPkt(cp,h.getSrcAdr(),inQ,outQ);
	return true;
}

/** Handle a join comtree request.
 *  This requires selecting a path from the client's access router
 *  to the comtree and allocating link bandwidth along that path.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success; the operation
 *  fails if it cannot allocate required packets, or if no path
 *  can be found between the client's access router and the comtree.
 */
bool handleJoinComtReq(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
	PacketHeader& h = ps->getHeader(p);
	if (!cp.isSet(COMTREE_NUM) || !cp.isSet(CLIENT_IP) ||
	    !cp.isSet(CLIENT_PORT)) {
		errReply(p,cp,outQ,"required attribute is missing");
		return true;
	}
	fAdr_t cliAdr = h.getSrcAdr();
	comt_t comt = cp.getAttr(COMTREE_NUM);
	ipa_t cliIp = cp.getAttr(CLIENT_IP);
	ipp_t cliPort = cp.getAttr(CLIENT_PORT);

	// find the client's router, based on its forest address
	int cliRtr;
	for (cliRtr = net->firstRouter(); cliRtr != 0;
	     cliRtr = net->nextRouter(cliRtr)) {
		if (cliAdr >= net->getFirstLeafAdr(cliRtr) &&
		    cliAdr <= net->getLastLeafAdr(cliRtr))
			break;
	}
	if (cliRtr == 0) {
		errReply(p,cp,outQ,"can't find client's access router");
		cerr << "handleJoinComt: cannot find client's access router "
			"in network topology\n";
		return false;
	}

	// acquire lock on comtree
	// hold this lock as long as this operation is in progress
	pthread_mutex_lock(&allComtLock);
	int ctx = net->lookupComtree(comt);
	pthread_mutex_unlock(&allComtLock);
	pthread_mutex_lock(&comtLock[ctx]);

	list<int> path; // used to hold list of links

	int tryCount = 1;
	while (true) {
		// lock link rates while looking for path
		pthread_mutex_lock(&rateLock);
		if (tryCount++ > 3 || !findPath(cliRtr,ctx,path)) {
			pthread_mutex_unlock(&rateLock);
			pthread_mutex_unlock(&comtLock[ctx]);
			errReply(p,cp,outQ,"cannot find path to comtree");
			return true;
		}
		reservePath(ctx,path); // update internal data structures
		pthread_mutex_unlock(&rateLock);
		// we now have the path reserved in the internal data structure,
		// so no other thread can interfere with completion
	
		// now configure routers on path and exit loop if successful
		if (addPath(ctx,path,inQ,outQ)) break;

		// failed to configure some router in path
		// so release reserved rates and update available rate info  
		// before trying again
		pthread_mutex_lock(&rateLock);
		releasePath(cliRtr,ctx);
		pthread_mutex_unlock(&rateLock);
		updatePath(ctx,path,inQ,outQ);
		path.clear();
	}

	// so, now we have a path from cliRtr to the comtree
	// all that's left is to add the comtree link at cliRtr
	fAdr_t cliRtrAdr = net->getNodeAdr(cliRtr);
	CtlPkt reqCp(ADD_COMTREE_LINK,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	reqCp.setAttr(PEER_IP,cliIp);
	reqCp.setAttr(PEER_PORT,cliPort);
	int reply = sendCtlPkt(reqCp,cliRtrAdr,inQ,outQ);
	CtlPkt repCp; string ss;
	string noRstr = "handleJoinComt: final add comtree link request to "
			+ net->getNodeName(cliRtr,ss);
	string negRstr = "";
	bool success = handleReply(reply,repCp,noRstr,negRstr);
	if (reply == 0) { // no reply
		dropPath(cliRtr,ctx,inQ,outQ);
		pthread_mutex_lock(&rateLock);
		releasePath(cliRtr,ctx);
		pthread_mutex_unlock(&rateLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"client router did not respond to final "
				   "add comtree link request");
		return false;
	} else if (!success) { // negative reply
		dropPath(cliRtr,ctx,inQ,outQ);
		pthread_mutex_lock(&rateLock);
		releasePath(cliRtr,ctx);
		pthread_mutex_unlock(&rateLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"client router could not add client "
				   "comtree link");
		return true;
	}
	net->incComtLnkCnt(ctx,cliRtr); // increment the link count for cliRtr
	int lnk = repCp.getAttr(LINK_NUM);

	// and set the rates on the link
	reqCp.reset(MOD_COMTREE_LINK,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	reqCp.setAttr(LINK_NUM,lnk);
	reqCp.setAttr(BIT_RATE_IN,net->getComtLeafBrUp(ctx));
	reqCp.setAttr(BIT_RATE_OUT,net->getComtLeafBrDown(ctx));
	reqCp.setAttr(PKT_RATE_IN,net->getComtLeafPrUp(ctx));
	reqCp.setAttr(PKT_RATE_OUT,net->getComtLeafPrDown(ctx));
	reply = sendCtlPkt(reqCp,cliRtrAdr,inQ,outQ);
	noRstr = "handleJoinComt: final mod comtree link request to "
		 + net->getNodeName(cliRtr,ss);
	success = handleReply(reply,repCp,noRstr,negRstr);
	if (reply == 0) { // no reply
		dropPath(cliRtr,ctx,inQ,outQ);
		pthread_mutex_lock(&rateLock);
		releasePath(cliRtr,ctx);
		pthread_mutex_unlock(&rateLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"client router did not respond to final "
				   "mod comtree link request");
		return false;
	} else if (!success) { // negative reply
		dropPath(cliRtr,ctx,inQ,outQ);
		pthread_mutex_lock(&rateLock);
		releasePath(cliRtr,ctx);
		pthread_mutex_unlock(&rateLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"client router could not setup client "
				   "comtree link");
		return true;
	}

	pthread_mutex_unlock(&comtLock[ctx]);

	// finally, send positive reply to client and return
	repCp.reset(cp.getCpType(),POS_REPLY, cp.getSeqNum());
	sendCtlPkt(repCp,cliAdr,inQ,outQ);
	return true;
}

/** Find a path from a router to a comtree.
 *  This method builds a shortest path tree with a given source node.
 *  The search is done over all network links that have the available capacity
 *  to handle the default backbone rates for the comtree.
 *  The search halts when it reaches any node in the comtree.
 *  The path from source to this node is returned in path.
 *  @param src is the place to start the path search
 *  @param ctx is a valid comtree index for the comtree of interest
 *  @param path is a reference to a path, which on completion will
 *  contain link numbers leading from src to the comtree; the first
 *  link on the list is the one incident to src.
 *  @return true if a path was found, else false
 */
bool findPath(int src, int ctx, list<int>& path) {
	path.clear();
	if (net->isComtNode(ctx,src)) return true;
	int n = 0;
	for (int r = net->firstRouter(); r != 0; r = net->nextRouter(r)) {
		n = max(r,n);
	}
	Heap h(n); int d[n+1]; int plnk[n+1];
	for (int r = net->firstRouter(); r != 0; r = net->nextRouter(r)) {
		plnk[r] = 0; d[r] = BIGINT;
	}
	int bru = net->getComtBrUp(ctx);
	int brd = net->getComtBrDown(ctx);
	int pru = net->getComtPrUp(ctx);
	int prd = net->getComtPrDown(ctx);
	h.insert(src,0);
	while (!h.empty()) {
		int r = h.deletemin();
		for (int lnk = net->firstLinkAt(r); lnk != 0;
			 lnk = net->nextLinkAt(r,lnk)) {
			if (lnk == plnk[r]) continue;
			int peer = net->getPeer(r,lnk);
			// if this link cannot take the default backbone
			// rate for this comtree, ignore it
			if (bru > net->getAvailBitRate(lnk,r) ||
			    brd > net->getAvailBitRate(lnk,peer) ||
			    pru > net->getAvailPktRate(lnk,r) ||
			    prd > net->getAvailPktRate(lnk,peer))
				continue;
			if (net->isComtNode(ctx,peer)) { // done
				plnk[peer] = lnk;
				int u = peer;
				for (int pl = plnk[u]; pl != 0; pl = plnk[u]) {
					path.push_back(pl);
					u = net->getPeer(u,plnk[u]);
				}
				return true;
			}
			if (d[peer] > d[r] + net->getLinkLength(lnk)) {
				plnk[peer] = lnk;
				d[peer] = d[r] + net->getLinkLength(lnk);
				if (h.member(peer)) h.changekey(peer,d[peer]);
				else h.insert(peer,d[peer]);
			}
		}
	}
	return false;
}

/** Reserve a path from a router to a comtree.
 *  This method adds a list of links to an existing comtree and
 *  reserves link capacity on those links.
 *  If it fails at any point, it undoes its previous
 *  changes before returning.
 *  @param ctx is a valid comtree index for the comtree of interest
 *  @param path is a reference to a path, containing a list of links,
 *  leading from some router outside the comtree to a router in the comtree
 *  @return true on success, else false
 */
bool reservePath(int ctx, list<int>& path) {
	// work from top going down, meaning from the end of the list
	if (path.rbegin() == path.rend()) return true;
	int lastLnk = *(path.rbegin());
	int rtr = net->getLinkL(lastLnk);
	if (!net->isComtNode(ctx,rtr)) rtr = net->getLinkR(lastLnk);
	list<int>::reverse_iterator pp;
	for (pp = path.rbegin(); pp != path.rend(); pp++) {
		int lnk = *pp; int child = net->getPeer(rtr,lnk);
		// add lnk to the comtree in net
		net->addComtLink(ctx,lnk,rtr);
		// get default backbone rates
		int bru = net->getComtBrUp(ctx);
		int brd = net->getComtBrDown(ctx);
		int pru = net->getComtPrUp(ctx);
		int prd = net->getComtPrDown(ctx);
		// set rates on comtree links to the default rates
		net->setComtBrUp(ctx,bru,lnk);
		net->setComtBrDown(ctx,brd,lnk);
		net->setComtPrUp(ctx,pru,lnk);
		net->setComtPrDown(ctx,prd,lnk);
		
		// adjust available link rates, backing out on a failure
		if (!net->addAvailBitRate(lnk,child,-bru)) {
			net->removeComtLink(ctx,lnk);
			releasePath(rtr,ctx);
			return false;
		} else if (!net->addAvailBitRate(lnk,rtr,-brd)) {
			net->addAvailBitRate(lnk,child,bru);
			net->removeComtLink(ctx,lnk);
			releasePath(rtr,ctx);
			return false;
		} else if (!net->addAvailPktRate(lnk,child,-pru)) {
			net->addAvailBitRate(lnk,child,bru);
			net->addAvailBitRate(lnk,rtr,brd);
			net->removeComtLink(ctx,lnk);
			releasePath(rtr,ctx);
			return false;
		} else if (!net->addAvailPktRate(lnk,rtr,-prd)) {
			net->addAvailBitRate(lnk,child,bru);
			net->addAvailBitRate(lnk,rtr,brd);
			net->addAvailPktRate(lnk,child,pru);
			net->removeComtLink(ctx,lnk);
			releasePath(rtr,ctx);
			return false;
		}
		rtr = child;
	}
	return true;
}

/** Release a path from a router in a comtree.
 *  This method releases the path from a leaf router in a comtree,
 *  up to the root or the first node with more than one child.
 *  It follows parent pointers in the internal comtree data
 *  structure, removing links as it goes and adjusting 
 *  the available capacity on the links.
 *  @param firstRtr is the leaf router in the path to be pruned
 *  @param ctx is a valid comtree index for the comtree of interest
 */
void releasePath(int firstRtr, int ctx) {
	int rtr = firstRtr;
	for (int lnk = net->getComtPlink(ctx,rtr); lnk != 0;
		 lnk = net->getComtPlink(ctx,rtr)) {
		if (net->isComtCoreNode(ctx,rtr) ||
		    net->getComtLnkCnt(ctx,rtr) > 1)
			return;
		int parent = net->getPeer(rtr,lnk);
		net->addAvailBitRate(lnk,rtr,net->getComtBrUp(ctx,lnk));
		net->addAvailBitRate(lnk,parent,net->getComtBrDown(ctx,lnk));
		net->addAvailPktRate(lnk,rtr,net->getComtPrUp(ctx,lnk));
		net->addAvailPktRate(lnk,parent,net->getComtPrDown(ctx,lnk));
		net->removeComtLink(ctx,lnk);
		rtr = parent;
	}
}

/** Configure the routers on a path to add them to a comtree.
 *  This method sends configuration configuration messages to the routers
 *  along a specified the path. The first link in the path should be
 *  incident to a leaf router, and the last link should be incident
 *  to a router in the comtree.  If it is unable to add any link, it unwinds
 *  its changes  before returning.
 *  @param ctx is a valid comtree index for the comtree of interest
 *  @param path is a reference to a list, defining a path in the network,
 *  with the last link in the path incident to the comtree
 *  @return true on success, else false
 */
bool addPath(int ctx, list<int>& path, Queue& inQ, Queue& outQ) {
	// work from top going down, meaning from the end of the list
	if (path.rbegin() == path.rend()) return true;
	int lastLnk = *(path.rbegin());
	int rtr = net->getLinkL(lastLnk);
	if (!net->isComtNode(ctx,rtr)) rtr = net->getLinkR(lastLnk);
	int comt = net->getComtree(ctx);
	list<int>::reverse_iterator pp;
	for (pp = path.rbegin(); pp != path.rend(); pp++) {
		int lnk = *pp; int child = net->getPeer(rtr,lnk);
		fAdr_t rtrAdr   = net->getNodeAdr(rtr);
		fAdr_t childAdr = net->getNodeAdr(child);

		// first, add the comtree link at rtr
		CtlPkt reqCp(ADD_COMTREE_LINK,REQUEST,0);
		reqCp.setAttr(COMTREE_NUM,comt);
		reqCp.setAttr(LINK_NUM,net->getLocLink(lnk,rtr));
		reqCp.setAttr(PEER_CORE_FLAG,net->isComtCoreNode(ctx,child));
		int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
		CtlPkt repCp; string s1, s2;
		string noRstr = "addPath: add comtree link request to "
			 	+ net->getNodeName(rtr,s1) + " for comtree "
		         	+ Misc::num2string(comt,s2);
		string negRstr = "";
		if (!handleReply(reply,repCp,noRstr,negRstr)) {
			dropPath(rtr,ctx,inQ,outQ); return false;
		}

		// next, set the comtree link rates at rtr
		reqCp.reset(MOD_COMTREE_LINK,REQUEST,0);
		reqCp.setAttr(COMTREE_NUM,comt);
		reqCp.setAttr(LINK_NUM,net->getLocLink(lnk,rtr));
		reqCp.setAttr(BIT_RATE_IN,net->getComtBrUp(ctx,lnk));
		reqCp.setAttr(BIT_RATE_OUT,net->getComtBrDown(ctx,lnk));
		reqCp.setAttr(PKT_RATE_IN,net->getComtPrUp(ctx,lnk));
		reqCp.setAttr(PKT_RATE_OUT,net->getComtPrDown(ctx,lnk));
		reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
		noRstr = "addPath: mod comtree link request to "
		 	 + net->getNodeName(rtr,s1) + " for comtree "
		       	 + Misc::num2string(comt,s2);
		negRstr = "";
		bool success = handleReply(reply,repCp,noRstr,negRstr);
		if (reply == 0) {
			dropPath(rtr,ctx,inQ,outQ); return false;
		} else if (!success) {
			CtlPkt abortCp(DROP_COMTREE_LINK,REQUEST,0);
			abortCp.setAttr(COMTREE_NUM,comt);
			abortCp.setAttr(LINK_NUM,net->getLocLink(lnk,rtr));
			int reply = sendCtlPkt(abortCp,rtrAdr,inQ,outQ);
			if (reply != 0) ps->free(reply);
			dropPath(rtr,ctx,inQ,outQ);
			return false;
		}

		// next, add the child to the comtree
		reqCp.reset(ADD_COMTREE,REQUEST,0);
		reqCp.setAttr(COMTREE_NUM,comt);
		reply = sendCtlPkt(reqCp,childAdr,inQ,outQ);
		noRstr = "addPath: add comtree request to "
		 	 + net->getNodeName(rtr,s1) + " for comtree "
		       	 + Misc::num2string(comt,s2);
		negRstr = "";
		success = handleReply(reply,repCp,noRstr,negRstr);
		if (!success) {
			dropPath(rtr,ctx,inQ,outQ); return false;
		}
		ps->free(reply);

		// and add the comtree link at the child
		// this must be done before we modify the comtree attributes,
		// so that parent link is in the comtree when we do modify op
		reqCp.reset(ADD_COMTREE_LINK,REQUEST,0);
		reqCp.setAttr(COMTREE_NUM,comt);
		reqCp.setAttr(LINK_NUM,net->getLocLink(lnk,child));
		reqCp.setAttr(PEER_CORE_FLAG,net->isComtCoreNode(ctx,rtr));
		reply = sendCtlPkt(reqCp,childAdr,inQ,outQ);
		noRstr = "addPath: add comtree link request to "
		 	 + net->getNodeName(child,s1) + " for comtree "
		       	 + Misc::num2string(comt,s2);
		negRstr = noRstr;
		success = handleReply(reply,repCp,noRstr,negRstr);
		if (reply == 0) {
			dropPath(rtr,ctx,inQ,outQ); return false;
		} else if (!success) {
			CtlPkt abortCp(DROP_COMTREE,REQUEST,0);
			abortCp.setAttr(COMTREE_NUM,comt);
			int reply = sendCtlPkt(abortCp,rtrAdr,inQ,outQ);
			if (reply != 0) ps->free(reply);
			dropPath(rtr,ctx,inQ,outQ);
			return false;
		}

		// now, set comtree attributes at child
		reqCp.reset(MOD_COMTREE,REQUEST,0);
		reqCp.setAttr(COMTREE_NUM,comt);
		reqCp.setAttr(PARENT_LINK,net->getLocLink(lnk,child));
		reqCp.setAttr(CORE_FLAG,net->isComtCoreNode(ctx,child));
		reply = sendCtlPkt(reqCp,childAdr,inQ,outQ);
		noRstr = "addPath: add mod comtree request to "
		 	 + net->getNodeName(child,s1) + " for comtree "
		       	 + Misc::num2string(comt,s2);
		negRstr = noRstr;
		success = handleReply(reply,repCp,noRstr,negRstr);
		if (reply == 0) {
			dropPath(rtr,ctx,inQ,outQ); return false;
		} else if (!success) {
			CtlPkt abortCp(DROP_COMTREE,REQUEST,0);
			abortCp.setAttr(COMTREE_NUM,comt);
			int reply = sendCtlPkt(abortCp,rtrAdr,inQ,outQ);
			if (reply != 0) ps->free(reply);
			dropPath(rtr,ctx,inQ,outQ);
			return false;
		}

		// finally, set the comtree link rates at the child
		reqCp.reset(MOD_COMTREE_LINK,REQUEST,0);
		reqCp.setAttr(COMTREE_NUM,comt);
		reqCp.setAttr(LINK_NUM,net->getLocLink(lnk,child));
		reqCp.setAttr(BIT_RATE_IN,net->getComtBrDown(ctx,lnk));
		reqCp.setAttr(BIT_RATE_OUT,net->getComtBrUp(ctx,lnk));
		reqCp.setAttr(PKT_RATE_IN,net->getComtPrDown(ctx,lnk));
		reqCp.setAttr(PKT_RATE_OUT,net->getComtPrUp(ctx,lnk));
		reply = sendCtlPkt(reqCp,childAdr,inQ,outQ);
		noRstr = "addPath: mod comtree link request to "
		 	 + net->getNodeName(child,s1) + " for comtree "
		       	 + Misc::num2string(comt,s2);
		negRstr = noRstr;
		success = handleReply(reply,repCp,noRstr,negRstr);
		if (reply == 0) {
			dropPath(rtr,ctx,inQ,outQ); return false;
		} else if (!success) {
			CtlPkt abortCp(DROP_COMTREE,REQUEST,0);
			abortCp.setAttr(COMTREE_NUM,comt);
			int reply = sendCtlPkt(abortCp,childAdr,inQ,outQ);
			if (reply != 0) ps->free(reply);
			dropPath(rtr,ctx,inQ,outQ);
			return false;
		}
		ps->free(reply);
		rtr = child;
	}
	return true;
}

/** Drop a path from a comtree.
 *  This method sends configuration messages to routers in order to
 *  remove a path from a comtree. For each link in the path, drop
 *  comtree link messages are sent to both endpoints and a drop comtree
 *  message is sent to the "child" node. 
 *  @param firstRtr specifies the starting point for the drop operation
 *  @param inQ is a reference to the thread's input queue
 *  @param outQ is a reference to the thread's output queue
 *  @return true if all routers are successfully reconfigured,
 *  else false
 */
bool dropPath(int firstRtr, int ctx, Queue& inQ, Queue& outQ) {
	int comt = net->getComtree(ctx);
	int rtr = firstRtr;
	bool status = true;
	for (int lnk = net->getComtPlink(ctx,rtr); lnk != 0;
		 lnk = net->getComtPlink(ctx,rtr)) {
		if (net->isComtCoreNode(ctx,rtr) ||
		    net->getComtLnkCnt(ctx,rtr) > 1)
			break;
		int parent = net->getPeer(rtr,lnk);

		// drop comtree at rtr
		fAdr_t rtrAdr = net->getNodeAdr(rtr);
		CtlPkt reqCp(DROP_COMTREE,REQUEST,0);
		reqCp.setAttr(COMTREE_NUM,comt);
		int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
		CtlPkt repCp; string s1, s2;
		string noRstr = "dropPath: drop comtree request to "
		 		+ net->getNodeName(rtr,s1) + " for comtree "
		       		+ Misc::num2string(comt,s2);
		string negRstr = noRstr;
		if (!handleReply(reply,repCp,noRstr,negRstr)) status = false;

		// now drop comtree link at parent
		fAdr_t pAdr = net->getNodeAdr(parent);
		reqCp.reset(DROP_COMTREE_LINK,REQUEST,0);
		reqCp.setAttr(COMTREE_NUM,comt);
		reqCp.setAttr(LINK_NUM,net->getLocLink(lnk,rtr));
		reply = sendCtlPkt(reqCp,pAdr,inQ,outQ);
		noRstr = "dropPath: mod comtree link request to "
		 	 + net->getNodeName(parent,s1) + " for comtree "
		       	 + Misc::num2string(comt,s2);
		negRstr = noRstr;
		if (!handleReply(reply,repCp,noRstr,negRstr)) status = false;

		rtr = parent;
	}
	return status;
}

/** Update the available link rates for the links in a path.
 */
void updatePath(int ctx, list<int>& path, Queue& inQ, Queue& outQ) {
	// work from top going down, meaning from the end of the list
	if (path.rbegin() == path.rend()) return;
	int lastLnk = *(path.rbegin());
	int rtr = net->getLinkL(lastLnk);
	if (!net->isComtNode(ctx,rtr)) rtr = net->getLinkR(lastLnk);
	int comt = net->getComtree(ctx);
	list<int>::reverse_iterator pp;
	for (pp = path.rbegin(); pp != path.rend(); pp++) {
		int lnk = *pp; int child = net->getPeer(rtr,lnk);
		fAdr_t rtrAdr   = net->getNodeAdr(rtr);
		fAdr_t childAdr = net->getNodeAdr(child);
		CtlPkt reqCp(GET_LINK,REQUEST,0);
		reqCp.setAttr(LINK_NUM,net->getLocLink(lnk,rtr));
		int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
		if (reply != 0) {
			CtlPkt repCp;
			repCp.unpack(ps->getPayload(reply),
	 			(ps->getHeader(reply)).getLength()
				 - Forest::OVERHEAD);
			if (repCp.getRrType() == POS_REPLY) {
				pthread_mutex_lock(&rateLock);
				net->addAvailBitRate(lnk,rtr,
					repCp.getAttr(AVAIL_BIT_RATE_OUT)
					- net->getAvailBitRate(lnk,rtr));
				net->addAvailPktRate(lnk,rtr,
					repCp.getAttr(AVAIL_PKT_RATE_OUT)
					- net->getAvailPktRate(lnk,rtr));
				net->addAvailBitRate(lnk,child,
					repCp.getAttr(AVAIL_BIT_RATE_IN)
					- net->getAvailBitRate(lnk,child));
				net->addAvailPktRate(lnk,child,
					repCp.getAttr(AVAIL_PKT_RATE_IN)
					- net->getAvailPktRate(lnk,child));
				pthread_mutex_unlock(&rateLock);
			}
		}
		rtr = child;
	}
}

/** Handle a request by a client to leave a comtree.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success; the operation
 *  fails if it ...
 */
bool handleLeaveComtReq(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
	PacketHeader& h = ps->getHeader(p);
	if (!cp.isSet(COMTREE_NUM) || !cp.isSet(CLIENT_IP) ||
	    !cp.isSet(CLIENT_PORT)) {
		errReply(p,cp,outQ,"required attribute is missing");
		return true;
	}
	fAdr_t cliAdr = h.getSrcAdr();
	comt_t comt = cp.getAttr(COMTREE_NUM);
	ipa_t cliIp = cp.getAttr(CLIENT_IP);
	ipp_t cliPort = cp.getAttr(CLIENT_PORT);

	// find the client's router, based on its forest address
	int cliRtr;
	for (cliRtr = net->firstRouter(); cliRtr != 0;
	     cliRtr = net->nextRouter(cliRtr)) {
		if (cliAdr >= net->getFirstLeafAdr(cliRtr) &&
		    cliAdr <= net->getLastLeafAdr(cliRtr))
			break;
	}
	if (cliRtr == 0) {
		errReply(p,cp,outQ,"can't find client's access router");
		cerr << "handleLeaveComt: cannot find client's access router "
			"in network topology\n";
		return false;
	}
	fAdr_t cliRtrAdr = net->getNodeAdr(cliRtr);

	// acquire the comtree lock
	pthread_mutex_lock(&allComtLock);
	int ctx = net->lookupComtree(comt);
	pthread_mutex_unlock(&allComtLock);
	pthread_mutex_lock(&comtLock[ctx]);

	if (!net->isComtNode(ctx,cliRtr)) {
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"invalid comtree for this client");
		return true;
	}

	// send drop comtree link packet to cliRtr, for access link
	CtlPkt reqCp(DROP_COMTREE_LINK,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	reqCp.setAttr(PEER_IP,cliIp);
	reqCp.setAttr(PEER_PORT,cliPort);
	int reply = sendCtlPkt(reqCp,cliRtrAdr,inQ,outQ);
	CtlPkt repCp; string s1, s2, s3;
	string noRstr = "handleLeaveComt: drop comtree link request to "
		 	 + net->getNodeName(cliRtr,s1) + " for comtree "
	       		 + Misc::num2string((int) comt,s2) + " client "
			 + Forest::fAdr2string(cliAdr,s3);
	string negRstr = noRstr;
	bool success = handleReply(reply,repCp,noRstr,negRstr);
	if (!success) {
		pthread_mutex_unlock(&comtLock[ctx]); return false;
	}

	// identify links to be removed from the comtree
	int r = cliRtr;
	int plnk = net->getComtPlink(ctx,r);
	list<int> path;
	while (plnk != 0 && net->getComtLnkCnt(ctx,r) < 3) {
		path.push_back(plnk);
		r = net->getPeer(r,plnk);
		plnk = net->getComtPlink(ctx,r);
	}
	net->decComtLnkCnt(ctx,cliRtr);

	// now, reconfigure routers to drop links
	dropPath(cliRtr,ctx,inQ,outQ);

	// and update internal data structures
	pthread_mutex_lock(&rateLock);
	releasePath(cliRtr,ctx);
	pthread_mutex_unlock(&rateLock);

	pthread_mutex_unlock(&comtLock[ctx]);
	
	// send positive reply and return
	repCp.reset(cp.getCpType(),POS_REPLY,cp.getSeqNum());
	sendCtlPkt(repCp,cliAdr,inQ,outQ);
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
int sendCtlPkt(CtlPkt& cp, fAdr_t dest, Queue& inQ, Queue& outQ) {
	int p = ps->alloc();
	if (p == 0) {
		cerr << "sendCtlPkt: no packets left in packet store\n";
		return 0;
	}
	int plen = cp.pack(ps->getPayload(p));
	if (plen == 0) {
		cerr << "sendCtlPkt: packing error\n";
		cp.write(cerr);
		ps->free(p);
		return 0;
	}
	PacketHeader& h = ps->getHeader(p);
	h.setLength(plen + Forest::OVERHEAD);
	if (cp.getCpType() < CLIENT_NET_SIG_SEP) {
		h.setPtype(CLIENT_SIG);
		h.setComtree(Forest::CLIENT_CON_COMT);
	} else {
		h.setPtype(NET_SIG);
		h.setComtree(Forest::NET_SIG_COMT);
	}
	h.setFlags(0);
	h.setDstAdr(dest);
	h.setSrcAdr(myAdr);
	h.pack(ps->getBuffer(p));

	if (cp.getRrType() != REQUEST) {
		outQ.enq(p); return 0;
	}
	int reply = sendAndWait(p,cp,inQ,outQ);
	ps->free(p);
	return reply;
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

	outQ.enq(copy);

	for (int i = 1; i < 3; i++) {
		int reply = inQ.deq(1000000000); // 1 sec timeout
		if (reply == -1) { // timeout
			// no reply, make new copy and send
			int retry = ps->fullCopy(p);
			if (retry == 0) {
				cerr << "sendAndWait: no packets "
					"left in packet store\n";
				return 0;
			}
			outQ.enq(retry);
		} else {
			return reply;
		}
	}
	return 0;
}

/** Handle common chores for reply packets.
 *  @param reply is a packet number for a reply to a control packet
 *  @param repCp is a reference to a CtlPkt object in which the reply
 *  information is returned
 *  @param noRstr is a string to be written to cerr if no reply is
 *  received; it will be preceded by a generic message indicating that
 *  no reply was received and followed by a new line. If noRstr has
 *  zero loength, then nothing is written
 *  @param negRstr is a string to be written to cerr if a negative reply is
 *  received; it will be preceded by a generic message indicating that
 *  a negative reply was received and followed by the error message
 *  returned as part of the reply packet. If negRstr has zero length,
 *  then nothing is written
 */
bool handleReply(int reply, CtlPkt& repCp, string& noRstr, string& negRstr) {
	if (reply == 0) {
		if (noRstr.length() != 0)
			cerr << "handleReply: no reply to control packet:\n"
			     << noRstr << endl;
		return false;
	}
	repCp.reset();
	repCp.unpack(ps->getPayload(reply),
		     (ps->getHeader(reply)).getLength() - Forest::OVERHEAD);
	if (repCp.getRrType() == NEG_REPLY) {
		if (negRstr.length() != 0)
			cerr << "handleReply: negative reply received:\n"
		     	     << negRstr << "\n("
			     << repCp.getErrMsg() << ")" << endl;
		ps->free(reply);
		return false;
	}
	ps->free(reply);
	return true;
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
	h.setComtree(Forest::CLIENT_CON_COMT);
	h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	sendToForest(p);
}

/** Send final disconnect packet to forest router.
 */
void disconnect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
	h.setComtree(Forest::CLIENT_CON_COMT);
	h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	sendToForest(p);
}
