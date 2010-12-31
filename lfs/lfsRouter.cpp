// usage:
//	lfsRouter lfsAdr ifTbl lnkTbl fltrTbl rteTbl stats finTime [ numData ]
//
// LfsRouter runs an lfs router. It waits for packets to arrive on the
// standard lfs port and forwards them appropriately.
// LfsAdr is the lfs address of the router. IfTbl, LnkTbl, fltrTbl and rteTbl
// are the names of files that contain the initial contents of the
// interface, link, filter and route tables.
// FinTime is the number of seconds the router should run.
// If zero, it runs forever.
// If the optional numData argument is present and non-zero,
// at most numData data packets are copied to the log file.
// NumData defaults to zero.

#include "lfsRouter.h"

main(int argc, char *argv[]) {
	int finTime;
	int ip0, ip1, ip2, ip3;
	ipa_t ipadr; ipa_t lfsAdr;
	int a3, a2, a1, a0;
	int numData = 0;
	if (argc < 8 || argc > 9 ||
	    sscanf(argv[1],"%d.%d.%d.%d", &a3, &a2, &a1, &a0) != 4 ||
	    sscanf(argv[7],"%d", &finTime) != 1 ||
	    (argc == 9 && sscanf(argv[8],"%d",&numData) != 1)) {
		fatal("usage: lfsRouter fAdr ifTbl lnkTbl fltrTbl, rteTbl stats finTime");
	}

	lfsAdr = ((a3 & 0xff) << 24) | ((a2 & 0xff) << 16) |
		 ((a1 & 0xff) <<  8) | (a0 & 0xff);
	lfsRouter router(lfsAdr);
	if (!router.init(argv[2],argv[3],argv[4],argv[5],argv[6])) {
		fatal("router: lfsRouter::init() failed");
	}
	router.dump(cout); 		// print tables
	router.run(1000000*finTime,numData);
	cout << endl;
	router.dump(cout); 		// print final tables
	cout << endl;
}

lfsRouter::lfsRouter(ipa_t myAdr1) : myAdr(myAdr1) {

	nLnks = 31; nFltrs = 1000; nRts = 1000;
	nPkts = 10000; nBufs = 10000; nQus = 1+nFltrs;

	lt = new lnkTbl(nLnks);
	ps = new pktStore(nPkts, nBufs);
	qm = new qMgr(nLnks+1, nPkts, nQus, nBufs-4*nLnks, ps, lt);
	ft = new fltrTbl(nFltrs,myAdr,lt,qm);
	rt = new rteTbl(nRts,myAdr,lt,qm);
	iop = new ioProc(lt, ps);
	avail = new int[nLnks+1];
	sm = new statsMod(100,lt,qm,avail);
}

lfsRouter::~lfsRouter() {
	delete lt; delete ft; delete rt; delete ps; delete qm; delete iop;
	delete sm;
}

bool lfsRouter::init(char iftf[],char ltf[],char ftf[],char rtf[],char smf[]) {
// Initialize ioProc, link table, filter table and routing table from
// files with names iftf, ltf, ftf and rtf. Perform consistency checks.
// Return true on success, false on failure.
        ifstream iftfs, ltfs, ftfs, rtfs, smfs;

        iftfs.open(iftf);
	if (iftfs.fail() || !(iftfs >> *iop)) {
		cerr << "lfsRouter::init: can't read interface table\n";
		return false;
	}
	iftfs.close();

        ltfs.open(ltf);
        if (ltfs.fail() || !(ltfs >> *lt)) {
		cerr << "lfsRouter::init: can't read link table\n";
		return false;
	}
        ltfs.close();

        ftfs.open(ftf);
        if (ftfs.fail() || !(ftfs >> *ft)) {
		cerr << "lfsRouter::init: can't read filter table\n";
		return false;
	}
        ftfs.close();

        rtfs.open(rtf);
        if (rtfs.fail() || !(rtfs >> *rt)) {
		cerr << "lfsRouter::init: can't read routing table\n";
		return false;
	}
        rtfs.close();

        smfs.open(smf);
        if (smfs.fail() || !(smfs >> *sm)) {
		cerr << "lfsRouter::init: can't read statistics specification\n";
		return false;
	}
        smfs.close();

	addLocalRoutes();

	// reserve half the bandwith of each link for datagram
	// traffic leaving the rest for lfs. For links with
	// capacity smaller than the minimum reservation,
	// lfs gets nothing.
	for (int i = 1; i <= nLnks; i++) {
		if (!lt->valid(i)) continue;
		int dgRate = lt->bitRate(i)/2;
		int dgQuantum;
		rateCalc(dgRate,dgRate,dgRate,dgQuantum);
		qm->quantum(i,1) = dgQuantum;
		avail[i] = max(0,lt->bitRate(i) - dgRate);
	}
	return true;
}

