// usage:
//	fRouter fAdr ifTbl lnkTbl comtTbl rteTbl stats finTime [ numData ]
//
// FRouter runs a forest router. It waits for packets to arrive on the
// standard forest port and forwards them appropriately.
// FAdr is the forest address of the router. IfTbl, LnkTbl, comtTbl and rteTbl
// are the names of files that contain the initial contents of the
// interface, link, comtree and route tables.
// FinTime is the number of seconds the router should run.
// If zero, it runs forever.
// If the optional numData argument is present and non-zero,
// at most numData data packets are copied to the log file.
// NumData defaults to zero.

#include "fRouter.h"

main(int argc, char *argv[]) {
	int finTime;
	int ip0, ip1, ip2, ip3;
	ipa_t ipadr; fAdr_t fAdr, fa1, fa2;
	int numData = 0;
	if (argc < 8 || argc > 9 ||
	    sscanf(argv[1],"%d.%d", &fa1, &fa2) != 2 ||
	    sscanf(argv[7],"%d", &finTime) != 1 ||
	    (argc == 9 && sscanf(argv[8],"%d",&numData) != 1)) {
		fatal("usage: fRouter fAdr ifTbl lnkTbl comtTbl, rteTbl stats finTime");
	}

	fAdr = (fa1 << 16) | (fa2 & 0xffff);
	fRouter router(fAdr);
	if (!router.init(argv[2],argv[3],argv[4],argv[5],argv[6])) {
		fatal("router: fRouter::init() failed");
	}
	router.dump(cout); 		// print tables
	router.run(1000000*finTime,numData);
	cout << endl;
	router.dump(cout); 		// print final tables
	cout << endl;
}

fRouter::fRouter(fAdr_t myAdr1) : myAdr(myAdr1) {

	nLnks = 31; nComts = 10000; nRts = 100000;
	nPkts = 500000; nBufs = 200000; nQus = 4000;

	lt = new lnkTbl(nLnks);
	ps = new pktStore(nPkts, nBufs);
	qm = new qMgr(nLnks+1, nPkts, nQus, nBufs-4*nLnks, ps, lt);
	ctt = new comtTbl(nComts,myAdr,lt,qm);
	rt = new rteTbl(nRts,myAdr,lt,ctt,qm);
	iop = new ioProc(lt, ps);
	sm = new statsMod(100,lt,qm);
}

fRouter::~fRouter() {
	delete lt; delete ctt; delete rt; delete ps; delete qm; delete iop;
	delete sm;
}


bool fRouter::init(char iftf[],char ltf[],char cttf[],char rtf[],char smf[]) {
// Initialize ioProc, link table, comt table and routing table from
// files with names iftf, ltf, cttf and rtf. Perform consistency checks.
// Return true on success, false on failure.
        ifstream iftfs, ltfs, cttfs, rtfs, smfs;

        iftfs.open(iftf);
	if (iftfs.fail() || !(iftfs >> *iop)) {
		cerr << "fRouter::init: can't read interface table\n";
		return false;
	}
	iftfs.close();

        ltfs.open(ltf);
        if (ltfs.fail() || !(ltfs >> *lt)) {
		cerr << "fRouter::init: can't read link table\n";
		return false;
	}
        ltfs.close();

        cttfs.open(cttf);
        if (cttfs.fail() || !(cttfs >> *ctt)) {
		cerr << "fRouter::init: can't read comt table\n";
		return false;
	}
        cttfs.close();

        rtfs.open(rtf);
        if (rtfs.fail() || !(rtfs >> *rt)) {
		cerr << "fRouter::init: can't read routing table\n";
		return false;
	}
        rtfs.close();

        smfs.open(smf);
        if (smfs.fail() || !(smfs >> *sm)) {
		cerr << "fRouter::init: can't read statistics specification\n";
		return false;
	}
        smfs.close();

	addLocalRoutes();
	return true;
}

