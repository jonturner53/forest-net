/* Notes
LfsDaemon is intended for use with the IPv4 fastpath. It is used in
lfs_fpd, (which is the LFS version of ip_fpd) to implement the  LFS
functionality. Lfs_fpd instantiates lfsDaemon and invokes init with
the names of the files that contain the intial configuration for the
LFS router (these are specified as command-line arguments to lfs_fpd).

This version of lfsDaemon uses fastpath filters to implement
nRts (=100) default routes and nFltrs (=100) flow-filters. These flow
filters are used for reserved bandwidth flows. It also uses the fastpath
filters to implement nLnks (=31) "bypass filters" needed to redirect
packets from the GPE out through the proper fastpath interface.
Consequently, we require nRts+nFltrs+nLnks (=231) fastpath filters to
be reserved on the NPE. The code does no checking that enough filters
have been reserved, so it if you don't reserve enough, you can expect
lfs_fpd to fail unpredictably.

This version uses nQus (=1+nFltrs=101) queues for each LFS overlay link.
This translates to nLnks*nQus (=31*101=3131) fastpath queues.
For each overlay link in the lnkTbl, it binds nQus queues
to the fastpath interface used by that link. This wastes queue
resources, but simplifies the management of queues and eliminates
the need to re-bind queues to different fastpath interfaces
(meaning we don't have to worry about packets that might be
left in queues we want to re-bind). Again, we assume that enough
queues are reserved and do no checking. Users must ensure that
enough are reserved. Note that the fastpath uses a global numbering
of queues, but this program numbers queues separately for each link.
So, a translation is required to get the fastpath queue number.
This is provided by the fpq() macro below.

Statistics counts are maintained for each filter and for each of
the default routes. So, nFltrs+nRts (=200) stats counts should
be reserved. The stats index for filter number i is i.
The stats index for route number i is nFltrs+i

This version expects nPkts=100*(nLnks+nFltrs) (=13100) packet buffers
to be reserved on the fastpath. Individual queues have a maximum size of
100 packets. This works since the number of active queues at one time
is at most nLnks+nFltrs.
*/

#include "lfsDaemon.h"

// Return the fastpath queue number for queue q, on link lnk
#define fpq(lnk,q) (((lnk)-1)*(nQus) + (q))

lfsDaemon::lfsDaemon(ipa_t myAdr1) : myAdr(myAdr1) {

	nLnks = 31; nFltrs = 100; nRts = 100;
	nQus = 1+nFltrs; qSiz = 100;
	nPkts = qSiz*(nLnks+nFltrs); nBufs = nPkts;

	lt = new lnkTbl(nLnks);
	ps = new pktStore(nPkts, nBufs);
	qm = new qMgr(nLnks+1, nPkts, nQus, nBufs-4*nLnks, ps, lt);
	ft = new fltrTbl(nFltrs,myAdr,lt,qm);
	rt = new rteTbl(nRts,myAdr,lt,qm);
	iop = new ioProc(lt, ps);
	sm = new statsMod(100,lt,qm,avail);
	avail = new int[nLnks+1];
}

lfsDaemon::~lfsDaemon() {
	delete lt; delete ft; delete rt; delete ps; delete qm; delete iop;
	delete sm;
}

bool lfsDaemon::init(char iftf[],char ltf[],char ftf[],char rtf[],char smf[]) {
// Initialize ioProc, link table, filter table and routing table from
// files with names iftf, ltf, ftf and rtf. Perform consistency checks.
// Return true on success, false on failure.
        ifstream iftfs, ltfs, ftfs, rtfs, smfs;

        iftfs.open(iftf);
	if (iftfs.fail() || !(iftfs >> *iop)) {
		cerr << "lfsDaemon::init: can't read interface table\n";
		return false;
	}
	iftfs.close();

        ltfs.open(ltf);
        if (ltfs.fail() || !(ltfs >> *lt)) {
		cerr << "lfsDaemon::init: can't read link table\n";
		return false;
	}
        ltfs.close();

        ftfs.open(ftf);
        if (ftfs.fail() || !(ftfs >> *ft)) {
		cerr << "lfsDaemon::init: can't read filter table\n";
		return false;
	}
        ftfs.close();

        rtfs.open(rtf);
        if (rtfs.fail() || !(rtfs >> *rt)) {
		cerr << "lfsDaemon::init: can't read routing table\n";
		return false;
	}
        rtfs.close();

        smfs.open(smf);
        if (smfs.fail() || !(smfs >> *sm)) {
		cerr << "lfsDaemon::init: can't read statistics specification\n";
		return false;
	}
        smfs.close();

	addInterfacesBypass();

	addQueues();

	addRoutesFilters();

	return true;
}

