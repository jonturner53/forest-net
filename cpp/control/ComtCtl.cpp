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
 *       ComtCtl nmIp myIp firstComt lastComt topoFile finTime
 * 
 *  Command line arguments include:
 *  nmIp - IP address of network manager
 *  myIp - IP address to be bound to sockets
 *  firstComt - first comtree in assigned range
 *  lastComt - last comtree in assigned range
 *  topoFile - topology file containing NetInfo and ComtInfo
 *  finTime - is number of seconds to run; if 0, run forever
 */
int main(int argc, char *argv[]) {
	ipa_t nmIp, myIp; int firstComt, lastComt; uint32_t finTime;

	if (argc != 7 ||
  	    (nmIp  = Np4d::ipAddress(argv[1])) == 0 ||
  	    (myIp  = Np4d::ipAddress(argv[2])) == 0 ||
	    (sscanf(argv[3],"%d", &firstComt)) != 1 ||
	    (sscanf(argv[4],"%d", &lastComt)) != 1 ||
	     sscanf(argv[6],"%d", &finTime) != 1) {
		fatal("usage: ComtCtl nmIp myIp firstComt lastComt topoFile "
		      "finTime");
	}
	if (!init(nmIp, myIp, firstComt, lastComt, argv[5])) {
		fatal("ComtCtl: initialization failure");
	}
	sub->run(finTime);
	cleanup();
	exit(0);
}

