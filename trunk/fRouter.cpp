/** \file fRouter.cpp */

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
	ctt = new ComtTbl(nComts,myAdr,lt,qm);
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
        if (cttfs.fail() || !ctt->readTable(cttfs)) {
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
		int comt = ctt->getComtree(ctte);
		n = ctt->links(ctte,lnkvec,nLnks);
		for (i = 0; i < n; i++) {
			j = lnkvec[i];
			if (lt->peerTyp(j) == ROUTER &&
			    forest::zipCode(lt->peerAdr(j))
			    == forest::zipCode(myAdr))
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
	os << "Comtree Table\n\n"; ctt->writeTable(os); os << endl;
	os << "Routing Table\n\n" << *rt << endl;
	os << "Statistics\n\n" << *sm << endl;
}

bool fRouter::pktCheck(int p, int ctte) {
// Perform error checks on forest packet.
// Return true if all checks pass, else false.
	header& h = ps->hdr(p);
	// check version and  length
	if (h.version() != FOREST_VERSION) return false;
	if (h.leng() != h.ioBytes() || h.leng() < HDR_LENG) return false;

	int inL = h.inLink();
	if (inL == Null) return false;

	// extra checks for packets from untrusted peers
	if (lt->peerTyp(inL) < TRUSTED) {
		// check for spoofed source address
		if (lt->peerAdr(inL) != h.srcAdr()) return false;
		// and that destination restrictions are respected
		if (lt->peerDest(inL) != 0 && 
		    h.dstAdr() != lt->peerDest(inL) && h.dstAdr() != myAdr)
			return false;
		// don't let untrusted peers send network signal packets
		if (h.ptype() >= NET_SIG) return false;
		// and user signalling packets must be sent on comtrees 1-99
		if (h.ptype() >= CLIENT_SIG && h.comtree() > 100) return false;
	}
	// check for invalid comtrees
	if (!ctt->valid(ctte) || !ctt->inComt(ctte,inL)) return false;
	return true;
}

void fRouter::subUnsub(int p, int ctte) {
// Perform subscription processing on p.
// Ctte is the comtree table entry for the packet.
// First payload word contains add/drop counts;
// these must sum to at most 350 and must match packet length

	header& h = ps->hdr(p);
	uint32_t *pp = ps->payload(p);
	// add/remove branches from routes
	// if non-core node, also propagate requests upward as
	// appropriate
	int inlnk = h.inLink();
	// ignore subscriptions from the parent or core neighbors
	if (inlnk == ctt->getPlink(ctte) || ctt->isClink(ctte,inlnk)) {
		ps->free(p); return;
	}
	int comt = h.comtree();
	bool propagate = false;
	int rte; fAdr_t addr;

	// add subscriptions
	int addcnt = ntohl(pp[0]);
	if (addcnt < 0 || addcnt > 350 || (addcnt + 8)*4 > h.leng()) {
		ps->free(p); return;
	}
	for (int i = 1; i <= addcnt; i++) {
		addr = ntohl(pp[i]);
		if (forest::ucastAdr(addr)) continue;  // ignore unicast
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
	    (addcnt + dropcnt + 8)*4 > h.leng() ) {
		ps->free(p); return;
	}
	for (int i = addcnt + 2; i <= addcnt + dropcnt + 1; i++) {
		addr = ntohl(pp[i]);
		if (forest::ucastAdr(addr)) continue; // ignore unicast
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
	if (propagate && !ctt->getCoreFlag(ctte) && ctt->getPlink(ctte) != Null) {
		ps->payErrUpdate(p);
		if (qm->enq(p,ctt->getPlink(ctte),ctt->getQnum(ctte),now)) {
			return;
		}
	}
	ps->free(p); return;
}

void fRouter::multiSend(int p, int ctte, int rte) {
// Send multiple copies of a packet.
// Ctte and rte are the comtree and routing table entries
// respectively. Ctte is assumed to be defined, rte may not be.
	int qn, n;
	uint16_t lnkvec[2*nLnks];
	header& h = ps->hdr(p);

	if (forest::ucastAdr(h.dstAdr())) { // flooding a unicast packet
		qn = ctt->getQnum(ctte);
		if (forest::zipCode(myAdr) == forest::zipCode(h.dstAdr()))
			n = ctt->llinks(ctte,lnkvec,nLnks);
		else
			n = ctt->rlinks(ctte,lnkvec,nLnks);
	} else { // forwarding a multicast packet
		n = 0;
		qn = ctt->getQnum(ctte);
		if (rte != Null) {
			if (rt->qnum(rte) != 0) qn = rt->qnum(rte);
			n = rt->links(rte,lnkvec,nLnks);
		}
		n += ctt->clinks(ctte,&lnkvec[n],nLnks);
		if (ctt->getPlink(ctte) != Null &&
		    !ctt->isClink(ctte,ctt->getPlink(ctte)))
			lnkvec[n++] = ctt->getPlink(ctte);
	}

	if (n == 0) { ps->free(p); return; }

	int lnk; int inlnk = h.inLink();
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

/** Send packet back to sender.
 *  Set REQ_REPLY flag, flip address fields and put it in the right queue.
 */
void fRouter::returnToSender(packet p, int paylen) {
	header& h = ps->hdr(p);
	h.leng() = HDR_LENG + paylen + 4;

	fAdr_t temp = h.dstAdr(); h.dstAdr() = h.srcAdr(); h.srcAdr() = temp;
	ps->pack(p);

	int qn = ctt->getQnum(ctt->lookup(h.comtree()));
	if (!qm->enq(p,h.inLink(),qn,now)) ps->free(p);
}

/** Send an error reply to a control packet.
 *  Reply packet re-uses the request packet p and its buffer.
 *  The string *s is included in the reply packet.
 */
void fRouter::errReply(packet p, CtlPkt& cp, const char *s) {
	cp.rrType = NEG_REPLY; cp.setErrMsg(s);
	returnToSender(p,4*cp.pack());
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
void fRouter::handleCtlPkt(int p) {
	int ctte, rte;
	header& h = ps->hdr(p);
	int inL = h.inLink();
	buffer_t& b = ps->buffer(p);
	CtlPkt cp(ps->payload(p));


	// first handle special cases of CONNECT/DISCONNECT
	if (h.ptype() == CONNECT) {
		if (lt->peerPort(inL) == 0)
			lt->peerPort(inL) = h.tunSrcPort();
		ps->free(p); return;
	}
	if (h.ptype() == DISCONNECT) {
		if (lt->peerPort(inL) == h.tunSrcPort())
			lt->peerPort(inL) = 0;
		ps->free(p); return;
	}

	// then onto normal signalling packets
	int len = (h.leng() - 24)/4;
	if (!cp.unpack(len)) {
		cerr << "misformatted control packet";
		cp.print(cerr);
		errReply(p,cp,"misformatted control packet");
		return;
	}
	if (h.ptype() != NET_SIG || h.comtree() < 100 || h.comtree() > 999) {
		// reject signalling packets on comtrees outside 100-999 range
		ps->free(p); return;
	}
		
	// Prepare positive reply packet for use where appropriate
	CtlPkt cp1(ps->payload(p));
	cp1.cpType = cp.cpType; cp1.rrType = POS_REPLY; cp1.seqNum = cp.seqNum;

	switch (cp.cpType) {
	// Configuring logical interfaces
	case ADD_IFACE:
		if (iop->addEntry(cp.getAttr(IFACE_NUM),
				  cp.getAttr(LOCAL_IP),
		     		  cp.getAttr(MAX_BIT_RATE),
				  cp.getAttr(MAX_PKT_RATE))) {
			returnToSender(p,4*cp1.pack());
		} else {
			errReply(p,cp1,"add iface: cannot add interface");
		}
		break;
        case DROP_IFACE:
		iop->removeEntry(cp.getAttr(IFACE_NUM));
		returnToSender(p,4*cp1.pack());
		break;
        case GET_IFACE: {
		int32_t iface = cp.getAttr(IFACE_NUM);
		if (iop->valid(iface)) {
			cp1.setAttr(IFACE_NUM,iface);
			cp1.setAttr(LOCAL_IP,iop->ipAdr(iface));
			cp1.setAttr(MAX_BIT_RATE,iop->maxBitRate(iface));
			cp1.setAttr(MAX_PKT_RATE,iop->maxPktRate(iface));
			returnToSender(p,4*cp1.pack());
		} else errReply(p,cp1,"get iface: invalid interface");
		break;
	}
        case MOD_IFACE: {
		int32_t iface = cp.getAttr(IFACE_NUM);
		if (iop->valid(iface)) {
			int br = iop->maxBitRate(iface);
			int pr = iop->maxPktRate(iface);
			if (cp.isSet(MAX_BIT_RATE))
				iop->setMaxBitRate(iface,
						   cp.getAttr(MAX_BIT_RATE));
			if (cp.isSet(MAX_PKT_RATE))
				iop->setMaxPktRate(iface,
						   cp.getAttr(MAX_PKT_RATE));
			if (iop->checkEntry(iface)) {
				returnToSender(p,4*cp1.pack());
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
			returnToSender(p,4*cp1.pack());
		else errReply(p,cp1,"add link: cannot add link");
		break;
        case DROP_LINK:
		if (lt->removeEntry(cp.getAttr(LINK_NUM)))
			returnToSender(p,4*cp1.pack());
		else errReply(p,cp1,"drop link: cannot drop link");
        case GET_LINK: {
		uint32_t link = cp.getAttr(LINK_NUM);
		if (lt->valid(link)) {
			cp1.setAttr(LINK_NUM, link);
			cp1.setAttr(IFACE_NUM, lt->interface(link));
			cp1.setAttr(PEER_IP, lt->peerIpAdr(link));
			cp1.setAttr(PEER_TYPE, lt->peerTyp(link));
			cp1.setAttr(PEER_PORT, lt->peerPort(link));
			cp1.setAttr(PEER_DEST, lt->peerDest(link));
			cp1.setAttr(BIT_RATE, lt->bitRate(link));
			cp1.setAttr(PKT_RATE, lt->pktRate(link));
			returnToSender(p,4*cp1.pack());
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
				lt->peerTyp(link) = (ntyp_t) pt;
			}
			if (cp.isSet(PEER_PORT)) {
				int pp = cp.getAttr(PEER_PORT);
				if (pp > 65535) {
					errReply(p,cp1,"mod link:bad peerPort");
					return;
				}
				lt->peerPort(link) = pp;
			}
			if (cp.isSet(PEER_DEST)) {
				fAdr_t pd = cp.getAttr(PEER_DEST);
				if (!forest::ucastAdr(pd)) {
					errReply(p,cp1,"mod link:bad peerDest");
					return;
				}
				lt->peerDest(link) = pd;
			}
			if (cp.isSet(BIT_RATE) != 0)
				lt->bitRate(link) = cp.getAttr(BIT_RATE);
			if (cp.isSet(PKT_RATE) != 0)
				lt->pktRate(link) = cp.getAttr(PKT_RATE);
			returnToSender(p,4*cp1.pack());
		} else errReply(p,cp1,"get link: invalid link number");
		break; }
        case ADD_COMTREE:
		if (ctt->addEntry(cp.getAttr(COMTREE_NUM)) != Null)
			returnToSender(p,4*cp1.pack());
		else errReply(p,cp1,"add comtree: cannot add comtree");
		break;
        case DROP_COMTREE:
		ctte = ctt->lookup(cp.getAttr(COMTREE_NUM));
		if (ctte != Null && ctt->removeEntry(ctte) != Null)
			returnToSender(p,4*cp1.pack());
		else
			errReply(p,cp1,"drop comtree: cannot drop comtree");
		break;
        case GET_COMTREE: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		ctte = ctt->lookup(comt);
		if (ctte == Null) {
			errReply(p,cp1,"get comtree: invalid comtree");
		} else {
			cp1.setAttr(COMTREE_NUM,comt);
			cp1.setAttr(CORE_FLAG,ctt->getCoreFlag(ctte) ? 1 : -1);
			cp1.setAttr(PARENT_LINK,ctt->getPlink(ctte));
			cp1.setAttr(QUEUE_NUM,ctt->getQnum(ctte));
			returnToSender(p,4*cp1.pack());
		}
		break;
	}
        case MOD_COMTREE: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		ctte = ctt->lookup(comt);
		if (ctte != Null) {
			if (cp.isSet(CORE_FLAG) != 0)
				ctt->setCoreFlag(ctte,cp.getAttr(CORE_FLAG));
			if (cp.isSet(PARENT_LINK) != 0)
				ctt->setPlink(ctte,cp.getAttr(PARENT_LINK));
			if (cp.isSet(QUEUE_NUM) != 0)
				ctt->setQnum(ctte,cp.getAttr(QUEUE_NUM));
			returnToSender(p,4*cp1.pack());
		} else errReply(p,cp1,"modify comtree: invalid comtree");
		break;
	}

        case ADD_ROUTE:
		if (rt->addEntry(cp.getAttr(COMTREE_NUM),
				 cp.getAttr(DEST_ADR),
				 cp.getAttr(LINK_NUM),
				 cp.getAttr(QUEUE_NUM)) != Null) {
			returnToSender(p,4*cp1.pack());
		} else errReply(p,cp1,"add route: cannot add route");
		break;
        case DROP_ROUTE:
		rte = rt->lookup(cp.getAttr(COMTREE_NUM),cp.getAttr(DEST_ADR));
		if (rte != Null) {
			rt->removeEntry(rte);
			returnToSender(p,4*cp1.pack());
		} else errReply(p,cp1,"drop route: invalid route");
		break;
        case GET_ROUTE: {
		comt_t comt = cp.getAttr(COMTREE_NUM);
		fAdr_t da = cp.getAttr(DEST_ADR);
		rte = rt->lookup(comt,da);
		if (rte != Null) {
			cp1.setAttr(COMTREE_NUM,comt);
			cp1.setAttr(DEST_ADR,da);
			cp1.setAttr(LINK_NUM,rt->link(rte));
			cp1.setAttr(QUEUE_NUM,rt->qnum(rte));
			returnToSender(p,4*cp1.pack());
		} else errReply(p,cp1,"get route: invalid route");
		break;
	}
        case MOD_ROUTE:
		rte = rt->lookup(cp.getAttr(COMTREE_NUM),cp.getAttr(DEST_ADR));
		if (rte != Null) {
			if (cp.isSet(LINK_NUM))
				rt->setLink(rte,cp.getAttr(LINK_NUM));
			if (cp.isSet(QUEUE_NUM))
				rt->qnum(rte) = cp.getAttr(QUEUE_NUM);
			returnToSender(p,4*cp1.pack());
		} else errReply(p,cp1,"mod route: invalid route");
		break;
	default:
		cerr << "unrecognized control packet " << h.ptype();
		ps->free(p);
		break;
	}
	return;
}

void fRouter::handleRteReply(int p, int ctte) {
// Handle route reply packet p.
	header& h = ps->hdr(p);
	int rte = rt->lookup(h.comtree(), h.dstAdr());
	if ((h.flags() & RTE_REQ) && rte != Null) sendRteReply(p,ctte);
	int adr = ntohl(ps->payload(p)[0]);
	if (forest::ucastAdr(adr) &&
	    rt->lookup(h.comtree(),adr) == Null) {
		rt->addEntry(h.comtree(),adr,h.inLink(),0); 
	}
	if (rte == Null) {
		// send to neighboring routers in comtree
		h.flags() = RTE_REQ;
		ps->pack(p); ps->hdrErrUpdate(p);
		multiSend(p,ctte,rte);
		return;
	}
	if (rte != Null && lt->peerTyp(rt->link(rte)) == ROUTER &&
	    qm->enq(p,rt->link(rte),ctt->getQnum(ctte),now))
		return;
}

void fRouter::sendRteReply(int p, int ctte) {
// Send route reply back towards p's source.
	header& h = ps->hdr(p);
	int p1 = ps->alloc(); header& h1 = ps->hdr(p1);
	h1.leng() = HDR_LENG + 8;
	h1.ptype() = RTE_REPLY;
	h1.flags() = 0;
	h1.comtree() = h.comtree();
	h1.srcAdr() = myAdr;
	h1.dstAdr() = h.srcAdr();
	ps->pack(p1);
	ps->payload(p1)[0] = htonl(h.dstAdr());
	ps->hdrErrUpdate(p1); ps->payErrUpdate(p1);
	qm->enq(p1,h.inLink(),ctt->getQnum(ctte),now);
}

void fRouter::forward(int p, int ctte) {
// Lookup routing entry and forward packet accordingly.
// P is assumed to be a CLIENT_DATA packet.
	header& h = ps->hdr(p);
	int rte = rt->lookup(h.comtree(),h.dstAdr());

	if (rte != Null) { // valid route case
		// reply to route request
		if ((h.flags() & RTE_REQ)) {
			sendRteReply(p,ctte);
			h.flags() = h.flags() & (~RTE_REQ);
			ps->pack(p);
			ps->hdrErrUpdate(p);
		}
		if (forest::ucastAdr(h.dstAdr())) {
			int qn = rt->qnum(rte);
			if (qn == 0) qn = ctt->getQnum(ctte);
			int lnk = rt->link(rte);
			if (lnk != h.inLink() && qm->enq(p,lnk,qn,now))
				return;
			ps->free(p); return;
		}
		// multicast data packet
		multiSend(p,ctte,rte);
		return;
	}
	// no valid route
	if (forest::ucastAdr(h.dstAdr())) {
		// send to neighboring routers in comtree
		h.flags() = RTE_REQ;
		ps->pack(p); ps->hdrErrUpdate(p);
	}
	multiSend(p,ctte,rte);
	return;
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
	const int MAXEVENTS = 500;
	struct { int sendFlag; uint32_t time; int link, pkt;} events[MAXEVENTS];
	int evCnt = 0;
	int statsTime = 0;		// time statistics were last processed
	bool didNothing;
	int controlCount = 20;		// used to limit overhead of control
					// packet processing
	queue<int> ctlQ;		// queue for control packets

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
			header& h = ps->hdr(p);
			if (evCnt < MAXEVENTS &&
			    (h.ptype() != CLIENT_DATA || numData > 0)) {
				int p1 = ps->clone(p);
				events[evCnt].sendFlag = 0;
				events[evCnt].link = h.inLink();
				events[evCnt].time = now;
				events[evCnt].pkt = p1;
				evCnt++;
				if (h.ptype() == CLIENT_DATA) numData--;
			}
			int ctte = ctt->lookup(h.comtree());
			if (!pktCheck(p,ctte)) {
				ps->free(p);
			} else if (h.ptype() == CLIENT_DATA) {
				forward(p,ctte);
			} else if (h.ptype() == SUB_UNSUB) {
				subUnsub(p,ctte);
			} else if (h.ptype() == RTE_REPLY) {
				handleRteReply(p,ctte);
			} else {
				ctlQ.push(p);
			}
		}

		// output processing
		int lnk;
		while ((lnk = qm->nextReady(now)) != Null) {
			didNothing = false;
			p = qm->deq(lnk); header& h = ps->hdr(p);
			if (evCnt < MAXEVENTS &&
			    (h.ptype() != CLIENT_DATA || numData > 0)) {
				int p2 = ps->clone(p);
				events[evCnt].sendFlag = 1;
				events[evCnt].link = lnk;
				events[evCnt].time = now;
				events[evCnt].pkt = p2;
				evCnt++;
				if (h.ptype() == CLIENT_DATA) numData--;
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
		int p = events[i].pkt; header& h = ps->hdr(p);
		h.print(cout,ps->buffer(p));
	}
	cout << endl;
	cout << lt->iPktCnt(0) << " packets received, "
	     <<	lt->oPktCnt(0) << " packets sent\n";
	cout << lt->iPktCnt(-1) << " from routers,    "
	     << lt->oPktCnt(-1) << " to routers\n";
	cout << lt->iPktCnt(-2) << " from clients,    "
	     << lt->oPktCnt(-2) << " to clients\n";
}
