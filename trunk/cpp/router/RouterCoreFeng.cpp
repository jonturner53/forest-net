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
	args.portNum = 0; args.finTime = 0;

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
		} else if (s.compare(0,8,"portNum=") == 0) {
			sscanf(&argv[i][8],"%hd",&args.portNum);
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
	nPkts = 100000; nBufs = 50000; nQus = 10000;

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
	iop = new IoProcessor(nIfaces, config.portNum, ift, lt, ps, sm);
	qm = new QuManager(nLnks, nPkts, nQus, min(50,5*nPkts/nLnks), ps, sm);
	pktLog = new PacketLog(ps);

	if (!booting)
		leafAdr = new UiSetPair((config.lastLeafAdr - firstLeafAdr)+1);

	seqNum = 1;
	pending = new map<uint64_t,ControlInfo>();
	pending1 = new map<comt_t,pktx>();
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
		if (iop->ready(iface)) continue;
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
		if (booting || lt->getPeerType(lnk) == Forest::ROUTER) continue;
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
			if (lt->getPeerType(lnk) == Forest::ROUTER)
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
		if (lt->getPeerIpAdr(lnk) == 0 &&
		    lt->getPeerType(lnk) == Forest::ROUTER) {
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
			if (lt->getPeerType(lnk) != Forest::ROUTER) {
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
			if (lt->getPeerType(lnk) == Forest::ROUTER &&
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
		CtlPkt cp(CtlPkt::BOOT_ROUTER,CtlPkt::REQUEST,0);
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
			} else if (ptype == Forest::CLIENT_DATA) {
				forward(px,ctx);
			} else if (ptype == Forest::SUB_UNSUB) {
				subUnsub(px,ctx);
			} else if (ptype == Forest::RTE_REPLY) {
				handleRteReply(px,ctx);
			} else if (ptype == Forest::CONNECT ||
				   ptype == Forest::DISCONNECT) {
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
			didNothing = false;
		}

		// every 300 ms, update statistics and check for un-acked
		// control packets
		if (now - statsTime > 300000000) {
			sm->record(now); statsTime = now; resendControl();
			didNothing = false;
		}

		// if did nothing on that pass, sleep for a millisecond.
		if (didNothing) { /* usleep(1000); */ }

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
		return 	p.tunIp == nmIp && p.type == Forest::NET_SIG
					&& p.comtree == Forest::NET_SIG_COMT;
	}
	if (p.type == Forest::CONNECT || p.type == Forest::DISCONNECT)
	    return (p.length == Forest::OVERHEAD+8);

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
	if (lt->getPeerType(inLink) < Forest::TRUSTED) {
		// check for spoofed source address
		if (lt->getPeerAdr(inLink) != p.srcAdr) return false;
		// and that destination restrictions are respected
		fAdr_t dest = ctt->getDest(cLnk);
		if (dest!=0 && p.dstAdr != dest && p.dstAdr != myAdr)
			return false;
		// verify that type is valid
		Forest::ptyp_t ptype = p.type;
		if (ptype != Forest::CLIENT_DATA &&
		    ptype != Forest::CONNECT && ptype != Forest::DISCONNECT &&
		    ptype != Forest::SUB_UNSUB && ptype != Forest::CLIENT_SIG)
			return false;
		int comt = ctt->getComtree(ctx);
		if ((ptype == Forest::CONNECT || ptype == Forest::DISCONNECT) &&
		     comt != (int) Forest::CONNECT_COMT)
			return false;
		if (ptype == Forest::CLIENT_SIG &&
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
	p1.type = Forest::RTE_REPLY;
	p1.flags = 0;
	p1.comtree = p.comtree;
	p1.srcAdr = myAdr;
	p1.dstAdr = p.srcAdr;

	p1.pack();
	(p1.payload())[0] = htonl(p.dstAdr);
	p1.hdrErrUpdate(); p.payErrUpdate();

	int cLnk = ctt->getComtLink(ctt->getComtree(ctx),p.inLink);
	if (!qm->enq(px1,ctt->getLinkQ(cLnk),now)) ps->free(px1);
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
	if (lt->getPeerType(dLnk) != Forest::ROUTER || !qm->enq(px,dLnk,now))
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

	// process ack from parent router - just remove packet from pending map
	if ((p.flags & Forest::ACK_FLAG) != 0) {
		handleControlReply(px); return;
	}

	// add/remove branches from routes
	// if non-core node, also propagate requests upward as appropriate
	int comt = ctt->getComtree(ctx);
	int inLink = p.inLink;
	int cLnk = ctt->getComtLink(comt,inLink);

	// ignore subscriptions from the parent or core neighbors
	if (inLink == ctt->getPlink(ctx) || ctt->isCoreLink(cLnk)) {
		ps->free(px); return;
	}

	// make copy to be used for ack
	pktx cx = ps->fullCopy(px);
	Packet& copy = ps->getPacket(cx);

	bool propagate = false;
	int rtx; fAdr_t addr;
	
	// add subscriptions
	int addcnt = ntohl(pp[2]);
	if (addcnt < 0 || addcnt > 350 ||
	    Forest::OVERHEAD + (addcnt + 4)*4 > p.length) {
		ps->free(px); ps->free(cx); return;
	}
	for (int i = 3; i <= addcnt + 2; i++) {
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
	int dropcnt = ntohl(pp[addcnt+3]);
	if (dropcnt < 0 || addcnt + dropcnt > 350 ||
	    Forest::OVERHEAD + (addcnt + dropcnt + 4)*4 > p.length) {
		ps->free(px); ps->free(cx); return;
	}
	for (int i = addcnt + 4; i <= addcnt + dropcnt + 3; i++) {
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
		pp[0] = htonl((uint32_t) (seqNum >> 32));
		pp[1] = htonl((uint32_t) (seqNum & 0xffffffff));
		p.srcAdr = myAdr;
		p.dstAdr = lt->getPeerAdr(ctt->getPlink(ctx));
		sendControl(px,seqNum++,ctt->getPlink(ctx));
	} else {
		ps->free(px);
	}
	// send ack back to sender
	// note that we send ack before getting ack from sender
	// this is by design
	copy.flags |= Forest::ACK_FLAG;
	copy.dstAdr = copy.srcAdr; copy.srcAdr = myAdr;
	copy.pack();
	int qid = ctt->getLinkQ(cLnk);
        if (!qm->enq(cx,qid,now)) ps->free(cx);	
	return;
}

/** Handle a CONNECT or DISCONNECT packet.
 *  @param px is the packet number of the packet to be handled.
 */
void RouterCore::handleConnDisc(pktx px) {
	Packet& p = ps->getPacket(px);
	int inLnk = p.inLink;

	if (p.srcAdr != lt->getPeerAdr(inLnk) ||
	    p.length != Forest::OVERHEAD + 8) {
		ps->free(px); return;
	}

	uint64_t nonce = ntohl(p.payload()[0]);
	nonce <<= 32; nonce |= ntohl(p.payload()[1]);
	if (nonce != lt->getNonce(inLnk)) { ps->free(px); return; }
	if ((p.flags & Forest::ACK_FLAG) != 0) {
		handleControlReply(px); return;
	}
	if (p.type == Forest::CONNECT) {
		if (lt->isConnected(inLnk) && !lt->revertEntry(inLnk)) {
			ps->free(px); return;
		}
		if (!lt->remapEntry(inLnk,p.tunIp,p.tunPort)) {
			ps->free(px); return;
		}
		lt->setConnectStatus(inLnk,true);
		if (nmAdr != 0 && lt->getPeerType(inLnk) == Forest::CLIENT) {
                        CtlPkt cp(CtlPkt::CLIENT_CONNECT,CtlPkt::REQUEST,0);
                        cp.adr1 = p.srcAdr; cp.adr2 = myAdr;
                        sendCpReq(cp,nmAdr);
                }
	} else if (p.type == Forest::DISCONNECT) {
		lt->setConnectStatus(inLnk,false);
		lt->revertEntry(inLnk);
		if (nmAdr != 0 && lt->getPeerType(inLnk) == Forest::CLIENT) {
			dropLink(inLnk);
			CtlPkt cp(CtlPkt::CLIENT_DISCONNECT,CtlPkt::REQUEST,0);
			cp.adr1 = p.srcAdr; cp.adr2 = myAdr;
			cerr<<"disConnect p.comtree:"<<p.comtree<<endl;
			sendCpReq(cp,nmAdr);
		}
	}
	// send ack back to sender
	p.flags |= Forest::ACK_FLAG; p.dstAdr = p.srcAdr; p.srcAdr = myAdr;
	p.pack();
	pktLog->log(px,inLnk,true,now); iop->send(px,inLnk);
	return;
}

/** Handle all signalling packets addressed to the router.
 *  Assumes packet has passed all basic checks.
 *  @param px is the index of some packet
 */
void RouterCore::handleCtlPkt(int px) {
	Packet& p = ps->getPacket(px);
	CtlPkt cp(p.payload(), p.length - Packet::OVERHEAD);

	/*if (p.type != Forest::NET_SIG || p.comtree != Forest::NET_SIG_COMT) {
		ps->free(px); return;
	}*/
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
	if (cp.mode != CtlPkt::REQUEST) { 
		handleControlReply(px); return;
	}
	// Prepare positive reply packet for use where appropriate
	CtlPkt reply(cp.type,CtlPkt::POS_REPLY,cp.seqNum);

	switch (cp.type) {

	// configuring logical interfaces
	case CtlPkt::ADD_IFACE: 	addIface(cp,reply); break;
        case CtlPkt::DROP_IFACE:	dropIface(cp,reply); break;
        case CtlPkt::GET_IFACE:		getIface(cp,reply); break;
        case CtlPkt::MOD_IFACE:		modIface(cp,reply); break;
    	case CtlPkt::GET_IFACE_SET:	getIfaceSet(cp,reply); break;

	// configuring links
        case CtlPkt::ADD_LINK:		addLink(cp,reply); break;
        case CtlPkt::DROP_LINK:		dropLink(cp,reply); break;
        case CtlPkt::GET_LINK:		getLink(cp,reply); break;
        case CtlPkt::MOD_LINK:		modLink(cp,reply); break;
        case CtlPkt::GET_LINK_SET:	getLinkSet(cp,reply); break;

	// configuring comtrees
        case CtlPkt::ADD_COMTREE:	addComtree(cp,reply); break;
        case CtlPkt::DROP_COMTREE:	dropComtree(cp,reply); break;
        case CtlPkt::GET_COMTREE:	getComtree(cp,reply); break;
        case CtlPkt::MOD_COMTREE:	modComtree(cp,reply); break;
	case CtlPkt::GET_COMTREE_SET: 	getComtreeSet(cp,reply); break;
	
	//handle client joins and leaves
	case CtlPkt::CLIENT_JOIN_COMTREE:   handleClientJoinComtree(px,cp,reply); return;
	case CtlPkt::CLIENT_LEAVE_COMTREE:  handleClientLeaveComtree(px,cp,reply);return;
	//handle add_branch request
	case CtlPkt::COMTREE_ADD_BRANCH:    handleComtAddBranch(px,cp,reply); return;
	//handle add_branch_confirm request
	case CtlPkt::ADD_BRANCH_CONFIRM:    handleAddBranchConfirm(px,cp,reply);return;
	//handle comtree_prune request
	case CtlPkt::COMTREE_PRUNE: 	    handleComtPrune(px,cp,reply);return;

	case CtlPkt::ADD_COMTREE_LINK:	addComtreeLink(cp,reply); break;
	case CtlPkt::DROP_COMTREE_LINK: dropComtreeLink(cp,reply); break;
	case CtlPkt::GET_COMTREE_LINK:	getComtreeLink(cp,reply); break;
	case CtlPkt::MOD_COMTREE_LINK:	modComtreeLink(cp,reply); break;

	// configuring routes
        case CtlPkt::ADD_ROUTE:		addRoute(cp,reply); break;
        case CtlPkt::DROP_ROUTE:	dropRoute(cp,reply); break;
	case CtlPkt::GET_ROUTE:         getRoute(cp,reply); break;
        case CtlPkt::MOD_ROUTE:         modRoute(cp,reply); break;
        case CtlPkt::GET_ROUTE_SET:     getRouteSet(cp,reply); break;
	
	// configuring filters and retrieving pacets
        case CtlPkt::ADD_FILTER:        addFilter(cp,reply); break;
        case CtlPkt::DROP_FILTER:       dropFilter(cp,reply); break;
        case CtlPkt::GET_FILTER:        getFilter(cp,reply); break;
        case CtlPkt::MOD_FILTER:        modFilter(cp,reply); break;
        case CtlPkt::GET_FILTER_SET:    getFilterSet(cp,reply); break;
        case CtlPkt::GET_LOGGED_PACKETS: getLoggedPackets(cp,reply); break;
        case CtlPkt::ENABLE_PACKET_LOG:	enablePacketLog(cp,reply); break;

        // setting parameters
        case CtlPkt::SET_LEAF_RANGE:    setLeafRange(cp,reply); break;

        default:
                cerr << "unrecognized control packet type " << cp.type
                     << endl;
                reply.errMsg = "invalid control packet for router";
                reply.mode = CtlPkt::NEG_REPLY;
                break;
        }


	returnToSender(px,reply);

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
	RateSpec rs(max(min(cp.rspec1.bitRateUp,  Forest::MAXBITRATE),
						  Forest::MINBITRATE),
		    max(min(cp.rspec1.bitRateDown,Forest::MAXBITRATE),
						  Forest::MINBITRATE),
		    max(min(cp.rspec1.pktRateUp,  Forest::MAXPKTRATE),
						  Forest::MINPKTRATE),
		    max(min(cp.rspec1.pktRateDown,Forest::MAXPKTRATE),
						  Forest::MINPKTRATE));
	if (ift->valid(iface)) {
		if (cp.iface != ift->getIpAdr(iface) ||
		    !rs.equals(ift->getRates(iface))) { 
			reply.errMsg = "add iface: requested interface "
				 	"conflicts with existing interface";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		// means reply to earlier add iface was lost
		reply.ip1 = ift->getIpAdr(iface);
		reply.port1 = ift->getPort(iface);
		return true;
	} else if (!ift->addEntry(iface, cp.ip1, 0, rs)) {
		reply.errMsg = "add iface: cannot add interface";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	} else if (!iop->setup(iface)) {
		reply.errMsg = "add iface: could not setup interface";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	reply.ip1 = ift->getIpAdr(iface);
	reply.port1 = ift->getPort(iface);
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
		reply.port1 = ift->getPort(iface);
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

/** Respond to a get iface set control packet.
 *  Control packet includes the first iface in the set to be retrieved plus
 *  a count of the number of iface to be returned; reply includes the
 *  first iface in the set, the count of the number of ifaces returned and
 *  the next iface in the table, following the last one in the returned set.
 *  @param cp is a reference to a received get iface set control packet
 *  @param reply is a reference to the reply packet with fields to be
 *  filled in
 *  @return true on success, false on failure
 */
bool RouterCore::getIfaceSet(CtlPkt& cp, CtlPkt& reply) {
	int ifIndex = cp.index1;
	if (ifIndex == 0) ifIndex = ift->firstIface(); // 0 means first ifzce
	else if (!ift->valid(ifIndex)) {
		reply.errMsg = "get iface set: invalid iface number";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	reply.index1 = ifIndex;
	int count = min(10,cp.count);
	int i = 0;
	string s; stringstream ss;
	while (i < count && ifIndex != 0) {
		ss.str(""); ss.clear(); ss << ifIndex << " ";
		reply.stringData.append(ss.str());
		ift->entry2string(ifIndex,s); //s.push_back('\n');
		reply.stringData.append(s);
		if (reply.stringData.length() > 1300) {
			reply.errMsg =  "get iface set: error while formatting "
					"reply";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		i++; ifIndex = ift->nextIface(ifIndex);
	}
	reply.index2 = ifIndex; reply.count = i;
	return true;
}

bool RouterCore::addLink(CtlPkt& cp, CtlPkt& reply) {
	Forest::ntyp_t peerType = cp.nodeType;
	if (peerType == Forest::ROUTER && cp.adr1 == 0) {
		reply.errMsg = "add link: adding link to router, but no peer "
				"address supplied";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int iface = cp.iface;

	int xlnk = lt->lookup(cp.ip1,cp.port1);
	if (xlnk != 0 || (cp.link != 0 && lt->valid(cp.link))) {
		if (cp.link != xlnk ||
		    (peerType != lt->getPeerType(xlnk)) ||
		    (cp.iface != lt->getIface(xlnk)) ||
		    (cp.adr1  != 0 && cp.adr1  != lt->getPeerAdr(xlnk)) ||
		    (cp.ip1   != 0 && cp.ip1   != ift->getIpAdr(iface)) ||
		    (cp.port1 != 0 && cp.port1 != ift->getPort(iface))) {
			reply.errMsg = "add link: new link conflicts "
						"with existing link";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		// assume response to previous add link was lost
		reply.link = xlnk;
		reply.adr1 = lt->getPeerAdr(xlnk);
		reply.ip1  = lt->getPeerIpAdr(xlnk);
		return true;
	}

	// first ensure that the interface has enough
	// capacity to support a new link of minimum capacity
	RateSpec rs(Forest::MINBITRATE, Forest::MINBITRATE,
		    Forest::MINPKTRATE, Forest::MINPKTRATE);
	RateSpec& availRates = ift->getAvailRates(iface);
	if (!rs.leq(availRates)) {
		reply.errMsg =	"add link: requested link "
				"exceeds interface capacity";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}

	// setting up link
	// case 1: adding link to leaf; peer (ip,port) not known, use nonce
	//	   subcase - lnk, peer forest address is known
	//		     (for preconfigured leaf)
	//	   subcase - lnk, peer address to be assigned by router
	// case 2: adding link to router not yet up; port not known, use nonce
	//	   lnk, peer address specified
	// case 3: adding link to a router that is already up
	//	   lnk, peer address specified, peer (ip,port) specified

	// add table entry with (ip,port) or nonce
	// note: when lt->addEntry succeeds, link rates are
	// initialized to Forest minimum rates
	int lnk = lt->addEntry(cp.link,cp.ip1,cp.port1,cp.nonce);
	if (lnk == 0) {
		reply.errMsg = "add link: cannot add requested link";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}

	if (peerType == Forest::ROUTER) {
		lt->setPeerAdr(lnk,cp.adr1);
	} else { // case 1
		fAdr_t peerAdr = 0;
		if (cp.adr1 == 0) peerAdr = allocLeafAdr();
		else if (allocLeafAdr(cp.adr1)) peerAdr = cp.adr1;
		if (peerAdr == 0) {
			lt->removeEntry(lnk);
			reply.errMsg =	"add link: cannot add link using "
					"specified address";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		lt->setPeerAdr(lnk,peerAdr);
	}
	
	availRates.subtract(rs);
	lt->setIface(lnk,iface);
	lt->setPeerType(lnk,peerType);
	lt->setConnectStatus(lnk,false);
	sm->clearLnkStats(lnk);
	if (peerType == Forest::ROUTER && cp.ip1 != 0 && cp.port1 != 0) {
		// link to a router that's already up, so connect
		sendConnDisc(lnk,Forest::CONNECT);
	}

	reply.link = lnk;
	reply.adr1 = lt->getPeerAdr(lnk);
	return true;
}

bool RouterCore::dropLink(CtlPkt& cp, CtlPkt& reply) {
	dropLink(cp.link, cp.adr1);
	return true;
}

/** Drop a specified link at this router.
 *  First, remove all comtree links associated with this link.
 *  This also removes the comtree link from all multicast routes.
 *  @param lnk is the link number of the link to be dropped
 *  @param peerAdr is the address of the peer of the remote end of the
 *  link, if lnk == 0; in this case, the link number is looked up using
 *  the peerAdr value
 */
void RouterCore::dropLink(int lnk, fAdr_t peerAdr) {
	if (lnk == 0) lnk = lt->lookup(peerAdr);
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
		reply.count = lt->getComtCount(link);
		return true;
	} 
	reply.errMsg = "get link: invalid link number";
	reply.mode = CtlPkt::NEG_REPLY;
	return false;
}

/** Respond to a get link set control packet.
 *  Control packet includes the first link in the set to be retrieved plus
 *  a count of the number of links to be returned; reply includes the
 *  first link in the set, the count of the number of links returned and
 *  the next link in the table, following the last one in the returned set.
 *  @param cp is a reference to a received get link set control packet
 *  @param reply is a reference to the reply packet with fields to be
 *  filled in
 *  @return true on success, false on failure
 */
bool RouterCore::getLinkSet(CtlPkt& cp, CtlPkt& reply) {
	int lnk = cp.index1;
	if (lnk == 0) lnk = lt->firstLink(); // 0 means start with first link
	else if (!lt->valid(lnk)) {
		reply.errMsg = "get link set: invalid link number";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	reply.index1 = lnk;
	int count = min(10,cp.count);
	int i = 0;
	string s; stringstream ss;
	while (i < count && lnk != 0) {
		ss.str(""); ss.clear(); ss << lnk << " ";
		reply.stringData.append(ss.str());
		lt->link2string(lnk,s); s.push_back('\n');
		reply.stringData.append(s);
		if (reply.stringData.length() > 1300) {
			reply.errMsg =  "get link set: error while formatting "
					"reply";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		i++; lnk = lt->nextLink(lnk);
	}
	reply.index2 = lnk; reply.count = i;
	return true;
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
		lt->getAvailRates(link).add(delta);
		qm->setLinkRates(link,cp.rspec1);
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
	int plink = ctt->getPlink(ctx);

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
	if (plink != 0) reply.rspec1 = lt->getAvailRates(plink);
	else reply.rspec1.set(0);
	return true;
}


bool RouterCore::dropComtree(comt_t comt) {
	int ctx = ctt->getComtIndex(comt);
	if (!ctt->validComtIndex(ctx))
		return true; // so dropComtree op is idempotent
	int plink = ctt->getPlink(ctx);
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

/** Respond to a get comtree set control packet.
 *  Control packet includes the first comtree in the set to be retrieved plus
 *  a count of the number of comtree to be returned; reply includes the
 *  first comtree in the set, the count of the number of comtrees returned and
 *  the next comtree in the table, following the last one in the returned set.
 *  @param cp is a reference to a received get comtree set control packet
 *  @param reply is a reference to the reply packet with fields to be
 *  filled in
 *  @return true on success, false on failure
 */
bool RouterCore::getComtreeSet(CtlPkt& cp, CtlPkt& reply) {
	int comtIndex = cp.index1;
	if (comtIndex == 0) comtIndex = ctt->firstComtIndex(); // 0 means first
	else if (!ctt->validComtIndex(comtIndex)) {
		reply.errMsg = "get comtree set: invalid comtree number";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	reply.index1 = comtIndex;
	int count = min(10,cp.count);
	int i = 0;
	while (i < count && comtIndex != 0) {
		string s; ctt->entry2string(comtIndex,s); //s.push_back('\n');
		reply.stringData.append(s);
		if (reply.stringData.length() > 1300) {
			reply.errMsg =  "get comtee set: error while formatting "
					"reply";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		i++; comtIndex = ctt->nextComtIndex(comtIndex);
	}
	reply.index2 = comtIndex; reply.count = i;
	return true;
}


//feng
bool RouterCore::addComtreeLink(comt_t comt, int link, CtlPkt& reply) {
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.errMsg = "panfeng add comtree link: invalid comtree";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int lnk = link; 
	
	if (!lt->valid(lnk)) {
		reply.errMsg = "add comtree link: invalid link or "
					"peer IP and port";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	bool isRtr = (lt->getPeerType(lnk) == Forest::ROUTER);
	bool isCore = false;
	
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
	if (lt->getPeerType(lnk) != Forest::ROUTER) {
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
	reply.rspec1 = lt->getAvailRates(lnk);
	return true;
}

bool RouterCore::modComtreeLink(comt_t comtree, int link, RateSpec rspec1,  CtlPkt& reply) {
	comt_t comt = comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.errMsg = "modify comtree link: invalid comtree";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	int lnk = link;
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

	RateSpec rs = rspec1;
	if (!rs.isSet()) return true;
	RateSpec diff = rs; diff.subtract(ctt->getRates(cLnk));
	if (!diff.leq(lt->getAvailRates(lnk))) {
		reply.errMsg = "modify comtree link: new rate spec "
				"exceeds available link capacity";
		reply.mode = CtlPkt::NEG_REPLY;
{
string s;
cerr << "mod comtree link exceeding link capacity on link " << lnk <<
"\nrequested " << rs.toString(s);
cerr << " only " << lt->getAvailRates(lnk).toString(s) << "available\n";
}
		return false;
	}
	lt->getAvailRates(lnk).subtract(diff);
	ctt->getRates(cLnk) = rs;
	reply.rspec1 = lt->getAvailRates(lnk); // return available rate on link
	return true;
}

bool RouterCore::modComtree(comt_t comtree, int link, CtlPkt& reply) {
	comt_t comt = comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx != 0) {
		//if (cp.coreFlag >= 0)
		//	ctt->setCoreFlag(ctx,cp.coreFlag);
		if (link != 0) {
			int plnk = link;
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
	} else if (cp.ip1 != 0 && cp.port1 != 0) {
		lnk = lt->lookup(cp.ip1, cp.port1);
	} else if (cp.adr1 != 0) {
		lnk = lt->lookup(cp.adr1);
	}
	if (!lt->valid(lnk)) {
		reply.errMsg = "add comtree link: invalid link or "
					"peer IP and port";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	bool isRtr = (lt->getPeerType(lnk) == Forest::ROUTER);
	bool isCore = false;
	if (isRtr) {
		if (cp.coreFlag < 0) {
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
	if (lt->getPeerType(lnk) != Forest::ROUTER) {
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
	reply.rspec1 = lt->getAvailRates(lnk);
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
	int lnk = 0; 
	if (cp.link != 0) {
		lnk = cp.link;
	} else if (cp.ip1 != 0 && cp.port1 != 0) {
		lnk = lt->lookup(cp.ip1, cp.port1);
	} else if (cp.adr1 != 0) {
		lnk = lt->lookup(cp.adr1);
	}
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
	reply.rspec1 = lt->getAvailRates(lnk);
	return true;
}

void RouterCore::dropComtreeLink(int ctx, int lnk, int cLnk) {
	// release the link bandwidth used by comtree link
	lt->getAvailRates(lnk).add(ctt->getRates(cLnk));

	// remove unicast route for this comtree
	fAdr_t peerAdr = lt->getPeerAdr(lnk);
	int comt = ctt->getComtree(ctx);
	if (lt->getPeerType(lnk) != Forest::ROUTER) {
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
		reply.mode = CtlPkt::NEG_REPLY;
{
string s;
cerr << "mod comtree link exceeding link capacity on link " << lnk <<
"\nrequested " << rs.toString(s);
cerr << " only " << lt->getAvailRates(lnk).toString(s) << "available\n";
}
		return false;
	}
	lt->getAvailRates(lnk).subtract(diff);
	ctt->getRates(cLnk) = rs;
	reply.rspec1 = lt->getAvailRates(lnk); // return available rate on link
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

/** Respond to a get route set control packet.
 *  Control packet includes the first route in the set to be retrieved plus
 *  a count of the number of route to be returned; reply includes the
 *  first route in the set, the count of the number of routes returned and
 *  the next route in the table, following the last one in the returned set.
 *  @param cp is a reference to a received get route set control packet
 *  @param reply is a reference to the reply packet with fields to be
 *  filled in
 *  @return true on success, false on failure
 */
bool RouterCore::getRouteSet(CtlPkt& cp, CtlPkt& reply) {
	int rIndex = cp.index1;
	if (rIndex == 0)
		rIndex = rt->firstRteIndex(); // 0 means first route
	else if (!rt->validRteIndex(rIndex)) {
		reply.errMsg = "get route set: invalid route number";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	reply.index1 = rIndex;
	int count = min(10,cp.count);
	int i = 0;
	while (i < count && rIndex != 0) {
		string s; rt->entry2string(rIndex,s); //s.push_back('\n');
		reply.stringData.append(s);
		if (reply.stringData.length() > 1300) {
			reply.errMsg =  "get route set: error while formatting "
					"reply";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		i++; rIndex = rt->nextRteIndex(rIndex);
	}
	reply.index2 = rIndex; reply.count = i;	
	return true;
}

/** Handle a join comtree request.
 *  This requires sending request packet to comtree controller 
 *  asking for a path from the client's access router to the comtree 
 *  and allocating link bandwidth along that path.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  sending an error reply is considered a success; the operation
 *  fails if it cannot allocate required packets, or if no path
 *  can be found between the client's access router and the comtree.
 */
void RouterCore::handleClientJoinComtree(pktx px, CtlPkt& cp, CtlPkt& reply) {
	Packet& p = ps->getPacket(px);	
	comt_t comt = cp.comtree;
	int cliAdr = p.srcAdr;
	int lnk = lt->lookup(cliAdr);
        if (ctt->validComtree(comt)) {
		int ctx = ctt->getComtIndex(comt);
		//if the comtree is locked, add request to the list
		if (ctt->isLocked(ctx)) {
			ctt->addRequest(ctx,px);
			return;
		} else{
			RateSpec rs = cp.rspec1;	
			RateSpec uRates = ctt->getUpperBoundRates(ctx);
			if (!uRates.isSet()) {
				ctt->addRequest(ctx,px);
				ctt->setLock(ctx,1);
				//send request to CC to get path
				CtlPkt cpp(CtlPkt::COMTREE_PATH,CtlPkt::REQUEST,0);
				cpp.comtree = comt;
				cpp.adr1 = cliAdr;
				sendCpReq(cpp,ccAdr);
                        	return;
			}	
			if (!rs.isSet())	
				rs = uRates;
			if (rs.leq(uRates)) {
				//setup link
                        	if (addComtreeLink(comt,lnk,reply))
                        		//allocate bandwidth
					if (modComtreeLink(comt,lnk,rs,reply)) {
						//send ok to CC
						CtlPkt cpn(CtlPkt::COMTREE_NEW_LEAF,CtlPkt::REQUEST,0);
						cpn.comtree = comt;
						cpn.adr1 = cliAdr;
						cpn.adr2 = myAdr;
						cpn.link = lnk;
						cpn.rspec1 = reply.rspec1;
						cpn.index1 = px;
						sendCpReq(cpn,ccAdr);
						return;
					}
						
			} else {
				reply.errMsg =  "exceed upper bound access link rate";
				reply.mode = CtlPkt::NEG_REPLY;
			}	
			//send reply to client
			returnToSender(px,reply);
			return;
		}	
        } else {
		//add new comtree to this router
		ctt->addEntry(comt);
		int ctx = ctt->getComtIndex(comt);
		ctt->addRequest(ctx,px);	
		ctt->setLock(ctx,1);
		//send request to CC to get path
		CtlPkt cpp(CtlPkt::COMTREE_PATH,CtlPkt::REQUEST,0);
		cpp.comtree = cp.comtree;
		cpp.adr1 = cliAdr;
		sendCpReq(cpp,ccAdr);
		return;
	}	
}

/** Handle a comtree path reply control packet 
 *  @param p is the packet number of the original request 
 *  packet in pending map
 *  @param cp is the control packet structure for comtree 
 *  path reply packet (already unpacked)
 *  @return
 *  sending an error reply is considered a success
 */
void RouterCore::handleComtPath(pktx px, CtlPkt& cpr) {
	Packet& p = ps->getPacket(px);
	CtlPkt cp(p);	
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	fAdr_t cliAdr = cp.adr1;
	int lnk = lt->lookup(cliAdr);
	//get link number and rspec of leaf node
	vector<pktx>& pktNums = ctt->getPktNums(ctx);
	if (pktNums.size() == 0)
		return;
	//the first pending request in the list
	vector<pktx>::iterator pb;
	pb = pktNums.begin();
	pktx pxb = *pb;
	Packet& pc = ps->getPacket(pxb);
	CtlPkt cpc(pc.payload(), pc.length - Packet::OVERHEAD);
	cpc.unpack();
	// Prepare pos reply packet for use where appropriate
        CtlPkt reply(cpc.type,CtlPkt::POS_REPLY,cpc.seqNum);
	if (!ctt->validComtree(comt)) {
		//send neg reply
		reply.errMsg = "comtree not defined at this router\n";
        	returnToSender(pxb,reply);	
		return;
	}
	//need to propogate the add_branch request
	if (cpr.ivec.size()!=0) {
		int pLnk = cpr.ivec[0];	
		RateSpec uRates = cpr.rspec2;
		//set upper bound rates
		ctt->setUpperBoundRates(ctx,uRates);
		RateSpec rs = cpc.rspec1;
		if (!rs.isSet())
                	rs = uRates;
		if (rs.leq(uRates)) {
			while (true) {
				//reserve leaf link
				if (!addComtreeLink(comt,lnk,reply)) break;
                                if (!modComtreeLink(comt,lnk,rs,reply)) break;
				//reserve parent link
				if (!addComtreeLink(comt,pLnk,reply)) break; 
				if (!modComtreeLink(comt,pLnk,rs,reply)) break;
				if (!modComtree(comt,pLnk,reply)) break;
				//propogate to parent node
				fAdr_t dstAdr = lt->getPeerAdr(pLnk);
				CtlPkt cpa(CtlPkt::COMTREE_ADD_BRANCH,CtlPkt::REQUEST,0);
				cpa.comtree = comt;
				cpa.ivec = cpr.ivec;
				cpa.index1 = 1;
				cpa.rspec1 = cpr.rspec1;
				cpa.rspec2 = cpr.rspec2;
				cpa.adr1 = myAdr;
				sendCpReq(cpa,dstAdr);
				return;
			}	
			//drop ComtreeLink
			int cLnk = ctt->getComtLink(comt,lnk);
			if (cLnk !=0)
				dropComtreeLink(ctx,lnk,cLnk);
		} else {
                        reply.errMsg =  "exceed upper bound access link rate";
                        reply.mode = CtlPkt::NEG_REPLY;
                }
		//send neg reply to client
		returnToSender(pxb,reply);	
	} else {
		//reply for the leaf comtree_path request
		RateSpec uRates = cpr.rspec2;
		ctt->setUpperBoundRates(ctx,uRates);
		pktNums.erase(pb);
		RateSpec rs = cpc.rspec1;
		if (!rs.isSet())
			rs = uRates;
		if (rs.leq(uRates)) {
			//setup link
			if (addComtreeLink(comt,lnk,reply))
				//allocate bandwidth
				if (modComtreeLink(comt,lnk,rs,reply)) {
					//send ok to cc
					CtlPkt cpn(CtlPkt::COMTREE_NEW_LEAF,CtlPkt::REQUEST,0);
					cpn.comtree = comt;
					cpn.adr1 = cliAdr;
					cpn.adr2 = myAdr;
					cpn.link = lnk;
					cpn.rspec1 = reply.rspec1;	
					cpn.index1 = pxb;
					sendCpReq(cpn,ccAdr);
					return;
				}
						
		} else {
			reply.errMsg =  "exceed upper bound access link rate";
			reply.mode = CtlPkt::NEG_REPLY;
		}
		returnToSender(pxb,reply);
		ctt->setLock(ctx,0);
		if (pktNums.size() != 0) {
			//reply to all pending request
			handleAllPending(pktNums, cpr);
			ctt->cleanPktNums(ctx);
		}
	}	
}
        
/** Handle a comtree_add_branch control packet
 *  setup links and propogate request to parent router
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @return
 *  sending an error reply is considered a success
 */
void RouterCore::handleComtAddBranch(pktx px, CtlPkt& cp, CtlPkt& reply) {
	Packet& p = ps->getPacket(px);	
	comt_t comt = cp.comtree;
	int lnk = p.inLink;
	RateSpec rs = cp.rspec1;
	RateSpec uRates = cp.rspec2;
        if (ctt->validComtree(comt)) {
		int ctx = ctt->getComtIndex(comt);
		if (ctt->isLocked(ctx)) {
			ctt->addRequest(ctx,px);
			return;
		} else {
			//reach the node in the comtree, reserve bandwidth and send pos reply
			int cLnk = ctt->getComtLink(comt,lnk);
			if(ctt->isLink(ctx,cLnk)) {
				if (reply.adr1 == 0)
                                	reply.adr1 = myAdr;
				returnToSender(px,reply);
				return;
			}
			if (addComtreeLink(comt,lnk,reply))
				if (modComtreeLink(comt,lnk,rs,reply)) {
					//send pos reply
					if (reply.adr1 == 0)	
						reply.adr1 = myAdr;
					returnToSender(px,reply);
					return;
				}	
			//else drop comtree link
			cLnk = ctt->getComtLink(comt,lnk);
			if (cLnk !=0)
				dropComtreeLink(ctx,lnk,cLnk);
		}
        } else {
                //add new comtree
                ctt->addEntry(comt);
		int ctx = ctt->getComtIndex(comt);
                ctt->setLock(ctx,1);
		ctt->setUpperBoundRates(ctx,uRates);
		vector<int>& ivec = cp.ivec;
		int index = cp.index1;
		int pLnk = 0;
		if (index < ivec.size()) 
			pLnk = ivec[index];
		while (pLnk != 0) {
			//reserve child link
			if (!addComtreeLink(comt,lnk,reply)) break;
			if (!modComtreeLink(comt,lnk,rs,reply)) break; 
			//reserve parent link
			if (!addComtreeLink(comt,pLnk,reply)) break;
			if (!modComtreeLink(comt,pLnk,rs,reply)) break;
			if (!modComtree(comt,pLnk,reply)) break; 
			//propogate request to parent
			fAdr_t dstAdr = lt->getPeerAdr(pLnk);
			cp.index1++;
			sendCpReq(cp,dstAdr);
			ctt->addRequest(ctx,px);
			return;
						
		}
		//else dropComtree
		dropComtree(comt);
        }
        //send neg reply
        returnToSender(px,reply);
        return;
}
/** Handle a request by a client to leave a comtree.
 *  this requires informing comtree controller (parent router if neccessary) 
 *  for consistency
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success
 */
void RouterCore::handleClientLeaveComtree(pktx px, CtlPkt& cp, CtlPkt& reply) {
	Packet& p = ps->getPacket(px);	
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);	
	int cliAdr = p.srcAdr;
	if (ctx == 0) {
                returnToSender(px,reply);
		return;
	}
	int lnk = lt->lookup(cliAdr);
	//drop child link	
        int cLnk = ctt->getComtLink(comt,lnk);
	if (cLnk !=0) {
        	dropComtreeLink(ctx,lnk,cLnk);
		CtlPkt cpp(CtlPkt::COMTREE_PRUNE,CtlPkt::REQUEST,0);
		cpp.comtree = comt;
		cpp.adr1 = cliAdr;
		sendCpReq(cpp,ccAdr);
		ctt->addRequest(ctx,px);
		ctt->setLock(ctx,1);
	} else
		returnToSender(px,reply);
}

/** Handle a comtree prune control packet
 *  drop child link and to see if neccessary to propate prune request
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success
 */
void RouterCore::handleComtPrune(pktx px, CtlPkt& cp, CtlPkt& reply) {
	Packet& p = ps->getPacket(px);
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		returnToSender(px,reply);
		return;
	}
	//drop child link
	int cLnk = ctt->getComtLink(comt,p.inLink);	
	if (cLnk !=0) {
		dropComtreeLink(ctx,p.inLink,cLnk);
		//check if this router need to be pruned
		int lnkCnt = ctt->getLinkCount(ctx);
		int pLnk = ctt->getPlink(ctx);
		if (lnkCnt <= 1 && !ctt->inCore(ctx) && pLnk != 0 && !ctt->isLocked(ctx)) {
			//inform CC
			CtlPkt cpp(CtlPkt::COMTREE_PRUNE,CtlPkt::REQUEST,0);
			cpp.comtree = comt;
			cpp.adr1 = myAdr;
			sendCpReq(cpp,ccAdr);
			ctt->addRequest(ctx,px);
			ctt->setLock(ctx,1);
			vector<pktx> pktNums = ctt->getPktNums(ctx);
		} else {
			returnToSender(px,reply);	
		}
	} else {
		returnToSender(px,reply);
	}
}

/** Handle a add_branch_confirm request
 */
void RouterCore::handleAddBranchConfirm(pktx px, CtlPkt& cp, CtlPkt& reply) {
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0)
        	return;
	if (!ctt->isLocked(ctx))
		returnToSender(px,reply);
	else
		ctt->addRequest(ctx,px);	
}

/** Handle a comtree_add_branch reply packet
 *  send comtree new leaf control packet to comtree controller
 *  if this router is the leaf router, unlock the comtree and 
 *  process all the pending request in the pending list
 *  @param p is the packet number of the original request packet
 *  in pending map
 *  @param cp is the control packet structure for p (already unpacked)
 *  @return true if the operation is completed successfully;
 *  sending an error reply is considered a success
 */
void RouterCore::handleAddBranchReply(pktx px, CtlPkt& cpr) {
	Packet& pr= ps->getPacket(px);
	CtlPkt cp(pr); 
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0)
                return;
	vector<pktx>& pktNums = ctt->getPktNums(ctx);
	if (pktNums.size() == 0)
		return;
	vector<pktx>::iterator pb;
	pb = pktNums.begin();
	pktx begin = *pb;
	pktNums.erase(pb);
	Packet& po= ps->getPacket(begin);
	CtlPkt cpo(po.payload(), po.length - Packet::OVERHEAD);
	cpo.unpack();
	CtlPkt reply(cpo.type,CtlPkt::POS_REPLY,cpo.seqNum);
	//if this is the leaf node, send ok to cc
	if (cp.index1 == 1) {
		fAdr_t cliAdr = po.srcAdr;
		int lnk = lt->lookup(cliAdr);
		CtlPkt cpn(CtlPkt::COMTREE_NEW_LEAF,CtlPkt::REQUEST,0);
		cpn.comtree= cp.comtree;
		cpn.ivec = cp.ivec;
		cpn.adr1 = cliAdr;
		cpn.adr2 = cpr.adr1;
		cpn.link = lnk;
		RateSpec rs = cpo.rspec1;
                if (!rs.isSet()) {
			RateSpec uRates = ctt->getUpperBoundRates(ctx);
                        rs = uRates;
                }
		cpn.rspec1 = rs;
		cpn.index1 = begin;
		sendCpReq(cpn,ccAdr);
		return;
	} else {
		//propogate the reply
		fAdr_t dest = po.srcAdr;	
		reply.adr1 = cpr.adr1;
		returnToSender(begin,reply);
		CtlPkt cpc(CtlPkt::ADD_BRANCH_CONFIRM,CtlPkt::REQUEST,0);
		cpc.comtree = comt;
		sendCpReq(cpc,dest);
	}
}

/** Handle a add_branch_confirm reply
 */
void RouterCore::handleConfirmReply(pktx px, CtlPkt& cpr) {
	Packet& pr= ps->getPacket(px);
	CtlPkt cp(pr);
	comt_t comt = cp.comtree;
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0)
                return;
	ctt->setLock(ctx,0);	
	vector<pktx>& pktNums = ctt->getPktNums(ctx);
	if (pktNums.size() != 0) {
		//reply to all pending request
		handleAllPending(pktNums,cpr);
		ctt->cleanPktNums(ctx);
	}
}

/** Process all the pending request in the list
 *  send comtree new leaf control packet to comtree controller
 *  if this router is the leaf router, unlock the comtree and 
 *  process all the pending request in the pending list
 *  @param pktNums is a vector of packet number indicating the pending requests
 *  @param cpr is a control packet reply 
 *  sending an error reply is considered a success
 */
void RouterCore::handleAllPending(vector<pktx>& pktNums, CtlPkt& cpr) {
	vector<pktx>::iterator pn;
        for (pn = pktNums.begin(); pn != pktNums.end(); pn++) {
        	pktx px = *pn;
		Packet& p = ps->getPacket(px);
		CtlPkt cp(p.payload(), p.length - Packet::OVERHEAD);
		cp.unpack();		
		// Prepare pos reply packet for use where appropriate
        	CtlPkt reply(cp.type,CtlPkt::POS_REPLY,cp.seqNum);	
		//if pos send ok to cc
		if (cp.type == CtlPkt::CLIENT_JOIN_COMTREE) {
			//deal with leaf request
			handleClientJoinComtree(px,cp,reply);
		}
		else if (cp.type == CtlPkt::COMTREE_ADD_BRANCH) { 
			//the address of the node that are already in the comtree
			reply.adr1 = cpr.adr1;
			handleComtAddBranch(px,cp,reply);
		} else if (cp.type == CtlPkt::CLIENT_LEAVE_COMTREE ||
			cp.type == CtlPkt::COMTREE_PRUNE
			|| cp.type == CtlPkt::ADD_BRANCH_CONFIRM) {
			returnToSender(px,reply);
		}	
	}
}

/** Handle a contree_prune reply 
 *  could be a reply from CC or parent router
 *  @param pktNums is a vector of packet number indicating the pending requests
 *  @param cpr is a control packet reply
 */
void RouterCore::handleComtPruneReply(pktx px, CtlPkt& cpr){
	Packet& copy = ps->getPacket(px);
	CtlPkt cp(copy.payload(), copy.length - Packet::OVERHEAD);
	cp.unpack();
	fAdr_t dstAdr = copy.dstAdr;
	int ctx = ctt->getComtIndex(cp.comtree);
	if (ctx == 0)
		return;
	//get reply from the parent router
	if (dstAdr != ccAdr) {
		vector<pktx> pktNums = ctt->getPktNums(ctx);
		int lnkCnt = ctt->getLinkCount(ctx);
		if (lnkCnt <= 1)
			dropComtree(cp.comtree);
		if (pktNums.size() != 0) {
			//some pending request cause of prune
			handleAllPending(pktNums,cpr);
		}
	}
	//get reply from ccAdr for pruning router request
	if(dstAdr == ccAdr && cp.adr1 == myAdr) {
			int pLnk = ctt->getPlink(ctx);
			fAdr_t parAdr = lt->getPeerAdr(pLnk);
			//propagate prune request to its parent
			if (pLnk != 0) {
				CtlPkt cpp(CtlPkt::COMTREE_PRUNE,CtlPkt::REQUEST,0);
				cpp.comtree = cp.comtree;
				cpp.adr1 = myAdr;
				sendCpReq(cpp,parAdr);
			}
	}
	//get reply from CC for pruning client
	if(dstAdr == ccAdr && cp.adr1 != myAdr) {
		int lnkCnt = ctt->getLinkCount(ctx);
		int pLnk = ctt->getPlink(ctx);
		//check if this router need to be pruned
		if (lnkCnt <= 1 && !ctt->inCore(ctx) && pLnk != 0) {
			//inform cc
			CtlPkt cpp1(CtlPkt::COMTREE_PRUNE,CtlPkt::REQUEST,0);
			cpp1.comtree = cp.comtree;
			cpp1.adr1 = myAdr;
			sendCpReq(cpp1,ccAdr);
		}
		else {
			vector<pktx>& pktNums = ctt->getPktNums(ctx);
			ctt->setLock(ctx,0);
			if (pktNums.size() != 0) {
				handleAllPending(pktNums,cpr);	
				ctt->cleanPktNums(ctx);
			}
		}
	}
}

/* Handle a new_leaf reply
 * send a reply to the client, process the pending request
 * in the pending list
 *  @param pktNums is a vector of packet number indicating the pending requests
 *  @param cpr is a control packet reply
 */
void RouterCore::handleComtNewLeafReply(pktx px, CtlPkt& cpr) {
	Packet& copy = ps->getPacket(px);
	CtlPkt cp(copy.payload(), copy.length - Packet::OVERHEAD);
	cp.unpack();
	pktx pxo = cp.index1;
	Packet& po = ps->getPacket(pxo);
	CtlPkt cp1(po);
	CtlPkt reply1(cp1.type,CtlPkt::POS_REPLY,cp1.seqNum);
	returnToSender(pxo,reply1);
	if (cp.ivec.size() != 0) {
		comt_t comt = cp.comtree;
		int ctx = ctt->getComtIndex(comt);
		if (ctx == 0)
			return;
		ctt->setLock(ctx,0);	
		vector<pktx>& pktNums = ctt->getPktNums(ctx);
		if (pktNums.size() != 0) {
			//reply to all pending request
			handleAllPending(pktNums,cpr);
			ctt->cleanPktNums(ctx);
		}
	}
}



/** Handle an add filter control packet.
 *  Adds the specified interface and prepares a reply packet.
 *  @param cp is the control packet structure (already unpacked)
 *  @param reply is a control packet structure for the reply, which
 *  the cpType has been set to match cp, the rrType is POS_REPLY
 *  and the seqNum has been set to match cp.
 *  @param return true on success, false on failure; on a successful
 *  return, all appropriate attributes in reply are set; on a failure
 *  return, the errMsg field of reply is set.
 */
bool RouterCore::addFilter(CtlPkt& cp, CtlPkt& reply) {
	fltx fx = pktLog->addFilter();
	if (fx == 0) {
		reply.errMsg = "add filter: cannot add filter";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	reply.index1 = fx;
	return true;
}

bool RouterCore::dropFilter(CtlPkt& cp, CtlPkt& reply) {
	pktLog->dropFilter(cp.index1);
	return true;
}

bool RouterCore::getFilter(CtlPkt& cp, CtlPkt& reply) {
	fltx fx = cp.index1;
	if (!pktLog->validFilter(fx)) {
		reply.errMsg = "get filter: invalid filter index";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	PacketFilter& f = pktLog->getFilter(fx);
	f.toString(reply.stringData);
	return true;
}

bool RouterCore::modFilter(CtlPkt& cp, CtlPkt& reply) {
	fltx fx = cp.index1;
	if (!pktLog->validFilter(fx)) {
		reply.errMsg = "mod filter: invalid filter index";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	PacketFilter& f = pktLog->getFilter(fx);
	f.fromString(cp.stringData);
	return true;
}

/** Respond to a get filter set control packet.
 *  Control packet includes the first filter in the set to be retrieved plus
 *  a count of the number of filters to be returned; reply includes the
 *  first filters in the set, the count of the number of filters returned and
 *  the next filters in the table, following the last one in the returned set.
 *  @param cp is a reference to a received get filter set control packet
 *  @param reply is a reference to the reply packet with fields to be
 *  filled in
 *  @return true on success, false on failure
 */
bool RouterCore::getFilterSet(CtlPkt& cp, CtlPkt& reply) {
	fltx fx = cp.index1;
	if (fx == 0) {
		fx = pktLog->firstFilter(); // 0 means start with first filter
	} else if (!pktLog->validFilter(fx)) {
		reply.errMsg = "get filter set: invalid filter index";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	reply.index1 = fx;
	int count = min(10,cp.count);
	int i = 0;
	string s; stringstream ss;
	while (i < count && fx != 0) {
		PacketFilter& f = pktLog->getFilter(fx);
		ss.str(""); ss.clear(); ss << fx << " ";
		reply.stringData.append(ss.str());
		reply.stringData.append(f.toString(s));
		reply.stringData.push_back('\n');

		if (reply.stringData.length() > 1300) {
			reply.errMsg =  "get filter set: error while "
					"formatting reply";
			reply.mode = CtlPkt::NEG_REPLY;
			return false;
		}
		i++; fx = pktLog->nextFilter(fx);
	}
	reply.index2 = fx; reply.count = i;
	return true;
}

/** Respond to a get logged packets control packet.
 *  @param cp is a reference to a received get logged packets control packet
 *  @param reply is a reference to the reply packet with fields to be
 *  filled in
 *  @return true on success, false on failure
 */
bool RouterCore::getLoggedPackets(CtlPkt& cp, CtlPkt& reply) {
	reply.count = pktLog->extract(1300, reply.stringData);
	return true;
}

/** Enable local packet logging.
 *  @param cp is a reference to a received get logged packets control packet
 *  @param reply is a reference to the reply packet with fields to be
 *  filled in
 *  @return true on success, false on failure
 */
bool RouterCore::enablePacketLog(CtlPkt& cp, CtlPkt& reply) {
	pktLog->turnOnLogging(cp.index1 == 1 ? true : false);
	pktLog->enableLocalLog(cp.index2 == 1 ? true : false);
	return true;
}

/** Set leaf address range.
 *  @param px is the number of the boot complete request packet
 *  @param cp is an unpacked control packet for p
 *  @param reply is a reference to a pre-formatted positive reply packet
 *  that will be returned to the sender; it may be modified to indicate
 *  a negative reply.
 *  @return true on success, false on failure
 */
bool RouterCore::setLeafRange(CtlPkt& cp, CtlPkt& reply) {
	if (!booting) {
		reply.errMsg =	"attempting to set leaf address range when "
				"not booting";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	firstLeafAdr = cp.adr1;
       	fAdr_t lastLeafAdr = cp.adr2;
	if (firstLeafAdr > lastLeafAdr) {
		reply.errMsg = "request contained empty leaf address range";
		reply.mode = CtlPkt::NEG_REPLY;
		return false;
	}
	leafAdr = new UiSetPair((lastLeafAdr - firstLeafAdr)+1);
	return true;
}

/** Send a connect packet to a peer router.
 *  @param lnk is link number of the link on which we want to connect.
 *  @param type is CONNECT or DISCONNECT
 */
void RouterCore::sendConnDisc(int lnk, Forest::ptyp_t type) {
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);

	uint64_t nonce = lt->getNonce(lnk);
	p.length = Forest::OVERHEAD + 8; p.type = type; p.flags = 0;
	p.comtree = Forest::CONNECT_COMT;
	p.srcAdr = myAdr; p.dstAdr = lt->getPeerAdr(lnk);
	p.payload()[0] = htonl((int) (nonce >> 32));
	p.payload()[1] = htonl((int) (nonce & 0xffffffff));

	sendControl(px,nonce,lnk);
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
	p.type = Forest::NET_SIG; p.flags = 0;
	p.comtree = Forest::NET_SIG_COMT;
	p.srcAdr = myAdr; p.dstAdr = dest;
	p.inLink = 0;
	sendControl(px,seqNum++,0);
	return true;
}

/** Send a control packet.
 */
bool RouterCore::sendControl(pktx px, uint64_t pid, int lnk) {
	Packet& p = ps->getPacket(px); p.pack();

	// now, make copy of packet and save it in pending
	pktx cx = ps->fullCopy(px);
	if (cx == 0) {
		cerr << "RouterCore::sendControl: no packets left in packet "
			"store\n";
		return false;
	}

	// save a record of the packet in pending map
	pair<uint64_t,ControlInfo> cpp;
	cpp.first = pid;
	cpp.second.px = cx; cpp.second.nSent = 1; cpp.second.timestamp = now;
	cpp.second.lnk = lnk;
	pending->insert(cpp);

	// send the packet
	comt_t comt = p.comtree;
	if (booting) {
		pktLog->log(px,lnk,true,now); iop->send(px,lnk);
	} else if (lnk != 0) {
		int ctx; int clnk; int qid;
		if ((ctx = ctt->getComtIndex(comt)) == 0 ||
		    (clnk = ctt->getComtLink(comt,lnk)) == 0 ||
                    (qid = ctt->getLinkQ(clnk)) == 0 ||
                    (!qm->enq(px,qid,now))) {
			ps->free(px);
		}
	} else if (booting) {
		pktLog->log(px,0,true,now); iop->send(px,0);
	} else {
		forward(px,ctt->getComtIndex(comt));
	}
	return true;
}

/** Retransmit any pending control packets that have timed out.
 *  This method checks the map of pending requests and whenever
 *  it finds a packet that has been waiting for an acknowledgement
 *  for more than a second, it retransmits it or gives up, 
 *  if it has already made three attempts.
 */
void RouterCore::resendControl() {
	map<uint64_t,ControlInfo>::iterator pp;
	list<uint64_t> dropList;
	for (pp = pending->begin(); pp != pending->end(); pp++) {
		if (now < pp->second.timestamp + 1000000000) continue;
		pktx px = pp->second.px;
		Packet& p = ps->getPacket(px);
		string s;
		if (pp->second.nSent >= 3) { // give up on this packet
			cerr << "RouterCore::resendControl: received no reply "
				"to control packet after three attempts\n"
			     << p.toString(s);
			ps->free(px); dropList.push_front(pp->first); continue;
		}
		// make copy of packet and send the copy
		pp->second.timestamp = now;
		pp->second.nSent++;
                pktx cx = ps->fullCopy(px);
                if (cx == 0) {
                        cerr << "RouterCore::resendControl: no packets left in "
                                "packet store\n";
                        break;
                }
                int lnk = pp->second.lnk; comt_t comt = p.comtree;
                if (booting) {
                        pktLog->log(cx,lnk,true,now); iop->send(cx,lnk);
                } else if (lnk != 0) {
                        int ctx; int clnk; int qid;
                        if ((ctx = ctt->getComtIndex(comt)) == 0 ||
                            (clnk = ctt->getComtLink(comt,lnk)) == 0 ||
                            (qid = ctt->getLinkQ(clnk)) == 0 ||
                            (!qm->enq(cx,qid,now)))
                                ps->free(cx);
                } else {
                        forward(cx,ctt->getComtIndex(comt));
                }
        }
        // remove expired entries from pending list
        for (uint64_t pid : dropList) pending->erase(pid);
}

/** Handle incoming replies to control packets.
 *  The reply is checked against the map of pending control packets,
 *  and if a match is found, the entry is removed from the map
 *  and the storage for the original control packet is freed.
 *  Currently, the only action on a reply is to print an error message
 *  to the log if we receive a negative reply.
 *  @param rx is the packet index of the reply packet
 */	
void RouterCore::handleControlReply(pktx rx) {
	Packet& reply = ps->getPacket(rx);
	uint64_t pid;
	CtlPkt cpr;
	if (reply.type == Forest::CONNECT || reply.type == Forest::DISCONNECT ||
	    reply.type == Forest::SUB_UNSUB) {
		pid  = ntohl(reply.payload()[0]); pid <<= 32;
		pid |= ntohl(reply.payload()[1]);
	} else if (reply.type == Forest::NET_SIG) {
		cpr.reset(reply); pid = cpr.seqNum;
	} else {
		string s;
		cerr << "RouterCore::handleControlReply: unexpected reply "
		     << reply.toString(s); 
		ps->free(rx); return;
	}
	map<uint64_t,ControlInfo>::iterator pp = pending->find(pid);
	if (pp == pending->end()) {
		// this is a reply to a request we never sent, or
		// possibly, a reply to a request we gave up on
		string s;
		cerr << "RouterCore::handleControlReply: unexpected reply "
		     << reply.toString(s);
		ps->free(rx); return;
	}
	// handle signalling packets
	if (reply.type == Forest::NET_SIG) {
		if (cpr.mode == CtlPkt::NEG_REPLY) {
			string s;
			cerr << "RouterCore::handleControlReply: got negative "
			        "reply to "
			     << ps->getPacket(pp->second.px).toString(s); 
			cerr << "reply=" << reply.toString(s);
		} else if (cpr.type == CtlPkt::BOOT_ROUTER) {
			if (booting && !setup()) {
				cerr << "RouterCore::handleControlReply: "
					"setup failed after completion of boot "
					"phase\n";
				perror("");
				pktLog->write(cout);
				exit(1);
			}
			iop->closeBootSock();
			booting = false;
		} else if (cpr.type == CtlPkt::COMTREE_PATH) {
			handleComtPath(pp->second.px,cpr);	
		} else if (cpr.type == CtlPkt::COMTREE_ADD_BRANCH) {
			handleAddBranchReply(pp->second.px,cpr);
		} else if (cpr.type == CtlPkt::ADD_BRANCH_CONFIRM) {
			handleConfirmReply(pp->second.px,cpr);	
		} else if (cpr.type == CtlPkt::COMTREE_NEW_LEAF) {
		        handleComtNewLeafReply(pp->second.px,cpr);		
		} else if (cpr.type == CtlPkt::COMTREE_PRUNE) {
			handleComtPruneReply(pp->second.px,cpr);	
		}
	}

	// free both packets and erase pending entry
	ps->free(pp->second.px); ps->free(rx); pending->erase(pp);
}


/**
 *  Update the length, flip the addresses and pack the buffer.
 *  @param px is the packet number
 *  @param cp is a control packet to be packed
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
