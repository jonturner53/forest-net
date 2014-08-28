/** @file Router.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Router.h"
#include "RouterInProc.h"
#include "RouterOutProc.h"

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
	args.portNum = 0; args.runLength = seconds(0);

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
			int runtime;
			sscanf(&argv[i][8],"%d",&runtime);
			args.runLength = seconds(runtime);
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
		Util::fatal("Router:: error processing command line arguments");
	Router router(args);

	router.run();
	return 0;
}

namespace forest {

/** Constructor for Router, initializes key parameters and allocates space.
 *  @param config is a RouterInfo structure which has been initialized to
 *  specify various router parameters
 */
Router::Router(const RouterInfo& config) {
	int nIfaces = 50; int nLnks = 1000;
	int nComts = 5000; int nRts = 100000;
	int nPkts = 100000; int nBufs = 50000;
	int nQus = 10000;

	myAdr = config.myAdr;
	bootIp = config.bootIp;
	nmAdr = config.nmAdr;
	nmIp = config.nmIp;
	ccAdr = config.ccAdr;
	runLength = config.runLength;
	leafAdr = 0;

	try {
		ps = new PacketStore(nPkts, nBufs);
		ift = new IfaceTable(nIfaces);
		lt = new LinkTable(nLnks);
		ctt = new ComtreeTable(nComts,10*nComts);
		rt = new RouteTable(nRts,myAdr,ctt);
		pktLog = new PacketLog(ps);
		qm = new QuManager(nLnks, nPkts, nQus, min(50,5*nPkts/nLnks),
				   ps);
		sock = new int[nIfaces+1];
		maxSockNum = -1;
	
		rip = new RouterInProc(this);
		rop = new RouterOutProc(this);

		setLeafAdrRange(config.firstLeafAdr, config.lastLeafAdr);
	} catch (std::bad_alloc e) {
		Util::fatal("Router: unable to allocate space for Router");
        }

	if (config.mode.compare("local") == 0) {
		if (!readTables(config) || !setup())
			Util::fatal("Router: could not complete local "
					"configuration\n");
	} else {
		booting = true;
	}
	seqNum = 0;
	tZero = high_resolution_clock::now();
}

Router::~Router() {
// consider thread cleanup
	delete rip; delete rop; delete rop;
	delete pktLog; delete qm; 
	delete rt; delete ctt; delete lt; delete ift; delete ps;
	delete leafAdr; delete [] sock;
}

/** Get the next sequence number.  */
uint64_t Router::nextSeqNum() {
	unique_lock<mutex> lck(snLock);
	uint64_t sn = ++seqNum;
	return sn;
}

/** Read router configuration tables from files.
 *  This method reads initial router configuration files (if present)
 *  and configures router tables as specified.
 *  @param config is a RouterInfo structure which has been initialized to
 *  specify various router parameters
 */
