/** @file NetMgr.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "NetMgr.h"
#include "CpHandler.h"

using namespace forest;

/** usage:
 *       NetMgr topoFile prefixFile finTime
 * 
 *  The first argument is the topology file (aka NetInfo file) that
 *  describes the network topology.
 *  PrefixFile is a file that maps IP prefixes of clients to forest routers.
 *  FinTime is the number of seconds to run.
 *  If zero, the NetMgr runs forever.
 */
int main(int argc, char *argv[]) {
	uint32_t finTime;

	if (argc != 4 ||
	     sscanf(argv[3],"%d", &finTime) != 1)
		fatal("usage: NetMgr myIp topoFile prefixFile finTime");
	if (!init(argv[1])) {
		fatal("NetMgr: initialization failure");
	}
	if (!readPrefixInfo(argv[2]))
		fatal("can't read prefix address info");
	sub->run(finTime);
	exit(0);
}

namespace forest {

/** Initialization of NetMgr.
 *  @return true on success, false on failure
 */
bool init(const char *topoFile) {
	Misc::getTimeNs(); // initialize time reference
	int nPkts = 10000;
	ps = new PacketStoreTs(nPkts+1);

	logger = new Logger();

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

	// mark all routers as DOWN
	for (int rtr = net->firstRouter(); rtr != 0;
		 rtr = net->nextRouter(rtr)) {
		net->setStatus(rtr,NetInfo::DOWN);
	}

	// find node information for netMgr and cliMgr and initialize
	// associated data
	myAdr = 0; cliMgrAdr = comtCtlAdr = 0;
	ipa_t rtrIp;
	for (int c = net->firstController(); c != 0;
		 c = net->nextController(c)) {
		net->setStatus(c,NetInfo::DOWN);
		string s;
		if (net->getNodeName(c,s) == "netMgr") {
			myIp = net->getLeafIpAdr(c);
			myAdr = net->getNodeAdr(c);
			int lnk = net->firstLinkAt(c);
			int rtr = net->getPeer(c,lnk);
			int llnk = net->getLLnum(lnk,rtr);
			int iface = net->getIface(llnk,rtr);
			if (iface == 0) {
				cerr << "NetMgr:init: can't find ip address "
				     << "of access router\n";
			}
			netMgr = c; nmRtr = rtr;
			rtrIp = net->getIfIpAdr(rtr,iface);
			rtrAdr = net->getNodeAdr(rtr);
		} else if (net->getNodeName(c,s) == "cliMgr") {
			cliMgrAdr = net->getNodeAdr(c);
		} else if (net->getNodeName(c,s) == "comtCtl") {
			comtCtlAdr = net->getNodeAdr(c);
		}
	}

	ipp_t rtrPort = 0; // until router boots
	uint64_t nonce = 0; // likewise
	sub = new Substrate(myAdr,myIp,rtrAdr,rtrIp,rtrPort,nonce,500,
			handler, Forest::NM_PORT, Forest::NM_PORT, ps, logger);
	if (!sub->init()) {
		logger->log("init: can't initialize substrate",2);
		return false;
	}
	sub->setRtrReady(false);

	if (myAdr == 0 || cliMgrAdr == 0) {
		cerr << "could not find netMgr or cliMgr in topology file\n";
		return false;
	}
	return true;
}

void cleanup() {
	delete ps; delete net; delete comtrees; delete sub;
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
	Queue& inq = ((Substrate::QueuePair *) qp)->in;
	Queue& outq = ((Substrate::QueuePair *) qp)->out;
	CpHandler cph(&inq, &outq, myAdr, logger, ps);

	while (true) {
		pktx px = inq.deq();
		bool success = false;
		if (px < 0) {
			// in this case, p is really a negated socket number
			// for a remote client
			int sock = -px;
			success = handleConsole(sock, cph);
		} else {
			Packet& p = ps->getPacket(px);
			CtlPkt cp(p.payload(),p.length-Forest::OVERHEAD);
			cp.unpack();
			switch (cp.type) {
			case CtlPkt::CLIENT_CONNECT:
			case CtlPkt::CLIENT_DISCONNECT:
				success = handleConDisc(px,cp,cph);
				break;
			case CtlPkt::NEW_SESSION:
				success = handleNewSession(px,cp,cph);
				break;
			case CtlPkt::BOOT_LEAF:
				cph.setTunnel(p.tunIp,p.tunPort);
				// allow just one node to boot at a time
				net->lock();
				success = handleBootLeaf(px,cp,cph);
				net->unlock();
				cph.setTunnel(0,0);
				break;
			case CtlPkt::BOOT_ROUTER:
				cph.setTunnel(p.tunIp,p.tunPort);
				// allow just one node to boot at a time
				net->lock();
				success = handleBootRouter(px,cp,cph);
				net->unlock();
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
			     << ps->getPacket(px).toString(s);
		}
		ps->free(px); // release px now that we're done
		outq.enq(0); // signal completion to main thread
	}
}

/** Handle a connection from the remote console.
 *  This involves forwarding the packet to a remote router.
 *  @param px is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param return true on successful completion, else false
 */
bool handleConsole(int sock, CpHandler& cph) {
        NetBuffer buf(sock,1024);

	// login user at remote console using password in netMgrUsers file

	// command processing loop
	string cmd, reply;
	while (buf.readAlphas(cmd)) {
		reply = "success";
		if (cmd == "getNet") {
			getNet(buf,reply);
		} else if (cmd == "getLinkTable") {
			getLinkTable(buf,reply);
		// more of these (getIfaceTable, getComtree,...)
		} else if (cmd == "over" && buf.nextLine()) {
			// ignore
		} else if (cmd == "overAndOut" && buf.nextLine()) {
			break;
		} else {
			reply = "unrecognized input";
		}
		if (sock == -1) break;
		reply += "\nover\n";
		Np4d::sendString(sock,reply);
	}
        return true;
}

void getNet(NetBuffer& buf, string& reply) {
	// return net->toString(s) where net is the netInfo object
}

void getLinkTable(NetBuffer& buf, string& reply) {
	// get list of links from router
	// for each link in list, get the link table entry from router
	// return each link to the Console, with one link per line,
	// each line starting with the word link, followed by the
	// link number and remaining fields, with spaces separating items
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
	cph.sendReply(repCp,p.srcAdr);

	// now, send notification to client manager
	pktx reply;
	if (cp.type == CtlPkt::CLIENT_CONNECT)
		reply = cph.clientConnect(cliMgrAdr,cp.adr1,p.srcAdr,repCp);
	else
		reply = cph.clientDisconnect(cliMgrAdr,cp.adr1,p.srcAdr,repCp);
	if (reply == 0) return false;
	ps->free(reply);
	return true;
}

/** Handle a new session request.
 *  This requires selecting a router and sending a sequence of
 *  control messages to configure the router to handle a new
 *  client connection.
 *  The operation can fail for a variety of reasons, at different
 *  points in the process. Any failure causes a negative reply
 *  to be sent back to the original requestor.
 *  @param px is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param cph is the control packet handler for this thread
 *  @return true if the operation is completed successfully, else false
 */
bool handleNewSession(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	// determine which router to use
	fAdr_t rtrAdr;
	if (!findCliRtr(cp.ip1,rtrAdr)) {
		cph.errReply(px,cp,"No router assigned to client's IP");
		return true;
	}
	int rtr = net->getNodeNum(rtrAdr);
	RateSpec clientRates = cp.rspec1;

	// find first iface for this router - refine this later
	int iface = 1;
	while (iface < net->getNumIf(rtr) && !net->validIf(rtr,iface)) iface++;

	// generate nonce to be used by client in connect packet
	uint64_t nonce = generateNonce();

	fAdr_t clientAdr = setupLeaf(0,px,cp,rtr,iface,nonce,cph);
	if (clientAdr == 0) return false;

	// send positive reply back to sender
	CtlPkt repCp(CtlPkt::NEW_SESSION,CtlPkt::POS_REPLY,cp.seqNum);
        repCp.adr1 = clientAdr; repCp.adr2 = rtrAdr; repCp.adr3 = comtCtlAdr;
	repCp.ip1 = net->getIfIpAdr(rtr,iface);
	repCp.port1 = net->getIfPort(rtr,iface);
	repCp.nonce = nonce;
	cph.sendReply(repCp,p.srcAdr);
	return true;
}

/** Setup a leaf by sending configuration packets to its access router.
 *  @param leaf is the node number for the leaf if this is a pre-configured
 *  leaf for which NetInfo object specifies various leaf parameters;
 *  it is 0 for leaf nodes that are added dynamically
 *  @param px is the packet number of the original request packet that
 *  initiated the setup process
 *  @param cp is the corresponding control packet (already unpacked)
 *  @param rtr is the node number of the access router for the new leaf
 *  @param iface is the interface of the access router where leaf connects
 *  @param nonce is the nonce that the leaf will use to connect
 *  @param cph is a reference to the control packet handler for the
 *  thread handling this operation
 *  @param useTunnel is an optional parameter that defaults to false;
 *  when it is true, it specifies that control packets should be
 *  addressed to the tunnel (ip,port) configured in the cph;
 *  @return the forest address of the new leaf on success, else 0
 */
fAdr_t setupLeaf(int leaf, pktx px, CtlPkt& cp, int rtr, int iface,
		 uint64_t nonce, CpHandler& cph, bool useTunnel) {

	Forest::ntyp_t leafType; int leafLink; fAdr_t leafAdr; RateSpec rates;

	// first add the link at the router
	fAdr_t rtrAdr = net->getNodeAdr(rtr);
	fAdr_t dest = (useTunnel ? 0 : rtrAdr);
	pktx reply; CtlPkt repCp;
	if (leaf == 0) { // dynamic client
		leafType = Forest::CLIENT; leafLink = 0;
		leafAdr = 0; rates = cp.rspec1;
		reply = cph.addLink(dest,leafType,iface,nonce,repCp);
		if (!processReply(px,cp,reply,repCp,cph,
				  "could not add link to leaf"))
			return 0;
		leafLink = repCp.link; leafAdr = repCp.adr1;
	} else { // static leaf with specified parameters
		leafType = net->getNodeType(leaf);
		int lnk = net->firstLinkAt(leaf);
		leafLink = net->getLLnum(lnk,rtr);
		leafAdr = net->getNodeAdr(leaf);
		rates = net->getLinkRates(lnk);
		ipa_t leafIp = net->getLeafIpAdr(leaf);
		reply = cph.addLink(dest,leafType,iface,leafLink,
				    leafIp,0,leafAdr,nonce,repCp);
		if (!processReply(px,cp,reply,repCp,cph,
				  "could not add link to leaf"))
			return 0;
	}

	// next set the link rate
	reply = cph.modLink(dest,leafLink,rates,repCp);
	if (!processReply(px,cp,reply,repCp,cph,"could not set link rates"))
		return 0;

	// now add the new leaf to the leaf connection comtree
	reply = cph.addComtreeLink(dest,Forest::CONNECT_COMT,
				   leafLink,-1,repCp);
	if (!processReply(px,cp,reply,repCp,cph,"could not add leaf to "
			  "connection comtree"))
		return 0;

	// now modify comtree link rate
	int ctx = comtrees->getComtIndex(Forest::CONNECT_COMT);
	rates = comtrees->getDefLeafRates(ctx);
	comtrees->releaseComtree(ctx);
	reply = cph.modComtreeLink(dest,Forest::CONNECT_COMT,leafLink,
				   rates,repCp);
	if (!processReply(px,cp,reply,repCp,cph,"could not set rate on "
			  "connection comtree"))
		return 0;

	// now add the new leaf to the client signaling comtree
	reply = cph.addComtreeLink(dest,Forest::CLIENT_SIG_COMT,
				   leafLink,-1,repCp);
	if (!processReply(px,cp,reply,repCp,cph,"could not add leaf to client "
			 "signalling comtree"))
		return 0;

	// and modify its comtree link rate
	ctx = comtrees->getComtIndex(Forest::CLIENT_SIG_COMT);
	rates = comtrees->getDefLeafRates(ctx);
	comtrees->releaseComtree(ctx);
	reply = cph.modComtreeLink(dest,Forest::CLIENT_SIG_COMT,leafLink,
				   rates,repCp);
	if (!processReply(px,cp,reply,repCp,cph,"could not set rate on client "
			  "signaling comtree"))
		return 0;

	if (leafType == Forest::CLIENT) return leafAdr;

	// for controllers, also add to the network signaling comtree
	reply = cph.addComtreeLink(dest,Forest::NET_SIG_COMT,
				   leafLink,-1,repCp);
	if (!processReply(px,cp,reply,repCp,cph,"could not add leaf to network "
			 "signalling comtree"))
		return 0;

	// and modify comtree link rate
	ctx = comtrees->getComtIndex(Forest::NET_SIG_COMT);
	rates = comtrees->getDefLeafRates(ctx);
	comtrees->releaseComtree(ctx);
	reply = cph.modComtreeLink(dest,Forest::NET_SIG_COMT,leafLink,
				   rates,repCp);
	if (!processReply(px,cp,reply,repCp,cph,"could not set rate on network "
			  "signaling comtree"))
		return 0;
	return leafAdr;
}

/** Handle boot process for a pre-configured leaf node.
 *  This handler is intended primarily for controllers.
 *  It first configures the router, so it will accept new leaf.
 *  It then sends leaf configuration information to a new leaf node,
 *  including address information for it, and its router,
 *  as well as a nonce to be used when it connects.
 *  @param px is the index of the incoming boot request packet
 *  @param cp is the control packet (already unpacked)
 *  @param cph is a reference to the control packet handler
 *  used to send packets out
 */
bool handleBootLeaf(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);

	// first, find leaf in NetInfo object
	int leaf;
	for (leaf = net->firstLeaf(); leaf != 0; leaf = net->nextLeaf(leaf)) {
		if (net->getLeafIpAdr(leaf) == p.tunIp) break;
	}
	if (leaf == 0) {
		cph.errReply(px,cp,"unknown leaf address");
		return false;
	}

	if (net->getStatus(leaf) == NetInfo::UP) {
		// final reply lost or delayed, resend and quit
		CtlPkt repCp(CtlPkt::BOOT_LEAF,CtlPkt::POS_REPLY,cp.seqNum);
		cph.sendReply(repCp,0);
		return true;
	}

	int lnk = net->firstLinkAt(leaf);
	int rtr = net->getPeer(leaf,lnk);
	fAdr_t rtrAdr = net->getNodeAdr(rtr);

	net->setStatus(leaf,NetInfo::BOOTING);

	if (net->getStatus(rtr) != NetInfo::UP) {
		cph.errReply(px,cp,"access router is not yet up");
		net->setStatus(leaf,NetInfo::DOWN);
		return false;
	}

	// find first iface for this router - refine this later
	int iface = 1;
	while (iface < net->getNumIf(rtr) && !net->validIf(rtr,iface)) iface++;
	uint64_t nonce = generateNonce();

	// add link at router and configure rates/comtrees
	CtlPkt repCp;
	if (setupLeaf(leaf,px,cp,rtr,iface,nonce,cph) == 0) {
		net->setStatus(leaf,NetInfo::DOWN);
		return false;
	}

	// Send configuration parameters to leaf
	// 0 destination address tells cph/substrate to send using
 	// cph's tunnel parameters; this sends it to leaf
	int reply = cph.configLeaf(0,net->getNodeAdr(leaf),rtrAdr,
				   net->getIfIpAdr(rtr,iface),
		   		   net->getIfPort(rtr,iface), nonce, repCp);
	if (!processReply(px,cp,reply,repCp,cph,"could not configure "
			  "leaf node")) {
		net->setStatus(leaf,NetInfo::DOWN);
		return false;
	}

	// finally, send positive ack
	repCp.reset(CtlPkt::BOOT_LEAF,CtlPkt::POS_REPLY,cp.seqNum);
	cph.sendReply(repCp,0); // 0 sends it directly back to leaf
	net->setStatus(leaf,NetInfo::UP);

	logger->log("completed leaf boot request",0,p);
	return true;
}

/** Return a random nonce suitable for use when connecting a leaf.  */
uint64_t generateNonce() {
	uint64_t nonce;
	do {
		nonce = Misc::currentTime(); // in seconds since epoch
		nonce *= random();
	} while (nonce == 0);
	return nonce;
}

/** Process the reply from an outgoing request packet
 *  This method implements a common pattern used by handlers
 *  that are configuring routers in response to a received request.
 *  The handler typically sends out a series of request packets and must
 *  then handle each reply. A missing or negative reply causes a negative 
 *  response to be sent back to the sender of the original request.
 *  @param px is the packet index of the incoming request that
 *  initiated the handler
 *  @param cp is the unpacked control packet for px
 *  @param reply is the packet index for a reply to some outgoing
 *  control packet that was sent by this handler
 *  @param repCp is a reference to the control packet for reply
 *  @param cph is a reference to this handler's control packet handler
 *  @param msg is an error message to be sent back to the original
 *  sender of px, if reply=0 or the associated control packet indicates
 *  a failure
 *  @return true if the reply is not zero and the control packet for
 *  reply indicates that the requested operation succeeded
 */
bool processReply(pktx px, CtlPkt& cp, pktx reply, CtlPkt& repCp,
		  CpHandler& cph, const string& msg) {
	if (reply == 0) {
		cph.errReply(px,cp,msg + " (no response from target)");
		ps->free(reply);
		return false;
	}
	if (repCp.mode != CtlPkt::POS_REPLY) {
		string s;
		cph.errReply(px,cp,msg + " (" + repCp.toString(s) + ")");
		ps->free(reply);
		return false;
	}
	ps->free(reply);
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
bool handleBootRouter(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);

	// find rtr index based on source address
	ipa_t rtrAdr = p.srcAdr;
string s;
cerr << "got boot request from " << Forest::fAdr2string(rtrAdr,s) << endl;
	int rtr;
	for (rtr = net->firstRouter(); rtr != 0; rtr = net->nextRouter(rtr)) {
		if (net->getNodeAdr(rtr) == rtrAdr) break;
	}
	if (rtr == 0) {
		cph.errReply(px,cp,"boot request from unknown router "
				   "rejected\n");
		logger->log("handleBootRequest: received boot request from "
			   "unknown router\n",2,p);
		return true;
	}

cerr << "A\n";
	if (net->getStatus(rtr) == NetInfo::UP) {
		// final reply lost or delayed, resend and quit
		CtlPkt repCp(CtlPkt::BOOT_ROUTER,CtlPkt::POS_REPLY,cp.seqNum);
		cph.sendReply(repCp,0);
		return true;
	}

cerr << "B\n";
	net->setStatus(rtr,NetInfo::BOOTING);

	// configure leaf address range
	CtlPkt repCp;
	pair<fAdr_t,fAdr_t> leafRange; net->getLeafRange(rtr,leafRange);
	int reply = cph.setLeafRange(0,leafRange.first,leafRange.second,repCp);
	if (!processReply(px,cp,reply,repCp,cph,
			  "could not configure leaf range")) {
		net->setStatus(rtr,NetInfo::DOWN);
		return false;
	}

cerr << "C\n";
	// add/configure interfaces
	// for each interface in table, do an add iface
	int nmLnk = net->getLLnum(net->firstLinkAt(netMgr),nmRtr);
	int nmIface = net->getIface(nmLnk,nmRtr);
	for (int i = 1; i <= net->getNumIf(rtr); i++) {
		if (!net->validIf(rtr,i)) continue;
		int reply = cph.addIface(0,i,net->getIfIpAdr(rtr,i),
				         net->getIfRates(rtr,i),repCp);
		if (!processReply(px,cp,reply,repCp,cph,
				  "could not add interface at router")) {
			net->setStatus(rtr,NetInfo::DOWN);
			return false;
		}
		net->setIfPort(rtr,i,repCp.port1);
		// if this is the network manager's router, configure port
		// in substrate
		if (rtr == nmRtr && i == nmIface) {
			sub->setRtrPort(repCp.port1);
		}
	}

	// add/configure links to other routers
	for (int lnk = net->firstLinkAt(rtr); lnk != 0;
		 lnk = net->nextLinkAt(rtr,lnk)) {
		int peer = net->getPeer(rtr,lnk);
		if (net->getNodeType(peer) != Forest::ROUTER) continue;
		if (!setupEndpoint(lnk,rtr,px,cp,cph,true)) {
			net->setStatus(rtr,NetInfo::DOWN);
			return false;
		}
	}

	// add/configure comtrees
	for (int ctx = comtrees->firstComtree(); ctx != 0;
		 ctx = comtrees->nextComtree(ctx)) {
		fAdr_t rtrAdr = net->getNodeAdr(rtr);
		if (!comtrees->isComtNode(ctx,rtrAdr)) continue;
		if (!setupComtree(ctx,rtr,px,cp,cph,true)) {
			comtrees->releaseComtree(ctx);
			net->setStatus(rtr,NetInfo::DOWN);
			return false;
		}
	}

	// if this is net manager's router, configure link
	if (rtr == nmRtr) {
		uint64_t nonce = generateNonce();
		sub->setNonce(nonce);
		if (setupLeaf(netMgr,px,cp,rtr,nmIface,nonce,cph,true) == 0) {
			fatal("cannot configure NetMgr's access link");
		}
	}

	// finally, send positive reply
	repCp.reset(CtlPkt::BOOT_ROUTER,CtlPkt::POS_REPLY,cp.seqNum);
	cph.sendReply(repCp,0);
	net->setStatus(rtr,NetInfo::UP);

	if (rtr == nmRtr) sub->setRtrReady(true);

	logger->log("completed boot request",0,p);
	return true;
}

/** Configure a link endpoint at a router.
 *  This method is used when booting a router.
 *  It configures a link going to another router.
 *  @param lnk is the global link number for the link being configured
 *  @param rtr is the number of the router that is to be configured
 *  @param px is the packet that triggered this operation
 *  @param cp is the control packet from px
 *  @param cph is the handler for this thread
 *  @param useTunnel is an optional parameter that defaults to false;
 *  when it is true, it specifies that control packets should be
 *  addressed to the tunnel (ip,port) configured in the cph;
 *  @return true if the configuration is successful, else false
 */
bool setupEndpoint(int lnk, int rtr, pktx px, CtlPkt& cp, CpHandler& cph,
		   bool useTunnel) {
	int llnk = net->getLLnum(lnk,rtr);
	int iface = net->getIface(llnk,rtr);
	fAdr_t rtrAdr = net->getNodeAdr(rtr);

	int peer = net->getPeer(rtr,lnk);
	fAdr_t peerAdr = net->getNodeAdr(peer);
	int plnk = net->getLLnum(lnk,peer);
	int i = net->getIface(plnk,peer);
	ipa_t peerIp = net->getIfIpAdr(peer,i);
	ipp_t peerPort = 0;

	uint64_t nonce;
	if (net->getStatus(peer) != NetInfo::UP) {
		nonce = generateNonce();
		net->setNonce(lnk,nonce);
	} else {
		peerPort = net->getIfPort(peer,i);
		nonce = net->getNonce(lnk);
	}
	CtlPkt repCp;
	fAdr_t dest = (useTunnel ? 0 : rtrAdr);
	int reply = cph.addLink(dest,Forest::ROUTER,iface,llnk,peerIp,
				peerPort,peerAdr,nonce,repCp);
	if (!processReply(px,cp,reply,repCp,cph,"could not add link at router"))
		return false;

	// now, send modify link message, to set data rates
	RateSpec rs = net->getLinkRates(lnk);
	if (rtr == net->getLeft(lnk)) rs.flip();
	reply = cph.modLink(dest,llnk,rs,repCp);
	if (!processReply(px,cp,reply,repCp,cph,
			  "could not set link rates at router"))
		return false;
	return true;
}

/** Setup a pre-configured comtree at a booting router.
 *  @param ctx is the index of the comtree being setup
 *  @param rtr is the number of the router
 *  @param px is the number of the packet that initiated the current operation
 *  @param cp is the control packet for px (already unpacked)
 *  @param cph is a reference to the control packet handler for this thread
 *  @return true on success, false on failure
 */
bool setupComtree(int ctx, int rtr, pktx px, CtlPkt& cp, CpHandler& cph,
		  bool useTunnel) {
	fAdr_t rtrAdr = net->getNodeAdr(rtr);
	
	comt_t comt = comtrees->getComtree(ctx);

	// first step is to add comtree at router
	CtlPkt repCp;
	fAdr_t dest = (useTunnel ? 0 : rtrAdr);
	int reply = cph.addComtree(dest,comt,repCp);
	if (!processReply(px,cp,reply,repCp,cph,
			  "could not add comtree at router"))
		return false;

	// next, add links to the comtree and set their data rates
	int plnk = comtrees->getPlink(ctx,rtrAdr);
	int parent = net->getPeer(rtr,plnk);
	for (int lnk = net->firstLinkAt(rtr); lnk != 0;
		 lnk = net->nextLinkAt(rtr,lnk)) {
		if (!comtrees->isComtLink(ctx,lnk)) continue;
		int peer = net->getPeer(rtr,lnk);
		if (net->getNodeType(peer) != Forest::ROUTER) continue;

		// get information about this link
		int llnk = net->getLLnum(lnk,rtr);
		fAdr_t peerAdr = net->getNodeAdr(peer);
		bool peerCoreFlag = comtrees->isCoreNode(ctx,peerAdr);

		// first, add comtree link
		reply = cph.addComtreeLink(dest,comt,llnk,
					   peerCoreFlag,repCp);
		if (!processReply(px,cp,reply,repCp,cph,
				  "could not add comtree link at router"))
			return false;

		// now, set link rates
		RateSpec rs;
		if (peer == parent) {
			rs = comtrees->getLinkRates(ctx,rtrAdr);
			rs.flip();
		} else {
			rs = comtrees->getLinkRates(ctx,peerAdr);
		}
		reply = cph.modComtreeLink(dest,comt,llnk,rs,repCp);
		if (!processReply(px,cp,reply,repCp,cph,
				  "could not set comtree link rates at router"))
			return false;
	}
	// finally, we need to modify overall comtree attributes
	reply = cph.modComtree(dest,comt,net->getLLnum(plnk,rtr),
				comtrees->isCoreNode(ctx,rtrAdr),repCp);
	if (!processReply(px,cp,reply,repCp,cph,
			  "could not set comtree parameters"))
		return false;
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
 
Expect to ditch this

 *  @return a packet number with a formatted control packet on success,
 *  0 on failure
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
	    (p.type != Forest::CLIENT_SIG && p.type != Forest::NET_SIG))
		fatal(" recvFromCons: misformatted packet from console");
        return px;
}
 */

/** Write a packet to the socket for the user interface.
 *  @param px is the packet to be sent to the CLI
void sendToCons(pktx px) {
	if (connSock >= 0) {
		Packet& p = ps->getPacket(px);
		p.pack();
		Np4d::sendBuf(connSock, (char *) p.buffer, p.length);
	}
	ps->free(px);
}
 */


} // ends namespace