extern int setupFpInt(uint32_t, int, int);
extern bool setupBypassFilter(int, uint32_t, int, int, int);
extern bool setupRoute(int, int, uint32_t, int, uint32_t, int, int, int);
extern bool setupFlowFilter(int, int, uint32_t, uint32_t, uint32_t, int, int, int);
extern bool bindQueue(int, int, int);
extern bool setQueueParams(int, int, int);
extern uint32_t *getBufPntr(mnpkt_t);
extern uint32_t getTunnelSrcIp(mnpkt_t);
extern int getTunnelSrcPort(mnpkt_t);
extern uint32_t getTunnelLocalIp(mnpkt_t);
extern int getTunnelLocalPort(mnpkt_t);
extern void back2fp(wunet::sockInet, mnpkt_t, uint32_t, int);

void lfsDaemon::addInterfacesBypass() {
// Configure fastpath for specified interfaces and links
	// configure a separate fastpath interface for each interface
	// specified in the interface table.
	for (int i = 1; i <= nLnks; i++) {
		if (!iop->valid(i)) continue;
		int ifn = setupFpInt(iop->ipAdr(i),LFS_PORT,iop->maxBitRate(i));
		if (ifn < 0) fatal("can't configure fastpath interfaces");
		iop->fpi(i) = ifn;
	}
	// for each LFS link, setup a fastpath bypass filter
	for (int i = 1; i <= nLnks; i++) {
		if (!lt->valid(i)) continue;
		if (!setupBypassFilter(nFltrs+nRts+i,lt->peerIpAdr(i),
				       lt->peerPort(i),fpq(i,1),nFltrs+i))
			fatal("can't configure bypass filters");
	}
}

void lfsDaemon::addQueues() {
// Configure datagram queues.
	// can probably drop this 
	/*
	if (!claimFpResources())
		fatal("can't acquire fastpath resources");
	*/
	for (int i = 1; i <= nLnks; i++) {
		if (!lt->valid(i)) continue;
		/*
		for (int j = 1; j <= nQus; j++) {
			if (!bindQueue(fpq(i,j),iop->fpi(lt->interface(i))))
				fatal("can't bind queue to interface");
		}
		*/
		if (!bindQueue(fpq(i,1),fpq(i,nLnks),
		    iop->fpi(lt->interface(i))))
			fatal("can't bind queues to interface");
		int dgRate = (0.8) * lt->bitRate(i);
		rateCalc(dgRate,dgRate,dgRate);
		setQueueParams(fpq(i,1),dgRate,qSiz);
		avail[i] = max(0,lt->bitRate(i) - dgRate);
	}
}