namespace forest {

/** Initialization of ComtCtl.
 *  @return true on success, false on failure
 */
bool init(ipa_t nmIp1, ipa_t myIp1, int firstComt1, int lastComt1,
	  const char *topoFile) {
	nmIp = nmIp1; myIp = myIp1;
	firstComt = firstComt1; lastComt = lastComt1;

	int nPkts = 10000;
	ps = new PacketStoreTs(nPkts+1);
	pool = new ThreadInfo[TPSIZE+1];
	threads = new UiSetPair(TPSIZE);
	logger = new Logger();

	if (firstComt < 1 || lastComt < 1 || firstComt > lastComt) {
		logger->log("init: invalid comtree range\n",2);
		return false;
	}

	// read NetInfo/ComtInfo from topology file
	int maxNode = 5000; int maxLink = 10000;
	int maxRtr = 4500; int maxCtl = 400;
	maxComtree = 100000; // declared in header file
	net = new NetInfo(maxNode, maxLink, maxRtr, maxCtl);
	comtrees = new ComtInfo(maxComtree,*net);
	if (!comtrees->init()) {
		logger->log("init: cannot initialize ComtInfo object",2);
		return false;
	}
	ifstream fs; fs.open(topoFile);
	if (fs.fail() || !net->read(fs) || !comtrees->read(fs)) {
		logger->log("ComtCtl::init: could not read topology file, or "
			"error in topology file",2);
		return false;
	}
	fs.close();

	// Mark all pre-configured comtrees in the range this controller
	// is responsible for as in-use".
	comtSet = new UiSetPair((lastComt-firstComt)+1);
	for (int ctx = comtrees->firstComtree(); ctx != 0;
		 ctx = comtrees->nextComtree(ctx)) {
		int comt = comtrees->getComtree(ctx);
		if (firstComt <= comt && comt <= lastComt) {
			comtSet->swap((comt-firstComt)+1);
		}
	}

	// boot from net manager
	fAdr_t rtrAdr; ipa_t rtrIp; ipp_t rtrPort; uint64_t nonce;
	if (!bootMe(nmIp, myIp, nmAdr, myAdr, rtrAdr, rtrIp, rtrPort, nonce)) {
		return false;
	}

	// configure substrate
	sub = new Substrate(myAdr,myIp,rtrAdr,rtrIp,rtrPort,nonce,
			    500, handler, 0, Forest::CC_PORT, ps, logger);
	if (!sub->init()) {
                logger->log("init: can't initialize substrate",2);
                return false;
        }
	sub->setRtrReady(true);

	// initialize comtSetLock
	if (pthread_mutex_init(&comtSetLock,NULL) != 0) {
		logger->log("init: could not initialize comtree set lock",2);
		return false;
	}
	return true;
}

bool bootMe(ipa_t nmIp, ipa_t myIp, fAdr_t& nmAdr, fAdr_t& myAdr,
	    fAdr_t& rtrAdr, ipa_t& rtrIp, ipp_t& rtrPort, uint64_t& nonce) {

	// open boot socket 
	int bootSock = Np4d::datagramSocket();
	if (bootSock < 0) return false;
	if (!Np4d::bind4d(bootSock,myIp,0) || !Np4d::nonblock(bootSock)) {
		close(bootSock);
		return false;
	}

	// setup boot leaf packet to net manager
	Packet p; buffer_t buf1; p.buffer = &buf1;
	CtlPkt cp(CtlPkt::BOOT_LEAF,CtlPkt::REQUEST,1,p.payload());
	int plen = cp.pack();
	if (plen == 0) { close(bootSock); return false; }
	p.length = Forest::OVERHEAD + plen; p.type = Forest::NET_SIG;
	p.flags = 0; p.srcAdr = p.dstAdr = 0; p.comtree = Forest::NET_SIG_COMT;
	p.pack();

	// setup reply packet
	Packet reply; buffer_t buf2; reply.buffer = &buf2;
	CtlPkt repCp;

	int resendTime = Misc::getTime();
	ipa_t srcIp; ipp_t srcPort;
	while (true) {
		int now = Misc::getTime();
		if (now > resendTime) {
			if (Np4d::sendto4d(bootSock,(void *) p.buffer, p.length,
					   nmIp,Forest::NM_PORT) == -1) {
				close(bootSock); return false;
			}
			resendTime += 1000000; // retry after 1 second
		}
		usleep(10000);
		int nbytes = Np4d::recvfrom4d(bootSock,reply.buffer,1500,
					      srcIp, srcPort);
		if (nbytes < 0) continue;
		reply.unpack();

		// do some checking
		if (srcIp != nmIp || reply.type != Forest::NET_SIG) {
			logger->log("unexpected response to boot request",
				     2,reply);
			close(bootSock); return false;
		}
		repCp.reset(reply);
		if (repCp.type != CtlPkt::CONFIG_LEAF ||
		    repCp.mode != CtlPkt::REQUEST) {
			logger->log("unexpected response from NetMgr",2,reply);
			close(bootSock); return false;
		}

		// unpack data from packet
		myAdr = repCp.adr1; rtrAdr = repCp.adr2;
		rtrIp = repCp.ip1; rtrPort = repCp.port1;
		nonce = repCp.nonce;
		nmAdr = reply.srcAdr;

		// send positive reply
		repCp.reset(CtlPkt::CONFIG_LEAF,CtlPkt::POS_REPLY,repCp.seqNum,
			    repCp.payload);
		plen = repCp.pack();
		if (plen == 0) { close(bootSock); return false; }
		reply.length = Forest::OVERHEAD + plen; 
		reply.srcAdr = myAdr; reply.dstAdr = nmAdr;
		reply.pack();
		if (Np4d::sendto4d(bootSock,(void *) reply.buffer, reply.length,
				   nmIp,Forest::NM_PORT) == -1) {
			close(bootSock); return false;
		}
		break; // proceed to next step
	}
	// we have the configuration information, now just wait for
	// final ack
	while (true) {
		int now = Misc::getTime();
		if (now > resendTime) {
			if (Np4d::sendto4d(bootSock,(void *) p.buffer, p.length,
					   nmIp,Forest::NM_PORT) == -1) {
				close(bootSock); return false;
			}
			resendTime += 1000000; // retry after 1 second
		}
		int nbytes = Np4d::recvfrom4d(bootSock,reply.buffer,1500,
					      srcIp, srcPort);
		if (nbytes < 0) { usleep(100000); continue; }
		reply.unpack();
		if (srcIp != nmIp || reply.type != Forest::NET_SIG) {
			logger->log("unexpected response to boot request",
				     2,reply);
			close(bootSock); return false;
		}
		// do some checking
		repCp.reset(reply);
		if (repCp.type == CtlPkt::CONFIG_LEAF &&
		    repCp.mode == CtlPkt::REQUEST) {
			// our reply must have been lost, send it again
			repCp.reset(CtlPkt::CONFIG_LEAF,CtlPkt::POS_REPLY,
				    repCp.seqNum);
			plen = repCp.pack();
			if (plen == 0) { close(bootSock); return false; }
			reply.length = Forest::OVERHEAD + plen; 
			reply.srcAdr = myAdr; reply.dstAdr = nmAdr;
			reply.pack();
			if (Np4d::sendto4d(bootSock,(void *) reply.buffer,
					   reply.length,nmIp,Forest::NM_PORT)
					   == -1) {
				close(bootSock); return false;
			}
		} else if (repCp.type != CtlPkt::BOOT_LEAF ||
			   repCp.mode != CtlPkt::POS_REPLY) {
			logger->log("unexpected response from NetMgr",
				     2,reply);
			close(bootSock); return false;
		}
		break; // success
	}
	close(bootSock); return true;
}

void cleanup() {
	pthread_mutex_destroy(&comtSetLock);
	delete ps; delete comtrees; delete net; delete sub;
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
	CpHandler cph(&inq, &outq, myAdr, logger, ps);

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
			CtlPkt cp(p);
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
			case CtlPkt::COMTREE_PATH:
				success = handleComtPath(px,cp,cph);
				break;
			case CtlPkt::COMTREE_NEW_LEAF:
				success = handleComtNewLeaf(px,cp,cph);
				break;
			case CtlPkt::COMTREE_PRUNE:
				success = handleComtPrune(px,cp,cph);
				break;
			default:
				cph.errReply(px,cp,"invalid control packet "
						   "type for ComtCtl");
				break;
			}
			if (!success) {
				string s;
				logger->log("handler: operation failed",2,p);
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
	NetBuffer buf(sock,1024);
	//const int BLEN = 100;
	//char buf[BLEN+1]; int i;
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
		string word;
		if (!buf.readAlphas(word)) {
			logger->log("handleComtreeDisplay: could not read "
				   "request from remote display",2);
			return true;
		}
		if (word == "getNet") {
			net->lock();
			string netString; net->toString(netString);
			net->unlock();
			if (Np4d::sendString(sock,netString) < 0) {
				logger->log("handleComtreeDisplay: unable to "
					"send network topology to display",2);
				return false;
			}
		} else if (word == "getComtSet") {
			stringstream comtSet;
			comtSet << "comtSet(";
			int count = 0;
			for (int ctx = comtrees->firstComtree(); ctx != 0;
				 ctx = comtrees->nextComtree(ctx)) {
				comtSet << comtrees->getComtree(ctx) << ",";
				count++;
			}
			string s = comtSet.str();
			if (count > 0) s.erase(s.end()-1); // remove last comma
			s += ")\n";
			if (Np4d::sendString(sock,s) < 0) {
				logger->log("handleComtreeDisplay: unable to "
					"send comtree set to display",2);
				return false;
			}
		} else if (word.compare("getComtree") == 0) {
			int comt; string s;
			if (!buf.readInt(comt)) {
				s = "invalid comtree request\n";
			}
			int ctx = comtrees->getComtIndex(comt);
			if (ctx == 0) {
				// send back error message
				s = "invalid comtree request\n";
			} else {
				comtrees->comtStatus2string(ctx,s);
			}
			comtrees->releaseComtree(ctx);
			if (Np4d::sendString(sock,s) < 0) {
				logger->log("handleComtreeDisplay: unable to "
					"send comtree status update to "
					"display",2);
				return false;
			}
		} else {
			logger->log("handleComtreeDisplay: unrecognized request "
			     	    + word + " from comtreeDisplay",2);
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
 *
 */
bool handleAddComtReq(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	if (cp.zipCode != 0) {
		cph.errReply(px,cp,"missing required attribute");
		return true;
	}
	int rootZip = cp.zipCode;

	int comt = newComtreeNum();
	if (comt == 0) {
		cph.errReply(px,cp,"no comtrees available to satisfy request");
		return true;
	}
	int ctx = comtrees->addComtree(comt);
	if (ctx == 0) {
		releaseComtreeNum(comt);
		cph.errReply(px,cp,"internal error prevents adding new "
				   "comtree");
		logger->log("handleAddComt: addComtree() failed due to program "
			"error\n",3);
		return false;
	}
	// comtree ctx is now locked in comtrees

	// find router in specified zipCode
	// if more than one, choose randomly (for now)
	// ultimately, we may want to select the one with the
	// most available capacity on its router-to-router links
	vector<int> matches(100,0); int cnt = 0;
	net->lock();
	for (int rtr = net->firstRouter(); rtr != 0;
		 rtr = net->nextRouter(rtr)) {
		if (Forest::zipCode(net->getNodeAdr(rtr)) == rootZip) {
			matches[cnt++] = rtr;
		}
	}
	net->unlock();
	if (cnt == 0) {
		releaseComtreeNum(comt);
		comtrees->removeComtree(ctx); // releases lock on ctx
		cph.errReply(px,cp,"network contains no router with specified "
				   "zip code");
		return true;
	}
	int rootRtr = matches[randint(0,cnt-1)];
	fAdr_t rootAdr = net->getNodeAdr(rootRtr);
	
	// configure root router to add comtree
	CtlPkt repCp;
	int reply = cph.addComtree(rtrAdr,comt,repCp);
	if (reply == 0 || repCp.mode != CtlPkt::POS_REPLY) {
		releaseComtreeNum(comt);
		comtrees->removeComtree(ctx); // releases lock on ctx
		cph.errReply(px,cp,(reply == 0 ?
				"root router never replied" :
				"root router could not add comtree"));
		if (reply != 0) ps->free(reply);
		return false;
	}
	
	// modify comtree
	reply = cph.modComtree(rtrAdr,comt,0,1,repCp);
	if (reply == 0 || repCp.mode != CtlPkt::POS_REPLY) {
		releaseComtreeNum(comt);
		comtrees->removeComtree(ctx);
		cph.errReply(px,cp,(reply == 0 ?
				"root router never replied" :
				"root router could not modify comtree"));
		if (reply != 0) ps->free(reply);
		return false;
	}
	
	// now update local data structure to reflect addition
	comtrees->addNode(ctx,rootAdr);
	comtrees->addCoreNode(ctx,rootAdr);
	comtrees->setRoot(ctx,rootAdr);
	fAdr_t cliAdr = p.srcAdr;
	comtrees->setOwner(ctx,cliAdr);
	comtrees->releaseComtree(ctx);

	// send positive reply back to client
	repCp.reset(cp.type, CtlPkt::POS_REPLY, cp.seqNum,repCp.payload); 
	cph.sendReply(repCp,p.srcAdr);

	return true;
}

/** Allocate a new comtree number from our assigned range.
 *  The comtSet object is locked during this operation
 *  @return allocated comtree number or 0 if none is available
 */
int newComtreeNum() {
	pthread_mutex_lock(&comtSetLock);
	int comt = comtSet->firstOut();
	if (comt == 0) {
		pthread_mutex_unlock(&comtSetLock);
		return 0;
	}
	comtSet->swap(comt);
	pthread_mutex_unlock(&comtSetLock);
	comt += (firstComt - 1);
	return comt;
}

/** Release a previously allocated comtree number.
 *  The comtSet object is locked during this operation
 *  @param comt is the comtree number to be released
 */
void releaseComtreeNum(int comt) {
	comt -= (firstComt - 1);
	pthread_mutex_lock(&comtSetLock);
	if (comtSet->isIn(comt)) comtSet->swap(comt);
	pthread_mutex_unlock(&comtSetLock);
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
	
	int ctx = comtrees->getComtIndex(comt);
	if (ctx == 0) { // treat this case as a success, already removed
		CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
		cph.sendReply(cp,cliAdr);
		return true;
	}
	if (cliAdr != comtrees->getOwner(ctx)) {
		// eventually should base this decision on actual owner
		// name not just address
		comtrees->releaseComtree(ctx);
		cph.errReply(px,cp,"only the owner can drop a comtree");
		return true;
	}
	
	// first, teardown comtrees at all routers
	for (fAdr_t rtr = comtrees->firstRouter(ctx); rtr != 0;
		    rtr = comtrees->nextRouter(ctx,rtr)) {
		teardownComtNode(ctx,rtr,cph);
	}

	// next, unprovision and remove
	net->lock();
	comtrees->unprovision(ctx);
	net->unlock();
	comtrees->removeComtree(ctx);
	releaseComtreeNum(comt);

	// send positive reply to client
	CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
	cph.sendReply(cp,p.srcAdr);
	return true;
}

/** Handle a comtree path request.
 *  A comtree path request, asks for a path that can be used to
 *  join an existing comtree. We look for a path with sufficient
 *  unused capacity and if successful, return that path in the
 *  form of a vector of router local link numbers.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success; the operation
 *  fails if it cannot allocate required packets, or if no path
 *  can be found between the client's access router and the comtree.
 */
bool handleComtPath(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	fAdr_t cliRtrAdr = p.srcAdr;
	comt_t comt = cp.comtree;

	net->lock();
	int cliRtr = net->getNodeNum(cliRtrAdr);
	if (cliRtr == 0) {
		net->unlock();
		cph.errReply(px,cp,"no such router");
		return true;
	}

	// acquire lock on comtree
	// hold this lock as long as this operation is in progress
	int ctx = comtrees->getComtIndex(comt);
	if (ctx == 0) {
		net->unlock();
		cph.errReply(px,cp,"no such comtree");
		return true;
	}

	RateSpec leafDefRates = comtrees->getDefLeafRates(ctx);
	RateSpec bbDefRates = comtrees->getDefBbRates(ctx);

	bool autoConfig = comtrees->getConfigMode(ctx);
	RateSpec pathRates = (autoConfig ?  leafDefRates : bbDefRates);
	vector<int> path;
	if (!comtrees->findRootPath(ctx,cliRtr,pathRates,path)) {
		net->unlock(); comtrees->releaseComtree(ctx);
		cph.errReply(px,cp,"cannot find path to comtree");
		return true;
	}
	net->unlock(); comtrees->releaseComtree(ctx);

	// send positive reply to client router and return
	CtlPkt repCp(cp.type, CtlPkt::POS_REPLY, cp.seqNum);
	repCp.rspec1 = pathRates; repCp.rspec2 = leafDefRates;
	repCp.ivec = path;
	cph.sendReply(repCp,cliRtrAdr);
	return true;
}

/** Handle a comtree new leaf request.
 *  A new leaf message informs us of the addition of a new leaf,
 *  plus possibly some other nodes. We update the local comtree
 *  data structure to include the new leaf, and update the rate specs
 *  for links.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @return true if the operation is completed successfully
 */
bool handleComtNewLeaf(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	fAdr_t cliRtrAdr = p.srcAdr;
	comt_t comt = cp.comtree;
	fAdr_t cliAdr = cp.adr1;
	fAdr_t branchRtrAdr = cp.adr2;
	vector<int>& path = cp.ivec;

	net->lock();
	int cliRtr = net->getNodeNum(cliRtrAdr);
	if (cliRtr == 0) {
		net->unlock();
		cph.errReply(px,cp,"no such router");
		return true;
	}

	// acquire lock on comtree
	// hold this lock as long as this operation is in progress
	int ctx = comtrees->getComtIndex(comt);
	if (ctx == 0) {
		net->unlock();
		cph.errReply(px,cp,"no such comtree");
		return true;
	}

	if (comtrees->isComtLeaf(ctx,cliAdr)) {
		// if client already in comtree, this is probably a
		// second attempt, caused by a lost acknowledgment
		net->unlock(); comtrees->releaseComtree(ctx);
		CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
		cph.sendReply(repCp,cliAdr);
		return true;
	}

	// should probably do some error checking here

	int len = path.size();
	int r = net->getNodeNum(cliRtrAdr);
	// first find the branch point in the path
	// note: must deal with the case where the branchRtrAdr in the
	// received packet is not currently a node in the local comtree
	// data structure
	int top = len;
	for (int i = 0; i < len; i++) {
		int lnk = net->getLinkNum(r,path[i]);
		fAdr_t radr = net->getNodeAdr(r);
		if ((radr == branchRtrAdr && comtrees->isComtRtr(ctx,radr)) ||
		    ((radr == branchRtrAdr || comtrees->isComtRtr(ctx,radr)) &&
		     top < len)) {
			top = i-1; break;
		}
	}
	RateSpec flipped = cp.rspec2; flipped.flip();
	// now, go down the path adding new routers and their links
	// note: some of these routers may already be in comtree;
	// addNode just returns normally in this case; we may change
	// comtree topology as a result of this process
	for (int i = top; i >= 0; i--) {
		int lnk = net->getLinkNum(r,path[i]);
		fAdr_t radr = net->getNodeAdr(r);
		comtrees->addNode(ctx, radr);
		comtrees->setPlink(ctx, radr, lnk);
		comtrees->getLinkRates(ctx, radr) = flipped;
		// note: may be better to have router return a
		// vector with the available rates on all the links;
		// this would reduce potential for inconsistent
		// values between routers and ComtCtl;
		// still another approach would be to update the
		// rates from link-state updates only and not track
		// the individual changes at all; leave for now
                if (r == net->getLeft(lnk)) {
                	net->getAvailRates(lnk).subtract(flipped);
		} else {
                	net->getAvailRates(lnk).subtract(cp.rspec1);
		}
		r = net->getPeer(r,lnk);
	}
	// finally, add the new leaf
	comtrees->addNode(ctx, cliAdr);
	comtrees->setParent(ctx, cliAdr, cliRtrAdr, cp.link);
	comtrees->getLinkRates(ctx, cliAdr) = cp.rspec1;

	net->unlock(); comtrees->releaseComtree(ctx);

	// send positive reply to router and return
	CtlPkt repCp(cp.type, CtlPkt::POS_REPLY, cp.seqNum);
	cph.sendReply(repCp,cliRtrAdr);
	return true;
}

/** Handle a comtree prune request.
 *  A prune message informs us that either a leaf has been dropped,
 *  or that a router is removing itself from the comtree.
 *  Update the local comtree data structures to reflect the change.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @return true if the operation is completed successfully
 */
bool handleComtPrune(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	fAdr_t rtrAdr = p.srcAdr;
	comt_t comt = cp.comtree;
	fAdr_t pruneAdr = cp.adr1;

	net->lock();
	int rtr = net->getNodeNum(rtrAdr);
	if (rtr == 0) {
		net->unlock();
		cph.errReply(px,cp,"no such router");
		return true;
	}

	// acquire lock on comtree
	// hold this lock as long as this operation is in progress
	int ctx = comtrees->getComtIndex(comt);
	if (ctx == 0) {
		net->unlock();
		cph.errReply(px,cp,"no such comtree");
		return true;
	}

	if (comtrees->isComtLeaf(ctx,pruneAdr)) {
		// if pruneAdr is a leaf, remove it
		int llnk = comtrees->getPlink(ctx,pruneAdr);
		int lnk = net->getLinkNum(rtr,llnk);
		net->getAvailRates(lnk).add(
			comtrees->getLinkRates(ctx,pruneAdr));
		comtrees->removeNode(ctx,pruneAdr);
	} else if (comtrees->isComtRtr(ctx,pruneAdr)) {
		if (pruneAdr != rtrAdr) {
			net->unlock(); comtrees->releaseComtree(ctx);
			cph.errReply(px,cp,"cannot prune a different router");
			return true;
		}
		if (comtrees->getLinkCnt(ctx,rtrAdr) == 1) {
			int lnk = comtrees->getPlink(ctx,rtrAdr);
			RateSpec rs = comtrees->getLinkRates(ctx,rtrAdr);
			if (rtr != net->getLeft(lnk)) rs.flip();
			net->getAvailRates(lnk).add(rs);
			comtrees->removeNode(ctx,rtrAdr);
		} else { // should not happen, but still need to handle it
			// remove entire subtree
			// write separate method for this
			// requires identifying comtree routers in subtree
			// then removing them and their leaf nodes; ugly
		}
	}
	net->unlock(); comtrees->releaseComtree(ctx);

	// send positive reply to router and return
	CtlPkt repCp(cp.type, CtlPkt::POS_REPLY, cp.seqNum);
	cph.sendReply(repCp,rtrAdr);
	return true;
}

/* not quite ready yet
void removeRtr(int ctx, rtrAdr) {
	set<fAdr_t> dropset;
	dropset.add(rtrAdr);
	if (comtrees->getLinkCnt(ctx,rtrAdr) > 1) {
		// remove all leaf children
		for (fAdr_t ladr = comtrees->firstLeaf(ctx); ladr != 0;
             		    ladr = comtrees-nextLeaf(ctx,ladr)) {
			if (comtrees->getParent(ctx,ladr) != rtrAdr) continue;
			int llnk = comtrees->getPlink(ctx,ladr);
			int lnk = net->getLinkNum(rtr,llnk);
			net->getAvailRates(lnk).add(
				comtrees->getLinkRates(ctx,ladr));
			comtrees->removeNode(ctx,ladr);
		}
	}
	if (comtrees->getLinkCnt(ctx,rtrAdr) > 1) {
		// if still have children, find and remove
		for (fAdr_t cadr = comtrees->firstRouter(ctx); cadr != 0;
             		    cadr = comtrees-nextRouter(ctx,cadr)) {
			if (comtrees->getParent(ctx,cadr) != rtrAdr) continue;
			removeRtr(ctx,cadr);
		}
	}
	int lnk = comtrees->getPlink(ctx,rtrAdr);
	RateSpec rs = comtrees->getLinkRates(ctx,rtrAdr);
	if (rtr != net->getLeft(lnk)) rs.flip();
	net->getAvailRates(lnk).add(rs);
	comtrees->removeNode(ctx,rtrAdr);
}
*/

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

	// find the client's router, based on its forest address
	int cliRtr;
	net->lock();
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
		net->unlock();
		cph.errReply(px,cp,"can't find client's access router");
		return false;
	}
	fAdr_t cliRtrAdr = net->getNodeAdr(cliRtr);
	net->unlock();

	// acquire lock on comtree
	// hold this lock as long as this operation is in progress
	int ctx = comtrees->getComtIndex(comt);
	if (ctx == 0) {
		cph.errReply(px,cp,"no such comtree");
		return true;
	}

	if (comtrees->isComtLeaf(ctx,cliAdr)) {
		// if client already in comtree, this is probably a
		// second attempt, caused by a lost acknowledgment
		comtrees->releaseComtree(ctx);
		CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
		cph.sendReply(repCp,cliAdr);
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
		net->lock(); // lock net while searching for path
		branchRtr = comtrees->findPath(ctx,cliRtr,pathRates,path);
		if (branchRtr == 0 || tryCount++ > 3) {
			net->unlock();
			comtrees->releaseComtree(ctx);
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
				net->unlock();
				comtrees->releaseComtree(ctx);
				cph.errReply(px,cp,"cannot add required "
					     "capacity to comtree backbone");
				return true;
			}
		}
		net->unlock();
		// we now have the path reserved in the internal data structure,
		// so no other thread can interfere with completion
	
		// now configure routers on path and exit loop if successful
		if (!setupPath(ctx,path,cph)) {
			// could not configure path at some router
			teardownPath(ctx,path,cph);
			net->lock();
			if (autoConfig) comtrees->unprovision(ctx,modList);
			leafDefRates.negate();
			comtrees->adjustSubtreeRates(
				ctx,cliRtrAdr,leafDefRates);
			leafDefRates.negate();
			comtrees->removePath(ctx,path);
			net->unlock();
		} else if (autoConfig &&
			   !modComtRates(ctx,modList,false,cph)) {
			net->lock();
			comtrees->unprovision(ctx,modList);
			modComtRates(ctx,modList,true,cph);
			leafDefRates.negate();
			comtrees->adjustSubtreeRates(
				ctx,cliRtrAdr,leafDefRates);
			leafDefRates.negate();
			comtrees->removePath(ctx,path);
			net->unlock();
			teardownPath(ctx,path,cph);
		} else { // all routers successfully configured
			break;
		}
		path.clear(); modList.clear();
	}

