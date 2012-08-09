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
int main(int argc, char *argv[]) {
	uint32_t finTime;

	if (argc != 6 ||
  	    (extIp  = Np4d::ipAddress(argv[1])) == 0 ||
	    (sscanf(argv[3],"%d", &firstComt)) != 1 ||
	    (sscanf(argv[4],"%d", &lastComt)) != 1 ||
	     sscanf(argv[5],"%d", &finTime) != 1)
		fatal("usage: ComtCtl extIp topoFile firstComt "
			"lastComt finTime");
	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress(); 
	if (extIp == 0) fatal("can't retrieve default IP address");

	if (!init(argv[2])) {
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
	reqMap = new IdMap(TPSIZE);
	tMap = new IdMap(TPSIZE);
	if (firstComt < 1 || lastComt < 1 || firstComt > lastComt) {
		cerr << "init: invalid comtree range\n";
		return false;
	}

	// read NetInfo data structure from file
	int maxNode = 5000; int maxLink = 10000;
	int maxRtr = 4500; int maxCtl = 400;
	maxComtree = 100000; // declared in header file
	net = new NetInfo(maxNode, maxLink, maxRtr, maxCtl);
	comtrees = new ComtInfo(maxComtree,*net);
	ifstream fs; fs.open(topoFile);
	if (fs.fail() || !net->read(fs) || !comtrees->read(fs)) {
		cerr << "ComtCtl::init: could not read topology file, or error "
		      	"in topology file";
		return false;
	}
	fs.close();

	comtSet = new UiSetPair((lastComt-firstComt)+1);
	// Mark all pre-configured comtrees in the range this controller
	// is responsible for as in-use".
	for (int ctx = comtrees->firstComtIndex(); ctx != 0;
		 ctx = comtrees->nextComtIndex(ctx)) {
		int comt = comtrees->getComtree(ctx);
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
			int llnk = net->getLLnum(lnk,rtr);
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
	return	Np4d::bind4d(extSock,extIp,Forest::CC_PORT) &&
		Np4d::listen4d(extSock) &&
		Np4d::nonblock(extSock);
}

void cleanup() {
	cout.flush(); cerr.flush();
	if (extSock > 0) close(extSock);
	if (intSock > 0) close(intSock);
	delete ps; delete [] pool; delete tMap; delete threads; delete reqMap;

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
		nothing2do = true;

		// check for connection requests from remote display
		int connSock = -1;
		if ((connSock = Np4d::accept4d(extSock)) > 0) {
			// let handler know this is socket# for remote host
			int t = threads->firstOut();
			if (t != 0) {
				threads->swap(t);
				pool[t].seqNum = 0;
				pool[t].qp.in.enq(-connSock);
			} else
				cerr << "run: thread pool is exhausted\n";
		} 
		// check for packets from the Forest net
		int p = rcvFromForest();
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
					uint64_t key = h.getSrcAdr();
					key <<= 32; key += cp.getSeqNum();
					if (reqMap->validKey(key)) {
						// in this case, we've got an
						// active thread handling this
						// request, so discard duplicate
						ps->free(p);
					} else if (t != 0) {
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

		// now handle outgoing packets from the thread pool
		for (int t = threads->firstIn(); t != 0;
			 t = threads->nextIn(t)) {
			if (pool[t].qp.out.empty()) continue;
			int p1 = pool[t].qp.out.deq();
			if (p1 == 0) { // means thread is done
				reqMap->dropPair(reqMap->getKey(t));
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
				if (cp1.getSeqNum() == 1) {
					// means this is a repeat of a pending
					// outgoing request
					if (tMap->validId(t)) {
						cp1.setSeqNum(tMap->getKey(t));
					} else {
						// in this case, reply has
						// arrived but was not yet seen
						// by thread so, suppress
						// duplicate request
						ps->free(p1); continue;
					}
				} else {
					if (tMap->validId(t))
						tMap->dropPair(tMap->getKey(t));
					tMap->addPair(seqNum,t);
					cp1.setSeqNum(seqNum++);
				}
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
	pthread_exit(NULL);
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
		bool success = false;
		if (p < 0) {
			// in this case, p is really a negated socket number
			// for a remote comtree display module
			int sock = -p;
			success = handleComtreeDisplay(sock,inQ,outQ);
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
				string s;
				cerr << "handler: operation failed\n"
				     << h.toString(ps->getBuffer(p),s);
			}
		}
		ps->free(p); // release p now that we're done
		outQ.enq(0); // signal completion to main thread
	}
}

/** Handle a connection to a remote comtree display module.
 *  @param sock is an open stream socket
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true if the interaction proceeds normally, followed
 *  by a normal close; return false if an error occurs
 */

static int cnt = 0;
bool handleComtreeDisplay(int sock, Queue& inQ, Queue& outQ) {
	const int BLEN = 100;
	char buf[BLEN+1]; int i;
	string savs = "";
	while (true) {
		// wait for a request from comtreeDisplay
		// these take three forms
		//
		//	getNet
		// 	getComtSet
		//	getComtree 1234
		//
		// each appears on a line by itself; the first
		// requests the network topology, the second, a list
		// of comtrees, and the third the comtree status for
		// the specified comtree

		// read characters into buffer until a newline;
		// expect remote display to send single comtree status request
		// and wait for reply before sending another
		i = 0;
		while (true) {
			int nbytes = recv(sock,&buf[i],BLEN-i,0);
			if (nbytes < 0) {
/* not sure why this is not working, but comment out for now
				if (errno != EINTR) {
cerr << "error errno=" << errno << "\n";
					cerr << "handleComtreeDisplay: unable "
						"to receive comtree number "
						"from remote display\n";
					perror("");
					return false;
				}
*/
				nbytes = 0;
			} else if (nbytes == 0) {
				close(sock); return true;
			}
			int limit = i + nbytes;
			while (i < limit && buf[i] != '\n') i++;
			if (i == BLEN || buf[i] == '\n') {
				buf[i] = EOS; break;
			}
		}
		stringstream ss(string(buf,i));
		string word; ss >> word;
		uint32_t comt = 0;
		if (word.compare("getNet") == 0) {
			string netString; net->toString(netString);
			if (Np4d::sendString(sock,netString) < 0) {
				cerr << "handleComtreeDisplay: unable to send "
					"network topology to display\n";
				return false;
			}
		} else if (word.compare("getComtSet") == 0) {
			pthread_mutex_lock(&allComtLock);
			stringstream comtSet;
			comtSet << "comtSet(";
			for (int ctx = comtrees->firstComtIndex(); ctx != 0;
				 ctx = comtrees->nextComtIndex(ctx)) {
				comtSet << comtrees->getComtree(ctx);
				if (comtrees->nextComtIndex(ctx) != 0)
					comtSet << ",";
			}
			comtSet << ")\n";
			pthread_mutex_unlock(&allComtLock);
			string s = comtSet.str();
			if (Np4d::sendString(sock,s) < 0) {
				cerr << "handleComtreeDisplay: unable to send "
					"comtree set to display\n";
				return false;
			}
		} else if (word.compare("getComtree") == 0) {
			ss >> comt;
			string s;
			pthread_mutex_lock(&allComtLock);
				int ctx = comtrees->getComtIndex(comt);
			pthread_mutex_unlock(&allComtLock);
			if (ctx == 0) {
				// send back error message
				s = "invalid comtree request\n";
			} else {
				pthread_mutex_lock(&comtLock[ctx]);
				comtrees->comtStatus2string(ctx,s);
				pthread_mutex_unlock(&comtLock[ctx]);
			}
			if (Np4d::sendString(sock,s) < 0) {
				cerr << "handleComtreeDisplay: unable to send "
					"comtree status update to display\n";
				return false;
			}
		} else {
			cerr << "handleComtreeDisplay: unrecognized request "
			     << word << " from comtreeDisplay";
			return false;
		}
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
// contain a comtree sequence #, which we store with the comtree and
// if a specific user requests two comtrees with the same sequence #,
// we assume they are the same. This seems ugly since we already have
// sequence number for packet. Maybe should retain record of requests
// by sequence number. How long to retain them? And what should we
// do on getting a repeat? Could send to handler and let handler
// figure out when we have a repeat.
//
// Not clear we can make addComtree operation idempotent
// Another approach is to require that a user first request a
// comtree number, then do addComtree using that comtree number.
// Then, a repeated request can be idempotent.
// Need to limit number of comtrees a user can request, so we
// need to track this and need to timeout old requests.
// Maybe just keep a list of up to 1000 pending comtrees and
// every time we add a new one, we recycle one of the old ones,
// if the list is full.
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
	if (!comtrees->addComtree(comt)) {
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		errReply(p,cp,outQ,"internal error prevents adding new "
				   "comtree");
		cerr << "handleAddComt: addComtree() failed due to program "
			"error\n";
		return false;
	}
	int ctx = comtrees->getComtIndex(comt);
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
		comtrees->removeComtree(ctx);
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
	if (reply == NORESPONSE) {
		pthread_mutex_lock(&allComtLock);
		comtrees->removeComtree(ctx);
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"root router never replied");
		return false;
	} else if (!success) {
		pthread_mutex_lock(&allComtLock);
		comtrees->removeComtree(ctx);
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
	if (reply == NORESPONSE) {
		pthread_mutex_lock(&allComtLock);
		comtrees->removeComtree(ctx);
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		errReply(p,cp,outQ,"root router never replied");
		return false;
	} else if (!success) {
		pthread_mutex_lock(&allComtLock);
		comtrees->removeComtree(ctx);
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
	comtrees->addNode(ctx,rootAdr);
	comtrees->addCoreNode(ctx,rootAdr);
	comtrees->setRoot(ctx,rootAdr);
	fAdr_t cliAdr = h.getSrcAdr();
	comtrees->setOwner(ctx,cliAdr);
	pthread_mutex_unlock(&comtLock[ctx]);

	// send positive reply back to client
	repCp.reset(cp.getCpType(), POS_REPLY, cp.getSeqNum()); 
	sendCtlPkt(repCp,h.getSrcAdr(),inQ,outQ);

	return true;
}

/* future refinement
add a refresh thread that periodically sends a refresh message to
each router saying, "I think comtree x exists at your location".
routers would record time of last refresh and drop any comtrees
for which refresh has not been received; can be a fairly long refresh
time, say ten minutes or more.
*/

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
	int ctx = comtrees->getComtIndex(comt);
	pthread_mutex_unlock(&allComtLock);
	pthread_mutex_lock(&comtLock[ctx]);
	if (ctx == 0) { // treat this case as a success
		CtlPkt repCp(cp.getCpType(),POS_REPLY,cp.getSeqNum());
		sendCtlPkt(cp,cliAdr,inQ,outQ);
		return true;
	}
	if (cliAdr != comtrees->getOwner(ctx)) {
		// eventually should base this decision on actual owner
		// name not just address
		errReply(p,cp,outQ,"only the owner can drop a comtree");
		return true;
	}
	
	// first, teardown comtrees at all routers
	for (fAdr_t rtr = comtrees->firstRouter(ctx); rtr != 0;
		    rtr = comtrees->nextRouter(ctx,rtr)) {
		teardownComtNode(ctx,rtr,inQ,outQ);
	}

	// next, unprovision and remove
	pthread_mutex_lock(&rateLock);
		comtrees->unprovision(ctx);
	pthread_mutex_unlock(&rateLock);
	pthread_mutex_lock(&allComtLock);
		comtrees->removeComtree(ctx);
		comtSet->swap(comt - (firstComt-1));
		pthread_mutex_unlock(&comtLock[ctx]);
	pthread_mutex_unlock(&allComtLock);

	// send positive reply to client
	CtlPkt repCp(cp.getCpType(),POS_REPLY,cp.getSeqNum());
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
		pair<fAdr_t,fAdr_t> leafRange;
		net->getLeafRange(cliRtr,leafRange);
		if (cliAdr >= leafRange.first && cliAdr <= leafRange.second)
			break;
		// replace this with an interval tree when we start using
		// configurations with more than 50 routers
	}
	if (cliRtr == 0) {
		errReply(p,cp,outQ,"can't find client's access router");
		cerr << "handleJoinComt: cannot find client's access router "
			"in network topology\n";
		return false;
	}
	fAdr_t cliRtrAdr = net->getNodeAdr(cliRtr);

	// acquire lock on comtree
	// hold this lock as long as this operation is in progress
	pthread_mutex_lock(&allComtLock);
	int ctx = comtrees->getComtIndex(comt);
	pthread_mutex_unlock(&allComtLock);
	if (ctx == 0) {
		errReply(p,cp,outQ,"no such comtree");
		return true;
	}
	pthread_mutex_lock(&comtLock[ctx]);

	if (comtrees->isComtLeaf(ctx,cliAdr)) {
		// if client already in comtree, this is probably a
		// second attempt, caused by a lost acknowledgment
		pthread_mutex_unlock(&comtLock[ctx]);
		CtlPkt repCp(cp.getCpType(),POS_REPLY, cp.getSeqNum());
		sendCtlPkt(repCp,cliAdr,inQ,outQ);
		return true;
	}

	list<LinkMod> path; // for new path added to comtree
	list<LinkMod> modList; // for link modifications in rest of comtree
	RateSpec bbDefRates, leafDefRates;
	comtrees->getDefRates(ctx,bbDefRates,leafDefRates);
	bool autoConfig = comtrees->getConfigMode(ctx);
	RateSpec pathRates = (autoConfig ?  leafDefRates : bbDefRates);
	int tryCount = 1; int branchRtr = 0;
	while (true) {
		// lock link rates while looking for path
		pthread_mutex_lock(&rateLock);
		branchRtr = comtrees->findPath(ctx,cliRtr,pathRates,path);
		if (branchRtr == 0 || tryCount++ > 3) {
			pthread_mutex_unlock(&rateLock);
			pthread_mutex_unlock(&comtLock[ctx]);
			errReply(p,cp,outQ,"cannot find path to comtree");
			return true;
		}
		comtrees->addPath(ctx,path);
		comtrees->adjustSubtreeRates(ctx,cliRtrAdr,leafDefRates);
		if (autoConfig) {
			if (comtrees->computeMods(ctx,modList)) {
				comtrees->provision(ctx,modList);
			} else {
				leafDefRates.negate();
				comtrees->adjustSubtreeRates(
					ctx,cliRtrAdr,leafDefRates);
				leafDefRates.negate();
				comtrees->removePath(ctx,path);
				pthread_mutex_unlock(&rateLock);
				pthread_mutex_unlock(&comtLock[ctx]);
				errReply(p,cp,outQ,"cannot add required "
					 "capacity to comtree backbone");
				return true;
			}
		}
		pthread_mutex_unlock(&rateLock);
		// we now have the path reserved in the internal data structure,
		// so no other thread can interfere with completion
	
		// now configure routers on path and exit loop if successful
		if (!setupPath(ctx,path,inQ,outQ)) {
			// could not configure path at some router
			teardownPath(ctx,path,inQ,outQ);
			pthread_mutex_lock(&rateLock);
			if (autoConfig) comtrees->unprovision(ctx,modList);
			leafDefRates.negate();
			comtrees->adjustSubtreeRates(
				ctx,cliRtrAdr,leafDefRates);
			leafDefRates.negate();
			comtrees->removePath(ctx,path);
			pthread_mutex_unlock(&rateLock);
		} else if (autoConfig &&
			   !modComtRates(ctx,modList,false,inQ,outQ)) {
			pthread_mutex_lock(&rateLock);
				comtrees->unprovision(ctx,modList);
			pthread_mutex_unlock(&rateLock);
			modComtRates(ctx,modList,true,inQ,outQ);
			pthread_mutex_lock(&rateLock);
				leafDefRates.negate();
				comtrees->adjustSubtreeRates(
					ctx,cliRtrAdr,leafDefRates);
				leafDefRates.negate();
				comtrees->removePath(ctx,path);
			pthread_mutex_unlock(&rateLock);
			teardownPath(ctx,path,inQ,outQ);
		} else { // all routers successfully configured
			break;
		}
		path.clear(); modList.clear();
	}

	// need to update local data structure and send messages to
	// client router.

	int llnk = setupClientLink(ctx,cliIp,cliPort,cliRtr,inQ,outQ);
	comtrees->addNode(ctx,cliAdr);
	comtrees->setParent(ctx,cliAdr,cliRtrAdr,llnk);
	comtrees->setLinkRates(ctx,cliAdr,leafDefRates);


	if (llnk == 0 || !setComtLeafRates(ctx,cliAdr,inQ,outQ)) {
		// tear it all down and fail
		comtrees->removeNode(ctx,cliAdr);
		pthread_mutex_lock(&rateLock);
			comtrees->unprovision(ctx,modList);
		pthread_mutex_unlock(&rateLock);
		modComtRates(ctx,modList,true,inQ,outQ);
		pthread_mutex_lock(&rateLock);
			leafDefRates.negate();
			comtrees->adjustSubtreeRates(
				ctx,cliRtrAdr,leafDefRates);
			comtrees->removePath(ctx,path);
			leafDefRates.negate();
		pthread_mutex_unlock(&rateLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		teardownPath(ctx,path,inQ,outQ);
		errReply(p,cp,outQ,"cannot configure leaf node");
		return true;
	}
	pthread_mutex_unlock(&comtLock[ctx]);

	// finally, send positive reply to client and return
	CtlPkt repCp(cp.getCpType(),POS_REPLY, cp.getSeqNum());
	sendCtlPkt(repCp,cliAdr,inQ,outQ);
	return true;
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
		pair<int,int> leafRange; net->getLeafRange(cliRtr,leafRange);
		if (cliAdr >= leafRange.first && cliAdr <= leafRange.second)
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
		int ctx = comtrees->getComtIndex(comt);
	pthread_mutex_unlock(&allComtLock);
	if (ctx == 0) {
		errReply(p,cp,outQ,"no such comtree");
		return true;
	}
	pthread_mutex_lock(&comtLock[ctx]);

	if (!comtrees->isComtLeaf(ctx,cliAdr)) {
		// if client is not in comtree, this is probably a
		// second attempt, caused by a lost acknowledgment
		pthread_mutex_unlock(&comtLock[ctx]);
		CtlPkt repCp(cp.getCpType(),POS_REPLY, cp.getSeqNum());
		sendCtlPkt(repCp,cliAdr,inQ,outQ);
		return true;
	}
	teardownClientLink(ctx,cliIp,cliPort,cliRtr,inQ,outQ);

	// reduce subtree rates in response to dropped client
	// then remove client from comtrees
	RateSpec rs;
	comtrees->getLinkRates(ctx,cliAdr,rs); rs.negate();
	comtrees->adjustSubtreeRates(ctx,cliRtrAdr,rs);
	comtrees->removeNode(ctx,cliAdr);

	// for autoConfig case, modify backbone rates
	if (comtrees->getConfigMode(ctx)) {
		list<LinkMod> modList;
		// note, since we've adjusted subtree rates, computeMods
		// returns negative rates, so provisioning these rates
		// effectively reduces allocation
		pthread_mutex_lock(&rateLock);
			comtrees->computeMods(ctx,modList);
			comtrees->provision(ctx,modList);
		pthread_mutex_unlock(&rateLock);
		modComtRates(ctx,modList,true,inQ,outQ);
	}

	// now, find path used only to support this client
	list<LinkMod> path;
	fAdr_t rtrAdr = cliRtrAdr;
	int rtr = net->getNodeNum(rtrAdr);
	while (true) {
		int plnk = comtrees->getPlink(ctx,rtrAdr);
		int lnkCnt = comtrees->getLinkCnt(ctx,rtrAdr);
		if (plnk == 0 || (rtrAdr == cliRtrAdr && lnkCnt > 1) ||
		    		 (rtrAdr != cliRtrAdr && lnkCnt > 2))
			break;
		RateSpec rs; comtrees->getLinkRates(ctx,rtrAdr,rs);
		path.push_back(LinkMod(plnk,rtr,rs));
		rtr = net->getPeer(rtr,plnk);
		rtrAdr = net->getNodeAdr(rtr);
	}

	// drop path from comtrees object, release allocated capacity
	// and remove comtree at routers on path
	pthread_mutex_lock(&rateLock);
		comtrees->removePath(ctx,path);
	pthread_mutex_unlock(&rateLock);
	teardownPath(ctx,path,inQ,outQ);
	pthread_mutex_unlock(&comtLock[ctx]);
	
	// send positive reply and return
	CtlPkt repCp(cp.getCpType(),POS_REPLY,cp.getSeqNum());
	sendCtlPkt(repCp,cliAdr,inQ,outQ);
	return true;
}

/** Setup a path to a comtree, by sending configuration messages to routers.
 *  @param ctx is a valid comtree index for the comtree of interest
 *  @param path is a reference to a list of LinkMod objects defining a path
 *  that has been added to the comtree in the comtrees object;
 *  the links appear in bottom-up order
 *  @param modList is a list of links that belong to the comtree for
 *  which the rate has been changed with the last link on the path incident
 *  to a node in the comtree
 *  @param inQ is the queue from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return false if any router fails to respond or rejects a configuration
 *  request
 */
bool setupPath(int ctx, list<LinkMod>& path, Queue& inQ, Queue& outQ) {
	// First, configure all routers on the path to add the comtree
	list<LinkMod>::iterator pp;
	for (pp = path.begin(); pp != path.end(); pp++) {
		if (!setupComtNode(ctx,pp->child,inQ,outQ)) {
			return false;
		}
	}
	
	// Next, add link at all routers and configure comtree attributes
	for (pp = path.begin(); pp != path.end(); pp++) {
		int parent = net->getPeer(pp->child,pp->lnk);
		if (!setupComtLink(ctx,pp->lnk,pp->child,inQ,outQ)) {
			return false;
		}
		if (!setupComtLink(ctx,pp->lnk,parent,inQ,outQ)) {
			return false;
		}
		if (!setupComtAttrs(ctx,pp->child,inQ,outQ)) {
			return false;
		}
		if (!setComtLinkRates(ctx,pp->lnk,pp->child,inQ,outQ)) {
			return false;
		}
		if (!setComtLinkRates(ctx,pp->lnk,parent,inQ,outQ)) {
			return false;
		}
	}
	return true;
}

/** Teardown a path in a comtree.
 *  This method sends configuration messages to routers in order to
 *  remove a path from a comtree. For each node in the path,
 *  a drop comtree message is sent to the "child" node. 
 *  @param path is a list of Link objects that specifies specifies the
 *  path in bottom-up order
 *  @param inQ is a reference to the thread's input queue
 *  @param outQ is a reference to the thread's output queue
 *  @return true if all routers are successfully reconfigured,
 *  else false
 */
bool teardownPath(int ctx, list<LinkMod>& path, Queue& inQ, Queue& outQ) {
	list<LinkMod>::iterator lp;
	bool status = true;
	for (lp = path.begin(); lp != path.end(); lp++) {
		if (!teardownComtNode(ctx,lp->child,inQ,outQ))
			status = false;
	}
	return status;
}


/** Configure a comtree at a router.
 *  @param ctx is a valid comtree index
 *  @param rtr is a node number for a router
 *  @param inQ is the queue from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true on success, false on failure
 */
bool setupComtNode(int ctx, int rtr, Queue& inQ, Queue& outQ) {
	int comt = comtrees->getComtree(ctx);
	fAdr_t rtrAdr = net->getNodeAdr(rtr);
	CtlPkt reqCp(ADD_COMTREE,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	string s1,s2;
	string noRstr = "setupComtNode: add comtree request to "
	 	 	+ net->getNodeName(rtr,s1) + " for comtree "
	       	 	+ Util::num2string(comt,s2);
	CtlPkt repCp;
	string negRstr = "";
	return handleReply(reply,repCp,noRstr,negRstr);
} 

/** Remove a comtree at a router
 *  @param ctx is a valid comtree index
 *  @param rtr is a node number for a router
 *  @param inQ is the queue from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true on success, false on failure
 */
bool teardownComtNode(int ctx, int rtr, Queue& inQ, Queue& outQ) {
	int comt = comtrees->getComtree(ctx);
	fAdr_t rtrAdr = net->getNodeAdr(rtr);
	CtlPkt reqCp(DROP_COMTREE,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	string s1,s2;
	string noRstr = "teardownComtNode: drop comtree request to "
	 	 	+ net->getNodeName(rtr,s1) + " for comtree "
	       	 	+ Util::num2string(comt,s2);
	string negRstr = "";
	CtlPkt repCp;
	return handleReply(reply,repCp,noRstr,negRstr);
} 

/** Configure a comtree link at a router.
 *  @param ctx is a valid comtree index
 *  @param link is a link number for a link in the comtree
 *  @param rtr is a node number for a router
 *  @param inQ is the queue from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true on success, false on failure
 */
bool setupComtLink(int ctx, int lnk, int rtr, Queue& inQ, Queue& outQ) {
	comt_t comt = comtrees->getComtree(ctx);
	fAdr_t rtrAdr = net->getNodeAdr(rtr);
	int parent = net->getPeer(rtr,lnk);

	CtlPkt reqCp(ADD_COMTREE_LINK,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	reqCp.setAttr(LINK_NUM,net->getLLnum(lnk,rtr));
	reqCp.setAttr(PEER_CORE_FLAG,comtrees->isCoreNode(ctx,parent));
	int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	CtlPkt repCp;
	string s1, s2;
	string noRstr = "setupComtLink: add comtree link request to "
	 	 + net->getNodeName(rtr,s1) + " for comtree "
	       	 + Util::num2string(comt,s2);
	string negRstr = noRstr;
	return handleReply(reply,repCp,noRstr,negRstr);
} 

/** Configure a comtree link to a client at a router.
 *  @param ctx is a valid comtree index
 *  @param cliIp is the IP address of the client
 *  @param cliPort is the port number of the client
 *  @param rtr is a node number for a router
 *  @param inQ is the queue from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return the local link number assigned by the router on success,
 *  0 on failure
 */
int setupClientLink(int ctx, ipa_t cliIp, ipp_t cliPort, int rtr,
		     Queue& inQ, Queue& outQ) {
	comt_t comt = comtrees->getComtree(ctx);
	fAdr_t rtrAdr = net->getNodeAdr(rtr);

	CtlPkt reqCp(ADD_COMTREE_LINK,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	reqCp.setAttr(PEER_IP,cliIp);
	reqCp.setAttr(PEER_PORT,cliPort);
	int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	CtlPkt repCp;
	string s1, s2;
	string noRstr = "setupComtLink: add comtree link request to "
	 	 + net->getNodeName(rtr,s1) + " for comtree "
	       	 + Util::num2string(comt,s2);
	string negRstr = noRstr;
	if (!handleReply(reply,repCp,noRstr,negRstr)) return 0;
	else return repCp.getAttr(LINK_NUM);
} 

/** Teardown a comtree link to a client at a router.
 *  @param ctx is a valid comtree index
 *  @param cliIp is the IP address of the client
 *  @param cliPort is the port number of the client
 *  @param rtr is a node number for the client's access router
 *  @param inQ is the queue from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return the true on success, false on failure
 */
bool teardownClientLink(int ctx, ipa_t cliIp, ipp_t cliPort, int rtr,
		     Queue& inQ, Queue& outQ) {
	comt_t comt = comtrees->getComtree(ctx);
	fAdr_t rtrAdr = net->getNodeAdr(rtr);

	CtlPkt reqCp(DROP_COMTREE_LINK,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	reqCp.setAttr(PEER_IP,cliIp);
	reqCp.setAttr(PEER_PORT,cliPort);
	int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	CtlPkt repCp; string s1, s2, s3;
	string noRstr = "handleLeaveComt: drop comtree link request to "
		 	 + net->getNodeName(rtr,s1) + " for comtree "
	       		 + Util::num2string((int) comt,s2) + " client "
			 + Np4d::ip2string(cliIp,s3);
	string negRstr = noRstr;
	if (!handleReply(reply,repCp,noRstr,negRstr)) { 
		return false;
	}
	return true;
} 

/** Configure comtree attributes at a router.
 *  @param ctx is a valid comtree index
 *  @param rtr is a node number for a router
 *  @param inQ is the queue from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true on success, false on failure
 */
bool setupComtAttrs(int ctx, int rtr, Queue& inQ, Queue& outQ) {
	comt_t comt = comtrees->getComtree(ctx);
	fAdr_t rtrAdr = net->getNodeAdr(rtr);
	int lnk = comtrees->getPlink(ctx,rtrAdr);

	CtlPkt reqCp(MOD_COMTREE,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	reqCp.setAttr(PARENT_LINK,net->getLLnum(lnk,rtr));
	reqCp.setAttr(CORE_FLAG,comtrees->isCoreNode(ctx,rtrAdr));
	int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	CtlPkt repCp; string s1,s2;
	string noRstr = "setupComtAttrs: mod comtree request to "
	 	 	+ net->getNodeName(rtr,s1) + " for comtree "
	       	 	+ Util::num2string(comt,s2);
	string negRstr = noRstr;
	return handleReply(reply,repCp,noRstr,negRstr);
}

/** Set the comtree link rates at a router
 *  @param ctx is a valid comtree index
 *  @param lnk is the link whose rate is to be configured
 *  @param rtr is a node number of the router at which the rate is to be
 *  configured
 *  @param inQ is the queue from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true on success, false on failure
 */
bool setComtLinkRates(int ctx, int lnk, int rtr, Queue& inQ, Queue& outQ) {
	comt_t comt = comtrees->getComtree(ctx);
	fAdr_t rtrAdr = net->getNodeAdr(rtr);
	int peer = net->getPeer(rtr,lnk);
	fAdr_t peerAdr = net->getNodeAdr(peer);
	RateSpec rs;
	if (rtrAdr == comtrees->getChild(ctx,lnk)) {
		comtrees->getLinkRates(ctx,rtrAdr,rs);
	} else {
		comtrees->getLinkRates(ctx,peerAdr,rs);
	}
	if (rtr != net->getLeft(lnk)) rs.flip();

	CtlPkt reqCp(MOD_COMTREE_LINK,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	reqCp.setAttr(LINK_NUM,net->getLLnum(lnk,rtr));
	reqCp.setAttr(BIT_RATE_IN,rs.bitRateDown);
	reqCp.setAttr(BIT_RATE_OUT,rs.bitRateUp);
	reqCp.setAttr(PKT_RATE_IN,rs.pktRateDown);
	reqCp.setAttr(PKT_RATE_OUT,rs.pktRateUp);
	int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	CtlPkt repCp; string s1, s2;
	string noRstr = "setComtLinkRates: mod comtree link request to "
	 	 + net->getNodeName(rtr,s1) + " for comtree "
	       	 + Util::num2string(comt,s2);
	noRstr += " link " + Util::num2string(net->getLLnum(lnk,rtr),s1)
		  + " with rateSpec " + rs.toString(s2);
	string negRstr = noRstr;
	bool success = handleReply(reply,repCp,noRstr,negRstr);
	if (reply == NORESPONSE) return false;
	else if (success) return true;

	// router rejected, probably because available rate at router
	// is different from information stored in comtrees object;
	// such differences can occur, when we have multiple comtree
	// controllers; request available rate at router and use it
	// to update local information
	reqCp.reset(GET_LINK,REQUEST,0);
	reqCp.setAttr(LINK_NUM,net->getLLnum(lnk,rtr));
	reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	if (reply != NORESPONSE) {
		CtlPkt repCp;
		repCp.unpack(ps->getPayload(reply),
 				(ps->getHeader(reply)).getLength()
			 	 - Forest::OVERHEAD);
		if (repCp.getRrType() == POS_REPLY) {
			pthread_mutex_lock(&rateLock);
			RateSpec avail;
			net->getAvailRates(lnk,avail);
			if (rtr == net->getLeft(lnk)) {
				avail.bitRateUp =
					repCp.getAttr(AVAIL_BIT_RATE_OUT);
				avail.pktRateUp =
					repCp.getAttr(AVAIL_PKT_RATE_OUT);
			} else {
				avail.bitRateDown =
					repCp.getAttr(AVAIL_BIT_RATE_OUT);
				avail.pktRateDown =
					repCp.getAttr(AVAIL_PKT_RATE_OUT);
			}
			net->setAvailRates(lnk,avail);
			pthread_mutex_unlock(&rateLock);
		}
	}
	return false;
}

/** Set the comtree link rates at a leaf.
 *  @param ctx is a valid comtree index
 *  @param leafAdr is the forest address of a leaf whose rate is to be
 *  configured
 *  @param inQ is the queue from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true on success, false on failure
 */
bool setComtLeafRates(int ctx, fAdr_t leafAdr, Queue& inQ, Queue& outQ) {
	comt_t comt = comtrees->getComtree(ctx);
	fAdr_t rtrAdr = comtrees->getParent(ctx, leafAdr);
	int rtr = net->getNodeNum(rtrAdr);
	RateSpec rs; comtrees->getLinkRates(ctx,leafAdr,rs);

	CtlPkt reqCp(MOD_COMTREE_LINK,REQUEST,0);
	reqCp.setAttr(COMTREE_NUM,comt);
	int llnk = comtrees->getPlink(ctx,leafAdr);
	reqCp.setAttr(LINK_NUM,llnk);
	reqCp.setAttr(BIT_RATE_IN,rs.bitRateUp);
	reqCp.setAttr(BIT_RATE_OUT,rs.bitRateDown);
	reqCp.setAttr(PKT_RATE_IN,rs.pktRateUp);
	reqCp.setAttr(PKT_RATE_OUT,rs.pktRateDown);
	int reply = sendCtlPkt(reqCp,rtrAdr,inQ,outQ);
	CtlPkt repCp; string s1, s2;
	string noRstr = "setComtLeafRates: mod comtree link request to "
	 	 	+ net->getNodeName(rtr,s1) + " for comtree "
	       	 	+ Util::num2string(comt,s2);
	noRstr += " link " + Util::num2string(llnk,s1)
		+ " with rateSpec " + rs.toString(s2);
	string negRstr = noRstr;
	bool success = handleReply(reply,repCp,noRstr,negRstr);
	if (reply == NORESPONSE) return false;
	else if (success) return true;
	return false;
}

/** Modify rates in a comtree.
 *  @param ctx is a valid comtree index
 *  @param modList is a list of LinkMod objects defining the links whose
 *  rates should be modified; the rates are obtained from the comtrees object
 *  @param nostop is true if the method should attempt to set the rates of
 *  all links, even if some routers fail to respond or refuse the request;
 *  when nostop is false, this method returns on the first failed request
 *  @param inQ is the queue from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return true on success, false on failure
 */
bool modComtRates(int ctx, list<LinkMod>& modList, bool nostop,
		  Queue& inQ, Queue& outQ) {
	list<LinkMod>::iterator pp;
	for (pp = modList.begin(); pp != modList.end(); pp++) {
		if (!nostop &&
		    !setComtLinkRates(ctx,pp->lnk,pp->child,inQ,outQ))
			return false;
		int parent = net->getPeer(pp->child,pp->lnk);
		if (!nostop &&
		    !setComtLinkRates(ctx,pp->lnk,parent,inQ,outQ))
			return false;
	}
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
	// set default seq# for requests - tells main thread to use "next" seq#
	if (cp.getRrType() == REQUEST) cp.setSeqNum(0);
	int plen = cp.pack(ps->getPayload(p));
	if (plen == 0) {
		string s;
		cerr << "sendCtlPkt: packing error\n" << cp.toString(s);
		ps->free(p);
		return 0;
	}
	PacketHeader& h = ps->getHeader(p);
	h.setLength(plen + Forest::OVERHEAD);
	if (cp.getCpType() < CLIENT_NET_SIG_SEP) {
		h.setPtype(CLIENT_SIG);
		h.setComtree(Forest::CLIENT_SIG_COMT);
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
		return NORESPONSE;
	}

	outQ.enq(copy);

	for (int i = 1; i < 3; i++) {
		int reply = inQ.deq(1000000000); // 1 sec timeout
		if (reply == Queue::TIMEOUT) {
			// no reply, make new copy and send
			int retry = ps->fullCopy(p);
			if (retry == 0) {
				cerr << "sendAndWait: no packets "
					"left in packet store\n";
				return NORESPONSE;
			}
			cp.setSeqNum(1);	// tag retry as a repeat
			cp.pack(ps->getPayload(retry));
			PacketHeader& hr = ps->getHeader(retry);
			hr.payErrUpdate(ps->getBuffer(retry));
			outQ.enq(retry);
		} else {
			return reply;
		}
	}
	return NORESPONSE;
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
	if (reply == NORESPONSE) {
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