void fRouter::addLocalRoutes() {
// Add routes for all directly attached hosts for all comtrees.
// Also add routes to adjacent routers in different zip codes.
	int ctte, i, j, n;
	uint16_t lnkvec[nLnks+1];
	for (ctte = 1; ctte <= nComts; ctte++) {
		if (!ctt->valid(ctte)) continue;
		int comt = ctt->comtree(ctte);
		n = ctt->links(ctte,lnkvec,nLnks);
		for (i = 0; i < n; i++) {
			j = lnkvec[i];
			if (lt->peerTyp(j) == ROUTER &&
			    zipCode(lt->peerAdr(j)) == zipCode(myAdr))
				continue;
			if (rt->lookup(comt,lt->peerAdr(j)) != Null)
				continue;
			rt->addEntry(comt,lt->peerAdr(j),j,0);
		}
	}
}

void fRouter::dump(ostream& os) {
	os << "Interface Table\n\n" << *iop << endl;
	os << "Link Table\n\n" << *lt << endl;
	os << "Comtree Table\n\n" << *ctt << endl;
	os << "Routing Table\n\n" << *rt << endl;
	os << "Statistics\n\n" << *sm << endl;
}

bool fRouter::pktCheck(int p, int ctte) {
// Perform error checks on forest packet.
// Return true if all checks pass, else false.
	// Check version, length and type
	char *bp1 = (char *) ps->buffer(p);
	int inL = ps->inLink(p);
	if ((bp1[0] >> 4) != FOREST_VERSION ||
	    ps->leng(p) != ps->ioBytes(p) || ps->leng(p) < 20 ||
	    (lt->peerTyp(inL) < TRUSTED && ucastAdr(ps->dstAdr(p)) &&
	     (ps->ptyp(p) != USERDATA && ps->ptyp(p) != SUB_UNSUB))) {
		return false;
	}
	// Check for spoofed addresses and restricted destinations
	if (inL == Null ||
	    (lt->peerTyp(inL) < TRUSTED && lt->peerAdr(inL) != ps->srcAdr(p)) ||
	    (lt->peerTyp(inL) == CLIENT && lt->peerDest(inL) != 0 &&
	     lt->peerDest(inL) != ps->dstAdr(p))) {
		return false;
	}
	// Check that comtree is valid at this router
	if (!ctt->valid(ctte) || !ctt->inComt(ctte,ps->inLink(p)))
		return false;
	return true;
}

