// usage:
//	wuRouter ipAdr wuAdr lnkTbl vnetTbl rteTbl stats finTime
//
// WuRouter runs a wunet router. It waits for packets to arrive on the
// standard wunet port and forwards them appropriately.
// IpAdr is the IP address of the machine on which wuRouter is
// being run (for ONL, this is the internal 192.168.x.x address).
// WuAdr is the wunet address of the router. LnkTbl, vnetTbl and rteTbl
// are the names of files that contain the initial contents of the
// link, vnet and route tables.
// FinTime is the number of seconds the router should run.
// If zero, it runs forever.


#include "stdinc.h"
#include "wunet.h"
#include "wuRouter.h"

main(int argc, char *argv[]) {
	int finTime;
	int ip0, ip1, ip2, ip3;
	ipa_t ipadr; wuAdr_t wuAdr;
	if (argc != 8 ||
	    sscanf(argv[1],"%d.%d.%d.%d", &ip3, &ip2, &ip1, &ip0) != 4 ||
	    sscanf(argv[2],"%d", &wuAdr) != 1 ||
	    sscanf(argv[7],"%d", &finTime) != 1)
		fatal("usage: wuRouter ipAdr wuAdr lnkTbl vnetTbl, rteTbl stats finTime");

	ipadr = ((ip3 & 0xff) << 24)  | ((ip2 & 0xff) << 16) |
	        ((ip1 & 0xff) <<  8)  | (ip0 & 0xff);

	wuRouter router(ipadr, wuAdr);
	if (!router.init(argv[3], argv[4],argv[5],argv[6])) {
		fatal("router: wuRouter::init() failed");
	}
	router.dump(cout); 		// print tables
	router.run(1000000*finTime);
	cout << endl;
	router.dump(cout); 		// print final tables
	cout << endl;
}

wuRouter::wuRouter(ipa_t myIpAdr1, wuAdr_t myAdr1)
	: myIpAdr(myIpAdr1), myAdr(myAdr1) {

	nLnks = 31; nVnets = 1000; nRts = 100000;
	nPkts = 500000; nBufs = 200000; nQus = 4000;

	lt = new lnkTbl(nLnks);
	ps = new pktStore(nPkts, nBufs);
	qm = new qMgr(nLnks+1, nPkts, nQus, nBufs-4*nLnks, ps, lt);
	vnt = new vnetTbl(nVnets,qm);
	rt = new rteTbl(nRts,qm);
	iop = new ioProc(myIpAdr, WUNET_PORT, lt, ps);
	sm = new statsMod(100,lt,qm);
}

wuRouter::~wuRouter() {
	delete lt; delete vnt; delete rt; delete ps; delete qm; delete iop;
	delete sm;
}


bool wuRouter::init(char ltf[], char vntf[], char rtf[], char smf[]) {
// Initialize link table, vnet table and routing table from
// files with names ltf, vntf and rtf. Perform consistency checks.
// Return true on success, false on failure.
        ifstream ltfs, vntfs, rtfs, smfs;

	if (!iop->init()) {
		cerr << "wuRouter::init: can't initialize iop\n";
		return false;
	}

        ltfs.open(ltf);
        if (ltfs.fail() || !(ltfs >> *lt)) {
		cerr << "wuRouter::init: can't read link table\n";
		return false;
	}
        ltfs.close();

        vntfs.open(vntf);
        if (vntfs.fail() || !(vntfs >> *vnt)) {
		cerr << "wuRouter::init: can't read vnet table\n";
		return false;
	}
        vntfs.close();

        rtfs.open(rtf);
        if (rtfs.fail() || !(rtfs >> *rt)) {
		cerr << "wuRouter::init: can't read routing table\n";
		return false;
	}
        rtfs.close();

        smfs.open(smf);
        if (smfs.fail() || !(smfs >> *sm)) {
		cerr << "wuRouter::init: can't read statistics specification\n";
		return false;
	}
        smfs.close();

	addLocalRoutes();
	if (!checkTables()) return false;
	return true;
}