void lfsDaemon::addRoutesFilters() {
// Add routes for all neighbors. All have prefix length 32 and
// a single link.
	for (int lnk = 1; lnk <= nLnks; lnk++) {
		if (!lt->valid(lnk)) continue;
		int rte = rt->lookup(lt->peerAdr(lnk));
		if (rte != 0 && rt->prefLeng(rte) == 32) continue;
		rte = rt->addEntry(lt->peerAdr(lnk),32);
		if (rte == Null)
			fatal("lfsDaemon::addRoutesFilters: can't add route.");
		rt->link(rte,1) = lnk;
		for (int i = 2; i <= rteTbl::maxNhops; i++)
			rt->link(rte,i) = 0;
	}
	rt->sort(); // sort routes by prefix length
	// Now install default route for each entry in route table
	int fnum = nFltrs+1;
	for (int rte = 1; rte <= nRts; rte++) {
		if (!rt->valid(rte)) continue;
		for (int i = 1; i <= nLnks; i++) {
			if (!iop->valid(i)) continue;
			// setup specified routes on all interfaces
			int lnk = rt->link(rte,1);
			if (!setupRoute(fnum++,iop->fpi(i),
					rt->prefix(rte),rt->prefLeng(rte),
					lt->peerIpAdr(lnk), lt->peerPort(lnk),
			        	fpq(lnk,1),nFltrs+lnk))
				fatal("can't configure filters for routes");
		}
	}
	// Now install filters that are specified in filter table
	for (int fte = 1; fte <= nFltrs; fte++) {
		if (!ft->valid(fte)) continue;
		int lnk = ft->link(fte);
		setupFilter(fte,ft->inLink(fte),ft->src(fte),ft->dst(fte),
			    lt->peerIpAdr(lnk),lt->peerPort(lnk),
			    fpq(lnk,ft->qnum(fte)),fte);
	}
}

void lfsDaemon::dump(ostream& os) {
	os << "Interface Table\n\n" << *iop << endl;
	os << "Link Table\n\n" << *lt << endl;
	os << "Filter Table\n\n" << *ft << endl;
	os << "Routing Table\n\n" << *rt << endl;
	os << "Statistics\n\n" << *sm << endl;
}

bool lfsDaemon::pktCheck(header& h) {
// Perform error checks on lfs packet.
// Return true if all checks pass, else false.
	// Check version, length and type
	if (h.leng() < 4*h.hleng()) return false;
	// Check for spoofed addresses
	int inL = h.inLink();
	if (inL == Null ||
	    (lt->peerTyp(inL) < TRUSTED && lt->peerAdr(inL) != h.srcAdr())) {
		return false;
	}
	return true;
}

void lfsDaemon::rateCalc(int rate, int maxRate, int& result) {
// Given a desired rate and a maximum available rate,
// calculate the best matching rate that the system can support.
// This version requires rate reservations be at least
// 1 Mb/s. If the maxRate is less than 1 Mb/s, the result returned is zero,
// but if maxRate is at least 1 Mb/s and rate is less than 1 Mb/s,
// the result is 1 Mb/s. Note, that rates are expressed in Kb/s.
	if (maxRate < 1000) { result = 0; return; }
	result = min(rate, maxRate);
	result = max(result, 1000);
	return;
}

void lfsDaemon::handleOptions(wunet::sockInet &mnsock, mnpkt_t &mnpkt) {
// Do the LFS packet processing.
	uint32_t *bufp = getBufPntr(mnpkt);
	if (buf == NULL) return;
	uint32_t x = ntohl(buf[0]);
	int leng = x & 0xffff;

	header h;
	buffer_t *buf = reinterpret_cast<buffer_t*>(bufp);
	h.unpack(*buf);

	h.srcIp() = getTunnelSrcIp(mnpkt);
	h.srcPort() = getTunnelSrcPort(mnpkt);
	ipa_t localIp = getTunnelLocalIp(mnpkt);
	ipp_t localPort = getTunnelLocalPort(mnpkt);

	int intf = ioProc->lookup(localIp);
	int lnk = lt->lookup(intf,h.srcIp(),h.srcPort());
	if (lnk == 0) return;
	h.inLink() = lnk;

	if (!pktCheck(h) || h.hleng() != 7 ||
	    h.optCode() != LFS_OPTION || h.optLeng() != 8)
		return;

	// handle dynamic port number assignment from end systems
	if (h.dstAdr() == myAdr) {
		if (lt->peerType(lnk) == ENDSYS && h.lfsOp() == Control) {
			if (h.lfsFlags() == Connect &&
			    lt->peerPort(lnk) == 0)
				lt->setPeerPort(lnk,h.srcPort());
			else
			if (h.lfsFlags() == Disconnect &&
		    	    lt->peerPort(lnk) == h.srcPort())
				lt->setPeerPort(lnk,0);
		}
		return; // nothing to forward
	}

	ipa_t nhIp; ipp_t nhPort;
	if ((int lnk = options(h)) != 0) {
		h.pack(*buf); h.hdrErrUpdate(*buf);
		back2fp(mnsock, mnpkt, lt->peerIpAdr(lnk), lt->peerPort(lnk));
	}
}

