/* Notes
LfsDaemon is intended for use with the IPv4 fastpath. It is used in
lfs_fpd, (which is the LFS version of ip_fpd) to implement the  LFS
functionality. Lfs_fpd instantiates lfsDaemon and invokes init with
the names of the files that contain the intial configuration for the
LFS router (these are specified as command-line arguments to lfs_fpd).

There is one tricky issue. Fastpath filters are used for several
purposes. Within LFS, they are used to implement both routes and
flow filters. To simplify user-configuration of routes, we have
defined routes to be independent of the packet's arrival interface.
Unfortunately, this conflicts with the way fastpath filters work,
so we actually define a fastpath filter on every interface, for
each route that the user defines. We must also set aside some
fastpath filters for packets coming back from the GPE that need
to be directed to the outgoing links (one per link) and two filters
are used to re-direct local delivery and exception packets.

The handling of queues is also tricky. They must be bound to an
interface on startup and can't be easily re-bound. Consequently,
we need to pre-configure a queue for each flow filter we plan to
use on every link (in addition to the default datagram queue on
each link). That is, the total number of fastpath queues
must be at least (1 + # of flow filters)*(# of links).
In addition, there must be two more for local delivery
and exception packets.

Statistics counts are maintained for each filter and for each
each link. Packets that aren't forwarded using a filter are
counted in the stats for the link on which they are forwarded.
So, nFltrs+nLnks stats counts should
be reserved. The stats index for filter number i is i.
The stats index for link number i is nFltrs+i
*/

#include "lfsDaemon.h"

// Return the fastpath queue number for queue q, on link lnk
#define fpq(lnk,q) (((lnk)-1)*(nQus) + (q))

lfsDaemon::lfsDaemon(ipa_t myAdr1) : myAdr(myAdr1) {
	avail = new int[MAXLNK+1];
}

lfsDaemon::~lfsDaemon() {
	delete lt; delete ft; delete rt; delete iop;
	delete sm;
}

bool lfsDaemon::init(const char iftf[], const char ltf[],
		     const char ftf[], const char rtf[], const char smf[],
		     const int fp_bw, const int fp_fltrs,
		     const int fp_qus, const int fp_bufs, const int fp_stats) {
// Initialize ioProc, link table, filter table and routing table from
// files with names iftf, ltf, ftf and rtf. Perform consistency checks.
// Return true on success, false on failure.
        ifstream iftfs, ltfs, ftfs, rtfs, smfs;

	fpBw = fp_bw; fpFltrs = fp_fltrs; fpQus = fp_qus;
	fpBufs = fp_bufs; fpStats = fp_stats;

	wulog(wulogClient, wulogInfo,
	      "initializing lfs daemon with fp_bw=%d, fp_fltrs=%d, "
	      "fp_qus=%d, fp_bufs=%d, fp_stats=%d\n", fp_bw, fp_fltrs,
	      fp_qus, fp_bufs, fp_stats);

	// define link table with max possible number of links (31)
	// interface table is defined for at most 20 interfaces
	lt = new lnkTbl(MAXLNK);
	iop = new ioProc(lt);

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

	// determine of configured links and interfaces
	// use max configured link number and interface number
	nLnks = nIntf = 0;
	for (int i = 1; i <= MAXLNK; i++) {
		if (lt->valid(i)) nLnks = i;
		if (iop->valid(i)) nIntf = i;
	}
	wulog(wulogClient, wulogInfo,
	      "Configuring %d interfaces and %d LFS links\n",nIntf,nLnks);

	// setup route table for 100 routes defined in external file
	rt = new rteTbl(100,myAdr,lt);

        rtfs.open(rtf);
        if (rtfs.fail() || !(rtfs >> *rt)) {
		cerr << "lfsDaemon::init: can't read routing table\n";
		return false;
	}
        rtfs.close();
	// determine number of configured routes
	nRts = 0;
	for (int i = 1; i <= 100; i++) if (rt->valid(i)) nRts++;
	// add nLnks more to account for routes to neighbors
	nRts += nLnks;

	// Each link needs its own set of fastpath queues.
	// The first will be used for datagram traffic, the remainder
	// are used by flow filters, with each flow filter having an
	// assigned queue on each link.
	// The last two queues are reserved for use by the local delivery
	// and exception traffic going from the fastpath to the GPE.
	// Also, we number queues from 1, while fastpath numbers from 0,
	// so we reduce the number by one more.
	nQus = (fpQus - 3)/nLnks;

	// number of fastpath filters must be at least
	// (# of interfaces)*(# of routes) + (# of links + 2)
	// + (# of flow filters)
	// So, get # of flow filters by subtracting off other demands.
	// If this is larger than nQus-1, adjust down.
	// If smaller, adjust nQus down.
	nFltrs = fp_fltrs - (nIntf*nRts + nLnks + 2);
	nFltrs = min(nFltrs, nQus-1);
	nQus = min(nQus, nFltrs+1); qSiz = fpBufs/(nLnks + (nQus-1) + 2);
	wulog(wulogClient, wulogInfo,
	      "Using %d queues per LFS link with length %d\n",nQus,qSiz);

	if (nFltrs < 2) fatal("not enough fastpath filters");
	wulog(wulogClient, wulogInfo,
	      "Skipping reading of filter table (not fully implemented) \n");
	ft = new fltrTbl(nFltrs,myAdr,lt);
        // ftfs.open(ftf);
        // if (ftfs.fail() || !(ftfs >> *ft)) {
	// 	cerr << "lfsDaemon::init: can't read filter table\n";
	// 	return false;
	// }
        // ftfs.close();
	wulog(wulogClient, wulogInfo,
	      "Configuring %d routes and %d flow filters\n",nRts,nFltrs);

	// setup table for stats maintained by daemon
	// at the moment, no mechanism for actually writing out stats
	sm = new statsMod(100,lt,avail);
        smfs.open(smf);
        if (smfs.fail() || !(smfs >> *sm)) {
		cerr << "lfsDaemon::init: can't read stats spec\n";
		return false;
	}
        smfs.close();
	if (fpStats < nRts + nFltrs) fatal("not enough fastpath stats indices");
	wulog(wulogClient, wulogInfo, "Using %d stats indices\n",nFltrs+nLnks);

	return true;
}