void lfsRouter::addLocalRoutes() {
// Add routes for all neighbors. All have prefix length 32 and
// a single link.
	for (int lnk = 1; lnk <= nLnks; lnk++) {
		if (!lt->valid(lnk)) continue;
		int rte = rt->lookup(lt->peerAdr(lnk));
		if (rte != 0 && rt->prefLeng(rte) == 32) continue;
		rte = rt->addEntry(lt->peerAdr(lnk),32);
		if (rte == Null)
			fatal("lfsRouter::addLocalRoutes: can't add route.");
		rt->link(rte,1) = lnk;
		for (int i = 2; i <= rteTbl::maxNhops; i++)
			rt->link(rte,i) = 0;
	}
}

void lfsRouter::dump(ostream& os) {
	os << "Interface Table\n\n" << *iop << endl;
	os << "Link Table\n\n" << *lt << endl;
	os << "Filter Table\n\n" << *ft << endl;
	os << "Routing Table\n\n" << *rt << endl;
	os << "Statistics\n\n" << *sm << endl;
}

bool lfsRouter::pktCheck(packet p) {
// Perform error checks on lfs packet.
// Return true if all checks pass, else false.
	// Check version, length and type
	char *bp1 = reinterpret_cast<char*>(&(ps->buffer(p))[0]);
	header& h = ps->hdr(p);
	int inL = h.inLink();
	if ((bp1[0] >> 4) != 4 ||
	    h.leng() != h.ioBytes() || h.leng() < 4*h.hleng() ||
	    h.hleng() != 5 && h.hleng() != 7) {
		return false;
	}
	// Check for spoofed addresses
	if (inL == Null ||
	    (lt->peerTyp(inL) < TRUSTED && lt->peerAdr(inL) != h.srcAdr())) {
		return false;
	}
	return true;
}

void lfsRouter::rateCalc(int rate, int maxRate, int& result, int& quantum) {
// Given a desired rate and a maximum available rate,
// calculate the best matching rate that the system can support
// and the corresponding quantum for the queue.
// This version requires rate reservations be at least
// 1000 Kb/s. A rate of 1000K corresponds to a quantum of 2000 bytes.
// This implies that one round of the WDRR scheduler takes about 16 ms.
// If the maxRate is less than 1000 Kb/s, the result returned is zero,
// but if maxRate is at least 1000 Kb/s and rate is less than 500 Kb/s,
// the result is 1000 Kb/s. Note, that rates are expressed
// in Kb/s and quanta in bytes.
	if (maxRate < 1000) { result = 0; return; }
	result = min(rate, maxRate);
	result = max(result, 1000);
	quantum = 2 * result;
	return;
}

