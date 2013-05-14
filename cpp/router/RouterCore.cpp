/** @file RouterCore.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RouterCore.h"

using namespace forest;

/** Process command line arguments for starting a forest router.
 *  All arguments are strings of the form "name=value".
 *  @param argc is the number of arguments (including the command name)
 *  @param argv contains the command line arguments as C-style strings
 *  @param is a reference to a RouterInfo structure; on return, it will
 *  contain all the values that were specified on the command line;
 *  unspecified numeric values are set to 0 and unspecified string
 *  values are set to the empty string
 *  @return true on success, false on failure
 */
bool processArgs(int argc, char *argv[], RouterInfo& args) {
	// set default values
	args.mode = "local";
	args.myAdr = args.bootIp = args.nmAdr = args.nmIp = 0;
	args.ccAdr = args.firstLeafAdr = args.lastLeafAdr = 0;
	args.ifTbl = ""; args.lnkTbl = ""; args.comtTbl = "";
	args.rteTbl = ""; args.statSpec = ""; 
	args.finTime = 0;

	string s;
	for (int i = 1; i < argc; i++) {
		s = argv[i];
		if (s.compare(0,10,"mode=local") == 0) {
			args.mode = "local";
		} else if (s.compare(0,11,"mode=remote") == 0) {
			args.mode = "remote";
		} else if (s.compare(0,6,"myAdr=") == 0) {
			args.myAdr = Forest::forestAdr(&argv[i][6]);
		} else if (s.compare(0,7,"bootIp=") == 0) {
			args.bootIp = Np4d::ipAddress(&argv[i][7]);
		} else if (s.compare(0,6,"nmAdr=") == 0) {
			args.nmAdr = Forest::forestAdr(&argv[i][6]);
		} else if (s.compare(0,5,"nmIp=") == 0) {
			args.nmIp = Np4d::ipAddress(&argv[i][5]);
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
			sscanf(&argv[i][8],"%d",&args.finTime);
		} else {
			cerr << "unrecognized argument: " << argv[i] << endl;
			return false;
		}
	}
	if (args.mode.compare("local") == 0 &&
	    (args.myAdr == 0 || args.firstLeafAdr == 0 ||
	     args.lastLeafAdr ==0 || args.lastLeafAdr < args.firstLeafAdr)) {
		cerr << "processArgs: local configuration requires myAdr, "
			"firstLeafAdr, lastLeafAdr and that firstLeafAdr "
			"be no larger than lastLeafAdr\n";
		return false;
	} else if (args.mode.compare("remote") == 0 &&
		   (args.bootIp == 0 || args.myAdr == 0 ||
		    args.nmIp == 0 || args.nmAdr == 0)) {
		cerr << "processArgs: remote configuration requires bootIp, "
		      	"myAdr, netMgrIp and netMgrAdr\n";
		return false;
	}
	return true;
}

/** Main program for starting a forest router.
 */
int main(int argc, char *argv[]) {
	RouterInfo args;
	if (!processArgs(argc, argv, args))
		fatal("fRouter: error processing command line arguments");
	bool booting = args.mode.compare("remote") == 0;
	RouterCore router(booting,args);

	if (!router.readTables(args))
		fatal("router: could not read specified config files");
	if (!booting) {
		if (!router.setup())
			fatal("router: inconsistency in config files");
	}
	router.run(args.finTime);
	cout << endl;
	router.dump(cout); 		// print final tables
	cout << endl;
    return 0;
}