bool lfsDaemon::setup() {

	addInterfaces();

	addQueues();

	addRoutesFilters();

	addBypass();

	return true;
}

void putIpAdr(ostream& os, ipa_t adr) {
	os << ((adr >> 24) & 0xff) << "."
	   << ((adr >> 16) & 0xff) << "."
	   << ((adr >>  8) & 0xff) << "."
	   << ((adr)       & 0xff);
}

char* ipAdrStr(ipa_t adr, char *buf) {
	sprintf(buf,"%d.%d.%d.%d",
		(adr >> 24) & 0xff, (adr >> 16) & 0xff,
		(adr >> 8) & 0xff, adr & 0xff);
	return buf;
}

void lfsDaemon::addInterfaces() {
// Configure a separate fastpath interface for each interface
// specified in the interface table.

	for (int i = 1; i <= MAXLNK; i++) {
		if (!iop->valid(i)) continue;
		char buf[100];
		wulog(wulogClient, wulogInfo,
		      "Setting up interface %d (%s:%d) with bw=%d Kb/s\n",
		      i,ipAdrStr(iop->ipAdr(i),buf),
		      LFS_PORT,iop->maxBitRate(i));
		int ifn = setupFpInt(iop->ipAdr(i),LFS_PORT,iop->maxBitRate(i));
		if (ifn < 0) fatal("can't configure fastpath interfaces");
		iop->fpi(i) = ifn;
	}
}

void lfsDaemon::addQueues() {
// Bind queues to interfaces and set datagram queue bandwidths.
// Note, there is a set of queues for each LFS link and there may
// be multiple links per interface.
	for (int i = 1; i <= MAXLNK; i++) {
		if (!lt->valid(i)) continue;
		wulog(wulogClient, wulogInfo,
			"Binding queues %d-%d to link %d on intface %d (%d)\n",
			fpq(i,1), fpq(i,nQus), i,
			lt->interface(i), iop->fpi(lt->interface(i)));
		if (!bindQueue(fpq(i,1),fpq(i,nQus),iop->fpi(lt->interface(i))))
			fatal("can't bind queues to interface");
		int dgRate = static_cast<int>((0.2) * lt->bitRate(i));
		rateCalc(dgRate,dgRate,dgRate);
		// allocate 1 Mb/s to datagram traffic in fastpath
		// when dgRate is larger than 1 Mb/s
		setQueueParams(fpq(i,1),min(1000,dgRate),qSiz);
		avail[i] = max(0,lt->bitRate(i) - dgRate);
		wulog(wulogClient, wulogInfo,
			"Available reserved bandwidth on link %d is %d Kb/s\n",
			i, avail[i]);
	}
	// Set queue parameters for local delivery and exception queues.
	// These are configured automatically using last two qids.
	// Note that fastpath queue ids are numbered from 0.
	setQueueParams(fpQus-2,1000,qSiz);
	setQueueParams(fpQus-1,1000,qSiz);
	wulog(wulogClient, wulogInfo,
		"Configuring local delivery and exception queues (%d, %d)\n",
		fpQus-2, fpQus-1);
}