	// now add client to comtree
	int llnk = setupClientLink(ctx,cliAdr,cliRtr,cph);
	comtrees->addNode(ctx,cliAdr);
	comtrees->setParent(ctx,cliAdr,cliRtrAdr,llnk);
	comtrees->getLinkRates(ctx,cliAdr) = leafDefRates;

	if (llnk == 0 || !setComtLeafRates(ctx,cliAdr,cph)) {
		// tear it all down and fail
		comtrees->removeNode(ctx,cliAdr);
		net->lock();
		comtrees->unprovision(ctx,modList);
		modComtRates(ctx,modList,true,cph);
		leafDefRates.negate();
		comtrees->adjustSubtreeRates(
			ctx,cliRtrAdr,leafDefRates);
		comtrees->removePath(ctx,path);
		net->unlock();
		leafDefRates.negate();
		teardownPath(ctx,path,cph);
		comtrees->releaseComtree(ctx);
		cph.errReply(px,cp,"cannot configure leaf node");
		return true;
	}
	comtrees->releaseComtree(ctx);

	// finally, send positive reply to client and return
	CtlPkt repCp(cp.type,CtlPkt::POS_REPLY, cp.seqNum);
	cph.sendReply(repCp,cliAdr);
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

	// find the client's router, based on its forest address
	int cliRtr;
	net->lock();
	for (cliRtr = net->firstRouter(); cliRtr != 0;
	     cliRtr = net->nextRouter(cliRtr)) {
		pair<int,int> leafRange; net->getLeafRange(cliRtr,leafRange);
		if (cliAdr >= leafRange.first && cliAdr <= leafRange.second)
			break;
	}
	if (cliRtr == 0) {
		cph.errReply(px,cp,"can't find client's access router");
		logger->log("handleLeaveComt: cannot find client's access "
			   "router in network topology\n",2,p);
		return false;
	}
	fAdr_t cliRtrAdr = net->getNodeAdr(cliRtr);
	net->unlock();

