/** @file RouterCore.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RouterCore.h"



/*
Notes

Planned transition to having the router obtain its configuration
from NetManager. Support both local and remote configuration.
More flexible command-line args: param=value form.

Arguments

mode (local or remote)
 - in both modes, read files if present
 - in remote mode, send initialize request to netMgr and accept
   packets from netMgr independent of normal router links;
   modify IpProcessor to handle this; maybe have separate
   packet processing loop; only call run after netMgr sends
   start message
 - in both modes, NetMgr can send control packets; this allows
   use of local mode with incremental addition of clients

myAdr (required)
myIp
nmIp
nmAdr
ccAdr
cliAdrRange

ifTbl
lnkTbl
comtTbl
rteTbl
statSpec

finTime - default to 0

Two modes
- local config mode - ifTbl, lnkTbl and comtTbl are required
- remote config mode - netMgrIp is required
*/

struct ArgInfo {
	string	mode;

	fAdr_t	myAdr;
	ipa_t	myIp;
	fAdr_t 	nmAdr;
	ipa_t 	nmIp;
	fAdr_t	ccAdr;
	fAdr_t	firstLeafAdr;
	fAdr_t	lastLeafAdr;

	string	ifTbl;
	string	lnkTbl;
	string	comtTbl;
	string	rteTbl;
	string	statSpec;

	int	finTime;
};
	

bool processArgs(int argc, char **argv[], ArgInfo& args) {
	// set default values
	args.myAdr = args.myIp = args.nmAdr = args.nmIp = 0;
	args.ccAdr = args.firstLeafAdr = args.lastLeafAdr = 0;
	args.ifTbl = ""; args.lnkTbl = ""; args.comtTbl = "";
	args.rteTbl = ""; args.statSpec = ""; 
	args.finTime = 0;

	string s;
	for (int i = 1; i <= argc; i++) {
		s = argv[i];
		if (s.compare(0,5,"mode=") == 0) {
			args.mode = &argv[i][5];
		} else if (s.compare(0,6,"myAdr=") == 0) {
			args.myAdr = Forest::forestAdr(&argv[i][6]);
		} else if (s.compare(0,5,"myIp=") == 0) {
			args.myIp = Misc::ipAddress(&argv[i][5]);
		} else if (s.compare(0,6,"nmAdr=") == 0) {
			args.nmAdr = Forest::forestAdr(&argv[i][6]);
		} else if (s.compare(0,5,"nmIp=") == 0) {
			args.nmIp = Misc::ipAddress(&argv[i][5]);
		} else if (s.compare(0,6,"ccAdr=") == 0) {
			args.ccAdr = Forest::forestAdr(&argv[i][6]);
		} else if (s.compare(0,13,"firstLeafAdr=") == 0) {
			args.firstLeafAdr = Forest::forestAdr(&argv[i][13]);
		} else if (s.compare(0,12,"lastLeafAdr=") == 0) {
			args.lastLeafAdr = Forest::forestAdr(&argv[i][12]);
		} else if (s.compare(0,6,"ifTbl=") == 0) {
			args.ifTbl = &argv[i][6];
		} else if (s.compare(0,7,"lnkTbl=") == 0) {
			args.lnkTbl = &argv[i][7];
		} else if (s.compare(0,8,"comtTbl=") == 0) {
			args.comtTbl = &argv[i][8];
		} else if (s.compare(0,7,"rteTbl=") == 0) {
			args.rteTbl = &argv[i][7];
		} else if (s.compare(0,9,"statSpec=") == 0) {
			args.statSpec = &argv[i][9];
		} else if (s.compare(0,8,"finTime=") == 0) {
			sscanf(&argv[i][8],"%d",&finTime);
		} else {
			cerr << "unrecognized argument: " << argv[i] << endl;
			return false;
		}
	}
	if (args.myAdr == 0 || args.firstLeafAdr == 0 || args.lastLeafAdr ==0 ||
	    args.lastLeafAdr < args.firstLeafAdr) {
		cerr << "One of the required arguments (myAdr, firstLeafAdr "
		     << "lastLeafAdr) is missing or lastLeafAdr<firstLeafAdr"
		     << endl;
	}
	return true;
}


/** usage:
 * 	fRouter arguments
 * 
 */