void lfsDaemon::addRoutesFilters() {
// Add routes for all neighbors. All have prefix length 32 and a single link.
// FP filters are allocated among flow filters, routes and bypass.
// So, the first nFltrs fp filters are used for flow filters,
// the next nRts*nIntf are used for routes and the next nLnks
// are used for bypass.

	for (int lnk = 1; lnk <= MAXLNK; lnk++) {
		if (!lt->valid(lnk)) continue;
		int rte = rt->lookup(lt->peerAdr(lnk));
		if (rte != 0 && rt->prefLeng(rte) == 32) continue;
		rte = rt->addEntry(lt->peerAdr(lnk),32);
		if (rte == Null)
			fatal("lfsDaemon::addRoutesFilters: can't add route.");
		rt->link(rte,1) = lnk;
		for (int i = 2; i <= rteTbl::maxNhops; i++)
			rt->link(rte,i) = 0;
		char buf[100];
		wulog(wulogClient, wulogInfo,
			"Adding route %d to %s on link %d\n",
			rte,ipAdrStr(lt->peerAdr(lnk),buf),lnk);
	}
	rt->sort(); // sort routes by prefix length
	// Now install default route for each entry in route table
	int fnum = nFltrs+1;
	for (int rte = 1; rte <= nRts; rte++) {
		if (!rt->valid(rte)) continue;
		int lnk = rt->link(rte,1);
		rt->fpf(rte) = fnum; // # of first filter for this route
		for (int i = 1; i <= MAXLNK; i++) {
			// setup specified routes for all "input" interfaces
			if (!iop->valid(i)) continue;
			char rteStr[100]; ipAdrStr(rt->prefix(rte),rteStr);
			wulog(wulogClient, wulogInfo,
				"Configuring fp filter %d for interface %d(%d)"
				" route %s/%d queue=%d stats=%d\n",
				fnum,i,iop->fpi(i),rteStr,rt->prefLeng(rte),
				fpq(lnk,1), nFltrs+lnk);
			if (!setupRoute(fnum,iop->fpi(i),
					rt->prefix(rte),rt->prefLeng(rte),
					lt->peerIpAdr(lnk), lt->peerPort(lnk),
			        	fpq(lnk,1),nFltrs+lnk))
				fatal("can't configure filters for routes");
			fnum++;
		}
	}
	// Now install filters that are specified in filter table
	// Skip until implementation is complete. Need to decide how
	// to handle queues. User-specified queue numbers for filters
	// could conflict with automatically allocated queue numbers.
	/*
	for (int fte = 1; fte <= nFltrs; fte++) {
		if (!ft->valid(fte)) continue;
		int lnk = ft->link(fte);
		setupFlowFilter(fte,ft->inLink(fte),ft->src(fte),ft->dst(fte),
			        lt->peerIpAdr(lnk),lt->peerPort(lnk),
			        fpq(lnk,ft->qnum(fte)),fte);
		ft->fpf(fte) = fte;
		wulog(wulogClient, wulogInfo,
			"Setting up filter %d using fp filter %d\n", fte, fte);
	}
	*/
}

