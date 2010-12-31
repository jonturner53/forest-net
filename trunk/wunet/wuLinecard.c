// usage:
//	wuLinecard lcn wuAdr lnkTbl vnetTbl rteTbl lcTbl stats finTime
//
// WuLinecard implements a linecard for a wunet router. It waits for
// packets to arrive on the standard wunet port and forwards them
// appropriately.
// Lcn is the number of this line card and must be between 1 and 30.
// WuAdr is the wunet address of the router (not the linecard).
// LnkTbl, vnetTbl, rteTbl and lcTbl are the names of files that contain
// the initial contents of the link, vnet, route and linecard tables.
// Stats is the name of a file that specifies statistics to
// be collected for the router.
// FinTime is the number of seconds the router should run.
// If zero, it runs forever.


#include "stdinc.h"
#include "wunet.h"
#include "wuLinecard.h"

main(int argc, char *argv[]) {
	int finTime, lcNum;
	ipa_t ipadr; wuAdr_t wuAdr;
	if (argc != 9 ||
	    sscanf(argv[1],"%d", &lcNum) != 1 ||
	    sscanf(argv[2],"%d", &wuAdr) != 1 ||
	    sscanf(argv[8],"%d", &finTime) != 1)
		fatal("usage: wuLinecard lcn wuAdr lnkTbl vnetTbl, rteTbl lcTbl stats finTime");

	wuLinecard linecard(lcNum, wuAdr);
	if (!linecard.init(argv[3], argv[4],argv[5],argv[6],argv[7])) {
		fatal("linecard: wuLinecard::init() failed");
	}
	linecard.dump(cout); 		// print tables
	linecard.run(1000000*finTime);
	cout << endl;
	linecard.dump(cout); 		// print final tables
	cout << endl;
}

wuLinecard::wuLinecard(int myLcn1, wuAdr_t myAdr1)
	: myLcn(myLcn1), myAdr(myAdr1) {

	nLnks = 31; nVnets = 1000; nRts = 100000;
	nPkts = 200000; nBufs = 100000; nQus = 4000;
	schedInterval = 2000;

	lt = new lnkTbl(nLnks);
	lct = new lcTbl(nLnks-1);
	ps = new pktStore(nPkts, nBufs);
	qm = new qMgr(nLnks+1, nPkts, nQus, nBufs-4*nLnks, ps, lt, lct, myLcn);
	vnt = new vnetTbl(nVnets,qm);
	rt = new rteTbl(nRts,qm);
	sm = new statsMod(100,lt, qm, lct, myLcn);
}

wuLinecard::~wuLinecard() {
	delete lt; delete vnt; delete rt; delete lct;
	delete ps; delete qm; delete iop; delete sm;
}


bool wuLinecard::init(char ltf[], char vntf[], char rtf[],
		      char lctf[], char smf[]) {
// Initialize link table, vnet table routing table, stats module table
// and linecard table from files with names ltf, vntf, rtf, smf and lctf.
// Perform basic consistency checks. Return true on success, false on failure.
        ifstream ltfs, lctfs, vntfs, rtfs, smfs;

	// read in linecard information table - get # and IP addresses

        ltfs.open(ltf);
        if (ltfs.fail() || !(ltfs >> *lt)) {
		cerr << "wuLinecard::init: can't read link table\n";
		return false;
	}
        ltfs.close();

        vntfs.open(vntf);
        if (vntfs.fail() || !(vntfs >> *vnt)) {
		cerr << "wuLinecard::init: can't read vnet table\n";
		return false;
	}
        vntfs.close();

        rtfs.open(rtf);
        if (rtfs.fail() || !(rtfs >> *rt)) {
		cerr << "wuLinecard::init: can't read routing table\n";
		return false;
	}
        rtfs.close();

        lctfs.open(lctf);
        if (lctfs.fail() || !(lctfs >> *lct)) {
		cerr << "wuLinecard::init: can't read linecard table\n";
		return false;
	}
        lctfs.close();

	schedPeriod = schedInterval * (lct->nlc()-1);
	minPktRate = int(2.1*(1000000/schedPeriod));
	minBitRate = int((minPktRate*truPktLeng(28)*8)/1000.0);

        smfs.open(smf);
        if (smfs.fail() || !(smfs >> *sm)) {
		cerr << "wuLinecard::init: can't read statistics specification\n";
		return false;
	}
        smfs.close();

	addLocalRoutes();
	if (!checkTables()) return false;

	myIpAdr = lct->ipAdr(myLcn);

	iop = new ioProc(myIpAdr, WUNET_PORT, lt, ps, lct, myLcn);
	if (!iop->init()) {
		cerr << "wuLinecard::init: can't initialize iop\n";
		return false;
	}

	return true;
}

