/** @file RouterCore.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RouterCore.h"

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
		qm->setLinkRates(lnk,lt->getBitRate(lnk),lt->getPktRate(lnk));
	}
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
			qm->setQRates(qid,Forest::MINBITRATE,
					  Forest::MINPKTRATE);
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
	int iface;
	for (iface = ift->firstIface(); iface != 0;
	     iface = ift->nextIface(iface)) {
		if (ift->getMaxBitRate(iface) < Forest::MINBITRATE ||
		    ift->getMaxBitRate(iface) > Forest::MAXBITRATE ||
		    ift->getMaxPktRate(iface) < Forest::MINBITRATE ||
		    ift->getMaxPktRate(iface) > Forest::MAXBITRATE) {
			cerr << "RouterCore::setAvailRates: interface rates "
				"outside allowed range\n";
			success = false;
		}
		ift->setAvailBitRate(iface, ift->getMaxBitRate(iface));
		ift->setAvailPktRate(iface, ift->getMaxPktRate(iface));
	}
	if (!success) return false;
	int lnk;
	for (lnk = lt->firstLink(); lnk != 0; lnk = lt->nextLink(lnk)) {
		if (lt->getBitRate(lnk) < Forest::MINBITRATE ||
		    lt->getBitRate(lnk) > Forest::MAXBITRATE ||
		    lt->getPktRate(lnk) < Forest::MINBITRATE ||
		    lt->getPktRate(lnk) > Forest::MAXBITRATE) {
			cerr << "RouterCore::setAvailRates: link rates "
				"outside allowed range\n";
			success = false;
		}
		iface = lt->getIface(lnk);
		if (!ift->addAvailBitRate(iface,-lt->getBitRate(lnk)) ||
		    !ift->addAvailPktRate(iface,-lt->getPktRate(lnk))) {
			cerr << "RouterCore::setAvailRates: oversubscribing "
				"interface " << iface << endl;
			success = false;
		}
		lt->setAvailInBitRate(lnk,lt->getBitRate(lnk));
		lt->setAvailInPktRate(lnk,lt->getPktRate(lnk));
		lt->setAvailOutBitRate(lnk,lt->getBitRate(lnk));
		lt->setAvailOutPktRate(lnk,lt->getPktRate(lnk));
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
			int ibr = ctt->getInBitRate(cLnk);
			int ipr = ctt->getInPktRate(cLnk);
			int obr = ctt->getOutBitRate(cLnk);
			int opr = ctt->getOutPktRate(cLnk);
			if (!lt->addAvailInBitRate(lnk,-ibr) ||
			    !lt->addAvailInPktRate(lnk,-ipr) ||
			    !lt->addAvailOutBitRate(lnk,-obr) ||
			    !lt->addAvailOutPktRate(lnk,-opr)) {
				cerr << "RouterCore::setAvailRates: "
					"oversubscribing link "
				     << lnk << endl;
				success = false;
			}
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
    #ifdef PROFILING // MAH
    Timer timer_loop(    "RouterCore::run() main loop                   ");
    Timer timer_deq(     "RouterCore::run() -> QuManager::deq()         ");
    //Timer timer_enq("RouterCore::run() -> QuManager::enq()");
    //Timer timer_lookup("RouterCore::run() -> UiHashTbl::lookup()");
    Timer timer_pktCheck("RouterCore::run() -> RouterCore::pktCheck()   ");
    Timer timer_pktLog(    "RouterCore::run() -> PacketLog::log()         ");    
    Timer timer_forward( "RouterCore::run() -> RouterCore::forward()    ");
    Timer timer_receive( "RouterCore::run() -> IoProcessor::receive()   ");
    Timer timer_send(    "RouterCore::run() -> IoProcessor::send()      ");    
    #endif
	now = Misc::getTimeNs();
	if (booting) {
		if (!iop->setupBootSock(bootIp,nmIp))
			fatal("RouterCore:run: could not setup boot socket\n");
		string s1;
		cout << "sending boot request to " << Np4d::ip2string(nmIp,s1)
		     << endl;
		CtlPkt cp(BOOT_REQUEST,REQUEST,0);
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
        #ifdef PROFILING // MAH
        timer_loop.start();
        timer_receive.start();
        #endif
		int p = iop->receive();
        #ifdef PROFILING // MAH
        if (p == 0) { timer_receive.cancel(); } // Don't count receive when no packet
        else { timer_receive.stop(); }
        #endif
		if (p != 0) {
			didNothing = false;
			PacketHeader& h = ps->getHeader(p);
			int ptype = h.getPtype();
			#ifdef PROFILING
            timer_pktLog.start();
			pktLog->log(p,h.getInLink(),false,now);
            timer_pktLog.stop();
            #else
            pktLog->log(p,h.getInLink(),false,now);
            #endif
			int ctx = ctt->getComtIndex(h.getComtree());
            #ifdef PROFILING // MAH
            timer_pktCheck.start();
            bool pkt_check = pktCheck(p,ctx);
            timer_pktCheck.stop();
            if (!pkt_check) {
			#else
			if (!pktCheck(p,ctx)) {
		    #endif
				ps->free(p);
			} else if (booting) {
				handleCtlPkt(p);
			} else if (ptype == CLIENT_DATA) {
                #ifdef PROFILING // MAH
                timer_forward.start();
                #endif
				forward(p,ctx);
                #ifdef PROFILING // MAH
                timer_forward.stop();
                #endif				
			} else if (ptype == SUB_UNSUB) {
				subUnsub(p,ctx);
			} else if (ptype == RTE_REPLY) {
				handleRteReply(p,ctx);
			} else if (ptype == CONNECT || ptype == DISCONNECT) {
				handleConnDisc(p);
			} else if (h.getDstAdr() != myAdr) {
                #ifdef PROFILING // MAH
                timer_forward.start();
                #endif
				forward(p,ctx);
                #ifdef PROFILING // MAH
                timer_forward.stop();
                #endif				
			} else {
				ctlQ.push(p);
			}
		}

        #ifdef PROFILING // MAH
        timer_deq.start();
        #endif
		// output processing
		int lnk;
		while ((p = qm->deq(lnk, now)) != 0) {
			didNothing = false;
		    #ifdef PROFILING
            timer_deq.stop();
            timer_pktLog.start();
            #endif
			pktLog->log(p,lnk,true,now);
			#ifdef PROFILING
			timer_pktLog.stop();
            timer_send.start();
            #endif
			iop->send(p,lnk);
			#ifdef PROFILING
            timer_send.stop();
            #endif
		}
        #ifdef PROFILING
        timer_deq.cancel();
        #endif

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
		#ifdef PROFILING // MAH
        timer_loop.stop();
        #endif
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
	#ifdef PROFILING // MAH
    cout << timer_loop << endl;
    cout << timer_deq << endl;
    cout << timer_pktCheck << endl;
    cout << timer_forward << endl;
    cout << timer_receive << endl;
    cout << timer_send << endl;
	#endif
}

/** Perform error checks on forest packet.
 *  @param p is a packet number
 *  @param ctx is the comtree index for p's comtree
 *  @return true if all checks pass, else false
 */