namespace forest {

/** Constructor for RouterCore, initializes key parameters and allocates space.
 *  @param config is a RouterInfo structure which has been initialized to
 *  specify various router parameters
 */
RouterCore::RouterCore(bool booting1, const RouterInfo& config)
			: booting(booting1) {
	nIfaces = 50; nLnks = 1000;
	nComts = 5000; nRts = 100000;
	nPkts = 200000; nBufs = 100000; nQus = 10000;

	myAdr = config.myAdr;
	bootIp = config.bootIp;
	nmAdr = config.nmAdr;
	nmIp = config.nmIp;
	ccAdr = config.ccAdr;
	firstLeafAdr = config.firstLeafAdr;

	ps = new PacketStore(nPkts, nBufs);
	ift = new IfaceTable(nIfaces);
	lt = new LinkTable(nLnks);
	ctt = new ComtreeTable(nComts,10*nComts,lt);
	rt = new RouteTable(nRts,myAdr,ctt);
	sm = new StatsModule(1000, nLnks, nQus, ctt);
	iop = new IoProcessor(nIfaces, ift, lt, ps, sm);
	qm = new QuManager(nLnks, nPkts, nQus, min(50,5*nPkts/nLnks), ps, sm);
	pktLog = new PacketLog(20000,500,ps);

	if (!booting)
		leafAdr = new UiSetPair((config.lastLeafAdr - firstLeafAdr)+1);

	seqNum = 1;
	pending = new map<uint64_t,CpInfo>;
}

RouterCore::~RouterCore() {
    delete pktLog; delete qm; delete iop; delete sm;
    delete rt; delete ctt; delete lt; delete ift; delete ps;
	if (leafAdr != 0) delete leafAdr;
}

/** Read router configuration tables from files.
 *  This method reads initial router configuration files (if present)
 *  and configures router tables as specified.
 *  @param config is a RouterInfo structure which has been initialized to
 *  specify various router parameters
 */
bool RouterCore::readTables(const RouterInfo& config) {
	if (config.ifTbl.compare("") != 0) {
		ifstream fs; fs.open(config.ifTbl.c_str());
		if (fs.fail() || !ift->read(fs)) {
			cerr << "RouterCore::init: can't read "
			     << "interface table\n";
			return false;
		}
		fs.close();
	}
	if (config.lnkTbl.compare("") != 0) {
		ifstream fs; fs.open(config.lnkTbl.c_str());
		if (fs.fail() || !lt->read(fs)) {
			cerr << "RouterCore::init: can't read "
			     << "link table\n";
			return false;
		}
		fs.close();
	}
	if (config.comtTbl.compare("") != 0) {
		ifstream fs; fs.open(config.comtTbl.c_str());
		if (fs.fail() || !ctt->read(fs)) {
			cerr << "RouterCore::init: can't read "
			     << "comtree table\n";
			return false;
		}
		fs.close();
	}
	if (config.rteTbl.compare("") != 0) {
		ifstream fs; fs.open(config.rteTbl.c_str());
		if (fs.fail() || !rt->read(fs)) {
			cerr << "RouterCore::init: can't read "
			     << "routing table\n";
			return false;
		}
		fs.close();
	}
	if (config.statSpec.compare("") != 0) {
		ifstream fs; fs.open(config.statSpec.c_str());
		if (fs.fail() || !sm->read(fs)) {
			cerr << "RouterCore::init: can't read "
			     << "statistics spec\n";
			return false;
		}
		fs.close();
	}
	return true;
}

/** Setup router after tables and interfaces have been configured.
 *  Invokes several setup and verification methods to ensure that the
 *  initial configuration is fully consistent.
 */
bool RouterCore::setup() {
	dump(cout);
	if (!setupIfaces()) return false;
	if (!setupLeafAddresses()) return false;
	if (!setupQueues()) return false;
	if (!checkTables()) return false;
	if (!setAvailRates()) return false;
	addLocalRoutes();

	return true;
}

/** Setup interfaces specified in the interface table.
 *  This involves opening a separate UDP socket for each interface.
 *  @return true on success, false on failure
 */
bool RouterCore::setupIfaces() {
	for (int iface = ift->firstIface(); iface != 0;
		 iface = ift->nextIface(iface)) {
		if (!iop->setup(iface)) {
			cerr << "RouterCore::setupIfaces: could not "
				"setup interface " << iface << endl;
			return false;
		}
	}
	return true;
}


/** Allocate addresses to peers specified in the initial link table.
 *  Verifies that the initial peer addresses are in the range of
 *  assignable leaf addresses, and allocates them if they are.
 *  @return true on success, false on failure
 */
bool RouterCore::setupLeafAddresses() {
	for (int lnk = lt->firstLink(); lnk != 0; lnk = lt->nextLink(lnk)) {
		if (booting || lt->getPeerType(lnk) == ROUTER) continue;
		if (!allocLeafAdr(lt->getPeerAdr(lnk)))
			return false;
	}
	return true;
}

/** Setup queues as needed to support initial comtree configuration.
 *  For each defined comtree, a queue is allocated to each of its links.
 *  For each comtree link, initial rates are the minimum bit rate and
 *  packet rate allowed.
 *  @return true on success, false on failure
 */
bool RouterCore::setupQueues() {
	// Set link rates in QuManager
	for (int lnk = lt->firstLink(); lnk != 0; lnk = lt->nextLink(lnk)) {
		qm->setLinkRates(lnk,lt->getRates(lnk));
	}
	RateSpec rs(Forest::MINBITRATE,Forest::MINBITRATE,
		    Forest::MINPKTRATE,Forest::MINPKTRATE);
	int ctx;
        for (ctx = ctt->firstComtIndex(); ctx != 0;
             ctx = ctt->nextComtIndex(ctx)) {
                set<int>& links = ctt->getLinks(ctx);
                set<int>::iterator p;
                for (p = links.begin(); p != links.end(); p++) {
                        int cLnk = *p; int lnk = ctt->getLink(cLnk);
			int qid = qm->allocQ(lnk);
			if (qid == 0) return false;
			ctt->setLinkQ(cLnk,qid);
			qm->setQRates(qid,rs);
			if (lt->getPeerType(lnk) == ROUTER)
				qm->setQLimits(qid,100,200000);
			else
				qm->setQLimits(qid,50,100000);
			sm->clearQuStats(qid);
		}
	}
	return true;
}

/** Check all router tables for mutual consistency.
 *  Sends error messages to cerr if inconsistencies are found.
 *  This does not verify the consistency of interface, link and comtree rates.
 *  That is left to the setAvailRates() method.
 *  @return true on success, false on failure
 */
bool RouterCore::checkTables() {
	bool success = true;

	// verify that the default interface is valid and
	// that each interface has a non-zero IP address
	if (!ift->valid(ift->getDefaultIface())) {
		cerr << "RouterCore::checkTables: specified default iface "
		     << ift->getDefaultIface() << " is invalid\n";
		success = false;
	}
	int iface;
	for (iface = ift->firstIface(); iface != 0;
	     iface = ift->nextIface(iface)) {
		if (ift->getIpAdr(iface) == 0) {
			cerr << "RouterCore::checkTables: interface "
			     << iface << " has zero for IP address\n";
			success = false;
		}
	}

	// verify that each link is assigned to a valid interface
	// that the peer Ip address and port are non-zero and consistent,
	// the the peer type is valid and that the peer address is a valid
	// unicast forest address
	int lnk;
	for (lnk = lt->firstLink(); lnk != 0; lnk = lt->nextLink(lnk)) {
		iface = lt->getIface(lnk);
		if (!ift->valid(iface)) {
			cerr << "RouterCore::checkTables: interface " << iface
			     << " for link " << lnk << " is not valid\n";
			success = false;
		}
		if (lt->getPeerIpAdr(lnk) == 0) {
			cerr << "RouterCore::checkTables: invalid peer IP "
			     << "for link " << lnk << endl;
			success = false;
		}
		if (!Forest::validUcastAdr(lt->getPeerAdr(lnk))) {
			cerr << "RouterCore::checkTables: invalid peer address "
			     << "for link " << lnk << endl;
			success = false;
		}
	}

	// verify that the links in each comtree are valid,
	// that the router links and core links refer to peer routers,
	// and that each comtree link is consistent
	int ctx;
	for (ctx = ctt->firstComtIndex(); ctx != 0;
	     ctx = ctt->nextComtIndex(ctx)) {
		int comt = ctt->getComtree(ctx);
		int plnk = ctt->getPlink(ctx);
		int pcLnk = ctt->getPCLink(ctx);
		if (plnk != ctt->getLink(pcLnk)) {
			cerr << "RouterCore::checkTables: parent link "
			     <<  plnk << " not consistent with pcLnk\n";
			success = false;
		}
		if (ctt->inCore(ctx) && plnk != 0 && !ctt->isCoreLink(pcLnk)) {
			cerr << "RouterCore::checkTables: parent link "
			     <<  plnk << " of core node does not lead to "
			     << "another core node\n";
			success = false;
		}
		set<int>& links = ctt->getLinks(ctx);
		set<int>::iterator p;
		for (p = links.begin(); p != links.end(); p++) {
			int cLnk = *p; int lnk = ctt->getLink(cLnk);
			if (!lt->valid(lnk)) {
				cerr << "RouterCore::checkTables: link "
				     << lnk << " in comtree " << comt
				     << " not in link table" << endl;
				success = false;
				continue;
			}
			fAdr_t dest = ctt->getDest(cLnk);
			if (dest != 0 && !Forest::validUcastAdr(dest)) {
				cerr << "RouterCore::checkTables: dest addr "
				     << "for " << lnk << " in comtree " << comt
				     << " is not valid" << endl;
				success = false;
			}
			int qid = ctt->getLinkQ(cLnk);
			if (qid == 0) {
				cerr << "RouterCore::checkTables: queue id "
				     << "for " << lnk << " in comtree " << comt
				     << " is zero" << endl;
				success = false;
			}
		}
		if (!success) break;
		set<int>& rtrLinks = ctt->getRtrLinks(ctx);
		for (p = rtrLinks.begin(); p != rtrLinks.end(); p++) {
			int cLnk = *p; int lnk = ctt->getLink(cLnk);
			if (!ctt->isLink(ctx,lnk)) {
				cerr << "RouterCore::checkTables: router link "
				     << lnk << " is not valid in comtree "
				     << comt << endl;
				success = false;
			}
			if (lt->getPeerType(lnk) != ROUTER) {
				cerr << "RouterCore::checkTables: router link "
				     << lnk << " in comtree " << comt 
				     << " connects to non-router peer\n";
				success = false;
			}
		}
		set<int>& coreLinks = ctt->getCoreLinks(ctx);
		for (p = coreLinks.begin(); p != coreLinks.end(); p++) {
			int cLnk = *p; int lnk = ctt->getLink(cLnk);
			if (!ctt->isRtrLink(ctx,lnk)) {
				cerr << "RouterCore::checkTables: core link "
				     << lnk << " is not a router link "
				     << comt << endl;
				success = false;
			}
		}
	}
	// come back later and add checks for route table
	return success;
}

/** Set available rates for interfaces and links.
 *  Sends error messages to cerr if specified rates lead to over-subscription.
 *  @return true on success, false if specified rates oversubscribe
 *  an interface or link
 */
bool RouterCore::setAvailRates() {
	bool success = true;
	RateSpec minRates(Forest::MINBITRATE,Forest::MINBITRATE,
			  Forest::MINPKTRATE,Forest::MINPKTRATE);
	RateSpec maxRates(Forest::MAXBITRATE,Forest::MAXBITRATE,
			  Forest::MAXPKTRATE,Forest::MAXPKTRATE);
	int iface;
	for (iface = ift->firstIface(); iface != 0;
	     iface = ift->nextIface(iface)) {
		RateSpec ifRates = ift->getRates(iface);
		if (!minRates.leq(ifRates) || !ifRates.leq(maxRates)) {
			cerr << "RouterCore::setAvailRates: interface rates "
				"outside allowed range\n";
			success = false;
		}
		ift->getAvailRates(iface) = ifRates;
	}
	if (!success) return false;
	int lnk;
	for (lnk = lt->firstLink(); lnk != 0; lnk = lt->nextLink(lnk)) {
		RateSpec lnkRates = lt->getRates(lnk);
		if (!minRates.leq(lnkRates) || !lnkRates.leq(maxRates)) {
			cerr << "RouterCore::setAvailRates: link rates "
				"outside allowed range\n";
			success = false;
		}
		iface = lt->getIface(lnk);
		RateSpec ifAvail = ift->getRates(iface);
		if (!lnkRates.leq(ifAvail)) {
			cerr << "RouterCore::setAvailRates: oversubscribing "
				"interface " << iface << endl;
			success = false;
		}
		ift->getAvailRates(iface).subtract(lnkRates);
		lnkRates.scale(.9); // allocate at most 90% of link
		lt->getAvailRates(lnk) = lnkRates;
		sm->clearLnkStats(lnk);
	}
	if (!success) return false;
	int ctx;
	for (ctx = ctt->firstComtIndex(); ctx != 0;
	     ctx = ctt->nextComtIndex(ctx)) {
		set<int>& comtLinks = ctt->getLinks(ctx);
		set<int>::iterator p;
		for (p = comtLinks.begin(); p != comtLinks.end(); p++) {
			int cLnk = *p; int lnk = ctt->getLink(cLnk);
			RateSpec comtRates = ctt->getRates(cLnk);
			if (!comtRates.leq(lt->getAvailRates(lnk))) {
				cerr << "RouterCore::setAvailRates: "
					"oversubscribing link "
				     << lnk << endl;
				success = false;
			}
			lt->getAvailRates(lnk).subtract(comtRates);
		}
	}
	return success;
}

/** Add routes to neighboring leaf nodes and to routers in foreign zip codes.
 *  Routes are added in all comtrees. 
 */
void RouterCore::addLocalRoutes() {
	for (int ctx = ctt->firstComtIndex(); ctx != 0;
	     	 ctx = ctt->nextComtIndex(ctx)) {
		int comt = ctt->getComtree(ctx);
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

/** Write the contents of all router tables to an output stream.
 *  @param out is an open output stream
 */
void RouterCore::dump(ostream& out) {
	string s;
	out << "Interface Table\n\n" << ift->toString(s) << endl;
	out << "Link Table\n\n" << lt->toString(s) << endl;
	out << "Comtree Table\n\n" << ctt->toString(s) << endl;
	out << "Routing Table\n\n" << rt->toString(s) << endl;
	out << "Statistics\n\n" << sm->toString(s) << endl;
}

/** Main router processing loop.
 *  This method executes a loop that does three primary things:
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
 *  Time is managed through a free-running clock, derived
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
	now = Misc::getTimeNs();
	if (booting) {
		if (!iop->setupBootSock(bootIp,nmIp))
			fatal("RouterCore:run: could not setup boot socket\n");
		string s1;
		cout << "sending boot request to " << Np4d::ip2string(nmIp,s1)
		     << endl;
		CtlPkt cp(CtlPkt::BOOT_REQUEST,CtlPkt::REQUEST,0);
		if (!sendCpReq(cp,nmAdr))
			fatal("RouterCore::run: could not send boot request\n");
	}
	// create packet log to record a sample of packets handled

	uint64_t statsTime = 0;		// time statistics were last processed
	bool didNothing;
	int controlCount = 20;		// used to limit overhead of control
					// packet processing
	queue<int> ctlQ;		// queue for control packets

	now = Misc::getTimeNs();
	finishTime *= 1000000000; // convert from seconds to ns
	while (finishTime == 0 || now < finishTime) {
		didNothing = true;

		// input processing
		pktx px = iop->receive();
		if (px != 0) {
			didNothing = false;
			Packet& p = ps->getPacket(px);
			int ptype = p.type;
            		pktLog->log(px,p.inLink,false,now);
			int ctx = ctt->getComtIndex(p.comtree);
			if (!pktCheck(px,ctx)) {
				ps->free(px);
			} else if (booting) {
				handleCtlPkt(px);
			} else if (ptype == CLIENT_DATA) {
				forward(px,ctx);
			} else if (ptype == SUB_UNSUB) {
				subUnsub(px,ctx);
			} else if (ptype == RTE_REPLY) {
				handleRteReply(px,ctx);
			} else if (ptype == CONNECT || ptype == DISCONNECT) {
				handleConnDisc(px);
			} else if (p.dstAdr != myAdr) {
				forward(px,ctx);
			} else {
				ctlQ.push(px);
			}
		}

		// output processing
		int lnk;
		while ((px = qm->deq(lnk, now)) != 0) {
			didNothing = false;
			pktLog->log(px,lnk,true,now);
			iop->send(px,lnk);
		}

		// control packet processing
		if (!ctlQ.empty() && (didNothing || --controlCount <= 0)) {
			handleCtlPkt(ctlQ.front());
			ctlQ.pop();
			controlCount = 20; // do one control packet for
					   // every 20 iterations when busy
		}

		// every 300 ms, update statistics and check for un-acked
		// control packets
		if (now - statsTime > 300000000) {
			sm->record(now);
			statsTime = now;
			resendCpReq();
			didNothing = false;
		}

		// if did nothing on that pass, sleep for a millisecond.
		if (didNothing) { usleep(1000); }

		// update current time
		now = Misc::getTimeNs();
	}

	// write out recorded events
	pktLog->write(cout);
	cout << endl;
	cout << sm->iPktCnt(0) << " packets received, "
	     <<	sm->oPktCnt(0) << " packets sent\n";
	cout << sm->iPktCnt(-1) << " from routers,    "
	     << sm->oPktCnt(-1) << " to routers\n";
	cout << sm->iPktCnt(-2) << " from clients,    "
	     << sm->oPktCnt(-2) << " to clients\n";
}

/** Perform error checks on forest packet.
 *  @param px is a packet index
 *  @param ctx is the comtree index for p's comtree
 *  @return true if all checks pass, else false
 */
bool RouterCore::pktCheck(pktx px, int ctx) {
	Packet& p = ps->getPacket(px);
	// check version and  length
	if (p.version != Forest::FOREST_VERSION) {
		return false;
	}

	if (p.length != p.bufferLen || p.length < Forest::HDR_LENG) {
		return false;
	}

	if (booting) {
		return 	p.srcAdr == nmAdr && p.dstAdr == myAdr &&
		    	p.type == NET_SIG && p.comtree == Forest::NET_SIG_COMT;
	}

	if (!ctt->validComtIndex(ctx)) {
		return false;
	}
	fAdr_t adr = p.dstAdr;
	if (!Forest::validUcastAdr(adr) && !Forest::mcastAdr(adr)) {
		return false;
	}

	int inLink = p.inLink;
	if (inLink == 0) return false;
	int cLnk = ctt->getComtLink(ctt->getComtree(ctx),inLink);
	if (cLnk == 0) {
		return false;
	}

	// extra checks for packets from untrusted peers
	if (lt->getPeerType(inLink) < TRUSTED) {
		// check for spoofed source address
		if (lt->getPeerAdr(inLink) != p.srcAdr) return false;
		// and that destination restrictions are respected
		fAdr_t dest = ctt->getDest(cLnk);
		if (dest!=0 && p.dstAdr != dest && p.dstAdr != myAdr)
			return false;
		// verify that type is valid
		ptyp_t ptype = p.type;
		if (ptype != CLIENT_DATA &&
		    ptype != CONNECT && ptype != DISCONNECT &&
		    ptype != SUB_UNSUB && ptype != CLIENT_SIG)
			return false;
		int comt = ctt->getComtree(ctx);
		if ((ptype == CONNECT || ptype == DISCONNECT) &&
		     comt != (int) Forest::CLIENT_CON_COMT)
			return false;
		if (ptype == CLIENT_SIG &&
		    comt != (int) Forest::CLIENT_SIG_COMT)
			return false;
	}
	return true;
}

/** Lookup routing entry and forward packet accordingly.
 *  There are two contexts in which this method is called.
 *  The most common case is to forward a CLIENT_DATA packet.
 *  The other case is to forward a control packet that originates
 *  from this router. In this case the inLink field of the packet's
 *  header should be zero.
 *  @param px is a packet number for a CLIENT_DATA packet
 *  @param ctx is the comtree table index for the comtree in p's header
 */
void RouterCore::forward(pktx px, int ctx) {
	Packet& p = ps->getPacket(px);
	int rtx = rt->getRteIndex(p.comtree,p.dstAdr);
	if (rtx != 0) { // valid route case
		// reply to route request
		if ((p.flags & Forest::RTE_REQ)) {
			sendRteReply(px,ctx);
			p.flags = (p.flags & (~Forest::RTE_REQ));
			p.pack();
			p.hdrErrUpdate();
		}
		if (Forest::validUcastAdr(p.dstAdr)) {
			int rcLnk = rt->getLink(rtx);
			int lnk = ctt->getLink(rcLnk);
			int qid = ctt->getLinkQ(rcLnk);
			if (lnk == p.inLink || !qm->enq(px,qid,now)) {
				ps->free(px);
			}
			return;
		}
		// multicast data packet
		multiSend(px,ctx,rtx);
		return;
	}
	// no valid route
// think about suppressing flooding, if address is in range for this router
	if (Forest::validUcastAdr(p.dstAdr)) {
		// send to neighboring routers in comtree
		p.flags = Forest::RTE_REQ;
		p.pack(); p.hdrErrUpdate();
	}
	multiSend(px,ctx,rtx);
	return;
}
/** Forward multiple copies of a packet.
 *  There are two contexts in which this method is called.
 *  The most common case is to forward a CLIENT_DATA packet.
 *  The other case is to forward a control packet that originates
 *  from this router. In this case the inLink field of the packet's
 *  header should be zero.
 *  @param px is the number of a multi-destination packet
 *  @param ctx is the comtree index for the comtree in p's header
 *  @param rtx is the route index for p, or 0 if there is no route
 */
void RouterCore::multiSend(pktx px, int ctx, int rtx) {
	int qvec[nLnks]; int n = 0;
	Packet& p = ps->getPacket(px);

	int inLink = p.inLink;
	if (Forest::validUcastAdr(p.dstAdr)) {
		// flooding a unicast packet to neighboring routers
		int myZip = Forest::zipCode(myAdr);
		int pZip = Forest::zipCode(p.dstAdr);
		set<int>& rtrLinks = ctt->getRtrLinks(ctx);
		set<int>::iterator lp;
		for (lp = rtrLinks.begin(); lp != rtrLinks.end(); lp++) {
			int rcLnk = *lp; int lnk = ctt->getLink(rcLnk);
			int peerZip = Forest::zipCode(lt->getPeerAdr(lnk));
			if (pZip == myZip && peerZip != myZip) continue;
			if (lnk == inLink) continue;
			qvec[n++] = ctt->getLinkQ(rcLnk);
		}
	} else { 
		// forwarding a multicast packet
		// first identify neighboring core routers to get copies
		int pLink = ctt->getPlink(ctx);	
		set<int>& coreLinks = ctt->getCoreLinks(ctx);
		set<int>::iterator lp;
		for (lp = coreLinks.begin(); lp != coreLinks.end(); lp++) {
			int rcLnk = *lp; int lnk = ctt->getLink(rcLnk);
			if (lnk == inLink || lnk == pLink) continue;
			qvec[n++] = ctt->getLinkQ(rcLnk);
		}
		// now copy for parent
		if (pLink != 0 && pLink != inLink) {
			qvec[n++] = ctt->getLinkQ(ctt->getPCLink(ctx));
		}
		// now, copies for subscribers if any
		if (rtx != 0) {
			set<int>& subLinks = rt->getSubLinks(rtx); 
			for (lp = subLinks.begin(); lp !=subLinks.end(); lp++) {
				int rcLnk = *lp; int lnk = ctt->getLink(rcLnk);
				if (lnk == inLink) continue;
				qvec[n++] = ctt->getLinkQ(rcLnk);
			}
		}
	}

	if (n == 0) { ps->free(px); return; }

	// make copies and queue them
        pktx px1 = px;
        for (int i = 0; i < n-1; i++) { // process first n-1 copies
                if (qm->enq(px1,qvec[i],now)) {
			px1 = ps->clone(px);
		}
        }
        // process last copy
        if (!qm->enq(px1,qvec[n-1],now)) {
		ps->free(px1);
	}
}

/** Send route reply back towards p's source.
 *  The reply is sent on the link on which p was received and
 *  is addressed to p's original sender.
 *  @param px is a packet number
 *  @param ctx is the comtree index for p's comtree
 */
void RouterCore::sendRteReply(pktx px, int ctx) {
	Packet& p = ps->getPacket(px);

	pktx px1 = ps->alloc();
	Packet& p1 = ps->getPacket(px1);
	p1.length = Forest::HDR_LENG + 8;
	p1.type = RTE_REPLY;
	p1.flags = 0;
	p1.comtree = p.comtree;
	p1.srcAdr = myAdr;
	p1.dstAdr = p.srcAdr;

	p1.pack();
	(p1.payload())[0] = htonl(p.dstAdr);
	p1.hdrErrUpdate(); p.payErrUpdate();

	int cLnk = ctt->getComtLink(ctt->getComtree(ctx),p.inLink);
	qm->enq(px1,ctt->getLinkQ(cLnk),now);
}

/** Handle a route reply packet.
 *  Adds a route to the destination of the original packet that
 *  triggered the route reply, if no route is currently defined.
 *  If there is no route to the destination address in the packet,
 *  the packet is flooded to neighboring routers.
 *  If there is a route to the destination, it is forwarded along
 *  that route, so long as the next hop is another router.
 */
void RouterCore::handleRteReply(pktx px, int ctx) {
	Packet& p = ps->getPacket(px);
	int rtx = rt->getRteIndex(p.comtree, p.dstAdr);
	int cLnk = ctt->getComtLink(ctt->getComtree(ctx),p.inLink);
	if ((p.flags & Forest::RTE_REQ) && rtx != 0)
		sendRteReply(px,ctx);
	int adr = ntohl((p.payload())[0]);
	if (Forest::validUcastAdr(adr) &&
	    rt->getRteIndex(p.comtree,adr) == 0) {
		rt->addEntry(p.comtree,adr,cLnk); 
	}
	if (rtx == 0) {
		// send to neighboring routers in comtree
		p.flags = Forest::RTE_REQ;
		p.pack(); p.hdrErrUpdate();
		multiSend(px,ctx,rtx);
		return;
	}
	int dcLnk = rt->getLink(rtx); int dLnk = ctt->getLink(dcLnk);
	if (lt->getPeerType(dLnk) != ROUTER || !qm->enq(px,dLnk,now))
		ps->free(px);
	return;
}

/** Perform subscription processing on a packet.
 *  The packet contains two lists of multicast addresses,
 *  each preceded by its length. The combined list lengths
 *  is limited to 350.
 *  @param px is a packet number
 *  @param ctx is the comtree index for p's comtree
 */
void RouterCore::subUnsub(pktx px, int ctx) {
	Packet& p = ps->getPacket(px);
	uint32_t *pp = p.payload();

	// add/remove branches from routes
	// if non-core node, also propagate requests upward as
	// appropriate
	int comt = ctt->getComtree(ctx);
	int inLink = p.inLink;
	int cLnk = ctt->getComtLink(comt,inLink);
	// ignore subscriptions from the parent or core neighbors
	if (inLink == ctt->getPlink(ctx) || ctt->isCoreLink(cLnk)) {
		ps->free(px); return;
	}
	bool propagate = false;
	int rtx; fAdr_t addr;

	// add subscriptions
	int addcnt = ntohl(pp[0]);
	if (addcnt < 0 || addcnt > 350 ||
	    Forest::OVERHEAD + (addcnt + 2)*4 > p.length) {
		ps->free(px); return;
	}
	for (int i = 1; i <= addcnt; i++) {
		addr = ntohl(pp[i]);
		if (!Forest::mcastAdr(addr)) continue;  // ignore unicast or 0
		rtx = rt->getRteIndex(comt,addr);
		if (rtx == 0) { 
			rtx = rt->addEntry(comt,addr,cLnk);
			propagate = true;
		} else if (!rt->isLink(rtx,cLnk)) {
			rt->addLink(rtx,cLnk);
			pp[i] = 0; // so, parent will ignore
		}
	}
	// remove subscriptions
	int dropcnt = ntohl(pp[addcnt+1]);
	if (dropcnt < 0 || addcnt + dropcnt > 350 ||
	    Forest::OVERHEAD + (addcnt + dropcnt + 2)*4 > p.length) {
		ps->free(px); return;
	}
	for (int i = addcnt + 2; i <= addcnt + dropcnt + 1; i++) {
		addr = ntohl(pp[i]);
		if (!Forest::mcastAdr(addr)) continue; // ignore unicast or 0
		rtx = rt->getRteIndex(comt,addr);
		if (rtx == 0) continue;
		rt->removeLink(rtx,cLnk);
		if (rt->noLinks(rtx)) {
			rt->removeEntry(rtx);
			propagate = true;
		} else {
			pp[i] = 0;
		}
	}
	// propagate subscription packet to parent if not a core node
	if (propagate && !ctt->inCore(ctx) && ctt->getPlink(ctx) != 0) {
		p.payErrUpdate();
		int qid = ctt->getLinkQ(ctt->getPCLink(ctx));
		if (qm->enq(px,qid,now)) {
			return;
		}
	}
	ps->free(px); return;
}

/** Handle a CONNECT or DISCONNECT packet.
 *  @param px is the packet number of the packet to be handled.
 */
void RouterCore::handleConnDisc(pktx px) {
	Packet& p = ps->getPacket(px);
	int inLnk = p.inLink;

	if (!validLeafAdr(p.srcAdr)) {
		ps->free(px); return;
	}
	if (p.type == CONNECT) {
		if (lt->getPeerPort(inLnk) == 0) {
			lt->setPeerPort(inLnk,p.tunPort);
		}
/*
		if (lt->getPeerPort(inLnk) != p.tunPort) {
			if (lt->getPeerPort(inLnk) != 0) {
				string s;
				cerr << "modifying peer port for host with ip "
				     << Np4d::ip2string(p.tunIp,s)
				     << endl;
			}
			lt->setPeerPort(inLnk,p.tunPort);
		}
*/
		if (nmAdr != 0 && lt->getPeerType(inLnk) == CLIENT) {
			CtlPkt cp(CtlPkt::CLIENT_CONNECT,CtlPkt::REQUEST,0);
			cp.adr1 = p.srcAdr; cp.adr2 = myAdr;
			sendCpReq(cp,nmAdr);
		}
	} else if (p.type == DISCONNECT) {
		if (lt->getPeerPort(inLnk) == p.tunPort) {
			dropLink(inLnk);
		}
		if (nmAdr != 0 && lt->getPeerType(inLnk) == CLIENT) {
			CtlPkt cp(CtlPkt::CLIENT_DISCONNECT,CtlPkt::REQUEST,0);
			cp.adr1 = p.srcAdr; cp.adr2 = myAdr;
			sendCpReq(cp,nmAdr);
		}
	}
	ps->free(px); return;
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
 *  @param px is a PacketStore index
 */
void RouterCore::handleCtlPkt(int px) {
	Packet& p = ps->getPacket(px);
	CtlPkt cp(p.payload(), p.length - Packet::OVERHEAD);

	if (!cp.unpack()) {
		string s;
		cerr << "RouterCore::handleCtlPkt: misformatted control "
			" packet\n" << p.toString(s);
		cp.reset(cp.type,CtlPkt::NEG_REPLY,cp.seqNum);
		cp.mode = CtlPkt::NEG_REPLY;
		cp.errMsg = "misformatted control packet";
		returnToSender(px,cp);
		return;
	}
	if (p.type != NET_SIG || p.comtree != Forest::NET_SIG_COMT) {
		ps->free(px); return;
	}
	if (cp.mode != CtlPkt::REQUEST) { 
		handleCpReply(px,cp); return;
	}
	
	// Prepare positive reply packet for use where appropriate
	CtlPkt reply(cp.type,CtlPkt::POS_REPLY,cp.seqNum);

	switch (cp.type) {

	// configuring logical interfaces
	case CtlPkt::ADD_IFACE: 	addIface(cp,reply); break;
        case CtlPkt::DROP_IFACE:	dropIface(cp,reply); break;
        case CtlPkt::GET_IFACE:		getIface(cp,reply); break;
        case CtlPkt::MOD_IFACE:		modIface(cp,reply); break;

	// configuring links
        case CtlPkt::ADD_LINK:		addLink(cp,reply); break;
        case CtlPkt::DROP_LINK:		dropLink(cp,reply); break;
        case CtlPkt::GET_LINK:		getLink(cp,reply); break;
        case CtlPkt::MOD_LINK:		modLink(cp,reply); break;

	// configuring comtrees
        case CtlPkt::ADD_COMTREE:	addComtree(cp,reply); break;
        case CtlPkt::DROP_COMTREE:	dropComtree(cp,reply); break;
        case CtlPkt::GET_COMTREE:	getComtree(cp,reply); break;
        case CtlPkt::MOD_COMTREE:	modComtree(cp,reply); break;
	case CtlPkt::ADD_COMTREE_LINK:	addComtreeLink(cp,reply); break;
	case CtlPkt::DROP_COMTREE_LINK: dropComtreeLink(cp,reply); break;
	case CtlPkt::GET_COMTREE_LINK:	getComtreeLink(cp,reply); break;
	case CtlPkt::MOD_COMTREE_LINK:	modComtreeLink(cp,reply); break;

	// configuring routes
        case CtlPkt::ADD_ROUTE:		addRoute(cp,reply); break;
        case CtlPkt::DROP_ROUTE:	dropRoute(cp,reply); break;
        case CtlPkt::GET_ROUTE:		getRoute(cp,reply); break;
        case CtlPkt::MOD_ROUTE:		modRoute(cp,reply); break;

	// finishing up boot phase
	case CtlPkt::BOOT_COMPLETE:	bootComplete(px,cp,reply); break;

	// aborting boot process
	case CtlPkt::BOOT_ABORT:	bootAbort(px,cp,reply); break;
	
	default:
		cerr << "unrecognized control packet type " << cp.type
		     << endl;
		reply.errMsg = "invalid control packet for router";
		reply.mode = CtlPkt::NEG_REPLY;
		break;
	}

	returnToSender(px,reply);

	if (reply.type == CtlPkt::BOOT_COMPLETE) {
		iop->closeBootSock();
		booting = false;
	}

	return;
}

/** Handle an ADD_IFACE control packet.
 *  Adds the specified interface and prepares a reply packet.
 *  @param px is a packet number
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param reply is a control packet structure for the reply, which
 *  the cpType has been set to match cp, the rrType is POS_REPLY
 *  and the seqNum has been set to match cp.
 *  @param return true on success, false on failure; on a successful
 *  return, all appropriate attributes in reply are set; on a failure
 *  return, the errMsg field of reply is set.
 */
bool RouterCore::addIface(CtlPkt& cp, CtlPkt& reply) {
	int iface = cp.iface;
	ipa_t localIp = cp.ip1;
	RateSpec rs(max(min(cp.rspec1.bitRateUp,  Forest::MAXBITRATE),
						  Forest::MINBITRATE),
		    max(min(cp.rspec1.bitRateDown,Forest::MAXBITRATE),
						  Forest::MINBITRATE),
		    max(min(cp.rspec1.pktRateUp,  Forest::MAXPKTRATE),
						  Forest::MINPKTRATE),
		    max(min(cp.rspec1.pktRateDown,Forest::MAXPKTRATE),
						  Forest::MINPKTRATE));
	if (ift->valid(iface)) {
		if (localIp != ift->getIpAdr(iface) ||
		    !rs.equals(ift->getRates(iface))) { 
			reply.errMsg = "add iface: requested interface "
				 	"conflicts with existing interface";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
	} else if (!ift->addEntry(iface, localIp, rs)) {
		reply.errMsg = "add iface: cannot add interface";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	return true;
}

bool RouterCore::dropIface(CtlPkt& cp, CtlPkt& reply) {
	ift->removeEntry(cp.iface);
	return true;
}

bool RouterCore::getIface(CtlPkt& cp, CtlPkt& reply) {
	int iface = cp.iface;
	if (ift->valid(iface)) {
		reply.iface = iface;
		reply.ip1 = ift->getIpAdr(iface);
		reply.rspec1 = ift->getRates(iface);
		reply.rspec2 = ift->getAvailRates(iface);
		return true;
	}
	reply.errMsg = "get iface: invalid interface";
	reply.mode = CtlPkt::NEG_REPLY;
	return false;
}

bool RouterCore::modIface(CtlPkt& cp, CtlPkt& reply) {
	int iface = cp.iface;
	if (ift->valid(iface)) {
		ift->getRates(iface) = cp.rspec1;
		return true;
	}
	reply.errMsg = "mod iface: invalid interface";
	reply.mode = CtlPkt::NEG_REPLY;
	return false;
}

bool RouterCore::addLink(CtlPkt& cp, CtlPkt& reply) {
	ntyp_t peerType = cp.nodeType;
	if (peerType == ROUTER && cp.adr1 == 0) {
		reply.errMsg = "add link: adding link to router, but no peer "
				"address supplied";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	ipa_t  pipa = cp.ip1;
	int lnk = cp.link;
	int iface = (cp.iface != 0 ? cp.iface : ift->getDefaultIface());
	ipp_t pipp = (cp.port1 != 0 ? cp.port1 :
		      (peerType == ROUTER ? Forest::ROUTER_PORT : 0));
	fAdr_t padr = cp.adr1;
	int xlnk = lt->lookup(pipa,pipp);
	if (xlnk != 0) { // this link already exists
		if ((lnk != 0 && lnk != xlnk) ||
		    (peerType != lt->getPeerType(xlnk)) ||
		    (cp.iface != 0 && cp.iface != lt->getIface(xlnk)) ||
		    (padr != 0 && padr != lt->getPeerAdr(xlnk))) {
			reply.errMsg = "add link: new link conflicts "
						"with existing link";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		lnk = xlnk; padr = lt->getPeerAdr(xlnk);
	} else { // adding a new link
		// first ensure that the interface has enough
		// capacity to support a new link of minimum capacity
		RateSpec rs(Forest::MINBITRATE, Forest::MINPKTRATE,
			    Forest::MINBITRATE, Forest::MINPKTRATE);
		RateSpec& availRates = ift->getAvailRates(iface);
		if (!rs.leq(availRates)) {
			reply.errMsg = "add link: requested link "
						"exceeds interface capacity";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		if ((peerType == ROUTER && pipp != Forest::ROUTER_PORT) ||
		    (peerType != ROUTER && pipp == Forest::ROUTER_PORT) ||
		    (lnk = lt->addEntry(lnk,pipa,pipp)) == 0) {
			reply.errMsg = "add link: cannot add "
				 		"requested link";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		// note: when lt->addEntry succeeds, link rates are
		// initializied to Forest minimum rates
		if (peerType != ROUTER && padr != 0 && !allocLeafAdr(padr)) {
			lt->removeEntry(lnk);
			reply.errMsg = "add link: specified peer "
						"address is in use";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		if (padr == 0) padr = allocLeafAdr();
		if (padr == 0) {
			lt->removeEntry(lnk);
			reply.errMsg = "add link: no available "
						"peer addresses";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		availRates.subtract(rs);
		lt->setIface(lnk,iface);
		lt->setPeerType(lnk,peerType);
		lt->setPeerAdr(lnk,padr);
		sm->clearLnkStats(lnk);
	}
	reply.link = lnk;
	reply.adr1 = padr;
	reply.ip1 = ift->getIpAdr(iface);
	return true;
}

bool RouterCore::dropLink(CtlPkt& cp, CtlPkt& reply) {
	dropLink(cp.link);
	return true;
}

void RouterCore::dropLink(int lnk) {
	int *comtVec = new int[lt->getComtSet(lnk).size()];
	int i = 0;
	for (int comt : lt->getComtSet(lnk)) comtVec[i++] = comt;
	while (--i >= 0) {
		int ctx = comtVec[i];
		int cLnk = ctt->getComtLink(ctt->getComtree(ctx),lnk);
		dropComtreeLink(ctx,lnk,cLnk);
	}
	int iface = lt->getIface(lnk);
	ift->getAvailRates(iface).add(lt->getRates(lnk));
	lt->removeEntry(lnk);
	freeLeafAdr(lt->getPeerAdr(lnk));
}

bool RouterCore::getLink(CtlPkt& cp, CtlPkt& reply) {
	int link = cp.link;
	if (lt->valid(link)) {
		reply.link = link;
		reply.iface = lt->getIface(link);
		reply.ip1 = lt->getPeerIpAdr(link);
		reply.nodeType = lt->getPeerType(link);
		reply.port1 = lt->getPeerPort(link);
		reply.adr1 = lt->getPeerAdr(link);
		reply.rspec1 = lt->getRates(link);
		reply.rspec2 = lt->getAvailRates(link);
		return true;
	} 
	reply.errMsg = "get link: invalid link number";
	reply.mode = CtlPkt::NEG_REPLY;
	return false;
}

bool RouterCore::modLink(CtlPkt& cp, CtlPkt& reply) {
	int link = cp.link;
	if (!lt->valid(link)) {
		reply.errMsg = "get link: invalid link number";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	reply.link = link;
	int iface = lt->getIface(link);
	if (cp.rspec1.isSet()) {
		RateSpec& linkRates = lt->getRates(link);
		RateSpec& ifAvail = ift->getAvailRates(iface);
		RateSpec delta = cp.rspec1;
		delta.subtract(linkRates);
		if (!delta.leq(ifAvail)) {
			string s;
			reply.errMsg = "mod link: request "
				+ cp.rspec1.toString(s) +
				"exceeds interface capacity";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		ifAvail.subtract(delta);
		linkRates = cp.rspec1;
		qm->setLinkRates(link,cp.rspec1);
		cp.rspec1.scale(.9);   // allocate at most 90% of link
		lt->getAvailRates(link) = cp.rspec1;
	}
	return true;
}

bool RouterCore::addComtree(CtlPkt& cp, CtlPkt& reply) {
	int comt = cp.comtree;
	if(ctt->validComtree(comt) || ctt->addEntry(comt) != 0)
		return true;
	reply.errMsg = "add comtree: cannot add comtree";
	reply.mode = CtlPkt::NEG_REPLY;
	return false;
}

bool RouterCore::dropComtree(CtlPkt& cp, CtlPkt& reply) {
	int comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (!ctt->validComtIndex(ctx))
		return true; // so dropComtree op is idempotent

	// remove all routes involving this comtree
	// also degisters each route in the comtree table
	rt->purgeRoutes(comt);

	// remove all the comtree links
	// first, copy out the comtree links, then remove them
	// two step process is needed because dropComtreeLink modifies linkSet
	set<int>& linkSet = ctt->getLinks(ctx);
	set<int>::iterator pp;
	int *clnks = new int[linkSet.size()]; int i = 0;
	for (pp = linkSet.begin(); pp != linkSet.end(); pp++) clnks[i++] = *pp;
	while (--i >= 0)  {
		dropComtreeLink(ctx,ctt->getLink(clnks[i]),clnks[i]);
	}
	delete [] clnks;

	ctt->removeEntry(ctx); // and finally drop entry in comtree table
	return true;
}

bool RouterCore::getComtree(CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.errMsg = "get comtree: invalid comtree";
		reply.mode = CtlPkt::NEG_REPLY; 
		return false;
	}
	reply.comtree = comt;
	reply.coreFlag = ctt->inCore(ctx);
	reply.link = ctt->getPlink(ctx);
	reply.count = ctt->getLinkCount(ctx);
	return true;
}

bool RouterCore::modComtree(CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx != 0) {
		if (cp.coreFlag >= 0)
			ctt->setCoreFlag(ctx,cp.coreFlag);
		if (cp.link != 0) {
			int plnk = cp.link;
			if (plnk != 0 && !ctt->isLink(ctx,plnk)) {
				reply.errMsg = "specified link does "
						"not belong to comtree";
				reply.mode = CtlPkt::NEG_REPLY;
				return false;
			}
			if (plnk != 0 && !ctt->isRtrLink(ctx,plnk)) {
				reply.errMsg = "specified link does "
						"not connect to a router";
				reply.mode = CtlPkt::NEG_REPLY;
				return false;
			}
			ctt->setPlink(ctx,plnk);
		}
		return true;
	} 
	reply.errMsg = "modify comtree: invalid comtree";
	reply.mode = CtlPkt::NEG_REPLY;
	return false;
}

bool RouterCore::addComtreeLink(CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.errMsg = "add comtree link: invalid comtree";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int lnk = 0; 
	if (cp.link != 0) {
		lnk = cp.link;
	} else {
		lnk = lt->lookup(cp.ip1, cp.port1);
	}
	if (!lt->valid(lnk)) {
		reply.errMsg = "add comtree link: invalid link or "
					"peer IP and port";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	bool isRtr = (lt->getPeerType(lnk) == ROUTER);
	bool isCore = false;
	if (isRtr) {
		if(cp.coreFlag < 0) {
			reply.errMsg = "add comtree link: must specify "
					"core flag on links to routers";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		isCore = cp.coreFlag;
	}
	int cLnk = ctt->getComtLink(comt,lnk);
	if (cLnk != 0) {
		if (ctt->isRtrLink(cLnk) == isRtr &&
		    ctt->isCoreLink(cLnk) == isCore) {
			reply.link = lnk;
			return true;
		} else {
			reply.errMsg = "add comtree link: specified "
				       "link already in comtree";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
	}
	// define new comtree link
	if (!ctt->addLink(ctx,lnk,isRtr,isCore)) {
		reply.errMsg =	"add comtree link: cannot add "
				"requested comtree link";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	cLnk = ctt->getComtLink(comt,lnk);

	// add unicast route to cLnk if peer is a leaf or a router
	// in a different zip code
	fAdr_t peerAdr = lt->getPeerAdr(lnk);
	if (lt->getPeerType(lnk) != ROUTER) {
		int rtx = rt->getRteIndex(comt,peerAdr);
		if (rtx == 0) rt->addEntry(comt,peerAdr,cLnk);
	} else {
		int zipPeer = Forest::zipCode(peerAdr);
		if (zipPeer != Forest::zipCode(myAdr)) {
			fAdr_t dest = Forest::forestAdr(zipPeer,0);
			int rtx = rt->getRteIndex(comt,dest);
			if (rtx == 0) rt->addEntry(comt,dest,cLnk);
		}
	}

	// allocate queue and bind it to lnk and comtree link
	int qid = qm->allocQ(lnk);
	if (qid == 0) {
		ctt->removeLink(ctx,cLnk);
		reply.errMsg = "add comtree link: no queues "
					"available for link";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	ctt->setLinkQ(cLnk,qid);

	// adjust rates for link comtree and queue
	RateSpec minRates(Forest::MINBITRATE,Forest::MINBITRATE,
		    	  Forest::MINPKTRATE,Forest::MINPKTRATE);
	if (!minRates.leq(lt->getAvailRates(lnk))) {
		reply.errMsg = "add comtree link: request "
			       "exceeds link capacity";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	lt->getAvailRates(lnk).subtract(minRates);
	ctt->getRates(cLnk) = minRates;

	qm->setQRates(qid,minRates);
	if (isRtr) qm->setQLimits(qid,500,1000000);
	else	   qm->setQLimits(qid,500,1000000);
	sm->clearQuStats(qid);
	reply.link = lnk;
	return true;
}

bool RouterCore::dropComtreeLink(CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.errMsg = "drop comtree link: invalid comtree";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int lnk = cp.link;
	if (lnk == 0) lnk = lt->lookup(cp.ip1,cp.port1);
	if (!lt->valid(lnk)) {
		reply.errMsg = "drop comtree link: invalid link "
			       "or peer IP and port";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int cLnk = ctt->getComtLink(comt,lnk);
	if (cLnk != 0) {
		dropComtreeLink(ctx,lnk,cLnk);
	}
	return true;
}

void RouterCore::dropComtreeLink(int ctx, int lnk, int cLnk) {
	// release the link bandwidth used by comtree link
	lt->getAvailRates(lnk).add(ctt->getRates(cLnk));

	// remove unicast route for this comtree
	fAdr_t peerAdr = lt->getPeerAdr(lnk);
	int comt = ctt->getComtree(ctx);
	if (lt->getPeerType(lnk) != ROUTER) {
		int rtx = rt->getRteIndex(comt,peerAdr);
		if (rtx != 0) rt->removeEntry(rtx);
	} else {
		int zipPeer = Forest::zipCode(peerAdr);
		if (zipPeer != Forest::zipCode(myAdr)) {
			fAdr_t dest = Forest::forestAdr(zipPeer,0);
			int rtx = rt->getRteIndex(comt,dest);
			if (rtx != 0) rt->removeEntry(rtx);
		}
	}
	// remove cLnk from multicast routes for this comtree
	// must first copy out the routes that are registered for
	// cLnk, then remove cLnk from all these routes;
	// two step process is needed because the removal of the link from
	// the route, also deregisters the route in the comtree table
	// causing the rteSet to change
	set<int>& rteSet = ctt->getRteSet(cLnk);
	set<int>::iterator rp;
	int *routes = new int[rteSet.size()];
	int i = 0;
	for (rp = rteSet.begin(); rp != rteSet.end(); rp++) routes[i++] = *rp;
	while (--i >= 0) rt->removeLink(routes[i],cLnk);
	delete [] routes;

	// release queue and remove link from comtree
	// (which also deregisters comtree with lnk)
	int qid = ctt->getLinkQ(cLnk);
	qm->freeQ(qid);
	if (!ctt->removeLink(ctx,cLnk)) {
		cerr << "dropComtreeLink: internal error detected "
			"final removeLink failed\n";
	}
}

bool RouterCore::modComtreeLink(CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.errMsg = "modify comtree link: invalid comtree";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int lnk = cp.link;
	if (!lt->valid(lnk)) {
		reply.errMsg = "modify comtree link: invalid link number";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int cLnk = ctt->getComtLink(comt,lnk);
	if (cLnk == 0) {
		reply.errMsg = "modify comtree link: specified link "
			       "not defined in specified comtree";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}

	RateSpec rs = cp.rspec1;
	if (!rs.isSet()) return true;
	RateSpec diff = rs; diff.subtract(ctt->getRates(cLnk));
	if (!diff.leq(lt->getAvailRates(lnk))) {
		reply.errMsg = "modify comtree link: new rate spec "
				"exceeds available link capacity";
		return false;
	}
	lt->getAvailRates(lnk).subtract(diff);
	ctt->getRates(cLnk) = rs;
	return true;
}

bool RouterCore::getComtreeLink(CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.errMsg = "get comtree link: invalid comtree";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int lnk = cp.link;
	if (!lt->valid(lnk)) {
		reply.errMsg = "get comtree link: invalid link number";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int cLnk = ctt->getComtLink(comt,lnk);
	if (cLnk == 0) {
		reply.errMsg = "get comtree link: specified link "
			  	"not defined in specified comtree";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	reply.comtree = comt;
	reply.link = lnk;
	reply.queue = ctt->getLinkQ(cLnk);
	reply.adr1 = ctt->getDest(cLnk);
	reply.rspec1 = ctt->getRates(cLnk);

	return true;
}

bool RouterCore::addRoute(CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	if (!ctt->validComtree(comt)) {
		reply.errMsg = "comtree not defined at this router\n";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	fAdr_t dest = cp.adr1;
	if (!Forest::validUcastAdr(dest) && !Forest::mcastAdr(dest)) {
		reply.errMsg = "invalid address\n";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int lnk = cp.link;
	int cLnk = ctt->getComtLink(comt,lnk);
	int rtx = rt->getRteIndex(comt,dest);
	if (rtx != 0) {
	   	if ((Forest::validUcastAdr(dest) && rt->getLink(rtx) == cLnk) ||
		    (Forest::mcastAdr(dest) && rt->isLink(rtx,cLnk))) {
			return true;
		} else {
			reply.errMsg = "add route: requested route "
				        "conflicts with existing route";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
	} else if (rt->addEntry(comt, dest, lnk)) {
		return true;
	}
	reply.errMsg = "add route: cannot add route";
	reply.mode = CtlPkt::NEG_REPLY;
	return false;
}

bool RouterCore::dropRoute(CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	if (!ctt->validComtree(comt)) {
		reply.errMsg = "comtree not defined at this router\n";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	fAdr_t dest = cp.adr1;
	if (!Forest::validUcastAdr(dest) && !Forest::mcastAdr(dest)) {
		reply.errMsg = "invalid address\n";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int rtx = rt->getRteIndex(comt,dest);
	rt->removeEntry(rtx);
	return true;
}

bool RouterCore::getRoute(CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	if (!ctt->validComtree(comt)) {
		reply.errMsg = "comtree not defined at this router\n";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	fAdr_t dest = cp.adr1;
	if (!Forest::validUcastAdr(dest) && !Forest::mcastAdr(dest)) {
		reply.errMsg = "invalid address\n";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int rtx = rt->getRteIndex(comt,dest);
	if (rtx != 0) {
		reply.comtree = comt;
		reply.adr1 = dest;
		if (Forest::validUcastAdr(dest)) {
			int lnk = ctt->getLink(rt->getLink(rtx));
			reply.link = lnk;
		} else {
			reply.link = 0;
		}
		return true;
	}
	reply.errMsg = "get route: no route for specified address";
	reply.mode = CtlPkt::NEG_REPLY;
	return false;
}
        
bool RouterCore::modRoute(CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	if (!ctt->validComtree(comt)) {
		reply.errMsg = "comtree not defined at this router\n";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	fAdr_t dest = cp.adr1;
	if (!Forest::validUcastAdr(dest) && !Forest::mcastAdr(dest)) {
		reply.errMsg = "invalid address\n";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int rtx = rt->getRteIndex(comt,dest);
	if (rtx != 0) {
		if (cp.link != 0) {
			if (Forest::mcastAdr(dest)) {
				reply.errMsg = "modify route: cannot "
					       	"set link in multicast route";
				reply.mode = CtlPkt::NEG_REPLY;
				return false;
			}
			rt->setLink(rtx,cp.link);
		}
		return true;
	}
	reply.errMsg = "modify route: invalid route";
	reply.mode = CtlPkt::NEG_REPLY;
	return false;
}

/** Handle boot complete message.
 *  At the end of the boot phase, we do some additional setup and checking
 *  of the configuration tables. If the tables are inconsistent in some way,
 *  the operation fails. On successful completion of the boot phase,
 *  we close the boot socket and sent the booting variable to false.
 *  @param px is the number of the boot complete request packet
 *  @param cp is an unpacked control packet for p
 *  @param reply is a reference to a pre-formatted positive reply packet
 *  that will be returned to the sender; it may be modified to indicate
 *  a negative reply.
 *  @return true on success, false on failure
 */
bool RouterCore::bootComplete(pktx px, CtlPkt& cp, CtlPkt& reply) {
	if (booting && !setup()) {
		cerr << "RouterCore::bootComplete: setup failed after "
			"completion of boot phase\n";
		perror("");
		reply.errMsg = "configured tables are not consistent\n";
		reply.mode = CtlPkt::NEG_REPLY;
		returnToSender(px,reply);
		pktLog->write(cout);
		exit(1);
	}
	return true;
}

bool RouterCore::bootAbort(pktx px, CtlPkt& cp, CtlPkt& reply) {
	cerr << "RouterCore::bootAbort: received boot abort message "
			"from netMgr; exiting\n";
	reply.mode = CtlPkt::POS_REPLY;
	returnToSender(px,reply);
	pktLog->write(cout);
	exit(1);
}

/** Send control packet request.
 *  Builds a packet containing the specified control packet and stores
 *  its packet number and a timestamp in the map of pending control packet
 *  requests. Then, makes a copy, which is sent to the destination.
 *  @param cp is a reference to the control packet to be sent
 *  @param dest is the destination address to which the packet is to be sent
 *  @return true on success, false on failure
 */
bool RouterCore::sendCpReq(CtlPkt& cp, fAdr_t dest) {
	pktx px = ps->alloc();
	if (px == 0) {
		cerr << "RouterCore::sendCpReq: no packets left in packet "
			"store\n";
		return false;
	}
	Packet& p = ps->getPacket(px);

	// pack cp into p, setting mode and seq number
	cp.mode = CtlPkt::REQUEST;
	cp.seqNum = seqNum;
	cp.payload = p.payload();
	if (cp.pack() == 0) {
		cerr << "RouterCore::sendCpReq: control packet packing error\n";
		return false;
	}
	p.length = Forest::OVERHEAD + cp.paylen;
	p.type = NET_SIG; p.flags = 0;
	p.comtree = Forest::NET_SIG_COMT;
	p.srcAdr = myAdr; p.dstAdr = dest;
	p.inLink = 0;
	p.pack();

	// save a record of the packet in pending map
	pair<uint64_t,CpInfo> cpp;
	cpp.first = seqNum;
	cpp.second.px = px; cpp.second.nSent = 1; cpp.second.timestamp = now;
	pending->insert(cpp);
	seqNum++;

	// now, make copy of packet and send the copy
	int copy = ps->fullCopy(px);
	if (copy == 0) {
		cerr << "RouterCore::sendCpReq: no packets left in packet "
			"store\n";
		return false;
	}
	if (booting) {
		iop->send(copy,0);
		pktLog->log(copy,0,true,now);
	} else
		forward(copy,ctt->getComtIndex(p.comtree));

	return true;
}

/** Retransmit any pending control packets that have timed out.
 *  This method checks the map of pending requests and whenever
 *  it finds a packet that has been waiting for an acknowledgement
 *  for more than a second, it retransmits it or gives up, 
 *  if it has already made three attempts.
 */
void RouterCore::resendCpReq() {
	map<uint64_t,CpInfo>::iterator pp;
	for (pp = pending->begin(); pp != pending->end(); pp++) {
		if (now < pp->second.timestamp + 1000000000) continue;
		pktx px = pp->second.px;
		Packet& p = ps->getPacket(px);
		if (pp->second.nSent >= 3) { // give up on this packet
			string s;
			cerr << "RouterCore::resendCpReq: received no reply to "
				"control packet after three attempts\n"
			     << p.toString(s);
			ps->free(px);
			pending->erase(pp->first);
			continue;
		}
		string s1;
		cout << "resending control packet\n" << p.toString(s1);
		// make copy of packet and send the copy
		pp->second.timestamp = now;
		pp->second.nSent++;
		pktx copy = ps->fullCopy(px);
		if (copy == 0) {
			cerr << "RouterCore::resendCpReq: no packets left in "
				"packet store\n";
			return;
		}
		if (booting) {
			pktLog->log(copy,0,true,now);
			iop->send(copy,0);
		} else
			forward(copy,ctt->getComtIndex(p.comtree));
	}
}

/** Handle incoming replies to control packets.
 *  The reply is checked against the map of pending request packets,
 *  and if a match is found, the entry is removed from the map
 *  and the storage for the request packet is freed.
 *  Currently, the only action on a reply is to print an error message
 *  to the log if we receive a negative reply.
 *  @param reply is the packet number of the reply packet
 *  @param cpr is a reference to an unpacked control packet for reply
 */
void RouterCore::handleCpReply(pktx reply, CtlPkt& cpr) {
	map<uint64_t,CpInfo>::iterator pp = pending->find(cpr.seqNum);
	if (pp == pending->end()) {
		// this is a reply to a request we never sent, or
		// possibly, a reply to a request we gave up on
		ps->free(reply);
		return;
	}
	// so, remove it from the map of pending requests
	ps->free(pp->second.px);
	pending->erase(pp);

	// and then handle the reply
	switch (cpr.type) {
	case CtlPkt::CLIENT_CONNECT: case CtlPkt::CLIENT_DISCONNECT:
		if (cpr.mode == CtlPkt::NEG_REPLY) {
			cerr << "RouterCore::handleCpReply: got negative reply "
				"to a connect or disconnect request: "
				<< cpr.errMsg << endl;
			break;
		}
		// otherwise, nothing to do
		break;
	case CtlPkt::BOOT_REQUEST: {
		if (cpr.mode == CtlPkt::NEG_REPLY) {
			cerr << "RouterCore::handleCpReply: got "
				"negative reply to a boot request: "
				<< cpr.errMsg << endl;
			break;
		}
		// unpack first and last leaf addresses, initialize leafAdr
		if (cpr.adr1 == 0 || cpr.adr2 == 0) {
			cerr << "RouterCore::handleCpReply: reply to boot "
				"request did not include leaf address range\n";
			break;
		}
		firstLeafAdr = cpr.adr1;
        	fAdr_t lastLeafAdr = cpr.adr2;
		if (firstLeafAdr > lastLeafAdr) {
			cerr << "RouterCore::handleCpReply: reply to boot "
				"request contained empty leaf address range\n";
			break;
		}
		leafAdr = new UiSetPair((lastLeafAdr - firstLeafAdr)+1);
		break;
	}
	default:
		cerr << "RouterCore::handleCpReply: unexpected control packet "
			"type\n";
		break;
	}
	ps->free(reply);
}

/** Send packet back to sender.
 *  Update the length, flip the addresses and pack the buffer.
 *  @param px is the packet number
 *  @param paylen is the length of the payload in bytes
 */
void RouterCore::returnToSender(pktx px, CtlPkt& cp) {
	Packet& p = ps->getPacket(px);
	cp.payload = p.payload(); 
	int paylen = cp.pack();
	if (paylen == 0) {
		cerr << "RouterCore::returnToSender: control packet formatting "
			"error, zero payload length\n";
		ps->free(px);
	}
	p.length = (Packet::OVERHEAD + paylen);
	p.flags = 0;
	p.dstAdr = p.srcAdr;
	p.srcAdr = myAdr;
	p.pack();

	if (booting) {
		pktLog->log(px,0,true,now);
		iop->send(px,0);
		return;
	}

	int cLnk = ctt->getComtLink(p.comtree,p.inLink);
	int qn = ctt->getLinkQ(cLnk);
	if (!qm->enq(px,qn,now)) { ps->free(px); }
}

} // ends namespace