bool wuLinecard::checkTables() {
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

void wuLinecard::addLocalRoutes() {
// Add routes for all directly attached hosts for all vnets.
	int vnet, i, j, n;
	uint16_t lnkvec[nLnks+1];
	for (vnet = 1; vnet <= nVnets; vnet++) {
		if (!vnt->valid(vnet)) continue;
 		n = vnt->links(vnet,lnkvec,nLnks);
                for (i = 0; i < n; i++) {
                        j = lnkvec[i];
			if (rt->lookup(vnet,lt->peerAdr(j)) == Null)
				rt->addEntry(vnet,lt->peerAdr(j),j,0);
		}
	}
}

void wuLinecard::dump(ostream& os) {
	os << "Linecard " << myLcn << " Tables\n\n";
	os << "Link Table\n\n" << *lt << endl;
	os << "Line Card Table\n\n" << *lct << endl;
	os << "Vnet Table\n\n" << *vnt << endl;
	os << "Routing Table\n\n" << *rt << endl;
	os << "Statistics\n\n" << *sm << endl;
}

bool wuLinecard::pktCheck(int p) {
// Perform error checks on Wunet packet.
// Return true if all checks pass, else false.
// These checks should only performed for ingress packets.
	// Make sure it's an ingress packet
	int inL = ps->inLink(p);
	if (inL == Null || inL != myLcn) return false;

	// Check version, length and type
	char *bp1 = (char *) ps->buffer(p);
	if ((bp1[0] >> 4) != WUNET_VERSION ||
	    ps->leng(p) != ps->ioBytes(p) || ps->leng(p) < 16 ||
	    (ucastAdr(ps->dstAdr(p)) && ps->ptyp(p) != DATA)) {
		return false;
	}

	// Check for spoofed addresses
	if (lt->peerTyp(inL) == HOST && lt->peerAdr(inL) != ps->srcAdr(p))
		return false;

	// Check that input link is in vnet
	vnet_t vnet = ps->vnet(p);
	if (!vnt->valid(vnet) || !vnt->inVnet(vnet,inL))
		return false;
	return true;
}

void wuLinecard::addRevRte(int p) {
// Check for route back to sender. If none, insert route.
// This should be called in the egress linecard only.
	if (rt->lookup(ps->vnet(p),ps->srcAdr(p)) == 0)
		rt->addEntry(ps->vnet(p),ps->srcAdr(p),ps->inLink(p),0);
}

int wuLinecard::ingress(int p) {
// Processing of ingress packet. Lookup routing entry and
// place packet in proper outgoing VOQ or VOQs.
// If packet (or any copies) of packet are discarded, return the
// number of discards. If no discards, return 0
	bool flood = false;
	int  n; uint16_t lnkvec[nLnks+2]; // vector of links
	int pvn = ps->vnet(p); 
	int plnk = vnt->plink(pvn);
	int rte = rt->lookup(pvn,ps->dstAdr(p));

	// for packets with multiple output links, place links in lnkvec
	if (ps->ptyp(p) == SUBSCRIBE || ps->ptyp(p) == UNSUBSCRIBE) {
		// forward copy to all output line cards in vnet
		if (!mcastAdr(ps->dstAdr(p))) { ps->free(p); return 1; }
		n = vnt->links(pvn,lnkvec,nLnks);
	} else if (rte != Null) { // valid route case
		if (ucastAdr(ps->dstAdr(p))) {
			if (qm->enq(p,rt->link(rte),1,now)) return 0;
			ps->free(p); return 1;
		} else { // multicast data
			n = rt->links(rte,lnkvec,nLnks);
			if (plnk != Null && plnk != myLcn) lnkvec[n++] = plnk;
		}
	} else { // no valid route
		if (ucastAdr(ps->dstAdr(p))) {
			// send to all neighboring routers, except upstream
			n = vnt->links(pvn,lnkvec,nLnks);
			flood = true; // send to all neighboring routers
		} else { // multicast data
			if (plnk != Null && plnk != myLcn) {
				if (qm->enq(p,plnk,1,now)) return 0;
			}
			ps->free(p); return 1;
		}
	}

	// forward packets going to multiple VOQs
	if (n == 0) { ps->free(p); return 1; }
	int dc = 0, p1 = p, lnk;
	for (int i = 0; i < n-1; i++) { // process first n-1 copies
		lnk = lnkvec[i];
		if (lnk == myLcn || (flood && lt->peerTyp(lnk) != ROUTER)) {
			; // do nothing
		} else {
			if (qm->enq(p1,lnk,1,now)) p1 = ps->clone(p);
			else dc++;
		}
	}
	// process last copy
	lnk = lnkvec[n-1];
	if (lnk == myLcn || (flood && lt->peerTyp(lnk) != ROUTER)) {
		; // do nothing
	} else {
		if (qm->enq(p1,lnk,1,now)) return dc;
		else dc++;
	}
	ps->free(p1);
	return dc;
}

int wuLinecard::egress(int p) {
// Processing of egress packet. Lookup exit table entry and
// put packet in proper queue for outgoing link.
// If packet (or any copies) of packet are discarded, return the
// number of discards. If no discards, return 0
	int pvn = ps->vnet(p); 
	int plnk = vnt->plink(pvn);
	int rte = rt->lookup(pvn,ps->dstAdr(p));
	int pqn = (rte != Null &&  rt->qnum(rte) != 0) ?
			rt->qnum(rte) : vnt->qnum(pvn);
	int inLc = ps->inLink(p);

	if (ps->ptyp(p) == SUBSCRIBE) {
		if (!mcastAdr(ps->dstAdr(p))) { ps->free(p); return 1; }
		if (rte != Null) {
			rt->addLink(rte,inLc);
			ps->free(p); return 1;
		}
		rt->addEntry(pvn,ps->dstAdr(p),inLc,0);
		if (plnk != myLcn) { ps->free(p); return 1; }
	} else if (ps->ptyp(p) == UNSUBSCRIBE) {
		if (!mcastAdr(ps->dstAdr(p)) || rte == Null) {
			ps->free(p); return 1;
		}
		rt->removeLink(rte,inLc);
		if (rt->noLinks(rte)) rt->removeEntry(rte);
		if (!(rt->noLinks(rte) && plnk == myLcn)) {
			ps->free(p); return 1;
		}
	} else if (ps->ptyp(p) == VOQSTATUS) {
		if (ps->leng(p) >= 28) {
			uint32_t *pbuf = (uint32_t *) ps->buffer(p);
			uint32_t x = ntohl(pbuf[4]); // sender's voq length
			uint32_t diff = x - lct->voqLen(inLc); // new - old
			lct->setVoqLen(inLc,x);
			lct->setInBklg(inLc,ntohl(pbuf[5]));
			lct->setOutBklg(inLc,ntohl(pbuf[6]));
			// adjust input-side backlog for myLcn
			lct->setInBklg(myLcn,lct->inBklg(myLcn)+diff);
		}
		voqUpdate(inLc);
		ps->free(p); return 0; // don't count as discards
	}
 
	if (qm->enq(p,myLcn,pqn,now)) return 0;
	ps->free(p); return 1;
}

void wuLinecard::sendVOQstatus() {
// Send one VOQ status packet. If next is the index of the next linecard
// to get a status packet, send it the size of "my" current voq length
// to linecard next, along with the output queue length for "my" external
// link and the total input side backlog directed to "me".
//
// This routine should be called at a rate that limits the load it imposes
// on busy line cards to no more than 5-10%.
// 
// Use a separate queue (number 2) for the VOQ status messages.
	static int next = 0;	// note: no process should have more than
				// one instance of wuLinecard

	while (1) {
		next = (next >= lct->nlc() ? 1 : next+1);
		if (next <= lct->nlc() && lct->valid(next) && next != myLcn)
			break;
	}
	int p = ps->alloc();
	if (p == Null) return;

	ps->setLeng(p,28);
	ps->setPtyp(p,VOQSTATUS);
	ps->setSrcAdr(p,myLcn);
	ps->setDstAdr(p,next);
	uint32_t *pbuf = (uint32_t *) ps->buffer(p);
	// Send to next: # of packets in my VOQ to next
	// # of packets in my output queue
	// # of packets that other inputs have for me
	pbuf[4] = htonl(qm->qlenBytes(next));
	pbuf[5] = htonl(lct->inBklg(myLcn));
	pbuf[6] = htonl(qm->qlenBytes(myLcn));
	ps->pack(p);

	if (!qm->enq(p,next,2,now)) ps->free(p);
}

void wuLinecard::voqUpdate(int x) {
// This routine is called when the status of linecard x has changed.
// Adjusts sending rates to reflect these changes and changes in myLcn.
//
	int i,j;
	static bool first = true; // note: no process should have >1 wuLinecard
	static int lc[MAXLC+1];   // lc[j] is linecard with j-th smallest bklg
	static int pos[MAXLC+1];  // pos[i] is position of linecard i in lc[]

	if (first) { // initialize sorted voq list and set initial rates
		j = 1;
		for (i = 1; i <= lct->nlc(); i++) {
			if (i == myLcn) continue;
			lc[j] = i; pos[i] = j++;
			lt->setBitRate(i,min(
				lct->maxBitRate(i)/(lct->nlc()-1),
				lct->maxBitRate(myLcn)/(lct->nlc()-1)
			));
			lt->setPktRate(i,min(
				lct->maxPktRate(i)/(lct->nlc()-1),
				lct->maxPktRate(myLcn)/(lct->nlc()-1)
			));
			if (lt->bitRate(i) < minBitRate ||
			    lt->pktRate(i) < minPktRate)
				fatal("inter-linecard bandwidth too small");
		}
		first = false;
	}
	
	// Adjust position of x in list
	i = pos[x];
	if (i < lct->nlc()-1 && lct->outBklg(x) > lct->outBklg(lc[i+1])) {
		while (i<lct->nlc()-1 && lct->outBklg(x)>lct->outBklg(lc[i+1])) {
			lc[i] = lc[i+1]; pos[lc[i]] = i; i++;
		}
	} else if (i > 1 && lct->outBklg(x) < lct->outBklg(lc[i-1])) {
		while (i > 1 && lct->outBklg(x) < lct->outBklg(lc[i-1])) {
			lc[i] = lc[i-1]; pos[lc[i]] = i; i--;
		}
	}
	lc[i] = x; pos[x] = i;

	// Start at pos[x] and adjust rates of those that are further
	// down the list.
	int bitRate = 0; int pktRate = 0;
	for (i = 1; i < pos[x]; i++) {
		bitRate += lt->bitRate(lc[i]); pktRate += lt->pktRate(lc[i]);
	}
	for (i = pos[x]; i <= lct->nlc()-1; i++) {
		double w = (lct->inBklg(lc[i]) == 0 ? 0 :
			    double(qm->qlenBytes(lc[i]))/lct->inBklg(lc[i])
			   );
		w = min(w,1.0);
		int br;
		br = int(w*(lct->maxBitRate(lc[i])-(lct->nlc()-1)*minBitRate));
		br = min(br,int(8000.0*qm->qlenBytes(lc[i])/schedPeriod));
		br = max(br,minBitRate);
		br = min(br,lct->maxBitRate(myLcn)
				-(bitRate+((lct->nlc()-1)-i)*minBitRate));
		int pr;
		pr = int(w*(lct->maxPktRate(lc[i])-(lct->nlc()-1)*minPktRate));
		pr = min(pr,int(1000000.0*qm->qlenPkts(lc[i])/schedPeriod));
		pr = max(pr,minPktRate);
		pr = min(pr,lct->maxPktRate(myLcn)
				-(pktRate+((lct->nlc()-1)-i)*minPktRate));
		lt->setBitRate(lc[i],br); lt->setPktRate(lc[i],pr);
		bitRate += br; pktRate += pr;
	}
}

void wuLinecard::run(uint32_t finishTime) {
// Main router loop, repeatedly retrieves and processes an input packet,
// then processes an output packet if there is one whose time has come.
// Time is managed through a free-running microsecond clock, derived
// from the time value returned by gettimeofday. The clock is updated
// at the start of each loop.
//
// Note, that high input rates can cause output processing to fall behind.
// Input rates must be limited to ensure correct operation.
	int schedTime = 0; // time last VOQ scheduling pkt was sent
	// record of first packet receptions, transmissions for debugging
	const int MAXEVENTS = 500;
	struct { int sendFlag; uint32_t time; int link, pkt;} events[MAXEVENTS];
	int evCnt = 0;
	// basic statistics - ingress and egress
	int inRcvd = 0; int inSent = 0, inDiscards;
	int egRcvd = 0; int egSent = 0, egDiscards;
	int statsTime = 0;	// time other statistics were last processed

	struct timeval ct, pt; // current and previous time values
	if (gettimeofday(&ct, NULL) < 0)
		fatal("wuLinecard::run: gettimeofday failure");
	now = 0;
	while (finishTime == 0 || now < finishTime) {
		// input processing
		int p = iop->receive();
		if (p != Null) {
			ps->unpack(p);
			if (evCnt < MAXEVENTS) {
				int p1 = ps->clone(p);
				events[evCnt].sendFlag = 0;
				events[evCnt].link = ps->inLink(p);
				events[evCnt].time = now;
				events[evCnt].pkt = p1;
				evCnt++;
			}
			if (ps->inLink(p) == myLcn) { // ingress case
				inRcvd++;
				if (pktCheck(p)) {
					if (ps->dstAdr(p) == myAdr) {
						// for now, just discard
						ps->free(p);
					} else {
						inDiscards += ingress(p);
					}
				} else {
					ps->free(p); inDiscards++;
				}
			} else { // egress case
				if (ps->ptyp(p) != VOQSTATUS) egRcvd++;
				addRevRte(p);
				egDiscards += egress(p);
			}
		}

		if (now >= schedTime + schedInterval) { // send VOQ sched packet
			sendVOQstatus();
			schedTime = now;
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
			if (lnk == myLcn) egSent++;
			else if (ps->ptyp(p) != VOQSTATUS) inSent++;
		}

		// update statistics every 300 ms
		if (now - statsTime > 300000) {
			sm->record(now);
			statsTime = now;
		}
		// update free-running clock (now) from timeofday clock
		pt = ct;
		if (gettimeofday(&ct, NULL) < 0)
			fatal("wuLinecard::run: gettimeofday failure");
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
	cout << "ingress packets received, sent, discarded: "
	     << inRcvd << " " << inSent << " " << inDiscards << endl;
	cout << " egress packets received, sent, discarded: "
	     << egRcvd << " " << egSent << " " << egDiscards << endl;
}
