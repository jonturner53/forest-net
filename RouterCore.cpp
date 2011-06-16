/** @file RouterCore.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RouterCore.h"

/** usage:
 * 	fRouter fAdr ifTbl lnkTbl comtTbl rteTbl stats finTime [ numData ]
 * 
 *  FRouter runs a forest router. It waits for packets to arrive on the
 *  standard forest port and forwards them appropriately.
 *  FAdr is the forest address of the router. IfTbl, LnkTbl, comtTbl and rteTbl
 *  are the names of files that contain the initial contents of the
 *  interface, link, comtree and route tables.
 *  FinTime is the number of seconds the router should run.
 *  If zero, it runs forever.
 *  If the optional numData argument is present and non-zero,
 *  at most numData data packets are copied to the log file.
 *  NumData defaults to zero.
 */
main(int argc, char *argv[]) {
	int finTime;
	int ip0, ip1, ip2, ip3;
	ipa_t ipadr; fAdr_t fAdr;
	int numData = 0;
	if (argc < 8 || argc > 9 ||
	    (fAdr = Forest::forestAdr(argv[1])) == 0 ||
	    sscanf(argv[7],"%d", &finTime) != 1 ||
	    (argc == 9 && sscanf(argv[8],"%d",&numData) != 1)) {
		fatal("usage: fRouter fAdr ifTbl lnkTbl comtTbl "
		      "rteTbl stats finTime");
	}

	RouterCore router(fAdr);
	if (!router.init(argv[2],argv[3],argv[4],argv[5],argv[6])) {
		fatal("router: fRouter::init() failed");
	}
	router.dump(cout); 		// print tables
	router.run(1000000*finTime,numData);
	cout << endl;
	router.dump(cout); 		// print final tables
	cout << endl;
}


RouterCore::RouterCore(fAdr_t myAdr1) : myAdr(myAdr1) {
	nLnks = 31; nComts = 5000; nRts = 10000;
	nPkts = 200000; nBufs = 100000; nQus = 4000;

	lt = new LinkTable(nLnks);
	ps = new PacketStore(nPkts, nBufs);
	qm = new QuManager(nLnks+1, nPkts, nQus, nBufs-4*nLnks, ps, lt);
	ctt = new ComtreeTable(nComts,myAdr,lt,qm);
	rt = new RouteTable(nRts,myAdr,lt,ctt,qm);
	iop = new IoProcessor(lt, ps);
	sm = new StatsModule(100,lt,qm);
}

RouterCore::~RouterCore() {
	delete lt; delete ctt; delete rt; delete ps;
	delete qm; delete iop; delete sm;
}


/** Initialize ioProc, link table, comt table and routing table from
 *  files with names iftf, ltf, cttf and rtf. Perform consistency checks.
 *  Return true on success, false on failure.
 */
bool RouterCore::init(char iftf[], char ltf[], char cttf[],
		      char rtf[], char smf[]) {
        ifstream iftfs, ltfs, cttfs, rtfs, smfs;

        iftfs.open(iftf);
	if (iftfs.fail() || !iop->read(iftfs)) {
		cerr << "RouterCore::init: can't read interface table\n";
		return false;
	}
	iftfs.close();

        ltfs.open(ltf);
        if (ltfs.fail() || !lt->read(ltfs)) {
		cerr << "RouterCore::init: can't read link table\n";
		return false;
	}
        ltfs.close();

        cttfs.open(cttf);
        if (cttfs.fail() || !ctt->readTable(cttfs)) {
		cerr << "RouterCore::init: can't read comt table\n";
		return false;
	}
        cttfs.close();

        rtfs.open(rtf);
        if (rtfs.fail() || !rt->read(rtfs)) {
		cerr << "RouterCore::init: can't read routing table\n";
		return false;
	}
        rtfs.close();

        smfs.open(smf);
        if (smfs.fail() || !sm->read(smfs)) {
		cerr << "RouterCore::init: can't read statistics spec\n";
		return false;
	}
        smfs.close();

	addLocalRoutes();
	return true;
}