int lfsDaemon::options(header& h) {
// Handle forwarding decisions for packets with LFS option.
// Return the link number on which the packet should be forwarded
// or 0 if it should be discarded.
	int lnk, qn, rte;
	ipa_t src = h.srcAdr(); ipa_t dst = h.dstAdr();
	int inLnk = h.inLink();
	int fte = ft->lookup(src, dst);
	int op = h.lfsOp();       int flags = h.lfsFlags();
	int rrate = h.lfsRrate(); int arate = h.lfsArate();
		
	switch (op) {
	case FirmReq:
		// if there is already a filter for this src/dst pair,
		// forward packet on the path, possibly adjusting rate
		if (fte != 0) {
			if (ft->inLink(fte) != inLnk) return 0;
			int lnk = ft->link(fte); 
			int qn = ft->qnum(fte);
			int frate = ft->rate(fte);
			if (frate != rrate) {
				// try to adjust to better match request
				int nuRate;
				rateCalc(rrate,avail[lnk],nuRate);
				if (nuRate != frate) {
					if (setQueueParams(fpq(lnk,qn),
							   nuRate,qSiz)) {
						ft->rate(fte) = nuRate;
						avail[lnk] -= (nuRate - frate);
						h.lfsArate() =min(arate,nuRate);
					}
				}
			}
			if (lt->peerTyp(lnk) == router) return lnk;
			return 0;
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
		if (lnk == 0) return false;
		fte = ft->addEntry(src,dst);
		if (fte == 0) { // forward as datagram
			lnk = rt->link(rte,1);
			if (lnk != inLnk && lt->peerTyp(lnk) == router)
				return lnk;
			return 0;
		}
		int nuRate;
		rateCalc(rrate,avail[lnk],nuRate);
		qn = 1+fte;
		ft->link(fte) = lnk; ft->qnum(fte) = qn;
		ft->rate(fte) = nuRate; avail[lnk] -= nuRate;
		ft->inLink(fte) = h.inLink();
		h.lfsArate() = min(arate,nuRate);
		setupFlowFilter(fte,iop->fpi(lt->interface(ft->inLink(fte))),
				src,dst,
				lt->peerIpAdr(lnk),lt->peerPort(lnk),qn,fte);
		setQueueParams(fpq(lnk,qn),nuRate,qSiz));
		if (lt->peerTyp(lnk) == router) return lnk
		return 0;
		break;
	case Release:
		// Remove the filter and forward the packet
		if (fte == 0) return 0;
		lnk = ft->link(fte); qn = ft->qnum(fte);
		avail[lnk] += ft->rate(fte);
		// Set src,dst fields of fastpath filter to 0, so it
		// won't match any packets
		setupFlowFilter(fte,iop->fpi(lt->interface(ft->inLink(fte))),
				0,0,
				lt->peerIpAdr(lnk),lt->peerPort(lnk),qn,fte);
		ft->removeEntry(fte);
		if (lt->peerTyp(lnk) == router) return lnk;
		return 0;
		break;
	default:
		// just forward any others
		if (fte == 0) {
			rte = rt->lookup(dst);
			if (rte == 0) return 0;
			lnk = rt->link(rte,1);
			if (lnk != inLnk && lt->peerTyp(lnk) == router)
				return lnk;
			return 0;
		}
		lnk = ft->link(fte);
		if (lt->peerTyp(lnk) == router) return lnk;
		return 0;
	}
}