bool Router::readTables(const RouterInfo& config) {
	if (config.ifTbl.compare("") != 0) {
		ifstream fs; fs.open(config.ifTbl.c_str());
		if (fs.fail() || !ift->read(fs)) {
			cerr << "Router::readTables: can't read "
			     << "interface table\n";
			return false;
		}
		fs.close();
	}
	if (config.lnkTbl.compare("") != 0) {
		ifstream fs; fs.open(config.lnkTbl.c_str());
		if (fs.fail() || !lt->read(fs)) {
			cerr << "Router::readTables: can't read "
			     << "link table\n";
			return false;
		}
		fs.close();
	}
	if (config.comtTbl.compare("") != 0) {
		ifstream fs; fs.open(config.comtTbl.c_str());
		if (fs.fail() || !ctt->read(fs)) {
			cerr << "Router::readTables: can't read "
			     << "comtree table\n";
			return false;
		}
		fs.close();
	}
	if (config.rteTbl.compare("") != 0) {
		ifstream fs; fs.open(config.rteTbl.c_str());
		if (fs.fail() || !rt->read(fs)) {
			cerr << "Router::readTables: can't read "
			     << "routing table\n";
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
bool Router::setup() {
cerr << "setting up\n";
	dump(cout);
	if (!setupAllIfaces()) return false;
	if (!setupLeafAddresses()) return false;
	if (!setupQueues()) return false;
	if (!checkTables()) return false;
	if (!setAvailRates()) return false;
	addLocalRoutes();

cerr << "done setting up\n";
	return true;
}

/** Setup interfaces specified in the interface table.
 *  This involves opening a separate UDP socket for each interface.
 *  @return true on success, false on failure
 */
bool Router::setupAllIfaces() {
	for (int iface = ift->firstIface(); iface != 0;
		 iface = ift->nextIface(iface)) {
		if (sock[iface] > 0) continue;
		if (!setupIface(iface)) {
			cerr << "Router::setupIfaces: could not "
				"setup interface " << iface << endl;
			return false;
		}
	}
	return true;
}

/** Setup an interface.
 *  Caller is assumed to hold the lock on the IfaceTable object.
 *  @param i is the number of a new interface to be configured
 *  @return true on success, false on failure.
 */
bool Router::setupIface(int i) {
	// create datagram socket
	sock[i] = Np4d::datagramSocket();
	if (sock[i] < 0) {
		cerr << "Router::setup: socket call failed\n";
                return false;
        }
	maxSockNum = max(maxSockNum, sock[i]);

	// bind it to an address and port
	IfaceTable::Entry& ifte = ift->getEntry(i);
        if (!Np4d::bind4d(sock[i], ifte.ipa, ifte.port)) {
		string s;
		cerr << "Router::setup: bind call failed for ("
		     << Np4d::ip2string(ifte.ipa)
		     << ", " << ifte.port << ") check interface's IP "
		     << "address and port\n";
                return false;
        }
	ifte.port = Np4d::getSockPort(sock[i]);

	return true;
}

/** Allocate addresses to peers specified in the initial link table.
 *  Verifies that the initial peer addresses are in the range of
 *  assignable leaf addresses, and allocates them if they are.
 *  @return true on success, false on failure
 */
bool Router::setupLeafAddresses() {
	for (int lnk = lt->firstLink(); lnk != 0; lnk = lt->nextLink(lnk)) {
		LinkTable::Entry& lte = lt->getEntry(lnk);
		if (lte.peerType == Forest::ROUTER) continue;
		if (!allocLeafAdr(lte.peerAdr))
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
bool Router::setupQueues() {
	// Set link rates in QuManager
	for (int lnk = lt->firstLink(); lnk != 0; lnk = lt->nextLink(lnk)) {
		LinkTable::Entry& lte = lt->getEntry(lnk);
		qm->setLinkRates(lnk,lte.rates);
	}
	RateSpec rs(Forest::MINBITRATE,Forest::MINBITRATE,
		    Forest::MINPKTRATE,Forest::MINPKTRATE);
	int ctx;
        for (ctx = ctt->firstComt(); ctx != 0; ctx = ctt->nextComt(ctx)) {
		for (int cLnk = ctt->firstComtLink(ctx); cLnk != 0;
			 cLnk = ctt->nextComtLink(ctx,cLnk)) {
                        int lnk = ctt->getLink(ctx,cLnk);
			int qid = qm->allocQ(lnk);
			if (qid == 0) return false;
			ctt->setLinkQ(ctx,cLnk,qid);
			qm->setQRates(qid,rs);
			if (lt->getEntry(lnk).peerType == Forest::ROUTER) {
				qm->setQLimits(qid,100,200000);
			} else {
				qm->setQLimits(qid,50,100000);
			}
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
bool Router::checkTables() {
	bool success = true;

	// verify that the default interface is valid and
	// that each interface has a non-zero IP address
	if (!ift->valid(ift->getDefaultIface())) {
		cerr << "Router::checkTables: specified default iface "
		     << ift->getDefaultIface() << " is invalid\n";
		success = false;
	}
	int iface;
	for (iface = ift->firstIface(); iface != 0;
	     iface = ift->nextIface(iface)) {
		if (ift->getEntry(iface).ipa == 0) {
			cerr << "Router::checkTables: interface "
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
		LinkTable::Entry& lte = lt->getEntry(lnk);
		if (!ift->valid(lte.iface)) {
			cerr << "Router::checkTables: interface " << iface
			     << " for link " << lnk << " is not valid\n";
			success = false;
		}
		if (lte.peerIp == 0 && lte.peerType == Forest::ROUTER) {
			cerr << "Router::checkTables: invalid peer IP "
			     << "for link " << lnk << endl;
			success = false;
		}
		if (!Forest::validUcastAdr(lte.peerAdr)) {
			cerr << "Router::checkTables: invalid peer address "
			     << "for link " << lnk << endl;
			success = false;
		}
	}

	// verify that the links in each comtree are valid,
	// that the router links and core links refer to peer routers,
	// and that each comtree link is consistent
	int ctx;
	for (ctx = ctt->firstComt(); ctx != 0;
	     ctx = ctt->nextComt(ctx)) {
		int comt = ctt->getComtree(ctx);
		int plnk = ctt->getPlink(ctx);
		int pcLnk = ctt->getPClnk(ctx);
		if (plnk != ctt->getLink(ctx,pcLnk)) {
			cerr << "Router::checkTables: parent link "
			     <<  plnk << " not consistent with pcLnk\n";
			success = false;
		}
		if (ctt->inCore(ctx) && plnk != 0 &&
		    !ctt->isCoreLink(ctx,pcLnk)) {
			cerr << "Router::checkTables: parent link "
			     <<  plnk << " of core node does not lead to "
			     << "another core node\n";
			success = false;
		}
		for (int cLnk = ctt->firstComtLink(ctx); cLnk != 0;
			 cLnk = ctt->nextComtLink(ctx,cLnk)) {
			int lnk = ctt->getLink(ctx,cLnk);
			if (!lt->valid(lnk)) {
				cerr << "Router::checkTables: link "
				     << lnk << " in comtree " << comt
				     << " not in link table" << endl;
				success = false;
				continue;
			}
			fAdr_t dest = ctt->getDest(ctx,cLnk);
			if (dest != 0 && !Forest::validUcastAdr(dest)) {
				cerr << "Router::checkTables: dest addr "
				     << "for " << lnk << " in comtree " << comt
				     << " is not valid" << endl;
				success = false;
			}
			int qid = ctt->getLinkQ(ctx,cLnk);
			if (qid == 0) {
				cerr << "Router::checkTables: queue id "
				     << "for " << lnk << " in comtree " << comt
				     << " is zero" << endl;
				success = false;
			}
		}
		if (!success) break;
		for (int cLnk = ctt->firstRtrLink(ctx); cLnk != 0;
			 cLnk = ctt->nextRtrLink(ctx,cLnk)) {
			int lnk = ctt->getLink(ctx,cLnk);
			if (!ctt->isLink(ctx,lnk)) {
				cerr << "Router::checkTables: router link "
				     << lnk << " is not valid in comtree "
				     << comt << endl;
				success = false;
			}
			if (lt->getEntry(lnk).peerType != Forest::ROUTER) {
				cerr << "Router::checkTables: router link "
				     << lnk << " in comtree " << comt 
				     << " connects to non-router peer\n";
				success = false;
			}
		}
		for (int cLnk = ctt->firstCoreLink(ctx); cLnk != 0;
			 cLnk = ctt->nextCoreLink(ctx,cLnk)) {
			int lnk = ctt->getLink(ctx,cLnk);
			if (!ctt->isRtrLink(ctx,lnk)) {
				cerr << "Router::checkTables: core link "
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
bool Router::setAvailRates() {
	bool success = true;
	RateSpec minRates(Forest::MINBITRATE,Forest::MINBITRATE,
			  Forest::MINPKTRATE,Forest::MINPKTRATE);
	RateSpec maxRates(Forest::MAXBITRATE,Forest::MAXBITRATE,
			  Forest::MAXPKTRATE,Forest::MAXPKTRATE);
	int iface;
	for (iface = ift->firstIface(); iface != 0;
	     iface = ift->nextIface(iface)) {
		IfaceTable::Entry& e = ift->getEntry(iface);
		if (!minRates.leq(e.rates) || !e.rates.leq(maxRates)) {
			cerr << "Router::setAvailRates: interface rates "
				"outside allowed range\n";
			success = false;
		}
		e.availRates = e.rates;
	}
	if (!success) return false;
	int lnk;
	for (lnk = lt->firstLink(); lnk != 0; lnk = lt->nextLink(lnk)) {
		LinkTable::Entry& lte = lt->getEntry(lnk);
		if (!minRates.leq(lte.rates) || !lte.rates.leq(maxRates)) {
			cerr << "Router::setAvailRates: link rates "
				"outside allowed range\n";
			success = false;
		}
		iface = lte.iface;
		IfaceTable::Entry& ifte = ift->getEntry(iface);
		if (!lte.rates.leq(ifte.availRates)) {
			cerr << "Router::setAvailRates: oversubscribing "
				"interface " << iface << endl;
			success = false;
		}
		ifte.availRates.subtract(lte.rates);
		lte.availRates = lte.rates;
		lte.availRates.scale(.9); // allocate at most 90% of link
	}
	if (!success) return false;
	int ctx;
	for (ctx = ctt->firstComt(); ctx != 0;
	     ctx = ctt->nextComt(ctx)) {
		for (int cLnk = ctt->firstComtLink(ctx); cLnk != 0;
		         cLnk = ctt->nextComtLink(ctx,cLnk)) {
			int lnk = ctt->getLink(ctx,cLnk);
			LinkTable::Entry& lte = lt->getEntry(lnk);
			RateSpec comtRates;
			comtRates = ctt->getRates(ctx, cLnk);
			if (!comtRates.leq(lte.availRates)) {
				cerr << "Router::setAvailRates: "
					"oversubscribing link "
				     << lnk << endl;
				success = false;
			}
			lte.availRates.subtract(comtRates);
		}
	}
	return success;
}

/** Add routes to neighboring leaf nodes and to routers in foreign zip codes.
 *  Routes are added in all comtrees. 
 */
void Router::addLocalRoutes() {
	for (int ctx = ctt->firstComt(); ctx != 0; ctx = ctt->nextComt(ctx)) {
		int comt = ctt->getComtree(ctx);
		for (int cLnk = ctt->firstComtLink(ctx); cLnk != 0;
		         cLnk = ctt->nextComtLink(ctx,cLnk)) {
			int lnk = ctt->getLink(ctx,cLnk);
			LinkTable::Entry& lte = lt->getEntry(lnk);
			if (lte.peerType == Forest::ROUTER &&
			    Forest::zipCode(lte.peerAdr)
			    == Forest::zipCode(myAdr))
				continue;
			if (rt->getRtx(comt,lte.peerAdr) != 0)
				continue;
			rt->addRoute(comt,lte.peerAdr,cLnk);
		}
	}
}

/** Write the contents of all router tables to an output stream.
 *  @param out is an open output stream
 */
void Router::dump(ostream& out) {
	out << "Interface Table\n\n" << ift->toString() << endl;
	out << "Link Table\n\n" << lt->toString() << endl;
	out << "Comtree Table\n\n" << ctt->toString() << endl;
	out << "Routing Table\n\n" << rt->toString() << endl;
}

void Router::run() {
	// start input and output threads
cerr << "launching inProc, outProc\n";
	thread inThred(RouterInProc::start,rip);
	thread outThred(RouterOutProc::start,rop);

	// wait for them to finish
cerr << "waiting for  inProc, outProc\n";
	inThred.join();
	outThred.join();
cerr << "and done\n";

	cout << endl;
	dump(cout); 		// print final tables
	cout << endl;
}

} // ends namespace