int lfsRouter::forward(packet p) {
// Handle forwarding decisions for packets without options.
// If packet is discarded, return 1. If not, return 0
	int lnk, qn;
	header& h = ps->hdr(p);
	ipa_t src = h.srcAdr(); ipa_t dst = h.dstAdr();
	int inLnk = h.inLink();
	int fte = ft->lookup(src, dst);
	if (fte != Null) {
		lnk = ft->link(fte); qn = ft->qnum(fte);
		if (lnk != inLnk && qm->enq(p,lnk,qn,now))
			return 0;
		ps->free(p); return 1;
	}
	int rte = rt->lookup(dst);
	if (rte != Null) lnk = rt->link(rte,1);
	// forward as a datagram (using queue 1)
	if (lnk != inLnk && qm->enq(p,lnk,1,now))
		return 0;
	ps->free(p); return 1;
}

int lfsRouter::options(packet p) {
// Handle forwarding decisions for packets with LFS option.
// If packet is discarded, return 1. If no discards, return 0
	int lnk, qn;
	header& h = ps->hdr(p);
	ipa_t src = h.srcAdr(); ipa_t dst = h.dstAdr();
	int inLnk = h.inLink();
	int fte = ft->lookup(src, dst);

	if (h.hleng() != 7 || h.optCode() != 53 || h.optLeng() != 8) {
		ps->free(p); return 1;
	}
	int rte;
	int op = h.lfsOp();       int flags = h.lfsFlags();
	int rrate = h.lfsRrate(); int arate = h.lfsArate();
	// rrate and arate are in kb/s
		
	switch (op) {
	case FirmReq:
		// if there is already a filter for this src/dst pair,
		// forward packet on the path, possibly adjusting rate
		if (fte != 0) {
			int lnk = ft->link(fte); int qn = ft->qnum(fte);
			int frate = ft->rate(fte);
			if (frate != rrate) {
				// try to adjust to better match request
				int nuRate, nuQuant;
				rateCalc(rrate,avail[lnk],nuRate,nuQuant);
				if (nuRate != frate) {
					ft->rate(fte) = nuRate;
					qm->quantum(lnk,qn) = nuQuant;
					avail[lnk] -= (nuRate - frate);
					if (nuRate < arate) {
						h.lfsArate() = nuRate;
						ps->pack(p);
						ps->hdrErrUpdate(p);
					}
				}
			}
			if (lnk != inLnk && lt->peerTyp(lnk) == router &&
			    qm->enq(p,lnk,qn,now))
				return 0;
			ps->free(p); return 1;
		}
		// since there is no filter, need to add one and allocate
		// as much of the requested bandwidth as possible
		rte = rt->lookup(dst);
		// look for first link with sufficient bandwidth
		lnk = 0;
		for (int i = 1; i <= rteTbl::maxNhops; i++) {
			if (rt->link(rte,i) == 0) break;
			if (rt->link(rte,i) == inLnk) continue;
			if (avail[rt->link(rte,i)] >= rrate) {
				lnk = rt->link(rte,i); break;
			}
		}
		if (lnk == 0) {
			// if no luck, look for link with most bandwidth
			for (int i = 1; i <= rteTbl::maxNhops; i++) {
				if (rt->link(rte,i) == 0) break;
				if (rt->link(rte,i) == inLnk) continue;
				if (lnk == 0 ||
				    avail[rt->link(rte,i)] > avail[lnk]) {
					lnk = rt->link(rte,i);
				}
			}
		}
		if (lnk == 0) { ps->free(p); return 1; }
		fte = ft->addEntry(src,dst);
		if (fte == 0) { // forward as datagram
			lnk = rt->link(rte,1);
			if (lnk != inLnk && lt->peerTyp(lnk) == router &&
			    qm->enq(p,lnk,1,now))
				return 0;
			ps->free(p); return 1;
		}
		int nuRate, nuQuant;
		rateCalc(rrate,avail[lnk],nuRate,nuQuant);
		qn = 1+fte;
		ft->link(fte) = lnk; ft->qnum(fte) = qn;
		ft->rate(fte) = nuRate; qm->quantum(lnk,qn) = nuQuant;
		avail[lnk] -= nuRate;
		if (nuRate < arate) {
			h.lfsArate() = nuRate;
			ps->pack(p); ps->hdrErrUpdate(p);
		}
		if (lt->peerTyp(lnk) == router && qm->enq(p,lnk,qn,now))
			return 0;
		ps->free(p); return 1;
		break;
	case Release:
		// Remove the filter and forward the packet
		if (fte == 0) { ps->free(p); return 1; }
		lnk = ft->link(fte); qn = ft->qnum(fte);
		avail[lnk] += ft->rate(fte);
		ft->removeEntry(fte);
		if (lt->peerTyp(lnk) == router && qm->enq(p,lnk,qn,now))
			return 0;
		ps->free(p); return 1;
		break;
	default:
		// just forward any others
		if (fte == 0) {
			rte = rt->lookup(dst);
			if (rte == 0) { ps->free(p); return 1; }
			lnk = rt->link(rte,1);
			if (lnk != inLnk && lt->peerTyp(lnk) == router &&
			    qm->enq(p,lnk,1,now))
				return 0;
			ps->free(p); return 1;
		}
		lnk = ft->link(fte); qn = ft->qnum(fte);
		if (lt->peerTyp(lnk) == router && qm->enq(p,lnk,qn,now))
			return 0;
		ps->free(p); return 1;
	}
}