bool wuRouter::checkTables() {
// Perform consistency checks on all tables. Return true if
// no inconsistencies detected, else false.
	int i, n, lte, vnet, rte;
	uint16_t lnkvec[nLnks+1];
	// check vnet table
	for (vnet = 1; vnet <= nVnets; vnet++) {
		if (!vnt->valid(vnet)) continue;
		// every link in vnet entry must be valid
		n = vnt->links(vnet,lnkvec,nLnks);
		for (i = 0; i < n; i++) {
			if (!lt->valid(lnkvec[i])) {
				cerr << "Error in vnet table[" << vnet
				     << "]: no valid entry in link table"
				     << " for link " << lnkvec[i] << "\n";
				return false;
			}
		}
	}
	// check routing table
	for (rte = 1; rte <= nRts; rte++) {
		if (!rt->valid(rte)) continue;
		// vnets in routing table must be valid
		if (!vnt->valid(rt->vnet(rte))) {
			cerr << "Error in routing table[" << rte
			     << "]: vnet " << rt->vnet(rte) 
			     << " not in vnet table\n";
			return false;
		}
	}
	return true;
}

void wuRouter::addLocalRoutes() {
// Add routes for all directly attached hosts for all vnets.
	int vnet, i, j, n;
	uint16_t lnkvec[nLnks+1];
	for (vnet = 1; vnet <= nVnets; vnet++) {
		if (!vnt->valid(vnet)) continue;
		n = vnt->links(vnet,lnkvec,nLnks);
		for (i = 0; i < n; i++) {
			j = lnkvec[i];
			if (lt->peerTyp(j) == ROUTER) continue;
			if (rt->lookup(vnet,lt->peerAdr(j))!=Null)
				continue;
			rt->addEntry(vnet,lt->peerAdr(j),j,0);
		}
	}
}

void wuRouter::dump(ostream& os) {
	os << "Link Table\n\n" << *lt << endl;
	os << "Vnet Table\n\n" << *vnt << endl;
	os << "Routing Table\n\n" << *rt << endl;
	os << "Statistics\n\n" << *sm << endl;
}

bool wuRouter::pktCheck(int p) {
// Perform error checks on Wunet packet.
// Return true if all checks pass, else false.
	// Check version, length and type
	char *bp1 = (char *) ps->buffer(p);
	if ((bp1[0] >> 4) != WUNET_VERSION ||
	    ps->leng(p) != ps->ioBytes(p) || ps->leng(p) < 16 ||
	    (ucastAdr(ps->dstAdr(p)) && ps->ptyp(p) != DATA)) {
		return false;
	}
	// Check for spoofed addresses
	int inL = ps->inLink(p);
	if (inL == Null ||
	    (lt->peerTyp(inL) == HOST && lt->peerAdr(inL) != ps->srcAdr(p))) {
		return false;
	}
	// Check that vnet is valid at this router
	vnet_t vnet = ps->vnet(p);
	if (!vnt->valid(vnet) || !vnt->inVnet(vnet,ps->inLink(p)))
		return false;
	return true;
}

void wuRouter::addRevRte(int p) {
// Check for route back to sender. If none, insert route.
	if (rt->lookup(ps->vnet(p),ps->srcAdr(p)) == 0)
		rt->addEntry(ps->vnet(p),ps->srcAdr(p),ps->inLink(p),0);
}