bool RouterCore::pktCheck(int p, int ctx) {
	PacketHeader& h = ps->getHeader(p);
	// check version and  length
	if (h.getVersion() != Forest::FOREST_VERSION) {
		return false;
	}

	if (h.getLength() != h.getIoBytes() || h.getLength()<Forest::HDR_LENG) {
		return false;
	}

	if (booting) {
		return 	h.getSrcAdr() == nmAdr && h.getDstAdr() == myAdr &&
		    	h.getPtype() == NET_SIG &&
		    	h.getComtree() == Forest::NET_SIG_COMT;
	}

	if (!ctt->validComtIndex(ctx)) {
		return false;
	}
	fAdr_t adr = h.getDstAdr();
	if (!Forest::validUcastAdr(adr) && !Forest::mcastAdr(adr)) {
		return false;
	}

	int inLink = h.getInLink();
	if (inLink == 0) return false;
	int cLnk = ctt->getComtLink(ctt->getComtree(ctx),inLink);
	if (cLnk == 0) {
		return false;
	}

	// extra checks for packets from untrusted peers
	if (lt->getPeerType(inLink) < TRUSTED) {
		// check for spoofed source address
		if (lt->getPeerAdr(inLink) != h.getSrcAdr()) return false;
		// and that destination restrictions are respected
		fAdr_t dest = ctt->getDest(cLnk);
		if (dest!=0 && h.getDstAdr() != dest && h.getDstAdr() != myAdr)
			return false;
		// verify that type is valid
		ptyp_t ptype = h.getPtype();
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
			int rcLnk = rt->getLink(rtx);
			int lnk = ctt->getLink(rcLnk);
			int qid = ctt->getLinkQ(rcLnk);
			if (lnk == h.getInLink() || !qm->enq(p,qid,now)) {
				ps->free(p);
			}
			return;
		}
		// multicast data packet
		multiSend(p,ctx,rtx);
		return;
	}
	// no valid route
// think about suppressing flooding, if address is in range for this router
	if (Forest::validUcastAdr(h.getDstAdr())) {
		// send to neighboring routers in comtree
		h.setFlags(Forest::RTE_REQ);
		ps->pack(p); ps->hdrErrUpdate(p);
	}
	multiSend(p,ctx,rtx);
	return;
}
/** Forward multiple copies of a packet.
 *  There are two contexts in which this method is called.
 *  The most common case is to forward a CLIENT_DATA packet.
 *  The other case is to forward a control packet that originates
 *  from this router. In this case the inLink field of the packet's
 *  header should be zero.
 *  @param p is the number of a multi-destination packet
 *  @param ctx is the comtree index for the comtree in p's header
 *  @param rtx is the route index for p, or 0 if there is no route
 */