void lfsRouter::run(uint32_t finishTime, int numData) {
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
	struct 	{ int sendFlag; uint32_t time; int link; packet pkt;
		} events[MAXEVENTS];
	int evCnt = 0;
	int nRcvd = 0; int nSent = 0; 	// counts of received and sent packets
	int discards = 0;		// count of number discards
	int statsTime = 0;		// time statistics were last processed
	bool didNothing;

	struct timeval ct, pt; // current and previous time values
	if (gettimeofday(&ct, NULL) < 0)
		fatal("lfsRouter::run: gettimeofday failure");
	now = 0;
	while (finishTime == 0 || now < finishTime) {
		didNothing = true;
		// input processing
		packet p = iop->receive();
		if (p != 0) {
			didNothing = false;
			nRcvd++;
			ps->unpack(p);
			header& h = ps->hdr(p);

			if (evCnt < MAXEVENTS) {
				packet p1 = ps->clone(p);
				events[evCnt].sendFlag = 0;
				events[evCnt].link = h.inLink();
				events[evCnt].time = now;
				events[evCnt].pkt = p1;
				evCnt++;
			}
			if (!pktCheck(p)) {
				ps->free(p); discards++;
			} else if (h.dstAdr() == myAdr) {
				int lnk = h.inLink();
				if (h.lfsOp() == Control &&
				    h.lfsFlags() == Connect &&
				    lt->peerPort(lnk) == 0)
					lt->setPeerPort(lnk,h.srcPort());
				else
				if (h.lfsOp() == Control &&
				    h.lfsFlags() == Disconnect &&
				    lt->peerPort(lnk) == h.srcPort())
					lt->setPeerPort(lnk,0);
				ps->free(p);
				discards++;
			} else if (h.hleng() == 5) {
				discards += forward(p);
			} else {
				discards += options(p);
			}
		}

		// output processing
		int lnk;
		while ((lnk = qm->nextReady(now)) != Null) {
			didNothing = false;
			p = qm->deq(lnk);
			if (evCnt < MAXEVENTS) {
				packet p2 = ps->clone(p);
				events[evCnt].sendFlag = 1;
				events[evCnt].link = lnk;
				events[evCnt].time = now;
				events[evCnt].pkt = p2;
				evCnt++;
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
			fatal("lfsRouter::run: gettimeofday failure");
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
			fatal("lfsRouter::run: gettimeofday failure");
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
		(ps->hdr(events[i].pkt)).print(cout,ps->buffer(events[i].pkt));
	}
	cout << endl;
	cout << nRcvd << " packets received, " << nSent << " packets sent, " 
	     << discards << " packets discarded\n";
}