int wuRouter::forward(int p) {
// Lookup routing entry and forward packet accordingly.
// If packet (or any copies) of packet are discarded, return the
// number of discards. If no discards, return 0
	bool flood = false;
	int  n; uint16_t lnkvec[nLnks+2]; // vector of links
	int pvn = ps->vnet(p); 
	int plnk = vnt->plink(pvn); int pqn = vnt->qnum(pvn);
	int rte = rt->lookup(pvn,ps->dstAdr(p));

	// for packets with multiple output links, place links in lnkvec
	if (rte != Null) { // valid route case
		if (rt->qnum(rte) != 0) pqn = rt->qnum(rte);
		if (ucastAdr(ps->dstAdr(p))) {
			if (qm->enq(p,rt->link(rte),pqn,now)) return 0;
			ps->free(p); return 1;
		} else if (ps->ptyp(p) == SUBSCRIBE) {
			rt->addLink(rte,ps->inLink(p));
			ps->free(p); return 1;
		} else if (ps->ptyp(p) == UNSUBSCRIBE) {
			rt->removeLink(rte,ps->inLink(p));
			if (rt->noLinks(rte)) {
				rt->removeEntry(rte);
				if (plnk != Null && qm->enq(p,plnk,pqn,now))
					return 0;
			}
			ps->free(p); return 1;
		}
		n = rt->links(rte,lnkvec,nLnks);
		if (plnk != Null) lnkvec[n++] = plnk; // all multicasts to parent
	} else { // no valid route
		if (mcastAdr(ps->dstAdr(p))) {
			if (ps->ptyp(p) == SUBSCRIBE) {
				rt->addEntry(pvn,ps->dstAdr(p),ps->inLink(p),0);
				if (plnk != Null && qm->enq(p,plnk,pqn,now))
					return 0;
			} else if (ps->ptyp(p) == DATA && plnk != Null) {
				if (qm->enq(p,plnk,pqn,now)) return 0;
			}
			ps->free(p); return 1;
		}
		// send to all neighboring routers, except upstream
		n = vnt->links(pvn,lnkvec,nLnks);
		flood = true;
	}

	// forward packets with multiple output links
	if (n == 0) { ps->free(p); return 1;}
	int dc = 0; int p1 = p; int lnk;
	for (int i = 0; i < n-1; i++) { // process first n-1 copies
		lnk = lnkvec[i];
		if (lnk != ps->inLink(p) &&
		    (flood == false || lt->peerTyp(lnk) == ROUTER) ) {
			if (qm->enq(p1,lnk,pqn,now))
				p1 = ps->clone(p);
			else dc++;
		}
	}
	// process last copy
	lnk = lnkvec[n-1];
	if (lnk != ps->inLink(p) &&
	    (flood == false || lt->peerTyp(lnk) == ROUTER) ) {
		if (qm->enq(p1,lnk,pqn,now)) return dc;
		dc++;
	}
	ps->free(p1);
	return dc;
}

void wuRouter::run(uint32_t finishTime) {
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
	struct { int sendFlag; uint32_t time; int link, pkt; } events[MAXEVENTS];
	int evCnt = 0;
	int nRcvd = 0; int nSent = 0; 	// counts of received and sent packets
	int discards = 0;		// count of number discards
	int statsTime = 0;		// time statistics were last processed

	struct timeval ct, pt; // current and previous time values
	if (gettimeofday(&ct, NULL) < 0)
		fatal("wuRouter::run: gettimeofday failure");
	now = 0;
	while (finishTime == 0 || now < finishTime) {
		// input processing
		int p = iop->receive();
		if (p != Null) {
			nRcvd++;
			ps->unpack(p);
			if (evCnt < MAXEVENTS) {
				int p1 = ps->clone(p);
				events[evCnt].sendFlag = 0;
				events[evCnt].link = ps->inLink(p);
				events[evCnt].time = now;
				events[evCnt].pkt = p1;
				evCnt++;
			}
			if (pktCheck(p)) {
				addRevRte(p);
				if (ps->dstAdr(p) == myAdr) {
					// for now, just discard
					ps->free(p);
				} else {
					discards += forward(p);
				}
			} else {
				ps->free(p);
				discards++;
			}
		}

		// output processing
		int lnk;
		while ((lnk = qm->nextReady(now)) != Null) {
			p = qm->deq(lnk);
			if (evCnt < MAXEVENTS) {
				int p2 = ps->clone(p);
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
			fatal("wuRouter::run: gettimeofday failure");
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
		cout << endl;
	}
	cout << endl;
	cout << nRcvd << " packets received, " << nSent << " packets sent, " 
	     << discards << " packets discarded\n";
}