void lfsDaemon::addBypass() {
// For each LFS link, setup a fastpath bypass filter, to route
// packets received from the GPE to the specified output link
// using the datagram queue for that link.
	for (int i = 1; i <= MAXLNK; i++) {
		if (!lt->valid(i)) continue;
		if (!setupBypassFilter(nFltrs+nRts*nIntf+i,lt->peerIpAdr(i),
					LFS_PORT, fpq(i,1),nFltrs+i))
			fatal("can't configure bypass filters");
		wulog(wulogClient, wulogInfo,
			"Adding bypass filter for link %d using fp "
			"filter %d lt->peerIpAdr: 0x%lx LFS_PORT: %d "
			"fpq(i,1): %d stats: %d\n",
			i, nFltrs+nRts*nIntf+i, lt->peerIpAdr(i),
			LFS_PORT, fpq(i,1), nFltrs+i);
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

bool lfsDaemon::handleConnectDisconnect(header& h) {
// Handle a connection packet - that is, set the peer port number for
// the link to the source port number specifiec in the packet.
	int lnk = h.inLink();
	
	if (h.lfsFlags() == Connect && lt->peerPort(lnk) == 0) {
		wulog(wulogClient, wulogInfo, "Processing Connect\n");
		lt->setPeerPort(lnk,h.srcPort());
	} else if (h.lfsFlags() == Disconnect &&
	           lt->peerPort(lnk) == h.srcPort()) {
		wulog(wulogClient, wulogInfo, "Processing Disconnect\n");
		lt->setPeerPort(lnk,0);
	} else return false;

	// now, modify fastpath filters that implement route to this endsys
	// first find route number
	int rte = rt->lookup(lt->peerAdr(lnk));
	if (rte == 0) {
		wulog(wulogClient, wulogInfo, "no route - dropping packet\n");
		return false;
	}
	// Fastpath filters for this route have indices
	// rt->fpf(rte) .. rt->fpf(rte)+(nIntf-1)
	char abuf[100];
	wulog(wulogClient, wulogInfo,
	      "Updating filter result for FP filters %d-%d to use port %d\n",
	      rt->fpf(rte), rt->fpf(rte)+(nIntf-1),lt->peerPort(lnk));
	int fnum = rt->fpf(rte);
	for (int i = 1; i <= MAXLNK; i++) {
		if (!lt->valid(i)) continue;
		if (!updateFlowFilterResult(fnum, lt->peerIpAdr(lnk),
		     lt->peerPort(lnk),fpq(lnk,1),nFltrs+lnk)) {
			wulog(wulogClient, wulogInfo,
			      "update failed, dropping packet\n");
			return false;
		}
		fnum++;
	}
	return true;
}

void lfsDaemon::handleOptions(wunet::sockInet &mnsock, mnpkt_t &mnpkt) {
// Do the LFS packet processing.
	uint32_t *bufp = getBufPntr(mnpkt);
	if (bufp == NULL) return;
	uint32_t x = ntohl(bufp[0]);
	int leng = x & 0xffff;

	header h;
	buffer_t *buf = reinterpret_cast<buffer_t*>(bufp);
	h.unpack(*buf);

	h.srcIp() = ntohl(getTunnelSrcIp(mnpkt));
	h.srcPort() = ntohs(getTunnelSrcPort(mnpkt));
	ipa_t localIp = ntohl(getTunnelLocalIp(mnpkt));
	ipp_t localPort = ntohs(getTunnelLocalPort(mnpkt));

	char tunSrcIpStr[100], tunLocalIpStr[100];
	ipAdrStr(h.srcIp(),tunSrcIpStr); ipAdrStr(localIp,tunLocalIpStr);
	wulog(wulogClient, wulogInfo,
	      "Processing LFS packet on tunnel %s:%d to %s:%d\n",
	      tunSrcIpStr,h.srcPort(),tunLocalIpStr,localPort);

	int intf = iop->lookup(localIp);
	int lnk = lt->lookup(intf,h.srcIp(),h.srcPort(),h.srcAdr());
	
	wulog(wulogClient, wulogInfo, "Packet Received on LFS link %d\n", lnk);
	char srcAdrStr[100], dstAdrStr[100];
	ipAdrStr(h.srcAdr(),srcAdrStr); ipAdrStr(h.dstAdr(),dstAdrStr);
	wulog(wulogClient,wulogInfo,"SrcAdr=%s DstAdr=%s\n",
				     srcAdrStr,dstAdrStr);

	if (lnk == 0) return;
	h.inLink() = lnk;

	if (!pktCheck(h) || h.hleng() != 7 ||
	    h.optCode() != LFS_OPTION || h.optLeng() != 8)
		return;

	wulog(wulogClient, wulogInfo, "Packet passed basic checks\n");

	// handle dynamic port number assignment from end systems
	if (h.dstAdr() == myAdr) {
		wulog(wulogClient, wulogInfo, "Packet addressed to router\n");
		if (lt->peerTyp(lnk) == ENDSYS && h.lfsOp() == Control) {
			handleConnectDisconnect(h);
		}
		return; // nothing to forward in this case
	}

	wulog(wulogClient,wulogInfo,"Handling reservation packet\n");
	ipa_t nhIp; ipp_t nhPort;
	if ((lnk = options(h)) != 0) {
		h.pack(*buf); h.hdrErrUpdate(*buf);
cerr << "\nFilter Table\n\n" << *ft << endl;
		wulog(wulogClient,wulogInfo,"Forwarding packet to fastpath\n");
		char tunDstIpStr[100]; ipAdrStr(lt->peerIpAdr(lnk),tunDstIpStr);
		wulog(wulogClient,wulogInfo,
		      "lnk=%d peerIpAdr=%s peerPort=%d\n",
		      lnk, tunDstIpStr, lt->peerPort(lnk));
		back2fp(mnsock, mnpkt,
			lt->peerIpAdr(lnk), lt->peerPort(lnk), LFS_PORT);
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
		wulog(wulogClient,wulogInfo,
		      "Reservation requesting %d %d\n",rrate,arate);
		// if there is already a filter for this src/dst pair,
		// forward packet on the path, possibly adjusting rate
		if (fte != 0) {
			wulog(wulogClient,wulogInfo, "Existing filter\n");
			if (ft->inLink(fte) != inLnk) return 0;
			int lnk = ft->link(fte); 
			int qn = ft->qnum(fte);
			int frate = ft->rate(fte);
			if (frate != rrate) {
				// try to adjust to better match request
				int nuRate;
				rateCalc(rrate,avail[lnk]+frate,nuRate);
				if (nuRate != frate) {
					if (setQueueParams(fpq(lnk,qn),
							   nuRate,qSiz)) {
						ft->rate(fte) = nuRate;
						avail[lnk] -= (nuRate - frate);
						h.lfsArate() =min(arate,nuRate);
						wulog(wulogClient,wulogInfo,
						      "Changing rate to %d\n",
						      nuRate);
					}
				}
			}
			if (lt->peerTyp(lnk) == ROUTER) return lnk;
			wulog(wulogClient,wulogInfo,
			      "Next hop is endsys, so packet dropped\n");
			return 0;
		}
		// since there is no filter, need to add one and allocate
		// as much of the requested bandwidth as possible
		wulog(wulogClient,wulogInfo,"Adding new filter\n");
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
			if (lnk != inLnk && lt->peerTyp(lnk) == ROUTER)
				return lnk;
			return 0;
		}
		int nuRate;
		rateCalc(rrate,avail[lnk],nuRate);
		qn = 1+fte; // add 1 to skip datagram queue
		ft->link(fte) = lnk; ft->qnum(fte) = qn;
		ft->rate(fte) = nuRate; avail[lnk] -= nuRate;
		ft->inLink(fte) = h.inLink();
		ft->fpf(fte) = fte; // fp filter numbers match fte
		h.lfsArate() = min(arate,nuRate);
		wulog(wulogClient,wulogInfo,
		      "Adding filter %d rate=%d link=%d queue=%d(%d)"
		      " stats=%d rate=%d qSiz=%d\n",
		      ft->fpf(fte),nuRate,lnk,qn,fpq(lnk,qn),fte,nuRate,qSiz);
		if (setupFlowFilter(ft->fpf(fte),
		    iop->fpi(lt->interface(ft->inLink(fte))),src,dst,
		    lt->peerIpAdr(lnk),lt->peerPort(lnk),fpq(lnk,qn),fte)) {
			if (!setQueueParams(fpq(lnk,qn),nuRate,qSiz))
				wulog(wulogClient,wulogInfo,
					"setQueueParams failed\n");
		} else {
			wulog(wulogClient,wulogInfo,"setupFlowFiltr failed\n");
		}
		if (lt->peerTyp(lnk) == ROUTER) return lnk;
		wulog(wulogClient,wulogInfo,
		      "Next hop is endsys, so packet dropped\n");
		return 0;
		break;
	case Release:
		// Remove the filter and forward the packet
		wulog(wulogClient,wulogInfo,"Releasing reservation\n");
		if (fte == 0) return 0;
		lnk = ft->link(fte); qn = ft->qnum(fte);
		avail[lnk] += ft->rate(fte);
		// Set src,dst fields of fastpath filter to 0, so it
		// won't match any packets
		// set queue rate in fast path to zero
		setQueueParams(fpq(lnk,ft->qnum(fte)),0,qSiz);
		removeFilter(ft->fpf(fte));
		ft->removeEntry(fte);
		if (lt->peerTyp(lnk) == ROUTER) return lnk;
		wulog(wulogClient,wulogInfo,
		      "Next hop is endsys, so packet dropped\n");
		return 0;
		break;
	default:
		// just forward any others
		wulog(wulogClient,wulogInfo,"Forward without processing\n");
		if (fte == 0) {
			rte = rt->lookup(dst);
			if (rte == 0) return 0;
			lnk = rt->link(rte,1);
			if (lnk != inLnk && lt->peerTyp(lnk) == ROUTER)
				return lnk;
			return 0;
		}
		lnk = ft->link(fte);
		if (lt->peerTyp(lnk) == ROUTER) return lnk;
		wulog(wulogClient,wulogInfo,
		      "Next hop is endsys, so packet dropped\n");
		return 0;
	}
}