void RouterCore::addLocalRoutes() {
// Add routes for all directly attached hosts for all comtrees.
// Also add routes to adjacent routers in different zip codes.
	int ctte, i, j, n;
	uint16_t lnkvec[nLnks+1];
	for (ctte = 1; ctte <= nComts; ctte++) {
		if (!ctt->valid(ctte)) continue;
		int comt = ctt->getComtree(ctte);
		n = ctt->getLinks(ctte,lnkvec,nLnks);
		for (i = 0; i < n; i++) {
			j = lnkvec[i];
			fAdr_t peerAdr = lt->getPeerAdr(j);
			if (lt->getPeerType(j) == ROUTER &&
			    Forest::zipCode(peerAdr)
			    == Forest::zipCode(myAdr))
				continue;
			if (rt->lookup(comt,peerAdr) != 0)
				continue;
			rt->addEntry(comt,peerAdr,j,0);
		}
	}
}

void RouterCore::dump(ostream& out) {
	out << "Interface Table\n\n"; iop->write(out); out << endl;
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
 *  @param numData is the maximum number of CLIENT_DATA packets to
 *  retain in the log.
 */
void RouterCore::run(uint32_t finishTime, int numData) {
	// record of first packet receptions, transmissions for debugging
	const int MAXEVENTS = 500;
	struct { int sendFlag; uint32_t time; int link, pkt;} events[MAXEVENTS];
	int evCnt = 0;
	int statsTime = 0;		// time statistics were last processed
	bool didNothing;
	int controlCount = 20;		// used to limit overhead of control
					// packet processing
	queue<int> ctlQ;		// queue for control packets

	now = Misc::getTime();
	while (finishTime == 0 || now < finishTime) {
		didNothing = true;

		// input processing
		int p = iop->receive();
		if (p != 0) {
			didNothing = false;
			PacketHeader& h = ps->getHeader(p);
			int ptype = h.getPtype();

			if (evCnt < MAXEVENTS &&
			    (ptype != CLIENT_DATA || numData > 0)) {
				int p1 = (ptype == CLIENT_DATA ? 
						ps->clone(p) : ps->fullCopy(p));
				events[evCnt].sendFlag = 0;
				events[evCnt].link = h.getInLink();
				events[evCnt].time = now;
				events[evCnt].pkt = p1;
				evCnt++;
				if (ptype == CLIENT_DATA) numData--;
			}
			int ctte = ctt->lookup(h.getComtree());
			if (!pktCheck(p,ctte)) {
				ps->free(p);
			} else if (ptype == CLIENT_DATA) {
				forward(p,ctte);
			} else if (ptype == SUB_UNSUB) {
				subUnsub(p,ctte);
			} else if (ptype == RTE_REPLY) {
				handleRteReply(p,ctte);
			} else {
				ctlQ.push(p);
			}
		}

		// output processing
		int lnk;
		while ((lnk = qm->nextReady(now)) != 0) {
			didNothing = false;
			p = qm->deq(lnk); PacketHeader& h = ps->getHeader(p);
			if (evCnt < MAXEVENTS &&
			    (h.getPtype() != CLIENT_DATA || numData > 0)) {
				int p2 = (h.getPtype() == CLIENT_DATA ? 
						ps->clone(p) : ps->fullCopy(p));
				events[evCnt].sendFlag = 1;
				events[evCnt].link = lnk;
				events[evCnt].time = now;
				events[evCnt].pkt = p2;
				evCnt++;
				if (h.getPtype() == CLIENT_DATA) numData--;
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
		if (now - statsTime > 300000) {
			sm->record(now);
			statsTime = now;
			didNothing = false;
		}

		// If did nothing on that pass, sleep for a millisecond.
		if (didNothing) {
			struct timespec sleeptime, rem;
			sleeptime.tv_sec = 0; sleeptime.tv_nsec = 1000000;
			nanosleep(&sleeptime,&rem);
		}

		// update current time
		now = Misc::getTime();
	}

	// write out recorded events
	for (int i = 0; i < evCnt; i++) {
		if (events[i].sendFlag) 
			cout << "send link " << setw(2) << events[i].link
			     << " at " << setw(8) << events[i].time << " ";
		else
			cout << "recv link " << setw(2) << events[i].link
			     << " at " << setw(8) << events[i].time << " ";
		int p = events[i].pkt;
		(ps->getHeader(p)).write(cout,ps->getBuffer(p));
	}
	cout << endl;
	cout << lt->iPktCnt(0) << " packets received, "
	     <<	lt->oPktCnt(0) << " packets sent\n";
	cout << lt->iPktCnt(-1) << " from routers,    "
	     << lt->oPktCnt(-1) << " to routers\n";
	cout << lt->iPktCnt(-2) << " from clients,    "
	     << lt->oPktCnt(-2) << " to clients\n";
}

bool RouterCore::pktCheck(int p, int ctte) {
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
	// check for invalid comtrees
	if (!ctt->valid(ctte) || !ctt->isLink(ctte,inL)) return false;
	return true;
}
void RouterCore::forward(int p, int ctte) {
// Lookup routing entry and forward packet accordingly.
// P is assumed to be a CLIENT_DATA packet.
	PacketHeader& h = ps->getHeader(p);
	int rte = rt->lookup(h.getComtree(),h.getDstAdr());

	if (rte != 0) { // valid route case
		// reply to route request
		if ((h.getFlags() & Forest::RTE_REQ)) {
			sendRteReply(p,ctte);
			h.setFlags(h.getFlags() & (~Forest::RTE_REQ));
			ps->pack(p);
			ps->hdrErrUpdate(p);
		}
		if (Forest::validUcastAdr(h.getDstAdr())) {
			int qn = rt->getQnum(rte);
			if (qn == 0) qn = ctt->getQnum(ctte);
			int lnk = rt->getLink(rte);
			if (lnk != h.getInLink() && qm->enq(p,lnk,qn,now))
				return;
			ps->free(p); return;
		}
		// multicast data packet
		multiSend(p,ctte,rte);
		return;
	}
	// no valid route
	if (Forest::validUcastAdr(h.getDstAdr())) {
		// send to neighboring routers in comtree
		h.setFlags(Forest::RTE_REQ);
		ps->pack(p); ps->hdrErrUpdate(p);
	}
	multiSend(p,ctte,rte);
	return;
}
void RouterCore::multiSend(int p, int ctte, int rte) {
// Send multiple copies of a packet.
// Ctte and rte are the comtree and routing table entries
// respectively. Ctte is assumed to be defined, rte may not be.
	int qn, n;
	uint16_t lnkvec[2*nLnks];
	PacketHeader& h = ps->getHeader(p);

	if (Forest::validUcastAdr(h.getDstAdr())) { // flooding a unicast packet
		qn = ctt->getQnum(ctte);
		if (Forest::zipCode(myAdr) == Forest::zipCode(h.getDstAdr()))
			n = ctt->getLlinks(ctte,lnkvec,nLnks);
		else
			n = ctt->getRlinks(ctte,lnkvec,nLnks);
	} else { // forwarding a multicast packet
		n = 0;
		qn = ctt->getQnum(ctte);
		if (rte != 0) {
			if (rt->getQnum(rte) != 0) qn = rt->getQnum(rte);
			n = rt->getLinks(rte,lnkvec,nLnks);
		}
		n += ctt->getClinks(ctte,&lnkvec[n],nLnks);
		if (ctt->getPlink(ctte) != 0 &&
		    !ctt->isClink(ctte,ctt->getPlink(ctte)))
			lnkvec[n++] = ctt->getPlink(ctte);
	}

	if (n == 0) { ps->free(p); return; }

	int lnk; int inlnk = h.getInLink();
	int p1 = p;
	for (int i = 0; i < n-1; i++) { // process first n-1 copies
		lnk = lnkvec[i];
		if (lnk == inlnk) continue;
		if (qm->enq(p1,lnk,qn,now)) {
			p1 = ps->clone(p);
		}
	}
	// process last copy
	lnk = lnkvec[n-1];
	if (lnk != inlnk) {
		if (qm->enq(p1,lnk,qn,now)) return;
	}
	ps->free(p1);
	return;
}

void RouterCore::sendRteReply(int p, int ctte) {
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
	qm->enq(p1,h.getInLink(),ctt->getQnum(ctte),now);
}

void RouterCore::handleRteReply(int p, int ctte) {
// Handle route reply packet p.
	PacketHeader& h = ps->getHeader(p);
	int rte = rt->lookup(h.getComtree(), h.getDstAdr());
	if ((h.getFlags() & Forest::RTE_REQ) && rte != 0) sendRteReply(p,ctte);
	int adr = ntohl(ps->getPayload(p)[0]);
	if (Forest::validUcastAdr(adr) &&
	    rt->lookup(h.getComtree(),adr) == 0) {
		rt->addEntry(h.getComtree(),adr,h.getInLink(),0); 
	}
	if (rte == 0) {
		// send to neighboring routers in comtree
		h.setFlags(Forest::RTE_REQ);
		ps->pack(p); ps->hdrErrUpdate(p);
		multiSend(p,ctte,rte);
		return;
	}
	if (rte != 0 && lt->getPeerType(rt->getLink(rte)) == ROUTER &&
	    qm->enq(p,rt->getLink(rte),ctt->getQnum(ctte),now))
		return;
}

// Perform subscription processing on p.
// Ctte is the comtree table entry for the packet.
// First payload word contains add/drop counts;
// these must sum to at most 350 and must match packet length
void RouterCore::subUnsub(int p, int ctte) {
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);
	// add/remove branches from routes
	// if non-core node, also propagate requests upward as
	// appropriate
	int inlnk = h.getInLink();
	// ignore subscriptions from the parent or core neighbors
	if (inlnk == ctt->getPlink(ctte) || ctt->isClink(ctte,inlnk)) {
		ps->free(p); return;
	}
	int comt = h.getComtree();
	bool propagate = false;
	int rte; fAdr_t addr;

	// add subscriptions
	int addcnt = ntohl(pp[0]);
	if (addcnt < 0 || addcnt > 350 || (addcnt + 8)*4 > h.getLength()) {
		ps->free(p); return;
	}
	for (int i = 1; i <= addcnt; i++) {
		addr = ntohl(pp[i]);
		if (!Forest::mcastAdr(addr)) continue;  // ignore unicast
		rte = rt->lookup(comt,addr);
		if (rte == 0) { 
			rte = rt->addEntry(comt,addr,inlnk,0);
			propagate = true;
		} else if (!rt->isLink(rte,inlnk)) {
			rt->addLink(rte,inlnk);
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
		rte = rt->lookup(comt,addr);
		if (rte == 0) continue;
		rt->removeLink(rte,inlnk);
		if (rt->noLinks(rte)) {
			rt->removeEntry(rte);
			propagate = true;
		} else {
			pp[i] = 0;
		}
	}
	// propagate subscription packet to parent if not a core node
	if (propagate && !ctt->getCoreFlag(ctte) && ctt->getPlink(ctte) != 0) {
		ps->payErrUpdate(p);
		if (qm->enq(p,ctt->getPlink(ctte),ctt->getQnum(ctte),now)) {
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
	int ctte, rte;
	PacketHeader& h = ps->getHeader(p);
	int inL = h.getInLink();
	buffer_t& b = ps->getBuffer(p);
	CtlPkt cp;

	// first handle special cases of CONNECT/DISCONNECT
	if (h.getPtype() == CONNECT) {
		if (lt->getPeerPort(inL) == 0)
			lt->setPeerPort(inL,h.getTunSrcPort());
		ps->free(p); return;
	}
	if (h.getPtype() == DISCONNECT) {
		if (lt->getPeerPort(inL) == h.getTunSrcPort())
			lt->setPeerPort(inL,0);
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
		if (iop->addEntry(cp.getAttr(IFACE_NUM),
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
        case ADD_LINK:
		if (lt->addEntry(cp.getAttr(LINK_NUM), cp.getAttr(IFACE_NUM),
		    (ntyp_t) cp.getAttr(PEER_TYPE), cp.getAttr(PEER_IP),
		    cp.getAttr(PEER_ADR)))
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		else errReply(p,cp1,"add link: cannot add link");
		break;
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
		if (ctt->addEntry(cp.getAttr(COMTREE_NUM)) != 0)
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		else errReply(p,cp1,"add comtree: cannot add comtree");
		break;
        case DROP_COMTREE:
		ctte = ctt->lookup(cp.getAttr(COMTREE_NUM));
		if (ctte != 0 && ctt->removeEntry(ctte) != 0)
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		else
			errReply(p,cp1,"drop comtree: cannot drop comtree");
		break;
        case GET_COMTREE: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		ctte = ctt->lookup(comt);
		if (ctte == 0) {
			errReply(p,cp1,"get comtree: invalid comtree");
		} else {
			cp1.setAttr(COMTREE_NUM,comt);
			cp1.setAttr(CORE_FLAG,ctt->getCoreFlag(ctte));
			cp1.setAttr(PARENT_LINK,ctt->getPlink(ctte));
			cp1.setAttr(QUEUE_NUM,ctt->getQnum(ctte));
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		}
		break;
	}
        case MOD_COMTREE: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		ctte = ctt->lookup(comt);
		if (ctte != 0) {
			if (cp.isSet(CORE_FLAG) != 0)
				ctt->setCoreFlag(ctte,cp.getAttr(CORE_FLAG));
			if (cp.isSet(PARENT_LINK) != 0)
				ctt->setPlink(ctte,cp.getAttr(PARENT_LINK));
			if (cp.isSet(QUEUE_NUM) != 0)
				ctt->setQnum(ctte,cp.getAttr(QUEUE_NUM));
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"modify comtree: invalid comtree");
		break;
	}

        case ADD_ROUTE:
		if (rt->addEntry(cp.getAttr(COMTREE_NUM),
				 cp.getAttr(DEST_ADR),
				 cp.getAttr(LINK_NUM),
				 cp.getAttr(QUEUE_NUM)) != 0) {
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"add route: cannot add route");
		break;
        case DROP_ROUTE:
		rte = rt->lookup(cp.getAttr(COMTREE_NUM),cp.getAttr(DEST_ADR));
		if (rte != 0) {
			rt->removeEntry(rte);
			returnToSender(p,cp1.pack(ps->getPayload(p)));
		} else errReply(p,cp1,"drop route: invalid route");
		break;
        case GET_ROUTE: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		fAdr_t da = cp.getAttr(DEST_ADR);
		rte = rt->lookup(comt,da);
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
	h.setLength(Forest::HDR_LENG + paylen + 4);

	fAdr_t temp = h.getDstAdr();
	h.setDstAdr(h.getSrcAdr());
	h.setSrcAdr(temp);

	ps->pack(p);

	int qn = ctt->getQnum(ctt->lookup(h.getComtree()));
	if (!qm->enq(p,h.getInLink(),qn,now)) { ps->free(p); }
}

/** Send an error reply to a control packet.
 *  Reply packet re-uses the request packet p and its buffer.
 *  The string *s is included in the reply packet.
 */
void RouterCore::errReply(packet p, CtlPkt& cp, const char *s) {
	cp.setRrType(NEG_REPLY); cp.setErrMsg(s);
	returnToSender(p,4*cp.pack(ps->getPayload(p)));
}