main(int argc, char *argv[]) {
	ArgInfo args;
	
	if (!processArgs(argc, argv, args))
		fatal("fRouter: error processing command line arguments";

	RouterCore router(args);

	if (!router.init(args)) {
		fatal("router: fRouter::init() failed");
	}
	router.dump(cout); 		// print tables
	router.run(1000000*args.finTime);
	cout << endl;
	router.dump(cout); 		// print final tables
	cout << endl;


/*

	int finTime;
	int ip0, ip1, ip2, ip3;
	int lBound, uBound;
	ipa_t ipadr; fAdr_t fAdr, netMgrAdr;
	int numData = 0;
	if (argc < 12 || argc > 13 ||
	    (fAdr = Forest::forestAdr(argv[1])) == 0 ||
	    (ipadr = Np4d::ipAddress(argv[2])) == 0 ||
	    sscanf(argv[8],"%d", &lBound) != 1 ||
	    sscanf(argv[9],"%d", &uBound) != 1 ||
	    sscanf(argv[10],"%d", &finTime) != 1 ||
	    (netMgrAdr = Forest::forestAdr(argv[11])) == 0 ||
	    (argc == 13 && sscanf(argv[12],"%d",&numData) != 1)) {
		fatal("usage: fRouter fAdr ifTbl lnkTbl comtTbl "
		      "rtxTbl stats finTime");
	}

	RouterCore router(fAdr,ipadr,netMgrAdr,lBound,uBound);
	if (!router.init(argv[3],argv[4],argv[5],argv[6],argv[7])) {
		fatal("router: fRouter::init() failed");
	}
	router.dump(cout); 		// print tables
	router.run(1000000*finTime,numData);
	cout << endl;
	router.dump(cout); 		// print final tables
	cout << endl;
*/
}


RouterCore::RouterCore(ArgInfo& args) {
	nIfaces = 50; nLnks = 1000;
	nComts = 5000; nRts = 100000;
	nPkts = 200000; nBufs = 100000; nQus = 10000;

	myAdr = args.myAdr;
	myIp = args.myIp;
	nmAdr = args.nmAdr;
	nmIp = args.nmIp;
	ccAdr = args.ccAdr;
	firstLeafAdr = args.firstLeafAdr;
	
	ps = new PacketStore(nPkts, nBufs);
	ift = new IfaceTable(nIfaces);
	lt = new LinkTable(nLnks);
	ctt = new ComtreeTable(nComts,10*nComts,lt);
	rt = new RouteTable(nRts,myAdr,ctt);
	sm = new StatsModule(1000, nLnks, nQus);
	iop = new IoProcessor(nIfaces, ift, lt, ps, sm);
	qm = new QuManager(nLnks, nPkts, nQus, min(10,nPkts/(2*nLnks)), ps, lt);

	leafAdr = new UiSetPair((args.lastLeafAdr - firstLeafAdr) + 1);
}

RouterCore::~RouterCore() {
	delete ift; delete lt; delete ctt; delete rt;
	delete ps; delete qm; delete iop; delete sm;
	if (leafAdr != 0) delete leafAdr;
}

/** Initialize router.
 */
bool RouterCore::init(const ArgInfo& args) {
	if (args.mode.compare("remote") == 0) {
		cerr << "remote configuration mode not yet implemented";
		return false;
	}

	if (args.ifTbl.compare("") != 0) {
		ifStream fs.open(args.ifTbl);
		if (fs.fail() || !ift->read(fs)) {
			cerr << "RouterCore::init: can't read "
			     << "interface table\n";
			return false;
		}
		fs.close();
	}
	if (args.lnkTbl.compare("") != 0) {
		ifStream fs.open(args.lnkTbl);
		if (fs.fail() || !lt->read(fs)) {
			cerr << "RouterCore::init: can't read "
			     << "link table\n";
			return false;
		}
		fs.close();
	}
	if (args.comtTbl.compare("") != 0) {
		ifStream fs.open(args.comtTbl);
		if (fs.fail() || !ctt->read(fs)) {
			cerr << "RouterCore::init: can't read "
			     << "comtree table\n";
			return false;
		}
		fs.close();
	}
	if (args.rteTbl.compare("") != 0) {
		ifStream fs.open(args.rteTbl);
		if (fs.fail() || !rt->read(fs)) {
			cerr << "RouterCore::init: can't read "
			     << "routing table\n";
			return false;
		}
		fs.close();
	}
	if (args.statSpec.compare("") != 0) {
		ifStream fs.open(args.statSpec);
		if (fs.fail() || !sm->read(fs)) {
			cerr << "RouterCore::init: can't read "
			     << "statistics spec\n";
			return false;
		}
		fs.close();
	}

	addLocalRoutes();
	return true;
}

// Add routes for all directly attached hosts for all comtrees.
// Also add routes to adjacent routers in different zip codes.
void RouterCore::addLocalRoutes() {
	for (int ctx = ct->firstComtIndex(); ctx != 0;
	     	 ctx = ct->nextComtIndex(ctx)) {
		set<int>& comtLinks = ctt->getLinks(ctx);
		set<int>::iterator p;
		for (p = comtLinks.begin(); p != comtLinks.end(); p++) {
			int cLnk = *p; int lnk = ctt->getLink(cLnk);
			fAdr_t peerAdr = lt->getPeerAdr(lnk);
			if (lt->getPeerType(lnk) == ROUTER &&
			    Forest::zipCode(peerAdr)
			    == Forest::zipCode(myAdr))
				continue;
			if (rt->getRteIndex(comt,peerAdr) != 0)
				continue;
			rt->addEntry(comt,peerAdr,cLnk);
		}
	}
}

void RouterCore::dump(ostream& out) {
	out << "Interface Table\n\n"; ift->write(out); out << endl;
	out << "Link Table\n\n"; lt->write(out); out << endl;
	out << "Comtree Table\n\n"; ctt->writeTable(out); out << endl;
	out << "Routing Table\n\n"; rt->write(out); out << endl;
	out << "Statistics\n\n"; sm->write(out); out << endl;
}

/** Main router processing loop.
 *  This program executes a loop that does three primary things:
 *   - checks to see if a packet has arrived and if so, processes it;
 *     this typically results in the packet being placed in one or more queues
 *   - checks to see if any of the links is ready to send a packet,
 *     and if so, retrieves and sends all packets whose time has come
 *   - checks to see if any control packets are waiting to be processed,
 *     and if so, processes one
 *  
 *  In addition, the main loop periodically writes taffic statistics
 *  to an external statistics file. It also maintains copies of a limited
 *  number of packets received and sent. These are printed to cout after
 *  the main loop exits, to facilitate debugging.
 *  
 *  Time is managed through a free-running microsecond clock, derived
 *  from the time value returned by gettimeofday. The clock is updated
 *  on each iteration of the loop.
 * 
 *  Note, that since the loop processes only one input packet per iteration,
 *  and possibly multiple output packets, excessive input traffic will cause
 *  packets to be discarded from socket buffers before they ever reach this
 *  program. This is intended to keep the system productive, even during
 *  overload. However, it is still best to limit input traffic rates
 *  where that is possible.
 *
 *  @param finishTime is the number of microseconds to run before stopping;
 *  if it is zero, the router runs without stopping (until killed)
 */
void RouterCore::run(uint64_t finishTime) {
	// record of first packet receptions, transmissions for debugging
	const int MAXEVENTS = 1000;
	const int NUMDATA = 500;
	struct {
		int sendFlag; uint64_t time; int link, pkt;
	} events[MAXEVENTS];
	int evCnt = 0; int numData = 0;

	uint64_t statsTime = 0;		// time statistics were last processed
	bool didNothing;
	int controlCount = 20;		// used to limit overhead of control
					// packet processing
	queue<int> ctlQ;		// queue for control packets

	now = Misc::getTimeNs();
	while (finishTime == 0 || now < finishTime) {
		didNothing = true;

		// input processing
		int p = iop->receive();
		if (p != 0) {
			didNothing = false;
			PacketHeader& h = ps->getHeader(p);
			int ptype = h.getPtype();

			if (evCnt < MAXEVENTS &&
			    (ptype != CLIENT_DATA || numData <= NUMDATA)) {
				int p1 = (ptype == CLIENT_DATA ? 
						ps->clone(p) : ps->fullCopy(p));
				events[evCnt].sendFlag = 0;
				events[evCnt].link = h.getInLink();
				events[evCnt].time = now;
				events[evCnt].pkt = p1;
				evCnt++;
				if (ptype == CLIENT_DATA) numData++;
			}
			int ctx = ctt->lookup(h.getComtree());
			if (!pktCheck(p,ctx)) {
				ps->free(p);
			} else if (ptype == CLIENT_DATA) {
				forward(p,ctx);
			} else if (ptype == SUB_UNSUB) {
				subUnsub(p,ctx);
			} else if (ptype == RTE_REPLY) {
				handleRteReply(p,ctx);
			} else if (h.getDstAdr() != myAdr) {
				forward(p,ctx);
			} else {
				ctlQ.push(p);
			}
		}

		// output processing
		int lnk;
		while ((p = qm->deq(lnk, now)) != 0) {
			didNothing = false;
			PacketHeader& h = ps->getHeader(p);
			if (evCnt < MAXEVENTS &&
			    (h.getPtype() != CLIENT_DATA || numData<=NUMDATA)) {
				int p2 = (h.getPtype() == CLIENT_DATA ? 
						ps->clone(p) : ps->fullCopy(p));
				events[evCnt].sendFlag = 1;
				events[evCnt].link = lnk;
				events[evCnt].time = now;
				events[evCnt].pkt = p2;
				evCnt++;
				if (h.getPtype() == CLIENT_DATA) numData++;
			}
			iop->send(p,lnk);
		}

		// control packet processing
		if (!ctlQ.empty() && (didNothing || --controlCount <= 0)) {
			handleCtlPkt(ctlQ.front());
			ctlQ.pop();
			didNothing = false;
			controlCount = 20; // do one control packet for
					   // every 20 iterations when busy
		}

		// update statistics every 300 ms
		if (now - statsTime > 300000000) {
			sm->record(now);
			statsTime = now;
			didNothing = false;
		}

		// if did nothing on that pass, sleep for a millisecond.
		if (didNothing) {
			struct timespec sleeptime, rem;
			sleeptime.tv_sec = 0; sleeptime.tv_nsec = 1000000;
			nanosleep(&sleeptime,&rem);
		}

		// update current time
		now = Misc::getTimeNs();
	}

	// write out recorded events
	for (int i = 0; i < evCnt; i++) {
		if (events[i].sendFlag) 
			cout << "send link " << setw(2) << events[i].link
			     << " at " << setw(8) << events[i].time/1000 << " ";
		else
			cout << "recv link " << setw(2) << events[i].link
			     << " at " << setw(8) << events[i].time/1000 << " ";
		int p = events[i].pkt;
		(ps->getHeader(p)).write(cout,ps->getBuffer(p));
	}
	cout << endl;
	cout << sm->iPktCnt(0) << " packets received, "
	     <<	sm->oPktCnt(0) << " packets sent\n";
	cout << sm->iPktCnt(-1) << " from routers,    "
	     << sm->oPktCnt(-1) << " to routers\n";
	cout << sm->iPktCnt(-2) << " from clients,    "
	     << sm->oPktCnt(-2) << " to clients\n";
}

bool RouterCore::pktCheck(int p, int ctx) {
// Perform error checks on forest packet.
// Return true if all checks pass, else false.
	PacketHeader& h = ps->getHeader(p);
	// check version and  length
	if (h.getVersion() != Forest::FOREST_VERSION) return false;

	if (h.getLength() != h.getIoBytes() || h.getLength() < Forest::HDR_LENG)
		return false;

	fAdr_t adr = h.getDstAdr();
	if (!Forest::validUcastAdr(adr) && !Forest::mcastAdr(adr))
		return false;

	int inL = h.getInLink();
	if (inL == 0) return false;
	// extra checks for packets from untrusted peers
	if (lt->getPeerType(inL) < TRUSTED) {
		// check for spoofed source address
		if (lt->getPeerAdr(inL) != h.getSrcAdr()) return false;
		// and that destination restrictions are respected
		if (lt->getPeerDest(inL) != 0 && 
		    h.getDstAdr() != lt->getPeerDest(inL) &&
		    h.getDstAdr() != myAdr)
			return false;
		// don't let untrusted peers send network signal packets
		if (h.getPtype() >= NET_SIG) return false;
		// and user signalling packets must be sent on comtrees 1-99
		if (h.getPtype() >= CLIENT_SIG && h.getComtree() > 100)
			return false;
	}
	// check for invalid comtrees or input links
	if (!ctt->validComtIndex(ctx) || !ctt->isLink(ctx,inL)) return false;
	return true;
}
/** Lookup routing entry and forward packet accordingly.
 *  @param p is a packet number for a CLIENT_DATA packet
 *  @param ctx is the comtree table index for the comtree in p's header
 */
void RouterCore::forward(int p, int ctx) {
	PacketHeader& h = ps->getHeader(p);
	int rtx = rt->getRteIndex(h.getComtree(),h.getDstAdr());
	if (rtx != 0) { // valid route case
		// reply to route request
		if ((h.getFlags() & Forest::RTE_REQ)) {
			sendRteReply(p,ctx);
			h.setFlags(h.getFlags() & (~Forest::RTE_REQ));
			ps->pack(p);
			ps->hdrErrUpdate(p);
		}
		if (Forest::validUcastAdr(h.getDstAdr())) {
			int cLnk = rt->getLink(rtx);
			int lnk = ctt->getLink(cLnk);
			int qn = ctt->getLinkQ(cLnk);
			if (lnk == h.getInLink() || !qm->enq(p,qn,now))
				ps->free(p);
			return;
		}
		// multicast data packet
		multiSend(p,ctx,rtx);
		return;
	}
	// no valid route
	if (Forest::validUcastAdr(h.getDstAdr())) {
		// send to neighboring routers in comtree
		h.setFlags(Forest::RTE_REQ);
		ps->pack(p); ps->hdrErrUpdate(p);
	}
	multiSend(p,ctx,rtx);
	return;
}
/** Send multiple copies of a packet.
 *  @param p is the number of a multi-destination packet
 *  @param ctx is the comtree index for the comtree in p's header
 *  @param rtx is the route index for p, or 0 if there is no route
 */
void RouterCore::multiSend(int p, int ctx, int rtx) {
	int qvec[nLnks]; int n = 0;
	PacketHeader& h = ps->getHeader(p);

	int p1 = p;
	int inLink = h.getInLink();
	if (Forest::validUcastAdr(h.getDstAdr())) {
		// flooding a unicast packet to neighboring routers
		int myZip = Forest::zipCode(myAdr);
		int pZip = Forest::zipCode(h.getDstAdr());
		set<int>& rtrLinks = getRtrLinks(ctx);
		set<int>::iterator it;
		for (it = rtrLinks.begin(); it != rtrLinks.end(); it++) {
			int cLnk = *it; int lnk = ctt->getLink(cLnk);
			int peerZip = Forest::zipCode(lt->getPeerAdr(lnk));
			if (pZip == myZip && peerZip != myZip) continue;
			if (lnk == inLink) continue;
			qvec[n++] = ctt->getLinkQ(cLnk);
		}
	} else { 
		// forwarding a multicast packet
		// first identify neighboring core routers to get copies
		int pLink = ctt->getPlink(ctx);	
		set<int>& coreLinks = getCoreLinks(ctx);
		set<int>::iterator lp;
		for (lp = coreLinks.begin(); lp != coreLinks.end(); lp++) {
			int cLnk = *lp; int lnk = ctt->getLink(cLnk);
			if (lnk == inLink || lnk == pLink) continue;
			qvec[n++] = ctt->getLinkQ(cLnk);
		}
		// now copy for parent
		if (pLink != 0 && pLink != inLink) {
			qvec[n++] = ctt->getLinkQ(ctt->getPCLnk(ctx));
		}
		// now, copies for subscribers if any
		if (rtx == 0) return;
		set<int>& subLinks = *rtt->getSubLinks(rtx); 
		for (lp = subLinks.begin(); lp != subLinks.end(); lp++) {
			int cLnk = *lp; int lnk = ctt->getLink(cLnk);
			if (lnk == inLink) continue;
			qvec[n++] = ctt->getLinkQ(cLnk);
		}
	}

	// make copies and queue them
        int p1 = p;
        for (int i = 0; i < n-1; i++) { // process first n-1 copies
                if (qm->enq(p1,qvec[i],now))
			p1 = ps->clone(p);
        }
        // process last copy
        if (!qm->enq(p1,qvec[n-1],now))
		ps->free(p1);
}

void RouterCore::sendRteReply(int p, int ctx) {
// Send route reply back towards p's source.
	PacketHeader& h = ps->getHeader(p);

	int p1 = ps->alloc();
	PacketHeader& h1 = ps->getHeader(p1);
	h1.setLength(Forest::HDR_LENG + 8);
	h1.setPtype(RTE_REPLY);
	h1.setFlags(0);
	h1.setComtree(h.getComtree());
	h1.setSrcAdr(myAdr);
	h1.setDstAdr(h.getSrcAdr());

	ps->pack(p1);
	ps->getPayload(p1)[0] = htonl(h.getDstAdr());
	ps->hdrErrUpdate(p1); ps->payErrUpdate(p1);

	int cLnk = ctt->getComtLink(h.getComtree(),h.getInLink());
	qm->enq(p1,ctt->getLinkQ(cLnk),now);
}

void RouterCore::handleRteReply(int p, int ctx) {
// Handle route reply packet p.
	PacketHeader& h = ps->getHeader(p);
	int rtx = rt->lookup(h.getComtree(), h.getDstAdr());
	if ((h.getFlags() & Forest::RTE_REQ) && rtx != 0)
		sendRteReply(p,ctx);
	int adr = ntohl(ps->getPayload(p)[0]);
	if (Forest::validUcastAdr(adr) &&
	    rt->lookup(h.getComtree(),adr) == 0) {
		rt->addEntry(h.getComtree(),adr,h.getInLink(),0); 
	}
	if (rtx == 0) {
		// send to neighboring routers in comtree
		h.setFlags(Forest::RTE_REQ);
		ps->pack(p); ps->hdrErrUpdate(p);
		multiSend(p,ctx,rtx);
		return;
	}
	int cLnk = rt->getLink(rtx); int lnk = ctt->getLink(cLnk);
	if (lt->getPeerType(lnk) != ROUTER &&
	    !qm->enq(p,ctt->getLinkQ(cLnk),now))
		ps->free(p);
	return;
}

// Perform subscription processing on p.
// Ctte is the comtree table entry for the packet.
// First payload word contains add/drop counts;
// these must sum to at most 350 and must match packet length
void RouterCore::subUnsub(int p, int ctx) {
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	// add/remove branches from routes
	// if non-core node, also propagate requests upward as
	// appropriate
	int comt = ctt->getComtree(ctx);
	int inlnk = h.getInLink();
	int inClnk = ctt->getComtLink(comt,inlnk);
	// ignore subscriptions from the parent or core neighbors
	if (inlnk == ctt->getPlink(ctx) || ctt->isCoreLink(inClnk)) {
		ps->free(p); return;
	}
	bool propagate = false;
	int rtx; fAdr_t addr;

	// add subscriptions
	int addcnt = ntohl(pp[0]);
	if (addcnt < 0 || addcnt > 350 || (addcnt + 8)*4 > h.getLength()) {
		ps->free(p); return;
	}
	for (int i = 1; i <= addcnt; i++) {
		addr = ntohl(pp[i]);
		if (!Forest::mcastAdr(addr)) continue;  // ignore unicast
		rtx = rt->lookup(comt,addr);
		if (rtx == 0) { 
			rtx = rt->addEntry(comt,addr,inClnk);
			propagate = true;
		} else if (!rt->isLink(rtx,inClnk)) {
			rt->addLink(rtx,inClnk);
			pp[i] = 0; // so, parent will ignore
		} 
	}
	// remove subscriptions
	int dropcnt = ntohl(pp[addcnt+1]);
	if (dropcnt < 0 || addcnt + dropcnt > 350 ||
	    (addcnt + dropcnt + 8)*4 > h.getLength() ) {
		ps->free(p); return;
	}
	for (int i = addcnt + 2; i <= addcnt + dropcnt + 1; i++) {
		addr = ntohl(pp[i]);
		if (!Forest::mcastAdr(addr)) continue; // ignore unicast
		rtx = rt->lookup(comt,addr);
		if (rtx == 0) continue;
		rt->removeLink(rtx,inlnk);
		if (rt->noLinks(rtx)) {
			rt->removeEntry(rtx);
			propagate = true;
		} else {
			pp[i] = 0;
		}
	}
	// propagate subscription packet to parent if not a core node
	if (propagate && !ctt->getCoreFlag(ctx) && ctt->getPlink(ctx) != 0) {
		ps->payErrUpdate(p);
		if (qm->enq(p,ctt->getPlink(ctx),ctt->getQnum(ctx),now)) {
			return;
		}
	}
	ps->free(p); return;
}

/** Handle all control packets addressed to the router.
 *  with the exception of SUB_UNSUB and RTE_REPLY which
 *  are handled "inline".
 *  Assumes packet has passed all basic checks.
 *  Return 0 if the packet should be forwarded, else 1.
 *
 *  Still need to add error checking for modify messages.
 *  This requires extensions to ioProc and lnkTbl to
 *  track available bandwidth.
 */
void RouterCore::handleCtlPkt(int p) {
	int ctx, rtx;
	PacketHeader& h = ps->getHeader(p);
	int inL = h.getInLink();
	buffer_t& b = ps->getBuffer(p);
	CtlPkt cp;
	// first handle special cases of CONNECT/DISCONNECT
	if (h.getPtype() == CONNECT) {
		if (lt->getPeerPort(inL) == 0)
			lt->setPeerPort(inL,h.getTunSrcPort());
		if(Forest::localAdr(h.getSrcAdr()) >= localLBound &&
		    Forest::localAdr(h.getSrcAdr()) <= localUBound) {
			int p1 = ps->alloc();
			cp.reset();
			cp.setCpType(CLIENT_CONNECT);
			cp.setRrType(REQUEST);
			cp.setAttr(CLIENT_ADR,h.getSrcAdr());
			cp.setAttr(RTR_ADR,myAdr);
			int paylen = cp.pack(ps->getPayload(p1));
			PacketHeader& h1 = ps->getHeader(p1);
			h1.setComtree(100);
			h1.setFlags(0); h1.setInLink(0);
			h1.setPtype(NET_SIG);
			h1.setLength(Forest::OVERHEAD + paylen);
			h1.setDstAdr(netMgrAdr); h1.setSrcAdr(myAdr);
			ps->pack(p1);
			forward(p1,ctt->lookup(100));
		}
		ps->free(p); return;
	}
	if (h.getPtype() == DISCONNECT) {
		if (lt->getPeerPort(inL) == h.getTunSrcPort())
			lt->setPeerPort(inL,0);
		if(Forest::localAdr(h.getSrcAdr()) >= localLBound &&
		    Forest::localAdr(h.getSrcAdr()) <= localUBound) {
			int p1 = ps->alloc();
			cp.reset();
			cp.setCpType(CLIENT_DISCONNECT);
			cp.setRrType(REQUEST);
			cp.setAttr(CLIENT_ADR,h.getSrcAdr());
			cp.setAttr(RTR_ADR,myAdr);
			int paylen = cp.pack(ps->getPayload(p1));
			PacketHeader& h1 = ps->getHeader(p1);
			h1.setComtree(100);
			h1.setFlags(0); h1.setInLink(0);
			h1.setPtype(NET_SIG);
			h1.setLength(Forest::OVERHEAD + paylen);
			h1.setDstAdr(netMgrAdr); h1.setSrcAdr(myAdr);
			ps->pack(p1);
			forward(p1,ctt->lookup(100));
		}
		ps->free(p); return;
	}

	// then onto normal signalling packets
	int len = h.getLength() - (Forest::HDR_LENG + 4);
	if (!cp.unpack(ps->getPayload(p), len)) {
		cerr << "misformatted control packet";
		cp.write(cerr);
		errReply(p,cp,"misformatted control packet");
		return;
	}
	if (h.getPtype() != NET_SIG ||
	    h.getComtree() < 100 || h.getComtree() > 999) {
		// reject signalling packets on comtrees outside 100-999 range
		ps->free(p); return;
	}
	
	// Prepare positive reply packet for use where appropriate
	CtlPkt cp1;
	cp1.setCpType(cp.getCpType());
	cp1.setRrType(POS_REPLY);
	cp1.setSeqNum(cp.getSeqNum());
	switch (cp.getCpType()) {
	// Configuring logical interfaces
	case ADD_IFACE:
		if(iop->valid(cp.getAttr(IFACE_NUM))) {
			if(iop->lookupEntry(cp.getAttr(IFACE_NUM),
				  cp.getAttr(LOCAL_IP),
		     		  cp.getAttr(MAX_BIT_RATE),
				  cp.getAttr(MAX_PKT_RATE)))
				returnToSender(p,cp1.pack(ps->getPayload(p)));
			else
				errReply(p,cp1,"add iface: iface already in table!");
		} else if (iop->addEntry(cp.getAttr(IFACE_NUM),
				  cp.getAttr(LOCAL_IP),
		     		  cp.getAttr(MAX_BIT_RATE),
				  cp.getAttr(MAX_PKT_RATE))) {
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else {
			errReply(p,cp1,"add iface: cannot add interface");
		}
		break;
        case DROP_IFACE:
		iop->removeEntry(cp.getAttr(IFACE_NUM));
		returnToSender(p,cp1.pack(ps->getPayload(p)));
		break;
        case GET_IFACE: {
		int32_t iface = cp.getAttr(IFACE_NUM);
		if (iop->valid(iface)) {
			cp1.setAttr(IFACE_NUM,iface);
			cp1.setAttr(LOCAL_IP,iop->getIpAdr(iface));
			cp1.setAttr(MAX_BIT_RATE,iop->getMaxBitRate(iface));
			cp1.setAttr(MAX_PKT_RATE,iop->getMaxPktRate(iface));
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"get iface: invalid interface");
		break;
	}
        case MOD_IFACE: {
		int32_t iface = cp.getAttr(IFACE_NUM);
		if (iop->valid(iface)) {
			int br = iop->getMaxBitRate(iface);
			int pr = iop->getMaxPktRate(iface);
			if (cp.isSet(MAX_BIT_RATE))
				iop->setMaxBitRate(iface,
						   cp.getAttr(MAX_BIT_RATE));
			if (cp.isSet(MAX_PKT_RATE))
				iop->setMaxPktRate(iface,
						   cp.getAttr(MAX_PKT_RATE));
			if (iop->checkEntry(iface)) {
				returnToSender(p,cp1.pack(ps->getPayload(p)));
			} else { // undo changes
				iop->setMaxBitRate(iface, br);
				iop->setMaxPktRate(iface, pr);
				errReply(p,cp1,"mod iface: invalid rate");
			}
		} else errReply(p,cp1,"mod iface: invalid interface");
		break;
	}
        case ADD_LINK: {
		if (!cp.isSet(PEER_IP) || !cp.isSet(PEER_TYPE)) {
			errReply(p,cp1,"add link: missing required attributes");
			break;
		}
		ntyp_t ntyp = (ntyp_t) cp.getAttr(PEER_TYPE);
		ipa_t  pipa = cp.getAttr(PEER_IP);
		int lnk = (cp.isSet(LINK_NUM) ? cp.getAttr(LINK_NUM) :
						lt->alloc());
		int iface = (cp.isSet(IFACE_NUM) ? cp.getAttr(IFACE_NUM) :
					  	   iop->getDefaultIface());
		fAdr_t padr;
		if(ntyp == ROUTER) {
			if(!cp.isSet(PEER_ADR)) {
				errReply(p,cp1,"add link: missing attribute");
				break;
			}
			padr = cp.getAttr(PEER_ADR);
			if(lt->ipValidLink(pipa,false)) {
				if(!lt->linkConsistent(lnk,iface,ntyp,pipa,padr)) {
					errReply(p,cp1,"add link: inconsistent link info");
					break;
				}
			} else if (!lt->addEntry(lnk,iface,ntyp,pipa,padr)) {
				errReply(p,cp1,"add link: cannot add link");
				break;
			}
		} else if(ntyp == CLIENT) {
			padr = Forest::forestAdr(Forest::zipCode(myAdr),localLBound + (currClient++));
			if(lt->ipValidLink(pipa,true)) {
				if(!lt->linkConsistent(lnk,iface,ntyp,pipa,padr)) {
					errReply(p,cp1,"add link: inconsistent link info");
					break;
				}
			} else if (!lt->addEntry(lnk,iface,ntyp,pipa,padr)) {
				errReply(p,cp1,"add link: cannot add link");
				break;
			}
		}
		cp1.setAttr(LINK_NUM,lnk);
		cp1.setAttr(PEER_ADR,padr);
		cp1.setAttr(RTR_IP,myIp);
		returnToSender(p,cp1.pack(ps->getPayload(p)));
		break;
	}
        case DROP_LINK:
		if (lt->removeEntry(cp.getAttr(LINK_NUM)))
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		else errReply(p,cp1,"drop link: cannot drop link");
        case GET_LINK: {
		uint32_t link = cp.getAttr(LINK_NUM);
		if (lt->valid(link)) {
			cp1.setAttr(LINK_NUM, link);
			cp1.setAttr(IFACE_NUM, lt->getInterface(link));
			cp1.setAttr(PEER_IP, lt->getPeerIpAdr(link));
			cp1.setAttr(PEER_TYPE, lt->getPeerType(link));
			cp1.setAttr(PEER_PORT, lt->getPeerPort(link));
			cp1.setAttr(PEER_DEST, lt->getPeerDest(link));
			cp1.setAttr(PEER_ADR, lt->getPeerAdr(link));
			cp1.setAttr(BIT_RATE, lt->getBitRate(link));
			cp1.setAttr(PKT_RATE, lt->getPktRate(link));
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"get link: invalid link number");
		break;
	}
        case MOD_LINK: {
		int link = cp.getAttr(LINK_NUM);
		if (lt->valid(link)) {
			cp1.setAttr(LINK_NUM,link);
			if (cp.isSet(PEER_TYPE)) {
				ntyp_t pt = (ntyp_t) cp.getAttr(PEER_TYPE);
				if (pt != CLIENT && pt != SERVER &&
				    pt != ROUTER && pt != CONTROLLER) {
					errReply(p,cp1,"mod link:bad peerType");
					return;
				}
				lt->setPeerType(link,pt);
			}
			if (cp.isSet(PEER_PORT)) {
				int pp = cp.getAttr(PEER_PORT);
				if (pp > 65535) {
					errReply(p,cp1,"mod link:bad peerPort");
					return;
				}
				lt->setPeerPort(link, pp);
			}
			if (cp.isSet(PEER_DEST)) {
				fAdr_t pd = cp.getAttr(PEER_DEST);
				if (!Forest::validUcastAdr(pd)) {
					errReply(p,cp1,"mod link:bad peerDest");
					return;
				}
				lt->setPeerDest(link, pd);
			}
			if (cp.isSet(BIT_RATE) != 0)
				lt->setBitRate(link, cp.getAttr(BIT_RATE));
			if (cp.isSet(PKT_RATE) != 0)
				lt->setPktRate(link, cp.getAttr(PKT_RATE));
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"get link: invalid link number");
		break; }
        case ADD_COMTREE:
		if(ctt->lookup(cp.getAttr(COMTREE_NUM)) != 0) {
			if(ctt->lookupEntry(cp.getAttr(COMTREE_NUM)))
				returnToSender(p,cp1.pack(ps->getPayload(p)));
			else
				errReply(p,cp1,"add comtree: comtree already in table!");
		} else if (ctt->addEntry(cp.getAttr(COMTREE_NUM)) != 0)
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		else errReply(p,cp1,"add comtree: cannot add comtree");
		break;
        case DROP_COMTREE:
		ctx = ctt->lookup(cp.getAttr(COMTREE_NUM));
		if (ctx != 0 && ctt->removeEntry(ctx) != 0)
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		else
			errReply(p,cp1,"drop comtree: cannot drop comtree");
		break;
        case GET_COMTREE: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		ctx = ctt->lookup(comt);
		if (ctx == 0) {
			errReply(p,cp1,"get comtree: invalid comtree");
		} else {
			cp1.setAttr(COMTREE_NUM,comt);
			cp1.setAttr(CORE_FLAG,ctt->getCoreFlag(ctx));
			cp1.setAttr(PARENT_LINK,ctt->getPlink(ctx));
			cp1.setAttr(LINK_COUNT,ctt->getLinkCount(ctx));
			cp1.setAttr(QUEUE_NUM,ctt->getQnum(ctx));
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		}
		break;
	}
        case MOD_COMTREE: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		int ctx = ctt->lookup(comt);
		if (ctx != 0) {
			if (cp.isSet(CORE_FLAG) != 0)
				ctt->setCoreFlag(ctx,cp.getAttr(CORE_FLAG));
			if (cp.isSet(PARENT_LINK) != 0)
				ctt->setPlink(ctx,cp.getAttr(PARENT_LINK));
			if (cp.isSet(QUEUE_NUM) != 0)
				ctt->setQnum(ctx,cp.getAttr(QUEUE_NUM));
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"modify comtree: invalid comtree");
		break;
	}
	case ADD_COMTREE_LINK: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		int ctx = ctt->lookup(comt);
		if (ctx == 0) {
			errReply(p,cp1,"add comtree link: invalid comtree");
			break;
		}
		int lnk = 0; 
		if (cp.isSet(LINK_NUM)) {
			lnk = cp.getAttr(LINK_NUM);
		} else if (cp.isSet(PEER_ADR)) {
			lnk = lt->lookup(cp.getAttr(PEER_ADR), true);
		}
		if (!lt->valid(lnk)) {
			errReply(p,cp1,"add comtree link: invalid link or peer address");
			break;
		}
		bool isRtr = ROUTER == lt->getPeerType(lnk);
		bool isCore = false;
		if(isRtr) {
			if(!cp.isSet(CORE_FLAG)) {
				errReply(p,cp1,"add comtree link: router had no core flag set");
				break;
			} else isCore = cp.getAttr(CORE_FLAG);
		}
		if(ctt->isLink(ctx,lnk)) {
			if(ctt->lookupLink(ctx,lnk,isRtr,isCore))
				returnToSender(p,cp1.pack(ps->getPayload(p)));
			else
				errReply(p,cp1,"add comtree link: ct link already exists");
		} else {
			ctt->addLink(ctx,lnk,isRtr,isCore);
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		}
		break;
	}
	case DROP_COMTREE_LINK: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		int ctx = ctt->lookup(comt);
		if (ctx == 0) {
			errReply(p,cp1,"drop comtree link: invalid comtree");
			break;
		}
		int lnk = 0;
		if (cp.isSet(LINK_NUM)) {
			lnk = cp.getAttr(LINK_NUM);
		} else if (cp.isSet(PEER_ADR)) {
			lnk = lt->lookup(cp.getAttr(PEER_ADR), true);
		}
		if (!lt->valid(lnk)) {
			errReply(p,cp1,"drop comtree link: invalid link or peer address");
			break;
		}
		ctt->removeLink(ctx,lnk);
		returnToSender(p,cp1.pack(ps->getPayload(p)));
		break;
	}
        case ADD_ROUTE:
		if (rt->lookup(cp.getAttr(COMTREE_NUM),
				cp.getAttr(DEST_ADR)) != 0) {
			if(rt->compareEntry(cp.getAttr(COMTREE_NUM),
					cp.getAttr(DEST_ADR),
				 	cp.getAttr(LINK_NUM),
					cp.getAttr(QUEUE_NUM))) {
				returnToSender(p,cp1.pack(ps->getPayload(p)));
			} else {
				errReply(p,cp1,"add route: conflicting route exists");
			}
		} else if (rt->addEntry(cp.getAttr(COMTREE_NUM),
				 cp.getAttr(DEST_ADR),
				 cp.getAttr(LINK_NUM),
				 cp.getAttr(QUEUE_NUM)) != 0) {
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"add route: cannot add route");
		break;
        case DROP_ROUTE:
		rtx = rt->lookup(cp.getAttr(COMTREE_NUM),cp.getAttr(DEST_ADR));
		if (rtx != 0) {
			rt->removeEntry(rtx);
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"drop route: invalid route");
		break;
        case GET_ROUTE: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		fAdr_t da = cp.getAttr(DEST_ADR);
		rtx = rt->lookup(comt,da);
		if (rte != 0) {
			cp1.setAttr(COMTREE_NUM,comt);
			cp1.setAttr(DEST_ADR,da);
			cp1.setAttr(LINK_NUM,rt->getLink(rte));
			cp1.setAttr(QUEUE_NUM,rt->getQnum(rte));
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"get route: invalid route");
		break;
	}
        case MOD_ROUTE:
		rte = rt->lookup(cp.getAttr(COMTREE_NUM),cp.getAttr(DEST_ADR));
		if (rte != 0) {
			if (cp.isSet(LINK_NUM))
				rt->setLink(rte, cp.getAttr(LINK_NUM));
			if (cp.isSet(QUEUE_NUM))
				rt->setQnum(rte, cp.getAttr(QUEUE_NUM));
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"mod route: invalid route");
		break;
	case CLIENT_CONNECT:
		break;
	case CLIENT_DISCONNECT:
		break;
	default:
		cerr << "unrecognized control packet " << h.getPtype();
		ps->free(p);
		break;
	}
	return;
}

/** Send packet back to sender.
 *  Update the length, flip the addresses and pack the buffer.
 *  @param p is the packet number
 *  @param paylen is the length of the payload in bytes
 */
void RouterCore::returnToSender(packet p, int paylen) {
	PacketHeader& h = ps->getHeader(p);
	h.setLength(Forest::HDR_LENG + paylen + sizeof(uint32_t));

	fAdr_t temp = h.getDstAdr();
	h.setDstAdr(h.getSrcAdr());
	h.setSrcAdr(temp);

	ps->pack(p);

	int cLnk = ctt->getComtLink(h.getComtree,h.getInLink());
	int qn = ctt->getLinkQ(cLnk);
	if (!qm->enq(p,qn,now)) { ps->free(p); }
}

/** Send an error reply to a control packet.
 *  Reply packet re-uses the request packet p and its buffer.
 *  The string *s is included in the reply packet.
 */
void RouterCore::errReply(packet p, CtlPkt& cp, const char *s) {
	cp.setRrType(NEG_REPLY); cp.setErrMsg(s);
	returnToSender(p,4*cp.pack(ps->getPayload(p)));
}

