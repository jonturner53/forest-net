/** @file ComtCtl.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ComtCtl.h"

using namespace forest;

/** usage:
 *       ComtCtl extIp topoFile firstComt lastComt finTime
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

namespace forest {

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
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,4*PTHREAD_STACK_MIN);
	size_t stacksize;
	pthread_attr_getstacksize(&attr,&stacksize);
	cerr << "min stack size=" << 4*PTHREAD_STACK_MIN << endl;
	cerr << "threads in pool have stacksize=" << stacksize << endl;
	if (stacksize != 4*PTHREAD_STACK_MIN)
		fatal("init: can't set stack size");
	for (int t = 1; t <= TPSIZE; t++) {
		if (!pool[t].qp.in.init() || !pool[t].qp.out.init())
			fatal("init: can't initialize thread queues\n");
		if (pthread_create(&pool[t].thid,&attr,handler,
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
		pktx px = rcvFromForest();
		if (px != 0) {
			// send px to a thread, possibly assigning one
			Packet& p = ps->getPacket(px);
			if (p.type == CLIENT_SIG || p.type == NET_SIG) {
				CtlPkt cp(p.payload(),
					  p.length-Forest::OVERHEAD);
				cp.unpack();
				int t;
				if (cp.mode == CtlPkt::REQUEST) {
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
						pool[t].seqNum = 0;
						pool[t].qp.in.enq(px);
					} else {
						cerr << "run: thread "
							"pool is exhausted\n";
						ps->free(px);
					}
				} else {
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

		// now handle outgoing packets from the thread pool
		for (int t = threads->firstIn(); t != 0;
			 t = threads->nextIn(t)) {
			if (pool[t].qp.out.empty()) continue;
			int px1 = pool[t].qp.out.deq();
			if (px1 == 0) { // means thread is done
				reqMap->dropPair(reqMap->getKey(t));
				pool[t].qp.in.reset();
				threads->swap(t);
				continue;
			}
			nothing2do = false;
			Packet& p1 = ps->getPacket(px1);
			CtlPkt cp1(p1.payload(),p1.length-Forest::OVERHEAD);
			cp1.unpack();
			if (cp1.mode == CtlPkt::REQUEST) {
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
				pool[t].seqNum = seqNum;
				pool[t].ts = now + 2000000000; // 2 sec timeout
				seqNum++;
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
	Queue& inq  = ((QueuePair *) qp)->in;
	Queue& outq = ((QueuePair *) qp)->out;
	CpHandler cph(&inq, &outq, myAdr, &logger, ps);

	while (true) {
		pktx px = inq.deq();
		bool success = false;
		if (px < 0) {
			// in this case, p is really a negated socket number
			// for a remote comtree display module
			int sock = -px;
			success = handleComtreeDisplay(sock);
		} else {
			Packet& p = ps->getPacket(px);
			CtlPkt cp(p.payload(),p.length - Forest::OVERHEAD);; 
			cp.unpack();
			switch (cp.type) {
			case CtlPkt::CLIENT_ADD_COMTREE:
				success = handleAddComtReq(px,cp,cph);
				break;
			case CtlPkt::CLIENT_DROP_COMTREE:
				success = handleDropComtReq(px,cp,cph);
				break;
			case CtlPkt::CLIENT_JOIN_COMTREE:
				success = handleJoinComtReq(px,cp,cph);
				break;
			case CtlPkt::CLIENT_LEAVE_COMTREE:
				success = handleLeaveComtReq(px,cp,cph);
				break;
			default:
				cph.errReply(px,cp,"invalid control packet "
						   "type for ComtCtl");
				break;
			}
			if (!success) {
				string s;
				cerr << "handler: operation failed\n"
				     << p.toString(s);
			}
		}
		ps->free(px); // release px now that we're done
		outq.enq(0); // signal completion to main thread
	}
}

/** Handle a connection to a remote comtree display module.
 *  @param sock is an open stream socket
 *  @return true if the interaction proceeds normally, followed
 *  by a normal close; return false if an error occurs
 */