void RouterCore::multiSend(int p, int ctx, int rtx) {
	int qvec[nLnks]; int n = 0;
	PacketHeader& h = ps->getHeader(p);

	int inLink = h.getInLink();
	if (Forest::validUcastAdr(h.getDstAdr())) {
		// flooding a unicast packet to neighboring routers
		int myZip = Forest::zipCode(myAdr);
		int pZip = Forest::zipCode(h.getDstAdr());
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

	if (n == 0) { ps->free(p); return; }

	// make copies and queue them
        int p1 = p;
        for (int i = 0; i < n-1; i++) { // process first n-1 copies
                if (qm->enq(p1,qvec[i],now)) {
			p1 = ps->clone(p);
		}
        }
        // process last copy
        if (!qm->enq(p1,qvec[n-1],now)) {
		ps->free(p1);
	}
}

/** Send route reply back towards p's source.
 *  The reply is sent on the link on which p was received and
 *  is addressed to p's original sender.
 *  @param p is a packet number
 *  @param ctx is the comtree index for p's comtree
 */
void RouterCore::sendRteReply(int p, int ctx) {
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

	int cLnk = ctt->getComtLink(ctt->getComtree(ctx),h.getInLink());
	qm->enq(p1,ctt->getLinkQ(cLnk),now);
}

/** Handle a route reply packet.
 *  Adds a route to the destination of the original packet that
 *  triggered the route reply, if no route is currently defined.
 *  If there is no route to the destination address in the packet,
 *  the packet is flooded to neighboring routers.
 *  If there is a route to the destination, it is forwarded along
 *  that route, so long as the next hop is another router.
 */
void RouterCore::handleRteReply(int p, int ctx) {
	PacketHeader& h = ps->getHeader(p);
	int rtx = rt->getRteIndex(h.getComtree(), h.getDstAdr());
	int cLnk = ctt->getComtLink(ctt->getComtree(ctx),h.getInLink());
	if ((h.getFlags() & Forest::RTE_REQ) && rtx != 0)
		sendRteReply(p,ctx);
	int adr = ntohl(ps->getPayload(p)[0]);
	if (Forest::validUcastAdr(adr) &&
	    rt->getRteIndex(h.getComtree(),adr) == 0) {
		rt->addEntry(h.getComtree(),adr,cLnk); 
	}
	if (rtx == 0) {
		// send to neighboring routers in comtree
		h.setFlags(Forest::RTE_REQ);
		ps->pack(p); ps->hdrErrUpdate(p);
		multiSend(p,ctx,rtx);
		return;
	}
	int dcLnk = rt->getLink(rtx); int dLnk = ctt->getLink(dcLnk);
	if (lt->getPeerType(dLnk) != ROUTER || !qm->enq(p,dLnk,now))
		ps->free(p);
	return;
}

/** Perform subscription processing on a packet.
 *  The packet contains two lists of multicast addresses,
 *  each preceded by its length. The combined list lengths
 *  is limited to 350.
 *  @param p is a packet number
 *  @param ctx is the comtree index for p's comtree
 */
void RouterCore::subUnsub(int p, int ctx) {
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	// add/remove branches from routes
	// if non-core node, also propagate requests upward as
	// appropriate
	int comt = ctt->getComtree(ctx);
	int inLink = h.getInLink();
	int cLnk = ctt->getComtLink(comt,inLink);
	// ignore subscriptions from the parent or core neighbors
	if (inLink == ctt->getPlink(ctx) || ctt->isCoreLink(cLnk)) {
		ps->free(p); return;
	}
	bool propagate = false;
	int rtx; fAdr_t addr;

	// add subscriptions
	int addcnt = ntohl(pp[0]);
	if (addcnt < 0 || addcnt > 350 ||
	    Forest::OVERHEAD + (addcnt + 2)*4 > h.getLength()) {
		ps->free(p); return;
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
	    Forest::OVERHEAD + (addcnt + dropcnt + 2)*4 > h.getLength()) {
		ps->free(p); return;
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
		ps->payErrUpdate(p);
		int qid = ctt->getLinkQ(ctt->getPCLink(ctx));
		if (qm->enq(p,qid,now)) {
			return;
		}
	}
	ps->free(p); return;
}

/** Handle a CONNECT or DISCONNECT packet.
 *  @param p is the packet number of the packet to be handled.
 */
void RouterCore::handleConnDisc(int p) {
	PacketHeader& h = ps->getHeader(p);
	int inLnk = h.getInLink();

	if (!validLeafAdr(h.getSrcAdr())) {
		ps->free(p); return;
	}
	if (h.getPtype() == CONNECT) {
		if (lt->getPeerPort(inLnk) != h.getTunSrcPort()) {
			if (lt->getPeerPort(inLnk) != 0) {
				string s;
				cerr << "modifying peer port for host with ip "
				     << Np4d::ip2string(h.getTunSrcIp(),s)
				     << endl;
			}
			lt->setPeerPort(inLnk,h.getTunSrcPort());
		}
		if (nmAdr != 0 && lt->getPeerType(inLnk) == CLIENT) {
			CtlPkt cp(CLIENT_CONNECT,REQUEST,0);
			cp.setAttr(CLIENT_ADR,h.getSrcAdr());
			cp.setAttr(RTR_ADR,myAdr);
			sendCpReq(cp,nmAdr);
		}
	} else if (h.getPtype() == DISCONNECT) {
		if (lt->getPeerPort(inLnk) == h.getTunSrcPort()) {
			dropLink(inLnk);
		}
		if (nmAdr != 0 && lt->getPeerType(inLnk) == CLIENT) {
			CtlPkt cp(CLIENT_DISCONNECT,REQUEST,0);
			cp.setAttr(CLIENT_ADR,h.getSrcAdr());
			cp.setAttr(RTR_ADR,myAdr);
			sendCpReq(cp,nmAdr);
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
 *  @param p is a packet number
 */
void RouterCore::handleCtlPkt(int p) {
	PacketHeader& h = ps->getHeader(p);
	CtlPkt cp;

	int len = h.getLength() - (Forest::HDR_LENG + 4);
	if (!cp.unpack(ps->getPayload(p), len)) {
		string s;
		cerr << "RouterCore::handleCtlPkt: misformatted control "
			" packet\n" << h.toString(ps->getBuffer(p),s);
		cp.setRrType(NEG_REPLY);
		cp.setErrMsg("misformatted control packet");
		returnToSender(p,cp.pack(ps->getPayload(p)));
		return;
	}
	if (h.getPtype() != NET_SIG || h.getComtree() != Forest::NET_SIG_COMT) {
		ps->free(p); return;
	}
	if (cp.getRrType() != REQUEST) { 
		handleCpReply(p,cp); return;
	}
	
	// Prepare positive reply packet for use where appropriate
	CtlPkt reply;
	reply.setCpType(cp.getCpType());
	reply.setRrType(POS_REPLY);
	reply.setSeqNum(cp.getSeqNum());
	switch (cp.getCpType()) {

	// configuring logical interfaces
	case ADD_IFACE: 	addIface(p,cp,reply); break;
        case DROP_IFACE:	dropIface(p,cp,reply); break;
        case GET_IFACE:		getIface(p,cp,reply); break;
        case MOD_IFACE:		modIface(p,cp,reply); break;

	// configuring links
        case ADD_LINK:		addLink(p,cp,reply); break;
        case DROP_LINK:		dropLink(p,cp,reply); break;
        case GET_LINK:		getLink(p,cp,reply); break;
        case MOD_LINK:		modLink(p,cp,reply); break;

	// configuring comtrees
        case ADD_COMTREE:	addComtree(p,cp,reply); break;
        case DROP_COMTREE:	dropComtree(p,cp,reply); break;
        case GET_COMTREE:	getComtree(p,cp,reply); break;
        case MOD_COMTREE:	modComtree(p,cp,reply); break;
	case ADD_COMTREE_LINK:	addComtreeLink(p,cp,reply); break;
	case DROP_COMTREE_LINK: dropComtreeLink(p,cp,reply); break;
	case GET_COMTREE_LINK:	getComtreeLink(p,cp,reply); break;
	case MOD_COMTREE_LINK:	modComtreeLink(p,cp,reply); break;

	// configuring routes
        case ADD_ROUTE:		addRoute(p,cp,reply); break;
        case DROP_ROUTE:	dropRoute(p,cp,reply); break;
        case GET_ROUTE:		getRoute(p,cp,reply); break;
        case MOD_ROUTE:		modRoute(p,cp,reply); break;

	// finishing up boot phase
	case BOOT_COMPLETE:	bootComplete(p,cp,reply); break;

	// aborting boot process
	case BOOT_ABORT:	bootAbort(p,cp,reply); break;
	
	default:
		cerr << "unrecognized control packet type " << cp.getCpType()
		     << endl;
		reply.setErrMsg("invalid control packet for router");
		reply.setRrType(NEG_REPLY);
		break;
	}

	returnToSender(p,reply.pack(ps->getPayload(p)));

	if (reply.getCpType() == BOOT_COMPLETE) {
		iop->closeBootSock();
		booting = false;
	}

	return;
}

/** Handle an ADD_IFACE control packet.
 *  Adds the specified interface and prepares a reply packet.
 *  @param p is a packet number
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param reply is a control packet structure for the reply, which
 *  the cpType has been set to match cp, the rrType is POS_REPLY
 *  and the seqNum has been set to match cp.
 *  @param return true on success, false on failure; on a successful
 *  return, all appropriate attributes in reply are set; on a failure
 *  return, the errMsg field of reply is set.
 */
bool RouterCore::addIface(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(IFACE_NUM) || !cp.isSet(LOCAL_IP) ||
	    !cp.isSet(MAX_BIT_RATE) || !cp.isSet(MAX_PKT_RATE)) {
		reply.setErrMsg("add iface: missing required attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int iface = cp.getAttr(IFACE_NUM);
	ipa_t localIp = cp.getAttr(LOCAL_IP);
	int bitRate = cp.getAttr(MAX_BIT_RATE);
	int pktRate = cp.getAttr(MAX_PKT_RATE);
	bitRate = max(bitRate, Forest::MINBITRATE);
	pktRate = max(pktRate, Forest::MINPKTRATE);
	bitRate = min(bitRate, Forest::MAXBITRATE);
	pktRate = min(pktRate, Forest::MAXPKTRATE);
	if (ift->valid(iface)) {
		if (localIp != ift->getIpAdr(iface) ||
		    bitRate != ift->getMaxBitRate(iface) ||
		    pktRate != ift->getMaxPktRate(iface)) {
			reply.setErrMsg("add iface: requested interface "
				 	"conflicts with existing interface");
			reply.setRrType(NEG_REPLY);
			return false;
		}
	} else if (!ift->addEntry(iface, localIp, bitRate, pktRate)) {
		reply.setErrMsg("add iface: cannot add interface");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	return true;
}

bool RouterCore::dropIface(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(IFACE_NUM)) {
		reply.setErrMsg("drop iface: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int iface = cp.getAttr(IFACE_NUM);
	ift->removeEntry(iface);
	return true;
}

bool RouterCore::getIface(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(IFACE_NUM)) {
		reply.setErrMsg("get iface: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int iface = cp.getAttr(IFACE_NUM);
	if (ift->valid(iface)) {
		reply.setAttr(IFACE_NUM,iface);
		reply.setAttr(LOCAL_IP,ift->getIpAdr(iface));
		reply.setAttr(AVAIL_BIT_RATE,ift->getAvailBitRate(iface));
		reply.setAttr(AVAIL_PKT_RATE,ift->getAvailPktRate(iface));
		reply.setAttr(MAX_BIT_RATE,ift->getMaxBitRate(iface));
		reply.setAttr(MAX_PKT_RATE,ift->getMaxPktRate(iface));
		return true;
	}
	reply.setErrMsg("get iface: invalid interface");
	reply.setRrType(NEG_REPLY);
	return false;
}

bool RouterCore::modIface(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(IFACE_NUM)) {
		reply.setErrMsg("add iface: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int iface = cp.getAttr(IFACE_NUM);
	if (ift->valid(iface)) {
		if (cp.isSet(MAX_BIT_RATE))
			ift->setMaxBitRate(iface,cp.getAttr(MAX_BIT_RATE));
		if (cp.isSet(MAX_PKT_RATE))
			ift->setMaxPktRate(iface,cp.getAttr(MAX_PKT_RATE));
		return true;
	}
	reply.setErrMsg("mod iface: invalid interface");
	reply.setRrType(NEG_REPLY);
	return false;
}

bool RouterCore::addLink(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(PEER_IP) || !cp.isSet(PEER_TYPE)) {
		reply.setErrMsg("add link: missing required attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	ntyp_t peerType = (ntyp_t) cp.getAttr(PEER_TYPE);
	if (peerType == ROUTER && !cp.isSet(PEER_ADR)) {
		reply.setErrMsg("add link: adding link to router, but no peer "
				"address supplied");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	ipa_t  pipa = cp.getAttr(PEER_IP);
	int lnk = (cp.isSet(LINK_NUM) ? cp.getAttr(LINK_NUM) : 0);
	int iface = (cp.isSet(IFACE_NUM) ? cp.getAttr(IFACE_NUM) :
				  	   ift->getDefaultIface());
	ipp_t pipp = (cp.isSet(PEER_PORT) ? cp.getAttr(PEER_PORT) :
		      (peerType == ROUTER ? Forest::ROUTER_PORT : 0));
	fAdr_t padr = (cp.isSet(PEER_ADR) ? cp.getAttr(PEER_ADR): 0);
	int xlnk = lt->lookup(pipa,pipp);
	if (xlnk != 0) { // this link already exists
		if ((lnk != 0 && lnk != xlnk) ||
		    (peerType != lt->getPeerType(xlnk)) ||
		    (cp.isSet(IFACE_NUM) &&
		     cp.getAttr(IFACE_NUM) != lt->getIface(xlnk)) ||
		    (padr != 0 && padr != lt->getPeerAdr(xlnk))) {
			reply.setErrMsg("add link: new link conflicts "
						"with existing link");
			reply.setRrType(NEG_REPLY);
			return false;
		}
		lnk = xlnk; padr = lt->getPeerAdr(xlnk);
	} else { // adding a new link
		// first ensure that the interface has enough
		// capacity to support a new link
		int br = Forest::MINBITRATE;
		int pr = Forest::MINPKTRATE;
		if (!ift->addAvailBitRate(iface,-br)) {
			reply.setErrMsg("add link: requested link "
						"exceeds interface capacity");
			reply.setRrType(NEG_REPLY);
			return false;
		}
		if (!ift->addAvailPktRate(iface,-pr)) {
			reply.setErrMsg("add link: requested link "
						"exceeds interface capacity");
			reply.setRrType(NEG_REPLY);
			return false;
		}
		if ((peerType == ROUTER && pipp != Forest::ROUTER_PORT) ||
		    (peerType != ROUTER && pipp == Forest::ROUTER_PORT) ||
		    (lnk = lt->addEntry(lnk,pipa,pipp)) == 0) {
			ift->addAvailBitRate(iface,br);
			ift->addAvailPktRate(iface,pr);
			reply.setErrMsg("add link: cannot add "
				 		"requested link");
			reply.setRrType(NEG_REPLY);
			return false;
		}
		// note: when lt->addEntry succeeds, link rates are
		// initializied to Forest minimum rates
		if (peerType != ROUTER && padr != 0 && !allocLeafAdr(padr)) {
			ift->addAvailBitRate(iface,br);
			ift->addAvailPktRate(iface,pr);
			lt->removeEntry(lnk);
			reply.setErrMsg("add link: specified peer "
						"address is in use");
			reply.setRrType(NEG_REPLY);
			return false;
		}
		if (padr == 0) padr = allocLeafAdr();
		if (padr == 0) {
			ift->addAvailBitRate(iface,br);
			ift->addAvailPktRate(iface,pr);
			lt->removeEntry(lnk);
			reply.setErrMsg("add link: no available "
						"peer addresses");
			reply.setRrType(NEG_REPLY);
			return false;
		}
		lt->setIface(lnk,iface);
		lt->setPeerType(lnk,peerType);
		lt->setPeerAdr(lnk,padr);
		sm->clearLnkStats(lnk);
	}
	reply.setAttr(LINK_NUM,lnk);
	reply.setAttr(PEER_ADR,padr);
	reply.setAttr(RTR_IP,ift->getIpAdr(iface));
	return true;
}

bool RouterCore::dropLink(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(LINK_NUM)) {
		reply.setErrMsg("drop link: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	dropLink(cp.getAttr(LINK_NUM));
	return true;
}

void RouterCore::dropLink(int lnk) {
	set<int>& comtSet = lt->getComtSet(lnk);
	set<int>::iterator cp;
	int *comtVec = new int[comtSet.size()]; int i = 0;
	for (cp = comtSet.begin(); cp != comtSet.end(); cp++) {
		comtVec[i++] = *cp;
	}
	while (--i >= 0) {
		int ctx = comtVec[i];
		int cLnk = ctt->getComtLink(ctt->getComtree(ctx),lnk);
		dropComtreeLink(ctx,lnk,cLnk);
	}
	int iface = lt->getIface(lnk);
	ift->addAvailBitRate(iface,lt->getBitRate(lnk));
	ift->addAvailPktRate(iface,lt->getPktRate(lnk));
	lt->removeEntry(lnk);
	freeLeafAdr(lt->getPeerAdr(lnk));
}

bool RouterCore::getLink(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(LINK_NUM)) {
		reply.setErrMsg("get link: missing required attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int link = cp.getAttr(LINK_NUM);
	if (lt->valid(link)) {
		reply.setAttr(LINK_NUM, link);
		reply.setAttr(IFACE_NUM, lt->getIface(link));
		reply.setAttr(PEER_IP, lt->getPeerIpAdr(link));
		reply.setAttr(PEER_TYPE, lt->getPeerType(link));
		reply.setAttr(PEER_PORT, lt->getPeerPort(link));
		reply.setAttr(PEER_ADR, lt->getPeerAdr(link));
		reply.setAttr(AVAIL_BIT_RATE_IN,
				lt->getAvailInBitRate(link));
		reply.setAttr(AVAIL_PKT_RATE_IN,
				lt->getAvailInPktRate(link));
		reply.setAttr(AVAIL_BIT_RATE_OUT,
				lt->getAvailOutBitRate(link));
		reply.setAttr(AVAIL_PKT_RATE_OUT,
				lt->getAvailOutPktRate(link));
		reply.setAttr(BIT_RATE, lt->getBitRate(link));
		reply.setAttr(PKT_RATE, lt->getPktRate(link));
		return true;
	} 
	reply.setErrMsg("get link: invalid link number");
	reply.setRrType(NEG_REPLY);
	return false;
}

bool RouterCore::modLink(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(LINK_NUM)) {
		reply.setErrMsg("modify link: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int link = cp.getAttr(LINK_NUM);
	if (lt->valid(link)) {
		reply.setAttr(LINK_NUM,link);
		int iface = lt->getIface(link);
		int br, dbr; br = dbr = 0;
		if (cp.isSet(BIT_RATE)) {
			br = cp.getAttr(BIT_RATE);
			dbr = br - lt->getBitRate(link);
			if (!ift->addAvailBitRate(iface,-dbr)) {
				reply.setErrMsg("mod link: request "
						"exceeds interface capacity");
				reply.setRrType(NEG_REPLY);
				return false;
			}
			lt->setBitRate(link, br); 
			lt->addAvailInBitRate(link, dbr); 
			lt->addAvailOutBitRate(link, dbr); 
			qm->setLinkRates(link,br,lt->getPktRate(link));
		}
		if (cp.isSet(PKT_RATE)) {
			int pr = cp.getAttr(PKT_RATE);
			int dpr = pr - lt->getPktRate(link);
			if (!ift->addAvailPktRate(iface,-dpr)) {
				if (cp.isSet(BIT_RATE)) { //undo earlier changes
					ift->addAvailBitRate(iface,dbr);
					lt->setBitRate(link,br-dbr);
					lt->addAvailInBitRate(link, -dbr);
					lt->addAvailOutBitRate(link, -dbr);
					qm->setLinkRates(link,br-dbr,
							 lt->getPktRate(link));
				}
				reply.setErrMsg("mod link: request "
					 "exceeds interface capacity");
				reply.setRrType(NEG_REPLY);
				return false;
			}
			lt->setPktRate(link, pr); 
			lt->addAvailInPktRate(link, dpr); 
			lt->addAvailOutPktRate(link, dpr); 
			qm->setLinkRates(link,lt->getBitRate(link),pr);
		}
		return true;
	}
	reply.setErrMsg("get link: invalid link number");
	reply.setRrType(NEG_REPLY);
	return false;
}

bool RouterCore::addComtree(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(COMTREE_NUM)) {
		reply.setErrMsg("add comtree: missing required "
			       "attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int comt = cp.getAttr(COMTREE_NUM);
	if(ctt->validComtree(comt) || ctt->addEntry(comt) != 0)
		return true;
	reply.setErrMsg("add comtree: cannot add comtree");
	reply.setRrType(NEG_REPLY);
	return false;
}

bool RouterCore::dropComtree(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(COMTREE_NUM)) {
		reply.setErrMsg("drop comtree: missing required "
			       "attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int comt = cp.getAttr(COMTREE_NUM);
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

bool RouterCore::getComtree(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(COMTREE_NUM)) {
		reply.setErrMsg("get comtree: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	comt_t comt = cp.getAttr(COMTREE_NUM);
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.setErrMsg("get comtree: invalid comtree");
		reply.setRrType(NEG_REPLY); 
		return false;
	}
	reply.setAttr(COMTREE_NUM,comt);
	reply.setAttr(CORE_FLAG,ctt->inCore(ctx));
	reply.setAttr(PARENT_LINK,ctt->getPlink(ctx));
	reply.setAttr(LINK_COUNT,ctt->getLinkCount(ctx));
	return true;
}

bool RouterCore::modComtree(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!cp.isSet(COMTREE_NUM)) {
		reply.setErrMsg("modify comtree: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	comt_t comt = cp.getAttr(COMTREE_NUM);
	int ctx = ctt->getComtIndex(comt);
	if (ctx != 0) {
		if (cp.isSet(CORE_FLAG))
			ctt->setCoreFlag(ctx,cp.getAttr(CORE_FLAG));
		if (cp.isSet(PARENT_LINK)) {
			int plnk = cp.getAttr(PARENT_LINK);
			if (plnk != 0 && !ctt->isLink(ctx,plnk)) {
				reply.setErrMsg("specified link does "
						"not belong to comtree");
				reply.setRrType(NEG_REPLY);
				return false;
			}
			if (plnk != 0 && !ctt->isRtrLink(ctx,plnk)) {
				reply.setErrMsg("specified link does "
						"not connect to a router");
				reply.setRrType(NEG_REPLY);
				return false;
			}
			ctt->setPlink(ctx,plnk);
		}
		return true;
	} 
	reply.setErrMsg("modify comtree: invalid comtree");
	reply.setRrType(NEG_REPLY);
	return false;
}

bool RouterCore::addComtreeLink(int p, CtlPkt& cp, CtlPkt& reply) {
	// Note: we require the comtree number and
	// either the link number or the peerIP and peerPort
	if (!(cp.isSet(COMTREE_NUM) &&
	      (cp.isSet(LINK_NUM) ||
	        (cp.isSet(PEER_IP) && cp.isSet(PEER_PORT))))) {
		reply.setErrMsg("add comtree link: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	comt_t comt = cp.getAttr(COMTREE_NUM);
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.setErrMsg("add comtree link: invalid comtree");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int lnk = 0; 
	if (cp.isSet(LINK_NUM)) {
		lnk = cp.getAttr(LINK_NUM);
	} else if (cp.isSet(PEER_IP) && cp.isSet(PEER_PORT)) {
		lnk = lt->lookup(cp.getAttr(PEER_IP),
				 cp.getAttr(PEER_PORT));
	}
	if (!lt->valid(lnk)) {
		reply.setErrMsg("add comtree link: invalid link or "
					"peer IP and port");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	bool isRtr = (lt->getPeerType(lnk) == ROUTER);
	bool isCore = false;
	if (isRtr) {
		if(!cp.isSet(PEER_CORE_FLAG)) {
			reply.setErrMsg("add comtree link: must "
				"specify core flag on links to routers");
			reply.setRrType(NEG_REPLY);
			return false;
		}
		isCore = cp.getAttr(PEER_CORE_FLAG);
	}
	int cLnk = ctt->getComtLink(comt,lnk);
	if (cLnk != 0) {
		if (ctt->isRtrLink(cLnk) == isRtr &&
		    ctt->isCoreLink(cLnk) == isCore) {
			reply.setAttr(LINK_NUM,lnk);
			return true;
		} else {
			reply.setErrMsg("add comtree link: specified "
				       "link already in comtree");
			reply.setRrType(NEG_REPLY);
			return false;
		}
	}
	// define new comtree link
	if (!ctt->addLink(ctx,lnk,isRtr,isCore)) {
		reply.setErrMsg("add comtree link: cannot add "
					"requested comtree link");
		reply.setRrType(NEG_REPLY);
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
		reply.setErrMsg("add comtree link: no queues "
					"available for link");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	ctt->setLinkQ(cLnk,qid);

	// adjust rates for link comtree and queue
	int br = Forest::MINBITRATE;
	int pr = Forest::MINPKTRATE;
	if (!(lt->addAvailInBitRate(lnk, -br) &&
	      lt->addAvailInPktRate(lnk, -pr) &&
	      lt->addAvailOutBitRate(lnk, -br) &&
	      lt->addAvailOutPktRate(lnk, -pr))) {
		reply.setErrMsg("add comtree link: request "
					"exceeds link capacity");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	ctt->setInBitRate(cLnk,br);
	ctt->setInPktRate(cLnk,pr);
	ctt->setOutBitRate(cLnk,br);
	ctt->setOutPktRate(cLnk,pr);

	qm->setQRates(qid,br,pr);
	if (isRtr) qm->setQLimits(qid,500,1000000);
	else	   qm->setQLimits(qid,500,1000000);
	sm->clearQuStats(qid);
	reply.setAttr(LINK_NUM,lnk);
	return true;
}

bool RouterCore::dropComtreeLink(int p, CtlPkt& cp, CtlPkt& reply) {
	// Note: we require either the link number or
	// the peerIP and peerPort
	if (!(cp.isSet(COMTREE_NUM) &&
	      (cp.isSet(LINK_NUM) ||
	        (cp.isSet(PEER_IP) && cp.isSet(PEER_PORT))))) {
		reply.setErrMsg("add comtree link: missing required "
			  		"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	comt_t comt = cp.getAttr(COMTREE_NUM);
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.setErrMsg("drop comtree link: invalid comtree");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int lnk;
	if (cp.isSet(LINK_NUM)) lnk = cp.getAttr(LINK_NUM);
	else lnk = lt->lookup(cp.getAttr(PEER_IP),cp.getAttr(PEER_PORT));
	if (!lt->valid(lnk)) {
		reply.setErrMsg("drop comtree link: invalid link "
			       "or peer IP and port");
		reply.setRrType(NEG_REPLY);
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
	lt->addAvailInBitRate(lnk,ctt->getInBitRate(cLnk));
	lt->addAvailInPktRate(lnk,ctt->getInPktRate(cLnk));
	lt->addAvailOutBitRate(lnk,ctt->getOutBitRate(cLnk));
	lt->addAvailOutPktRate(lnk,ctt->getOutPktRate(cLnk));

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

bool RouterCore::modComtreeLink(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!(cp.isSet(COMTREE_NUM) && cp.isSet(LINK_NUM))) {
		reply.setErrMsg("modify comtree link: missing required "
			  		"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	comt_t comt = cp.getAttr(COMTREE_NUM);
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.setErrMsg("modify comtree link: invalid comtree");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int lnk = cp.getAttr(LINK_NUM);
	if (!lt->valid(lnk)) {
		reply.setErrMsg("modify comtree link: invalid link "
					"number");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int cLnk = ctt->getComtLink(comt,lnk);
	if (cLnk == 0) {
		reply.setErrMsg("modify comtree link: specified link "
			       "not defined in specified comtree");
		reply.setRrType(NEG_REPLY);
		return false;
	}

	int ibr = ctt->getInBitRate(cLnk);
	int ipr = ctt->getInPktRate(cLnk);
	int obr = ctt->getOutBitRate(cLnk);
	int opr = ctt->getOutPktRate(cLnk);

	if (cp.isSet(BIT_RATE_IN))  ibr = cp.getAttr(BIT_RATE_IN);
	if (cp.isSet(PKT_RATE_IN))  ipr = cp.getAttr(PKT_RATE_IN);
	if (cp.isSet(BIT_RATE_OUT)) obr = cp.getAttr(BIT_RATE_OUT);
	if (cp.isSet(PKT_RATE_OUT)) opr = cp.getAttr(PKT_RATE_OUT);

	int dibr = ibr - ctt->getInBitRate(cLnk);
	int dipr = ipr - ctt->getInPktRate(cLnk);
	int dobr = obr - ctt->getOutBitRate(cLnk);
	int dopr = opr - ctt->getOutPktRate(cLnk);

	bool success = true;
	if (lt->getAvailInBitRate(lnk) < dibr) {
		reply.setErrMsg("modify comtree link: increase in "
				"input bit rate exceeds link capacity");
		success = false;
	}
	if (lt->getAvailInPktRate(lnk) < dipr) {
		reply.setErrMsg("modify comtree link: increase in "
				"input packet rate exceeds link capacity");
		success = false;
	}
	if (lt->getAvailOutBitRate(lnk) < dobr) {
		reply.setErrMsg("modify comtree link: increase in "
				"output bit rate exceeds link capacity");
		success = false;
	}
	if (lt->getAvailOutPktRate(lnk) < dopr) {
		reply.setErrMsg("modify comtree link: increase in "
				"output packet rate exceeds link capacity");
		success = false;
	}
	if (!success) { reply.setRrType(NEG_REPLY); return false; }

	if (dibr != 0) {
		lt->addAvailInBitRate(lnk,-dibr);  ctt->setInBitRate(cLnk,ibr);
	}
	if (dipr != 0) {
		lt->addAvailInPktRate(lnk,-dipr);  ctt->setInPktRate(cLnk,ipr); 
	}
	if (dobr != 0) {
		lt->addAvailOutBitRate(lnk,-dobr); ctt->setOutBitRate(cLnk,obr);
	}
	if (dopr != 0) {
		lt->addAvailOutPktRate(lnk,-dopr); ctt->setOutPktRate(cLnk,opr);
	}
	return true;
}

bool RouterCore::getComtreeLink(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!(cp.isSet(COMTREE_NUM) && cp.isSet(LINK_NUM))) {
		reply.setErrMsg("get comtree link: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	comt_t comt = cp.getAttr(COMTREE_NUM);
	int ctx = ctt->getComtIndex(comt);
	if (ctx == 0) {
		reply.setErrMsg("get comtree link: invalid comtree");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int lnk = cp.getAttr(LINK_NUM);
	if (!lt->valid(lnk)) {
		reply.setErrMsg("get comtree link: invalid link "
					"number");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int cLnk = ctt->getComtLink(comt,lnk);
	if (cLnk == 0) {
		reply.setErrMsg("get comtree link: specified link "
			  	"not defined in specified comtree");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	reply.setAttr(COMTREE_NUM,comt);
	reply.setAttr(LINK_NUM,lnk);
	reply.setAttr(QUEUE_NUM,ctt->getLinkQ(cLnk));
	reply.setAttr(PEER_DEST,ctt->getDest(cLnk));
	reply.setAttr(BIT_RATE_IN,ctt->getInBitRate(cLnk));
	reply.setAttr(PKT_RATE_IN,ctt->getInPktRate(cLnk));
	reply.setAttr(BIT_RATE_OUT,ctt->getOutBitRate(cLnk));
	reply.setAttr(PKT_RATE_OUT,ctt->getOutPktRate(cLnk));

	return true;
}

bool RouterCore::addRoute(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!(cp.isSet(COMTREE_NUM) && cp.isSet(DEST_ADR) &&
	      cp.isSet(LINK_NUM))) {
		reply.setErrMsg("add route: missing required "
			 		"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	comt_t comt = cp.getAttr(COMTREE_NUM);
	if (!ctt->validComtree(comt)) {
		reply.setErrMsg("comtree not defined at this router\n");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	fAdr_t dest = cp.getAttr(DEST_ADR);
	if (!Forest::validUcastAdr(dest) && !Forest::mcastAdr(dest)) {
		reply.setErrMsg("invalid address\n");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int lnk = cp.getAttr(LINK_NUM);
	int cLnk = ctt->getComtLink(comt,lnk);
	int rtx = rt->getRteIndex(comt,dest);
	if (rtx != 0) {
	   	if ((Forest::validUcastAdr(dest) && rt->getLink(rtx) == cLnk) ||
		    (Forest::mcastAdr(dest) && rt->isLink(rtx,cLnk))) {
			return true;
		} else {
			reply.setErrMsg("add route: requested route "
				        "conflicts with existing route");
			reply.setRrType(NEG_REPLY);
			return false;
		}
	} else if (rt->addEntry(comt, dest, lnk)) {
		return true;
	}
	reply.setErrMsg("add route: cannot add route");
	reply.setRrType(NEG_REPLY);
	return false;
}

bool RouterCore::dropRoute(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!(cp.isSet(COMTREE_NUM) && cp.isSet(DEST_ADR))) {
		reply.setErrMsg("drop route: missing required "
		 			"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	comt_t comt = cp.getAttr(COMTREE_NUM);
	if (!ctt->validComtree(comt)) {
		reply.setErrMsg("comtree not defined at this router\n");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	fAdr_t dest = cp.getAttr(DEST_ADR);
	if (!Forest::validUcastAdr(dest) && !Forest::mcastAdr(dest)) {
		reply.setErrMsg("invalid address\n");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int rtx = rt->getRteIndex(comt,dest);
	rt->removeEntry(rtx);
	return true;
}

bool RouterCore::getRoute(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!(cp.isSet(COMTREE_NUM) && cp.isSet(DEST_ADR))) {
		reply.setErrMsg("get route: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	comt_t comt = cp.getAttr(COMTREE_NUM);
	if (!ctt->validComtree(comt)) {
		reply.setErrMsg("comtree not defined at this router\n");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	fAdr_t dest = cp.getAttr(DEST_ADR);
	if (!Forest::validUcastAdr(dest) && !Forest::mcastAdr(dest)) {
		reply.setErrMsg("invalid address\n");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int rtx = rt->getRteIndex(comt,dest);
	if (rtx != 0) {
		reply.setAttr(COMTREE_NUM,comt);
		reply.setAttr(DEST_ADR,dest);
		if (Forest::validUcastAdr(dest)) {
			int lnk = ctt->getLink(rt->getLink(rtx));
			reply.setAttr(LINK_NUM,lnk);
		} else {
			reply.setAttr(LINK_NUM,0);
		}
		return true;
	}
	reply.setErrMsg("get route: no route for specified address");
	reply.setRrType(NEG_REPLY);
	return false;
}
        
bool RouterCore::modRoute(int p, CtlPkt& cp, CtlPkt& reply) {
	if (!(cp.isSet(COMTREE_NUM) && cp.isSet(DEST_ADR))) {
		reply.setErrMsg("mod route: missing required "
					"attribute");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	comt_t comt = cp.getAttr(COMTREE_NUM);
	if (!ctt->validComtree(comt)) {
		reply.setErrMsg("comtree not defined at this router\n");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	fAdr_t dest = cp.getAttr(DEST_ADR);
	if (!Forest::validUcastAdr(dest) && !Forest::mcastAdr(dest)) {
		reply.setErrMsg("invalid address\n");
		reply.setRrType(NEG_REPLY);
		return false;
	}
	int rtx = rt->getRteIndex(comt,dest);
	if (rtx != 0) {
		if (cp.isSet(LINK_NUM)) {
			if (Forest::mcastAdr(dest)) {
				reply.setErrMsg("modify route: cannot "
					       	"set link in multicast route");
				reply.setRrType(NEG_REPLY);
				return false;
			}
			rt->setLink(rtx,cp.getAttr(LINK_NUM));
		}
		return true;
	}
	reply.setErrMsg("modify route: invalid route");
	reply.setRrType(NEG_REPLY);
	return false;
}

/** Handle boot complete message.
 *  At the end of the boot phase, we do some additional setup and checking
 *  of the configuration tables. If the tables are inconsistent in some way,
 *  the operation fails. On successful completion of the boot phase,
 *  we close the boot socket and sent the booting variable to false.
 *  @param p is the number of the boot complete request packet
 *  @param cp is an unpacked control packet for p
 *  @param reply is a reference to a pre-formatted positive reply packet
 *  that will be returned to the sender; it may be modified to indicate
 *  a negative reply.
 *  @return true on success, false on failure
 */
bool RouterCore::bootComplete(int p, CtlPkt& cp, CtlPkt& reply) {
	if (booting && !setup()) {
		cerr << "RouterCore::bootComplete: setup failed after "
			"completion of boot phase\n";
		perror("");
		reply.setErrMsg("configured tables are not consistent\n");
		reply.setRrType(NEG_REPLY);
		returnToSender(p,reply.pack(ps->getPayload(p)));
		pktLog->write(cout);
		exit(1);
	}
	return true;
}

bool RouterCore::bootAbort(int p, CtlPkt& cp, CtlPkt& reply) {
	cerr << "RouterCore::bootAbort: received boot abort message "
			"from netMgr; exiting\n";
	reply.setRrType(POS_REPLY);
	returnToSender(p,reply.pack(ps->getPayload(p)));
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
	int p = ps->alloc();
	if (p == 0) {
		cerr << "RouterCore::sendCpReq: no packets left in packet "
			"store\n";
		return false;
	}
	PacketHeader& h = ps->getHeader(p);

	// pack cp into p, setting rr type and seq number
	cp.setRrType(REQUEST);
	cp.setSeqNum(seqNum);
	int paylen = cp.pack(ps->getPayload(p));
	if (paylen == 0) {
		cerr << "RouterCore::sendCpReq: control packet packing error\n";
		return false;
	}
	h.setLength(Forest::OVERHEAD + paylen);
	h.setPtype(NET_SIG); h.setFlags(0);
	h.setComtree(Forest::NET_SIG_COMT);
	h.setSrcAdr(myAdr); h.setDstAdr(dest);
	h.setInLink(0);
	ps->pack(p);

	// save a record of the packet in pending map
	pair<uint64_t,CpInfo> cpp;
	cpp.first = seqNum;
	cpp.second.p = p; cpp.second.nSent = 1; cpp.second.timestamp = now;
	pending->insert(cpp);
	seqNum++;

	// now, make copy of packet and send the copy
	int copy = ps->fullCopy(p);
	if (copy == 0) {
		cerr << "RouterCore::sendCpReq: no packets left in packet "
			"store\n";
		return false;
	}
	if (booting) {
		iop->send(copy,0);
		pktLog->log(copy,0,true,now);
	} else
		forward(copy,ctt->getComtIndex(h.getComtree()));

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
		int p = pp->second.p;
		PacketHeader h = ps->getHeader(p);
		if (pp->second.nSent >= 3) { // give up on this packet
			string s;
			cerr << "RouterCore::resendCpReq: received no reply to "
				"control packet after three attempts\n"
			     << h.toString(ps->getBuffer(p),s);
			ps->free(p);
			pending->erase(pp->first);
			continue;
		}
		string s1;
		cout << "resending control packet\n"
		     << h.toString(ps->getBuffer(p),s1);
		// make copy of packet and send the copy
		pp->second.timestamp = now;
		pp->second.nSent++;
		int copy = ps->fullCopy(p);
		if (copy == 0) {
			cerr << "RouterCore::resendCpReq: no packets left in "
				"packet store\n";
			return;
		}
		if (booting) {
			pktLog->log(copy,0,true,now);
			iop->send(copy,0);
		} else
			forward(copy,ctt->getComtIndex(h.getComtree()));
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
void RouterCore::handleCpReply(int reply, CtlPkt& cpr) {
	PacketHeader hr = ps->getHeader(reply);
	map<uint64_t,CpInfo>::iterator pp = pending->find(cpr.getSeqNum());
	if (pp == pending->end()) {
		// this is a reply to a request we never sent, or
		// possibly, a reply to a request we gave up on
		ps->free(reply);
		return;
	}
	// so, remove it from the map of pending requests
	ps->free(pp->second.p);
	pending->erase(pp);

	// and then handle the reply
	switch (cpr.getCpType()) {
	case CLIENT_CONNECT: case CLIENT_DISCONNECT:
		if (cpr.getRrType() == NEG_REPLY) {
			cerr << "RouterCore::handleCpReply: got negative reply "
				"to a connect or disconnect request: "
				<< cpr.getErrMsg() << endl;
			break;
		}
		// otherwise, nothing to do
		break;
	case BOOT_REQUEST: {
		if (cpr.getRrType() == NEG_REPLY) {
			cerr << "RouterCore::handleCpReply: got "
				"negative reply to a boot request: "
				<< cpr.getErrMsg() << endl;
			break;
		}
		// unpack first and last leaf addresses, initialize leafAdr
		if (!(cpr.isSet(FIRST_LEAF_ADR) && cpr.isSet(LAST_LEAF_ADR))) {
			cerr << "RouterCore::handleCpReply: reply to boot "
				"request did not include leaf address range\n";
			break;
		}
		firstLeafAdr = cpr.getAttr(FIRST_LEAF_ADR);
        	fAdr_t lastLeafAdr = cpr.getAttr(LAST_LEAF_ADR);
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
 *  @param p is the packet number
 *  @param paylen is the length of the payload in bytes
 */
void RouterCore::returnToSender(packet p, int paylen) {
	PacketHeader& h = ps->getHeader(p);
	if (paylen == 0) {
		cerr << "RouterCore::returnToSender: control packet formatting "
			"error, zero payload length\n";
		ps->free(p);
	}
	h.setLength(Forest::OVERHEAD + paylen);
	h.setFlags(0);
	h.setDstAdr(h.getSrcAdr());
	h.setSrcAdr(myAdr);

	ps->pack(p);

	if (booting) {
		pktLog->log(p,0,true,now);
		iop->send(p,0);
		return;
	}

	int cLnk = ctt->getComtLink(h.getComtree(),h.getInLink());
	int qn = ctt->getLinkQ(cLnk);
	if (!qm->enq(p,qn,now)) { ps->free(p); }
}