	// acquire the comtree lock
	int ctx = comtrees->getComtIndex(comt);
	if (ctx == 0) {
		cph.errReply(px,cp,"no such comtree");
		return true;
	}

	if (!comtrees->isComtLeaf(ctx,cliAdr)) {
		// if client is not in comtree, this is probably a
		// second attempt, caused by a lost acknowledgment
		comtrees->releaseComtree(ctx);
		CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
		cph.sendReply(repCp,cliAdr);
		return true;
	}
	teardownClientLink(ctx,cliAdr,cliRtr,cph);

	// reduce subtree rates in response to dropped client
	// then remove client from comtrees
	RateSpec rs;
	rs = comtrees->getLinkRates(ctx,cliAdr); rs.negate();
	net->lock();
	comtrees->adjustSubtreeRates(ctx,cliRtrAdr,rs);
	comtrees->removeNode(ctx,cliAdr);

	// for autoConfig case, modify backbone rates
	if (comtrees->getConfigMode(ctx)) {
		list<LinkMod> modList;
		// note, since we've adjusted subtree rates, computeMods
		// returns negative rates, so provisioning these rates
		// effectively reduces allocation
		comtrees->computeMods(ctx,modList);
		comtrees->provision(ctx,modList);
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
	comtrees->removePath(ctx,path);
	net->unlock();
	teardownPath(ctx,path,cph);
	comtrees->releaseComtree(ctx);
	
	// send positive reply and return
	CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
	cph.sendReply(repCp,cliAdr);
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
	for (LinkMod& lm : path) {
		if (!setupComtNode(ctx,lm.child,cph)) {
			return false;
		}
	}
	
	// Next, add link at all routers and configure comtree attributes
	for (LinkMod& lm : path) {
		int parent = net->getPeer(lm.child,lm.lnk);
		if (!setupComtLink(ctx,lm.lnk,lm.child,cph)) {
			return false;
		}
		if (!setupComtLink(ctx,lm.lnk,parent,cph)) {
			return false;
		}
		if (!setupComtAttrs(ctx,lm.child,cph)) {
			return false;
		}
		if (!setComtLinkRates(ctx,lm.lnk,lm.child,cph)) {
			return false;
		}
		if (!setComtLinkRates(ctx,lm.lnk,parent,cph)) {
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
	bool status = true;
	for (LinkMod& lm : path) {
		if (!teardownComtNode(ctx,lm.child,cph))
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
	CtlPkt repCp;
	int reply = cph.addComtree(net->getNodeAdr(rtr),
				   comtrees->getComtree(ctx),repCp);
	if (reply == 0) return false;
	ps->free(reply);
	return repCp.mode == CtlPkt::POS_REPLY;
} 

/** Remove a comtree at a router
 *  @param ctx is a valid comtree index
 *  @param rtr is a node number for a router
 *  @return true on success, false on failure
 */
bool teardownComtNode(int ctx, int rtr, CpHandler& cph) {
	CtlPkt repCp;
	int reply = cph.dropComtree(net->getNodeAdr(rtr),
				    comtrees->getComtree(ctx),repCp);
	if (reply == 0) return false;
	ps->free(reply);
	return repCp.mode == CtlPkt::POS_REPLY;
} 

/** Configure a comtree link at a router.
 *  @param ctx is a valid comtree index
 *  @param link is a link number for a link in the comtree
 *  @param rtr is a node number for a router
 *  @return true on success, false on failure
 */
bool setupComtLink(int ctx, int lnk, int rtr, CpHandler& cph) {
	int parent = net->getPeer(rtr,lnk);
	CtlPkt repCp;
	int reply = cph.addComtreeLink(net->getNodeAdr(rtr),
				   	comtrees->getComtree(ctx),
				   	net->getLLnum(lnk,rtr),
				   	comtrees->isCoreNode(ctx,parent),repCp);
	if (reply == 0) return false;
	ps->free(reply);
	return repCp.mode == CtlPkt::POS_REPLY;
} 

/** Configure a comtree link to a client at a router.
 *  @param ctx is a valid comtree index
 *  @param cliAdr is the forest address of the client
 *  @param rtr is a node number for a router
 *  @return the local link number assigned by the router on success,
 *  0 on failure
 */
int setupClientLink(int ctx, fAdr_t cliAdr, int rtr, CpHandler& cph) {
	CtlPkt repCp;
	pktx reply = cph.addComtreeLink(net->getNodeAdr(rtr),
					comtrees->getComtree(ctx),
				   	cliAdr, repCp);
	if (reply == 0) return 0;
	ps->free(reply);
	return (repCp.mode == CtlPkt::POS_REPLY ? repCp.link : 0);
} 

/** Teardown a comtree link to a client at a router.
 *  @param ctx is a valid comtree index
 *  @param cliAdr is the forest address of the client
 *  @param rtr is a node number for the client's access router
 *  @return the true on success, false on failure
 */
bool teardownClientLink(int ctx, fAdr_t cliAdr, int rtr, CpHandler& cph) {
	CtlPkt repCp;
	int reply = cph.dropComtreeLink(net->getNodeAdr(rtr),
					comtrees->getComtree(ctx),
				   	0, cliAdr, repCp);
	if (reply == 0) return false;
	ps->free(reply);
	return repCp.mode == CtlPkt::POS_REPLY;
} 

/** Configure comtree attributes at a router.
 *  @param ctx is a valid comtree index
 *  @param rtr is a node number for a router
 *  @return true on success, false on failure
 */
bool setupComtAttrs(int ctx, int rtr, CpHandler& cph) {
	fAdr_t rtrAdr = net->getNodeAdr(rtr);
	int llnk = net->getLLnum(comtrees->getPlink(ctx,rtrAdr),rtr);
	CtlPkt repCp;
	int reply = cph.modComtree(rtrAdr, comtrees->getComtree(ctx), llnk,
				   comtrees->isCoreNode(ctx,rtrAdr),repCp);
	if (reply == 0) return false;
	ps->free(reply);
	return repCp.mode == CtlPkt::POS_REPLY;
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
	CtlPkt repCp;
	int reply = cph.modComtreeLink( rtrAdr,
				   	comtrees->getComtree(ctx),
					net->getLLnum(lnk,rtr),rs,repCp);
	if (reply == 0) return false;
	ps->free(reply);
	if (repCp.mode == CtlPkt::POS_REPLY) return true;

	// router rejected, probably because available rate at router
	// is different from information stored in comtrees object;
	// such differences can occur, when we have multiple comtree
	// controllers; request available rate at router and use it
	// to update local information
	reply = cph.getLink(rtrAdr,net->getLLnum(lnk,rtr),repCp);
	if (reply == 0) return false;
	ps->free(reply);
	if (repCp.mode != CtlPkt::POS_REPLY) return false;
	if (repCp.rspec2.isSet()) {
		if (rtr == net->getLeft(lnk)) repCp.rspec2.flip();
		net->getAvailRates(lnk) = repCp.rspec2;
	}
	return false;
}

/** Set the comtree link rates at a leaf.
 *  @param ctx is a valid comtree index
 *  @param leafAdr is the forest address of a leaf whose rate is to be
 *  configured
 *  @return true on success, false on failure
 */
bool setComtLeafRates(int ctx, fAdr_t leafAdr, CpHandler& cph) {
	CtlPkt repCp;
	int reply = cph.modComtreeLink( comtrees->getParent(ctx, leafAdr),
				   	comtrees->getComtree(ctx),
				   	comtrees->getPlink(ctx,leafAdr),
					comtrees->getLinkRates(ctx,leafAdr),
				        repCp);
	if (reply == 0) return false;
	ps->free(reply);
	return repCp.mode == CtlPkt::POS_REPLY;
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
bool modComtRates(int ctx, list<LinkMod>& modList, bool nostop,CpHandler& cph) {
	for (LinkMod& lm : modList) {
		if (!nostop && !setComtLinkRates(ctx,lm.lnk,lm.child,cph))
			return false;
		int parent = net->getPeer(lm.child,lm.lnk);
		if (!nostop && !setComtLinkRates(ctx,lm.lnk,parent,cph))
			return false;
	}
	return true;
}

} // ends namespace