int fRouter::subUnsub(int p, int ctte) {
// Perform subscription processing on p.
// Return 1 if  p is discarded, else 0.
// Ctte is the comtree table entry for the packet.
// First payload word contains add/drop counts;
// these must sum to at most 350 and must match packet length

	uint32_t *pp = ps->payload(p);
	// add/remove branches from routes
	// if non-core node, also propagate requests upward as
	// appropriate
	int inlnk = ps->inLink(p);
	// ignore subscriptions from the parent or core neighbors
	if (inlnk == ctt->plink(ctte) || ctt->isClink(ctte,inlnk)) {
		ps->free(p); return 1;
	}
	int comt = ps->comtree(p);
	bool propagate = false;
	int rte; fAdr_t addr;

	// add subscriptions
	int addcnt = ntohl(pp[0]);
	if (addcnt < 0 || addcnt > 350 || (addcnt + 8)*4 > ps->leng(p)) {
		ps->free(p); return 1;
	}
	for (int i = 1; i <= addcnt; i++) {
		addr = ntohl(pp[i]);
		if (ucastAdr(addr)) { cout << "z1\n"; continue; }// ignore unicast
		rte = rt->lookup(comt,addr);
		if (rte == Null) { 
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
	    (addcnt + dropcnt + 8)*4 > ps->leng(p) ) {
		ps->free(p); return 1;
	}
	for (int i = addcnt + 2; i <= addcnt + dropcnt + 1; i++) {
		addr = ntohl(pp[i]);
		if (ucastAdr(addr)) continue; // ignore unicast
		rte = rt->lookup(comt,addr);
		if (rte == Null) continue;
		rt->removeLink(rte,inlnk);
		if (rt->noLinks(rte)) {
			rt->removeEntry(rte);
			propagate = true;
		} else {
			pp[i] = 0;
		}
	}
	// propagate subscription packet to parent if not a core node
	if (propagate && !ctt->inCore(ctte) && ctt->plink(ctte) != Null) {
		ps->payErrUpdate(p);
		if (qm->enq(p,ctt->plink(ctte),ctt->qnum(ctte),now)) {
			return 0;
		}
	}
	ps->free(p); return 1;
}

int fRouter::multiSend(int p, int ctte, int rte) {
// Send multiple copies of a packet.
// Ctte and rte are the comtree and routing table entries
// respectively. Ctte is assumed to be defined, rte may not be.
	int qn, n;
	uint16_t lnkvec[2*nLnks];

	if (ucastAdr(ps->dstAdr(p))) { // flooding a unicast packet
		qn = ctt->qnum(ctte);
		if (zipCode(myAdr) == zipCode(ps->dstAdr(p)))
			n = ctt->llinks(ctte,lnkvec,nLnks);
		else
			n = ctt->rlinks(ctte,lnkvec,nLnks);
	} else { // forwarding a multicast packet
		n = 0;
		qn = ctt->qnum(ctte);
		if (rte != Null) {
			if (rt->qnum(rte) != 0) qn = rt->qnum(rte);
			n = rt->links(rte,lnkvec,nLnks);
		}
		n += ctt->clinks(ctte,&lnkvec[n],nLnks);
		if (ctt->plink(ctte) != Null &&
		    !ctt->isClink(ctte,ctt->plink(ctte)))
			lnkvec[n++] = ctt->plink(ctte);
	}

	if (n == 0) { ps->free(p); return 1; }

	int lnk; int inlnk = ps->inLink(p);
	int dc = 0; int p1 = p;
	for (int i = 0; i < n-1; i++) { // process first n-1 copies
		lnk = lnkvec[i];
		if (lnk == inlnk) continue;
		if (qm->enq(p1,lnk,qn,now)) p1 = ps->clone(p);
		else dc++;
	}
	// process last copy
	lnk = lnkvec[n-1];
	if (lnk != inlnk) {
		if (qm->enq(p1,lnk,qn,now)) return dc;
		dc++;
	}
	ps->free(p1);
	return dc;
}

int fRouter::forward(int p, int ctte) {
// Lookup routing entry and forward packet accordingly.
// If packet (or any copies) of packet are discarded, return the
// number of discards. If no discards, return 0

	int rte = rt->lookup(ps->comtree(p),ps->dstAdr(p));
	// reply to route request if we have valid entry
	if ((ps->flags(p) & RTE_REQ) && rte != Null) {
		int p1 = ps->alloc();
		ps->setLeng(p1,28);
		ps->setPtyp(p1,RTE_REPLY);
		ps->setFlags(p1,0);
		ps->setComtree(p1,ps->comtree(p));
		ps->setSrcAdr(p1,myAdr);
		ps->setDstAdr(p1,ps->srcAdr(p));
		ps->pack(p1);
		ps->payload(p1)[0] = htonl(ps->dstAdr(p));
		ps->hdrErrUpdate(p1); ps->payErrUpdate(p1);
		qm->enq(p1,ps->inLink(p),ctt->qnum(ctte),now);

		ps->setFlags(p,ps->flags(p) & (~RTE_REQ));
		ps->pack(p);
		ps->hdrErrUpdate(p);
	}

	int adr, plnk;
	switch (ps->ptyp(p)) {
	case USERDATA:
		if (rte != Null) { // valid route case
			if (ucastAdr(ps->dstAdr(p))) {
				int qn = rt->qnum(rte);
				if (qn == 0) qn = ctt->qnum(ctte);
				int lnk = rt->link(rte);
				if (lnk != ps->inLink(p) &&
				    qm->enq(p,lnk,qn,now))
					return 0;
				ps->free(p); return 1;
			}
			// multicast data packet
			return multiSend(p,ctte,rte);
		}
		// no valid route
		if (ucastAdr(ps->dstAdr(p))) {
			// send to neighboring routers in comtree
			ps->setFlags(p,RTE_REQ);
			ps->pack(p); ps->hdrErrUpdate(p);
		}
		return multiSend(p,ctte,rte);
		break;
	case SUB_UNSUB:
		return subUnsub(p,ctte);
	case RTE_REPLY:
		adr = ntohl(ps->payload(p)[0]);
		if (ucastAdr(adr) && rt->lookup(ps->comtree(p),adr) == Null) {
			rt->addEntry(ps->comtree(p),adr,ps->inLink(p),0); 
		}
		break;
	default: break;
	}
	// packet discarded if control reaches here
	ps->free(p); return 1;
}

void fRouter::run(uint32_t finishTime, int numData) {
// Main router loop, repeatedly retrieves and processes an input packet,
// then processes an output packet if there is one whose time has come.
// Time is managed through a free-running microsecond clock, derived
// from the time value returned by gettimeofday. The clock is updated
// at the start of each loop.
//
// Note, that high input rates can cause output processing to fall behind.
// Input rates must be limited to ensure correct operation.
	// record of first packet receptions, transmissions for debugging
	const int MAXEVENTS = 200;
	struct { int sendFlag; uint32_t time; int link, pkt;} events[MAXEVENTS];
	int evCnt = 0;
	int nRcvd = 0; int nSent = 0; 	// counts of received and sent packets
	int discards = 0;		// count of number discards
	int statsTime = 0;		// time statistics were last processed
	bool didNothing;

	struct timeval ct, pt; // current and previous time values
	if (gettimeofday(&ct, NULL) < 0)
		fatal("fRouter::run: gettimeofday failure");
	now = 0;
	while (finishTime == 0 || now < finishTime) {
		didNothing = true;
		// input processing
		int p = iop->receive();
		if (p != Null) {
			didNothing = false;
			nRcvd++;
			ps->unpack(p);
			if (evCnt < MAXEVENTS &&
			    (ps->ptyp(p) != USERDATA || numData > 0)) {
				int p1 = ps->clone(p);
				events[evCnt].sendFlag = 0;
				events[evCnt].link = ps->inLink(p);
				events[evCnt].time = now;
				events[evCnt].pkt = p1;
				evCnt++;
				if (ps->ptyp(p) == USERDATA) numData--;
			}
			int ctte = ctt->lookup(ps->comtree(p));
			if (ctte == Null || 
			    !ctt->inComt(ctte,ps->inLink(p)) ||
			    !pktCheck(p,ctte)) {
				ps->free(p); discards++;
			} else {
				if (ps->dstAdr(p) == myAdr &&
				    ps->ptyp(p) != SUB_UNSUB) {
					// for now, just discard
					ps->free(p);
					discards++;
				} else {
					discards += forward(p,ctte);
				}
			}
		}

		// output processing
		int lnk;
		while ((lnk = qm->nextReady(now)) != Null) {
			didNothing = false;
			p = qm->deq(lnk);
			if (evCnt < MAXEVENTS &&
			    (ps->ptyp(p) != USERDATA || numData > 0)) {
				int p2 = ps->clone(p);
				events[evCnt].sendFlag = 1;
				events[evCnt].link = lnk;
				events[evCnt].time = now;
				events[evCnt].pkt = p2;
				evCnt++;
				if (ps->ptyp(p) == USERDATA) numData--;
			}
			iop->send(p,lnk);
			nSent++;
		}

		// update statistics every 300 ms
		if (now - statsTime > 300000) {
			sm->record(now);
			statsTime = now;
		}
		// update free-running clock (now) from timeofday clock
		pt = ct;
		if (gettimeofday(&ct, NULL) < 0)
			fatal("fRouter::run: gettimeofday failure");
                now += (ct.tv_sec == pt.tv_sec ?
                           ct.tv_usec - pt.tv_usec :
                           ct.tv_usec + (1000000 - pt.tv_usec)
                        );

		// If did nothing on that pass, sleep for a millisecond.
		if (!didNothing) continue;
		struct timespec sleeptime, rem;
		sleeptime.tv_sec = 0; sleeptime.tv_nsec = 1000000;
		nanosleep(&sleeptime,&rem);
		// update time variables again following sleep
		pt = ct;
		if (gettimeofday(&ct,Null) < 0)
			fatal("fRouter::run: gettimeofday failure");
		now += (ct.tv_sec == pt.tv_sec ?
                           ct.tv_usec - pt.tv_usec :
                           ct.tv_usec + (1000000 - pt.tv_usec)
			);
	}

	// print recorded events
	for (int i = 0; i < evCnt; i++) {
		if (events[i].sendFlag) 
			cout << "send link " << setw(2) << events[i].link
			     << " at " << setw(8) << events[i].time << " ";
		else
			cout << "recv link " << setw(2) << events[i].link
			     << " at " << setw(8) << events[i].time << " ";
		ps->print(cout,events[i].pkt);
	}
	cout << endl;
	cout << nRcvd << " packets received, " << nSent << " packets sent, " 
	     << discards << " packets discarded\n";
}