bool handleComtreeDisplay(int sock) {
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
 *  @return true if the operation was completed successfully,
 *  otherwise false; an error reply is considered a successful
 *  completion; the operation can fail if it cannot allocate
 *  packets, or if one of the routers that must be configured
 *  fails to respond.
 */
bool handleAddComtReq(pktx px, CtlPkt& cp, CpHandler& cph) {
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
	Packet& p = ps->getPacket(px);
	if (cp.zipCode != 0) {
		cph.errReply(px,cp,"missing required attribute");
		return true;
	}
	int rootZip = cp.zipCode;

	pthread_mutex_lock(&allComtLock);
	int comt = comtSet->firstOut();
	if (comt == 0) {
		pthread_mutex_unlock(&allComtLock);
		cph.errReply(px,cp,"no comtrees available to satisfy request");
		return true;
	}
	comtSet->swap(comt);
	comt += (firstComt - 1); // convert comtSet index to actual comtree #
	if (!comtrees->addComtree(comt)) {
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		cph.errReply(px,cp,"internal error prevents adding new "
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
		cph.errReply(px,cp,"network contains no router with specified "
				   "zip code");
		return true;
	}
	int rootRtr = matches[randint(0,cnt-1)];
	fAdr_t rootAdr = net->getNodeAdr(rootRtr);
	
	// configure root router to add comtree
	int reply = cph.addComtree(rtrAdr,comt);
	CtlPkt repCp;
	if (reply == 0 || !cph.getCp(reply,repCp)) {
		pthread_mutex_lock(&allComtLock);
		comtrees->removeComtree(ctx);
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		if (reply == 0)
		cph.errReply(px,cp,(reply == 0 ?
				"root router never replied" :
				"root router could not add comtree"));
		return false;
	}
	
	// modify comtree
	reply = cph.modComtree(rtrAdr,comt,0,1);
	if (reply == 0 || !cph.getCp(reply,repCp)) {
		pthread_mutex_lock(&allComtLock);
		comtrees->removeComtree(ctx);
		comt -= (firstComt - 1);
		comtSet->swap(comt);
		pthread_mutex_unlock(&allComtLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		cph.errReply(px,cp,(reply == 0 ?
				"root router never replied" :
				"root router could not modify comtree"));
		return false;
	}
	
	// now update local data structure to reflect addition
	comtrees->addNode(ctx,rootAdr);
	comtrees->addCoreNode(ctx,rootAdr);
	comtrees->setRoot(ctx,rootAdr);
	fAdr_t cliAdr = p.srcAdr;
	comtrees->setOwner(ctx,cliAdr);
	pthread_mutex_unlock(&comtLock[ctx]);

	// send positive reply back to client
	repCp.reset(cp.type, CtlPkt::POS_REPLY, cp.seqNum); 
	cph.sendCtlPkt(repCp,p.srcAdr);

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
 *  @return true if the operation was completed successfully,
 *  otherwise false; an error reply is considered a successful
 *  completion; the operation can fail if it cannot allocate
 *  packets, or if one of the routers that must be configured
 *  fails to respond.
 */
bool handleDropComtReq(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	if (cp.comtree == 0) {
		cph.errReply(px,cp,"missing required attribute");
		return true;
	}
	int comt = cp.comtree;
	fAdr_t cliAdr = p.srcAdr;
	
	pthread_mutex_lock(&allComtLock);
	int ctx = comtrees->getComtIndex(comt);
	pthread_mutex_unlock(&allComtLock);
	pthread_mutex_lock(&comtLock[ctx]);
	if (ctx == 0) { // treat this case as a success
		CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
		cph.sendCtlPkt(cp,cliAdr);
		return true;
	}
	if (cliAdr != comtrees->getOwner(ctx)) {
		// eventually should base this decision on actual owner
		// name not just address
		cph.errReply(px,cp,"only the owner can drop a comtree");
		return true;
	}
	
	// first, teardown comtrees at all routers
	for (fAdr_t rtr = comtrees->firstRouter(ctx); rtr != 0;
		    rtr = comtrees->nextRouter(ctx,rtr)) {
		teardownComtNode(ctx,rtr,cph);
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
	CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
	cph.sendCtlPkt(cp,p.srcAdr);
	return true;
}

static int cnt = 0;

/** Handle a join comtree request.
 *  This requires selecting a path from the client's access router
 *  to the comtree and allocating link bandwidth along that path.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success; the operation
 *  fails if it cannot allocate required packets, or if no path
 *  can be found between the client's access router and the comtree.
 */
bool handleJoinComtReq(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	fAdr_t cliAdr = p.srcAdr;
	comt_t comt = cp.comtree;
	ipa_t cliIp = cp.ip1; ipp_t cliPort = cp.port1;

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
		cph.errReply(px,cp,"can't find client's access router");
		return false;
	}
	fAdr_t cliRtrAdr = net->getNodeAdr(cliRtr);

	// acquire lock on comtree
	// hold this lock as long as this operation is in progress
	pthread_mutex_lock(&allComtLock);
	int ctx = comtrees->getComtIndex(comt);
	pthread_mutex_unlock(&allComtLock);
	if (ctx == 0) {
		cph.errReply(px,cp,"no such comtree");
		return true;
	}
	pthread_mutex_lock(&comtLock[ctx]);

	if (comtrees->isComtLeaf(ctx,cliAdr)) {
		// if client already in comtree, this is probably a
		// second attempt, caused by a lost acknowledgment
		pthread_mutex_unlock(&comtLock[ctx]);
		CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
		cph.sendCtlPkt(repCp,cliAdr);
		return true;
	}

	list<LinkMod> path; // for new path added to comtree
	list<LinkMod> modList; // for link modifications in rest of comtree
	RateSpec bbDefRates, leafDefRates;
	leafDefRates = comtrees->getDefLeafRates(ctx);
	bbDefRates = comtrees->getDefBbRates(ctx);
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
			cph.errReply(px,cp,"cannot find path to comtree");
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
				cph.errReply(px,cp,"cannot add required "
					 "capacity to comtree backbone");
				return true;
			}
		}
		pthread_mutex_unlock(&rateLock);
		// we now have the path reserved in the internal data structure,
		// so no other thread can interfere with completion
	
		// now configure routers on path and exit loop if successful
		if (!setupPath(ctx,path,cph)) {
			// could not configure path at some router
			teardownPath(ctx,path,cph);
			pthread_mutex_lock(&rateLock);
			if (autoConfig) comtrees->unprovision(ctx,modList);
			leafDefRates.negate();
			comtrees->adjustSubtreeRates(
				ctx,cliRtrAdr,leafDefRates);
			leafDefRates.negate();
			comtrees->removePath(ctx,path);
			pthread_mutex_unlock(&rateLock);
		} else if (autoConfig &&
			   !modComtRates(ctx,modList,false,cph)) {
			pthread_mutex_lock(&rateLock);
				comtrees->unprovision(ctx,modList);
			pthread_mutex_unlock(&rateLock);
			modComtRates(ctx,modList,true,cph);
			pthread_mutex_lock(&rateLock);
				leafDefRates.negate();
				comtrees->adjustSubtreeRates(
					ctx,cliRtrAdr,leafDefRates);
				leafDefRates.negate();
				comtrees->removePath(ctx,path);
			pthread_mutex_unlock(&rateLock);
			teardownPath(ctx,path,cph);
		} else { // all routers successfully configured
			break;
		}
		path.clear(); modList.clear();
	}

	// now add client to comtree
	int llnk = setupClientLink(ctx,cliIp,cliPort,cliRtr,cph);
	comtrees->addNode(ctx,cliAdr);
	comtrees->setParent(ctx,cliAdr,cliRtrAdr,llnk);
	comtrees->getLinkRates(ctx,cliAdr) = leafDefRates;

	if (llnk == 0 || !setComtLeafRates(ctx,cliAdr,cph)) {
		// tear it all down and fail
		comtrees->removeNode(ctx,cliAdr);
		pthread_mutex_lock(&rateLock);
			comtrees->unprovision(ctx,modList);
		pthread_mutex_unlock(&rateLock);
		modComtRates(ctx,modList,true,cph);
		pthread_mutex_lock(&rateLock);
			leafDefRates.negate();
			comtrees->adjustSubtreeRates(
				ctx,cliRtrAdr,leafDefRates);
			comtrees->removePath(ctx,path);
			leafDefRates.negate();
		pthread_mutex_unlock(&rateLock);
		pthread_mutex_unlock(&comtLock[ctx]);
		teardownPath(ctx,path,cph);
		cph.errReply(px,cp,"cannot configure leaf node");
		return true;
	}
	pthread_mutex_unlock(&comtLock[ctx]);

	// finally, send positive reply to client and return
	CtlPkt repCp(cp.type,CtlPkt::POS_REPLY, cp.seqNum);
	cph.sendCtlPkt(repCp,cliAdr);
	return true;
}

/** Handle a request by a client to leave a comtree.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success
 */
bool handleLeaveComtReq(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	fAdr_t cliAdr = p.srcAdr;
	comt_t comt = cp.comtree;
	ipa_t cliIp = cp.ip1; ipp_t cliPort = cp.port1;

	// find the client's router, based on its forest address
	int cliRtr;
	for (cliRtr = net->firstRouter(); cliRtr != 0;
	     cliRtr = net->nextRouter(cliRtr)) {
		pair<int,int> leafRange; net->getLeafRange(cliRtr,leafRange);
		if (cliAdr >= leafRange.first && cliAdr <= leafRange.second)
			break;
	}
	if (cliRtr == 0) {
		cph.errReply(px,cp,"can't find client's access router");
		logger.log("handleLeaveComt: cannot find client's access "
			   "router in network topology\n",2,p);
		return false;
	}
	fAdr_t cliRtrAdr = net->getNodeAdr(cliRtr);

	// acquire the comtree lock
	pthread_mutex_lock(&allComtLock);
		int ctx = comtrees->getComtIndex(comt);
	pthread_mutex_unlock(&allComtLock);
	if (ctx == 0) {
		cph.errReply(px,cp,"no such comtree");
		return true;
	}
	pthread_mutex_lock(&comtLock[ctx]);

	if (!comtrees->isComtLeaf(ctx,cliAdr)) {
		// if client is not in comtree, this is probably a
		// second attempt, caused by a lost acknowledgment
		pthread_mutex_unlock(&comtLock[ctx]);
		CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
		cph.sendCtlPkt(repCp,cliAdr);
		return true;
	}
	teardownClientLink(ctx,cliIp,cliPort,cliRtr,cph);

	// reduce subtree rates in response to dropped client
	// then remove client from comtrees
	RateSpec rs;
	rs = comtrees->getLinkRates(ctx,cliAdr); rs.negate();
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
		modComtRates(ctx,modList,true,cph);
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
		RateSpec rs = comtrees->getLinkRates(ctx,rtrAdr);
		path.push_back(LinkMod(plnk,rtr,rs));
		rtr = net->getPeer(rtr,plnk);
		rtrAdr = net->getNodeAdr(rtr);
	}

	// drop path from comtrees object, release allocated capacity
	// and remove comtree at routers on path
	pthread_mutex_lock(&rateLock);
		comtrees->removePath(ctx,path);
	pthread_mutex_unlock(&rateLock);
	teardownPath(ctx,path,cph);
	pthread_mutex_unlock(&comtLock[ctx]);
	
	// send positive reply and return
	CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
	cph.sendCtlPkt(repCp,cliAdr);
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
 *  @return false if any router fails to respond or rejects a configuration
 *  request
 */
bool setupPath(int ctx, list<LinkMod>& path, CpHandler& cph) {
	// First, configure all routers on the path to add the comtree
	list<LinkMod>::iterator pp;
	for (pp = path.begin(); pp != path.end(); pp++) {
		if (!setupComtNode(ctx,pp->child,cph)) {
			return false;
		}
	}
	
	// Next, add link at all routers and configure comtree attributes
	for (pp = path.begin(); pp != path.end(); pp++) {
		int parent = net->getPeer(pp->child,pp->lnk);
		if (!setupComtLink(ctx,pp->lnk,pp->child,cph)) {
			return false;
		}
		if (!setupComtLink(ctx,pp->lnk,parent,cph)) {
			return false;
		}
		if (!setupComtAttrs(ctx,pp->child,cph)) {
			return false;
		}
		if (!setComtLinkRates(ctx,pp->lnk,pp->child,cph)) {
			return false;
		}
		if (!setComtLinkRates(ctx,pp->lnk,parent,cph)) {
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
 *  @return true if all routers are successfully reconfigured,
 *  else false
 */
bool teardownPath(int ctx, list<LinkMod>& path, CpHandler& cph) {
	list<LinkMod>::iterator lp;
	bool status = true;
	for (lp = path.begin(); lp != path.end(); lp++) {
		if (!teardownComtNode(ctx,lp->child,cph))
			status = false;
	}
	return status;
}


/** Configure a comtree at a router.
 *  @param ctx is a valid comtree index
 *  @param rtr is a node number for a router
 *  @return true on success, false on failure
 */
bool setupComtNode(int ctx, int rtr, CpHandler& cph) {
	int reply = cph.addComtree(net->getNodeAdr(rtr),
				   comtrees->getComtree(ctx));
	CtlPkt repCp;
	return cph.getCp(reply,repCp);
} 

/** Remove a comtree at a router
 *  @param ctx is a valid comtree index
 *  @param rtr is a node number for a router
 *  @return true on success, false on failure
 */
bool teardownComtNode(int ctx, int rtr, CpHandler& cph) {
	int reply = cph.dropComtree(net->getNodeAdr(rtr),
				    comtrees->getComtree(ctx));
	CtlPkt repCp;
	return cph.getCp(reply,repCp);
} 

/** Configure a comtree link at a router.
 *  @param ctx is a valid comtree index
 *  @param link is a link number for a link in the comtree
 *  @param rtr is a node number for a router
 *  @return true on success, false on failure
 */
bool setupComtLink(int ctx, int lnk, int rtr, CpHandler& cph) {
	int parent = net->getPeer(rtr,lnk);
	int reply = cph.addComtreeLink(net->getNodeAdr(rtr),
				   	comtrees->getComtree(ctx),
				   	net->getLLnum(lnk,rtr),
				   	comtrees->isCoreNode(ctx,parent));
	CtlPkt repCp;
	return cph.getCp(reply,repCp);
} 

/** Configure a comtree link to a client at a router.
 *  @param ctx is a valid comtree index
 *  @param cliIp is the IP address of the client
 *  @param cliPort is the port number of the client
 *  @param rtr is a node number for a router
 *  @return the local link number assigned by the router on success,
 *  0 on failure
 */
int setupClientLink(int ctx, ipa_t cliIp, ipp_t cliPort, int rtr,
		    CpHandler& cph) {
	pktx reply = cph.addComtreeLink( net->getNodeAdr(rtr),
					comtrees->getComtree(ctx),
				   	cliIp, cliPort);
	CtlPkt repCp;
	cph.getCp(reply,repCp);
	if (repCp.mode != CtlPkt::POS_REPLY) {
		return 0;
	}
	return repCp.link;
} 

/** Teardown a comtree link to a client at a router.
 *  @param ctx is a valid comtree index
 *  @param cliIp is the IP address of the client
 *  @param cliPort is the port number of the client
 *  @param rtr is a node number for the client's access router
 *  @return the true on success, false on failure
 */
bool teardownClientLink(int ctx, ipa_t cliIp, ipp_t cliPort, int rtr,
			CpHandler& cph) {
	int reply = cph.dropComtreeLink(net->getNodeAdr(rtr),
					comtrees->getComtree(ctx),
				   	cliIp, cliPort);
	CtlPkt repCp;
	return cph.getCp(reply,repCp);
} 

/** Configure comtree attributes at a router.
 *  @param ctx is a valid comtree index
 *  @param rtr is a node number for a router
 *  @return true on success, false on failure
 */
bool setupComtAttrs(int ctx, int rtr, CpHandler& cph) {
	fAdr_t rtrAdr = net->getNodeAdr(rtr);
	int llnk = net->getLLnum(comtrees->getPlink(ctx,rtrAdr),rtr);
	int reply = cph.modComtree(rtrAdr, comtrees->getComtree(ctx),
				   llnk, comtrees->isCoreNode(ctx,rtrAdr));
	CtlPkt repCp;
	bool status = cph.getCp(reply,repCp);
	return status;
}

/** Set the comtree link rates at a router
 *  @param ctx is a valid comtree index
 *  @param lnk is the link whose rate is to be configured
 *  @param rtr is a node number of the router at which the rate is to be
 *  configured
 *  @return true on success, false on failure
 */
bool setComtLinkRates(int ctx, int lnk, int rtr, CpHandler& cph) {
	fAdr_t rtrAdr = net->getNodeAdr(rtr);
	fAdr_t peerAdr = net->getNodeAdr(net->getPeer(rtr,lnk));
	RateSpec rs;
	if (rtrAdr == comtrees->getChild(ctx,lnk)) {
		rs = comtrees->getLinkRates(ctx,rtrAdr);
		rs.flip(); // make up in comtree ratespec match input at rtr
	} else {
		rs = comtrees->getLinkRates(ctx,peerAdr);
	}
	int reply = cph.modComtreeLink( rtrAdr,
				   	comtrees->getComtree(ctx),
					net->getLLnum(lnk,rtr),rs);
	CtlPkt repCp;
	if (reply == 0) return false;
	if (cph.getCp(reply,repCp)) return true;

	// router rejected, probably because available rate at router
	// is different from information stored in comtrees object;
	// such differences can occur, when we have multiple comtree
	// controllers; request available rate at router and use it
	// to update local information
	reply = cph.getLink(rtrAdr,net->getLLnum(lnk,rtr));
	if (reply == 0 || !cph.getCp(reply,repCp)) return false;
	pthread_mutex_lock(&rateLock);
	if (repCp.rspec2.isSet()) {
		if (rtr == net->getLeft(lnk)) repCp.rspec2.flip();
		net->getAvailRates(lnk) = repCp.rspec2;
	}
	pthread_mutex_unlock(&rateLock);
	return false;
}

/** Set the comtree link rates at a leaf.
 *  @param ctx is a valid comtree index
 *  @param leafAdr is the forest address of a leaf whose rate is to be
 *  configured
 *  @return true on success, false on failure
 */
bool setComtLeafRates(int ctx, fAdr_t leafAdr, CpHandler& cph) {
	int reply = cph.modComtreeLink( comtrees->getParent(ctx, leafAdr),
				   	comtrees->getComtree(ctx),
				   	comtrees->getPlink(ctx,leafAdr),
					comtrees->getLinkRates(ctx,leafAdr));
	CtlPkt repCp;
	bool status = cph.getCp(reply,repCp);
	return status;
}

/** Modify rates in a comtree.
 *  @param ctx is a valid comtree index
 *  @param modList is a list of LinkMod objects defining the links whose
 *  rates should be modified; the rates are obtained from the comtrees object
 *  @param nostop is true if the method should attempt to set the rates of
 *  all links, even if some routers fail to respond or refuse the request;
 *  when nostop is false, this method returns on the first failed request
 *  @return true on success, false on failure
 */
bool modComtRates(int ctx, list<LinkMod>& modList, bool nostop,
		  CpHandler& cph) {
	list<LinkMod>::iterator pp;
	for (pp = modList.begin(); pp != modList.end(); pp++) {
		if (!nostop &&
		    !setComtLinkRates(ctx,pp->lnk,pp->child,cph))
			return false;
		int parent = net->getPeer(pp->child,pp->lnk);
		if (!nostop &&
		    !setComtLinkRates(ctx,pp->lnk,parent,cph))
			return false;
	}
	return true;
}

/** Check for next packet from the Forest network.
 *  @return next packet or 0, if no report has been received.
 */
pktx rcvFromForest() { 
	pktx px = ps->alloc();
	if (px == 0) return 0;
	Packet& p = ps->getPacket(px);

	int nbytes = Np4d::recv4d(intSock,p.buffer,1500);
	if (nbytes < 0) { ps->free(px); return 0; }
	p.unpack();

	return px;
}

/** Send packet to Forest router.
 */
void sendToForest(pktx px) {
	Packet p = ps->getPacket(px);
	p.pack();
	int rv = Np4d::sendto4d(intSock,(void *) p.buffer, p.length,
		    	      	rtrIp,Forest::ROUTER_PORT);
	if (rv == -1) fatal("sendToForest: failure in sendto");
	ps->free(px);
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
void connect() {
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);

	p.length = 4*(5+1); p.type = CONNECT; p.flags = 0;
	p.comtree = Forest::CLIENT_CON_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	sendToForest(px);
}

/** Send final disconnect packet to forest router.
 */
void disconnect() {
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);

	p.length = 4*(5+1); p.type = DISCONNECT; p.flags = 0;
	p.comtree = Forest::CLIENT_CON_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	sendToForest(px);
}

} // ends namespace

