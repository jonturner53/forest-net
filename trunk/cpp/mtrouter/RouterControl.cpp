/** @file RouterControl.cpp 
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RouterControl.h"

using namespace forest;

RouterControl::RouterControl(Router *rtr1, int thx,
			Quu<int> inQ1, Quu<pair<int,int>> outQ1) 
			: rtr(rtr1), myThx(thx), inQ(inQ1), outQ(outQ1) {
}

void RouterControl::start(RouterControl *self) { self->run(); }
	
void RouterControl::run() {
	while (true) {
		pktx px = inQ.deq(); // wait for incoming request packet
		Packet& p = rtr->ps->getPacket(px);
		if (p.type != Forest::CLIENT_SIG && p.type != Forest::NET_SIG)
			return;
		CtlPkt cp(p);
		handleRequest(px,cp);
	}
}
	
/** Handle incoming signalling requests addressed to the router.
 *  Assumes packet has passed all basic checks.
 *  @param px is the index of some packet
 *  @param cp is a reference to a control packet unpacked from the payload
 */
void RouterControl::handleRequest(int px, CtlPkt& cp) {
	Packet& p = rtr->ps->getPacket(px);
	
	switch (cp.type) {

	// configuring logical interfaces
	case CtlPkt::ADD_IFACE: 	addIface(cp,rcp); break;
        case CtlPkt::DROP_IFACE:	dropIface(cp,rcp); break;
        case CtlPkt::GET_IFACE:		getIface(cp,rcp); break;
        case CtlPkt::MOD_IFACE:		modIface(cp,rcp); break;
    	case CtlPkt::GET_IFACE_SET:	getIfaceSet(cp,rcp); break;

	// configuring links
        case CtlPkt::ADD_LINK:		addLink(cp,rcp); break;
        case CtlPkt::DROP_LINK:		dropLink(cp,rcp); break;
        case CtlPkt::GET_LINK:		getLink(cp,rcp); break;
        case CtlPkt::MOD_LINK:		modLink(cp,rcp); break;
        case CtlPkt::GET_LINK_SET:	getLinkSet(cp,rcp); break;

	// configuring comtrees
        case CtlPkt::ADD_COMTREE:	addComtree(cp,rcp); break;
        case CtlPkt::DROP_COMTREE:	dropComtree(cp,rcp); break;
        case CtlPkt::GET_COMTREE:	getComtree(cp,rcp); break;
        case CtlPkt::MOD_COMTREE:	modComtree(cp,rcp); break;
	case CtlPkt::GET_COMTREE_SET: 	getComtreeSet(cp,rcp); break;

	case CtlPkt::ADD_COMTREE_LINK:	addComtreeLink(cp,rcp); break;
	case CtlPkt::DROP_COMTREE_LINK: dropComtreeLink(cp,rcp); break;
	case CtlPkt::GET_COMTREE_LINK:	getComtreeLink(cp,rcp); break;
	case CtlPkt::MOD_COMTREE_LINK:	modComtreeLink(cp,rcp); break;

	// configuring routes
        case CtlPkt::ADD_ROUTE:		addRoute(cp,rcp); break;
        case CtlPkt::DROP_ROUTE:	dropRoute(cp,rcp); break;
        case CtlPkt::GET_ROUTE:		getRoute(cp,rcp); break;
        case CtlPkt::MOD_ROUTE:		modRoute(cp,rcp); break;
    	case CtlPkt::GET_ROUTE_SET:	getRouteSet(cp,rcp); break;

	// configuring filters and retrieving pacets
        case CtlPkt::ADD_FILTER:	addFilter(cp,rcp); break;
        case CtlPkt::DROP_FILTER:	dropFilter(cp,rcp); break;
        case CtlPkt::GET_FILTER:	getFilter(cp,rcp); break;
        case CtlPkt::MOD_FILTER:	modFilter(cp,rcp); break;
        case CtlPkt::GET_FILTER_SET:	getFilterSet(cp,rcp); break;
        case CtlPkt::GET_LOGGED_PACKETS: getLoggedPackets(cp,rcp); break;
        case CtlPkt::ENABLE_PACKET_LOG:	enablePacketLog(cp,rcp); break;

	// setting parameters
	case CtlPkt::SET_LEAF_RANGE:	setLeafRange(cp,rcp); break;

	// comtree setup
	case CtlPkt::JOIN:		joinComtree(cp); break
	case CtlPkt::LEAVE:		leaveComtree(cp); break
	case CtlPkt::ADD_BRANCH:	addBranch(cp); break
	case CtlPkt::PRUNE:		prune(cp); break
	case CtlPkt::CONFIRM:		confirm(cp); break
	case CtlPkt::ABORT:		abort(cp); break

	default:
		cerr << "unrecognized control packet type " << cp.type
		     << endl;
		rcp.errMsg = "invalid control packet for router";
		rcp.mode = CtlPkt::NEG_REPLY;
		break;
	}
	returnToSender(px,cp);
	return;
}

/** Send packet back to sender.
 *  Update the length, flip the addresses and pack the buffer.
 *  @param px is the packet number
 *  @param cp is a control packet to be packed
 */
void RouterControl::returnToSender(pktx px, CtlPkt& cp) {
	Packet& p = rtr->ps->getPacket(px);
	p.length = Packet::OVERHEAD + cp.paylen;
	p.length = (p.length + 3)/4; // round up to next multiple of 4
	p.flags = 0;
	p.dstAdr = p.srcAdr;
	p.srcAdr = rtr->myAdr;
	p.pack();
	outQ.enq(pair<int,int>(myThx,px));
}

/** Handle an ADD_IFACE control packet.
 *  Adds interface specified in the control packet.
 *  @param cp is the control packet structure for some received packet;
 *  on return, it is modified to form the reply
 */
void RouterControl::addIface(CtlPkt& cp) {
	int iface; ipa_t ip; RateSpec rates;
	if (!cp.xtrAddIface(iface, ip, rates)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	rates.bitRateUp = max(min(rates.bitRateUp,
				Forest::MAXBITRATE), Forest::MINBITRATE),
	rates.bitRateDown = max(min(rates.bitRateDown,
				Forest::MAXBITRATE), Forest::MINBITRATE),
	rates.pktRateUp = max(min(rates.pktRateUp,
				Forest::MAXBITRATE), Forest::MINBITRATE),
	rates.pktRateDown = max(min(rates.pktRateDown,
				Forest::MAXBITRATE), Forest::MINBITRATE),

	unique_lock iftLock(rtr->iftMtx);
	if (rtr->ift->valid(iface)) {
		cp.fmtError("addIface: requested interface "
			    "conflicts with existing interface");
		return;
	} else if (!rtr->ift->addEntry(iface, cp.ip1, 0, rs)) {
		cp.fmtError("addIface: cannot add interface");
		return;
	} else if (!rtr->setupIface(iface)) {
		cp.fmtError("addIface: could not setup interface");
		return;
	}
	IfaceTable::Entry& ifte = rtr->ift->getEntry(iface);
	cp.fmtAddIfaceReply(ifte.ipa, ifte.port);
}

/** Handle a DROP_IFACE control packet.
 *  Drops a specified interface.
 *  @param cp is the control packet structure for some packet;
 *  on return, it is modified to form the reply
 */
void RouterControl::dropIface(CtlPkt& cp) {
	int iface; if (!cp.xtrDropIface(iface)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	unique_lock iftLock(rtr->iftMtx);
	rtr->ift->removeEntry(iface);
	cp.fmtDropIfaceReply();
}

/** Handle an GET_IFACE control packet.
 *  Gets the information describing a specified interface.
 *  @param cp is the control packet structure for some received packet;
 *  on return, it is modified to form the reply
 */
void RouterControl::getIface(CtlPkt& cp) {
	int iface; if (!cp.xtrGetIface(iface)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	unique_lock iftLock(rtr->iftMtx);
	if (rtr->ift->valid(iface)) {
		IfaceTable::Entry& ifte = rtr->ift->getEntry(iface);
		cp.fmtGetIfaceReply(iface, ifte.ipa, ifte.port,
				    ifte.rates, ifte.availRates);
		return;
	}
	rcp.fmtError("get iface: invalid interface");
}

/** Handle a MOD_IFACE control packet.
 *  Modifies an interface as specified in the control packet;
 *  only the rate spec for the interface can be changed.
 *  @param cp is the control packet structure for some received packet;
 *  on return, it is modified to form the reply
 */
void RouterControl::modIface(CtlPkt& cp) {
	int iface; RateSpec rates;
	if (!cp.xtrModIface(iface, rates)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	unique_lock iftLock(rtr->iftMtx);
	if (rtr->ift->valid(iface)) {
		IfaceTable::Entry& ifte = rtr->ift->getEntry(iface);
		ifte.rates = rates
		cp.fmtModIfaceReply(); return;
	}
	cp.fmtError("mod iface: invalid interface");
}

/** Respond to a get iface set control packet.
 *  Control packet includes the first iface in the set to be retrieved plus
 *  a count of the number of ifaces to be returned; reply includes the
 *  the count of the number of ifaces returned, the index of the next iface
 *  in the table, following the last one in the returned set (or 0, if no more),
 *  plus a string listing the returned interfaces
 *  @param cp is the control packet structure for some received packet;
 *  on return, it is modified to form the reply
 */
void RouterControl::getIfaceSet(CtlPkt& cp) {
	int iface, count;
	if (!cp.xtrGetIfaceSet(iface, count)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	unique_lock iftLock(rtr->iftMtx);
	if (iface == 0) iface = rtr->ift->firstIface(); // 0 means 1st iface
	else if (!rtr->ift->valid(iface)) {
		cp.fmtError("get iface set: invalid iface number");
		return;
	}
	count = min(10, count);
	int i = 0; string s;
	while (i < count && iface != 0) {
		s.append(to_string(ifIndex) + " "
			 + rtr->ift->entry2string(ifIndex));
		if (s.length() > 1300) {
			cp.fmtError("getIfaceSet: reply string too long");
			return
		}
		i++; iface = rtr->ift->nextIface(iface);
	}
	cp.fmtGetIfaceSetReply(i, iface, s);
}

void RouterControl::addLink(CtlPkt& cp) {
	Forest::ntyp_t ntyp; int iface, lnk; ipa_t peerIp; ipp_t peerPort;
	fAdr_t peerAdr; uint64_t nonce;
	if (!cp.xtrAddLink(ntyp, iface, lnk, peerIp, peerPort, peerAdr, nonce)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	if (peerType == Forest::ROUTER && peerAdr == 0) {
		cp.fmtError("add link: adding link to router, but no peer "
				"address supplied");
		return;
	}

	// lock both iface table and link table
	unique_lock iftLock(rtr->iftMtx,defer_lock);
	unique_lock  ltLock( rtr->ltMtx,defer_lock);
	lock(iftLock, ltLock);

	if (rtr->lt->lookup(peerIp, peerPort) != 0 ||
	    (lnk != 0 && rtr->lt->valid(lnk))) {
		cp.fmtError("add link: new link conflicts with existing link");
		return;
	}

	IfaceTable::Entry& ifte = rtr->ift->getEntry(iface);

	// first ensure that the interface has enough
	// capacity to support a new link of minimum capacity
	RateSpec rs(Forest::MINBITRATE, Forest::MINBITRATE,
		    Forest::MINPKTRATE, Forest::MINPKTRATE);
	if (!rs.leq(ifte.availRates)) {
		cp.fmtError("add link: requested link "
				"exceeds interface capacity");
		return;
	}

	// setting up link
	// case 1: adding link to leaf; peer (ip,port) not known, use nonce
	//	   subcase - lnk, peer forest address is known
	//		     (for preconfigured leaf)
	//	   subcase - lnk, peer address to be assigned by router
	// case 2: adding link to router not yet up; port not known, use nonce
	//	   lnk, peer address specified
	// case 3: adding link to a router that is already up
	//	   lnk, peer address specified, peer (ip,port) specified

	// add table entry with (ip,port) or nonce
	// note: when rtr->lt->addEntry succeeds, link rates are
	// initialized to Forest minimum rates
	lnk = rtr->lt->addEntry(lnk,peerIp,peerPort,nonce);
	if (lnk == 0) {
		cp.fmtError("add link: cannot add requested link");
		return;
	}
	LinkTable::Entry& lte = rtr->lt->getEntry(lnk);

	if (peerType == Forest::ROUTER) {
		lte.peerAdr = peerAdr;
	} else { // case 1
		lte.peerAdr = 0;
		if (peerAdr == 0) lte.peerAdr = rtr->allocLeafAdr();
		else if (rtr->allocLeafAdr(peerAdr)) lte.peerAdr = peerAdr;
		if (lte.peerAdr == 0) {
			rtr->lt->removeEntry(lnk);
			cp.fmtError("add link: cannot add link using "
					"specified address";
			return;
		}
	}
	
	ifte.availRates.subtract(rs);
	lte.iface = iface;
	lte.peerType = peerType;
	lte.isConnected = false;
	sm->clearLnkStats(lnk);
	if (peerType == Forest::ROUTER && peerIp != 0 && peerPort != 0) {
		// link to a router that's already up, so send connect
		pktx px = rtr->ps->alloc();
		Packet& p = rtr->ps->getPacket(px);

		p.length = Forest::OVERHEAD + 8;
		p.type = Forest::CONNECT; p.flags = 0;
		p.comtree = Forest::NABOR_COMT;
		p.srcAdr = myAdr; p.dstAdr = lte.peerAdr;
		int64_t seqNum = rtr->nextSeqNum();
		(p.payload())[0] = htonl((int32_t) ((seqNum>>32) & 0xffffffff));
		(p.payload())[1] = htonl((int32_t) (seqNum & 0xffffffff));
		p.payload()[2] = htonl((uint32_t) (lte.nonce >> 32));
		p.payload()[3] = htonl((uint32_t) (lte.nonce & 0xffffffff));
		p.outLink = lnk;
		p.pack();
		p.hdrErrUpdate();p.payErrUpdate();
		outQ.enq(pair<int,int>(myThx,px));
	}
	cp.fmtAddLinkReply(lnk,peerAdr);
	return;
}

/** Drop a link at this router.
 *  First, remove all routes and comtree links associated with this link.
 *  @param lnk is the link number of the link to be dropped
 *  @param peerAdr is the address of the peer of the remote end of the
 *  link, if lnk == 0; in this case, the link number is looked up using
 *  the peerAdr value
 */
void RouterControl::dropLink(CtlPkt& cp) {
	int lnk; fAdr_t peerAdr;
	if (!cp.xtrDropLink(lnk,peerAdr)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock iftLock(rtr->iftMtx,defer_lock);
	unique_lock  ltLock( rtr->ltMtx,defer_lock);
	unique_lock cttLock(rtr->cttMtx,defer_lock);
	unique_lock  rtLock( rtr->rtMtx,defer_lock);
	lock(iftLock, ltLock, cttLock, rtrLock);

	if (lnk == 0) lnk = rtr->lt->lookup(peerAdr);

	// remove all routes for all comtrees that use this link
	Dlist comtList = rtr->ctt->getComtList(lnk);
	for (int ctx = comtList.first(); ctx != 0; ctx = comtList.next()) {
		rtr->lt->purge(rtr->ctt->getComtree(ctx),
				rtr->ctt->getClnkNum(ctx,lnk));
	}
	// now remove the link from all comtrees that it
	// this may remove some comtrees as well
	rtr->ctt->purgeLink(lnk);

	// now update the interface's ratespec and free the peer's address
	LinkTable::Entry&  lte = rtr->lt->getEntry(lnk);
	IfaceTable::Entry& ifte = rtr->ift->getEntry(lte.iface);
	ifte.availRates.add(lte.rates);
	freeLeafAdr(lte.peerAdr); // ignores addresses outside leaf adr range

	// and finally, remove the link from the link table
	rtr->lt->removeEntry(lnk);
	cp.fmtDropLinkReply();
}

void RouterControl::getLink(CtlPkt& cp) {
	int lnk;
	if (!cp.xtrGetLink(lnk)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	unique_lock ltLock(rtr->ltMtx);
	if (rtr->lt->valid(lnk)) {
		cp.fmtGetLinkReply(lnk, lte.iface, lte.peerIp, lte.peerPort,
				   lte.peerType, lte.peerAdr, lte.rates,
				   lte.availRates);
		return;
	} 
	cp.fmtError("get link: invalid link number";
	return;
}

/** Respond to a get link set control packet.
 *  Control packet includes the first link in the set to be retrieved plus
 *  a count of the number of links to be returned; reply includes the
 *  first link in the set, the count of the number of links returned and
 *  the next link in the table, following the last one in the returned set.
 *  @param cp is a reference to a received get link set control packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return true on success, false on failure
 */
void RouterControl::getLinkSet(CtlPkt& cp) {
	int lnk, count;
	xtrGetLinkSet(lnk,count);

	unique_lock ltLock(rtr->ltMtx);
	if (lnk == 0) lnk = rtr->lt->firstLink(); // 0 means start with first
	else if (!rtr->lt->valid(lnk)) {
		cp.fmtError("get link set: invalid link number";
		return;
	}
	count = min(10,count);
	int i = 0;
	string s;
	while (i < count && lnk != 0) {
		rcp.stringData.append(to_string(lnk) + " ");
		rcp.stringData.append(rtr->lt->link2string(lnk) + "\n");
		if (rcp.stringData.length() > 1300) {
			cp.fmtError( "get link set: error while formatting "
					"reply";
			return;
		}
		i++; lnk = rtr->lt->nextLink(lnk);
	}
	cp.fmtGetLinkSetReply(count, lnk);
	return;
}

void RouterControl::modLink(CtlPkt& cp) {
	int lnk; RateSpec rates;
	if (!cp.xtrModLink(lnk,rates)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock iftLock(rtr->iftMtx,defer_lock);
	unique_lock ltLock(rtr->ltMtx,defer_lock);
	lock(iftLock, ltLock);

	if (!rtr->lt->valid(lnk)) {
		cp.fmtError("get link: invalid link number";
		return;
	}
	LinkTable::Entry& lte = rtr->lt->getEntry(lnk);
	IfaceTable::Entry& ifte = rtr->ift->getEntry(lte.iface);
	RateSpec delta = rates;
	delta.subtract(lte.rates);
	if (!delta.leq(ifte.availRates)) {
		string s;
		cp.fmtError("mod link: request " + rates.toString(s) +
			"exceeds interface capacity";
		return;
	}
	ifte.availRates.subtract(delta);
	lte.rates = rates;
	lte.availRates.add(delta);
	rtr->qm->setLinkRates(lnk,rates);
	cp.fmtModLinkReply():
	return;
}

void RouterControl::addComtree(CtlPkt& cp) {
	comt_t comt;
	if (!cp.xtrAddComtree(comt)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock cttLock(rtr->cttMtx);
	if(rtr->ctt->validComtree(comt) || rtr->ctt->addEntry(comt) != 0) {
		cp.fmtAddComtreeReply();
		return;
	}
	cp.fmtError("add comtree: cannot add comtree";
	return;
}

void RouterControl::dropComtree(CtlPkt& cp) {
	comt_t comt;
	if (!cp.xtrDropComtree(comt)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock ltLock(rtr->ltMtx,defer_lock);
	unique_lock cttLock(rtr->cttMtx,defer_lock);
	unique_lock rtLock(rtr->rtMtx,defer_lock);
	lock(ltLock, cttLock, rtLock);

	int ctx = rtr->ctt->getComtIndex(comt);
	if (ctx == 0) {
		cp.fmtError("dropComtree: no such comtree";
		return;
	}
	ComtreeTable::Entry& cte = rtr->ctt->getEntry(ctx);
	int plink = cte.pLnk;

	int cLnk = rtr->ctt->firstComtLink(ctx);
	RateSpec pRates;
	while (cLnk != 0) {
		// remove all routes involving this comtree
		rtr->lt->purge(comt,cLnk);

		// return assigned link bandwidth to link
		int lnk = rtr->ctt->getLink(ctx,cLnk);
		RouteTable::entry& lte = rtr->lt->getEntry(lnk);
		lte.availRates.add(cte.rates);
		
		// return availRates on parent link
		if (lnk == cte.pLnk) pRates = lte.availRates;

		// de-allocate queue
		qm->freeQ(rtr->ctt->getLinkQ(cLnk));
		
		rtr->ctt->removeLink(ctx,cLnk);
		cLnk = rtr->ctt->firstComtLink(ctx);
	}
	rtr->ctt->removeEntry(ctx); // and finally drop entry in comtree table
	cp.fmtDropComtreeReply(pRates);
	return;
}

void RouterControl::getComtree(CtlPkt& cp) {
	comt_t comt;
	if (!cp.xtrGetComtree(comt)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock cttLock(rtr->cttMtx);
	int ctx = rtr->ctt->getComtIndex(comt);
	if (ctx == 0) {
		cp.fmtError("get comtree: invalid comtree";
		return;
	}
	ComtreeTable::Entry& cte = rtr->ctt->getEntry(ctx);
	cp.fmtGetComtreeReply(comt, cte.coreFlag, cte.pLnk,
				rtr->ctt->getLinkCount(ctx));
	return;
}

void RouterControl::modComtree(CtlPkt& cp) {
	comt_t comt, int coreFlag, int plnk;
	if (!cp.xtrModComtree(comt, coreFlag, plnk)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	unique_lock cttLock(rtr->cttMtx);

	int ctx = rtr->ctt->getComtIndex(comt);
	if (ctx != 0) {
		ComtreeTable::Entry& cte = rtr->ctt->getEntry(ctx);
		if (coreFlag >= 0)
			cte.coreFlag = coreFlag;
		if (cp.link != 0) {
			if (plnk != 0 && !rtr->ctt->isLink(ctx,plnk)) {
				cp.fmtError("specified link does "
						"not belong to comtree";
				return;
			}
			if (plnk != 0 && !rtr->ctt->isRtrLink(ctx,plnk)) {
				cp.fmtError("specified link does "
						"not connect to a router";
				return;
			}
			cte.pLnk = plnk;
			cte.pClnk = rtr->ctt->getClnkNum(comt,plnk);
		}
		cp.fmtModComtreeReply();
		return;
	} 
	cp.fmtError("modify comtree: invalid comtree";
	return;
}

/** Respond to a get comtree set control packet.
 *  Control packet includes the first comtree in the set to be retrieved plus
 *  a count of the number of comtree to be returned; reply includes the
 *  first comtree in the set, the count of the number of comtrees returned and
 *  the next comtree in the table, following the last one in the returned set.
 *  @param cp is a reference to a received get comtree set control packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return on success, false on failure
 */
void RouterControl::getComtreeSet(CtlPkt& cp) {
	comt_t comt; int count;
	if (!cp.xtrGetComtreeSet(comt,count)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock cttLock(rtr->cttMtx);
	int ctx;
	if (comt == 0) // 0 means first
		ctx = rtr->ctt->firstComtIndex();
	else
		ctx = rtr->ctt->getComtIndex(comt);
	if (ctx == 0) {
		cp.fmtError("get comtree set: invalid comtree number";
		return;
	}
	count = min(10,count);
	int i = 0;
	while (i < count && ctx != 0) {
		rcp.stringData.append(rtr->ctt->entry2string(ctx));
		if (rcp.stringData.length() > 1300) {
			cp.fmtError( "get comtee set: error while formatting "
					"reply";
			return;
		}
		i++; ctx = rtr->ctt->nextComtIndex(ctx);
	}
	cp.fmtGetComtreeSetReply(i, ctx == 0 ? 0 : rtr->ctt->getComtree(ctx));
	return;
}

void RouterControl::addComtreeLink(CtlPkt& cp) {
	comt_t comt; int lnk, coreFlag; ipa_t peerIp; ipp_t peerPort;
	fAdr_t peerAdr;
	xtrAddComtreeLink(comt,lnk,coreFlag,peerIp,peerPort,peerAdr);

	unique_lock  ltLock(rtr->ltMtx,defer_lock);
	unique_lock cttLock(rtr->cttMtx,defer_lock);
	unique_lock  rtLock(rtr->rtMtx,defer_lock);
	lock(ltLock, cttLock, rtLock);
	int ctx = rtr->ctt->getComtIndex(comt);
	if (ctx == 0) {
		cp.fmtError("add comtree link: invalid comtree";
		return;
	}
	if (lnk == 0 && cp.ip1 != 0 && cp.port1 != 0) {
		lnk = rtr->lt->lookup(cp.ip1, cp.port1);
	} else if (lnk == 0 && peerAdr != 0) {
		lnk = rtr->lt->lookup(peerAdr);
	}
	if (!rtr->lt->valid(lnk)) {
		cp.fmtError("add comtree link: invalid link or "
					"peer IP and port";
		return;
	}
	LinkTable::Entry& rte = rtr->lt->getEntry(lnk);
	ComtreeTable::Entry& cte = rtr->ctt->getEntry(ctx);
	bool isRtr = false;
	if (lte.peerType == Forest::ROUTER) {
		isRtr = true;
		if (!coreFlag) {
			cp.fmtError("add comtree link: must specify "
					"core flag on links to routers";
			return;
		}
	}
	int cLnk = rtr->ctt->getClnkNum(comt,lnk);
	if (cLnk != 0) {
		cp.fmtError("addComtreeLink: specified "
			       "link already in comtree";
		return;
	}
	// define new comtree link
	if (!rtr->ctt->addLink(ctx,lnk,isRtr,coreFlag)) {
		cp.fmtError("add comtree link: cannot add "
				"requested comtree link";
		return;
	}
	cLnk = rtr->ctt->getClnkNum(comt,lnk);
	ComtreeTable::ClnkInfo& cli = rtr->ctt->getClinkInfo(ctx,cLnk);
	cli.dest = 0;

	// add unicast route to cLnk if peer is a leaf or a router
	// in a different zip code
	if (lte.peerType != Forest::ROUTER) {
		int rtx = rtr->rt->getRtx(comt,peerAdr);
		if (rtx == 0) rtr->rt->addEntry(comt,lte.peerAdr,cLnk);
	} else {
		int zipPeer = Forest::zipCode(lte.peerAdr);
		if (zipPeer != Forest::zipCode(myAdr)) {
			fAdr_t dest = Forest::forestAdr(zipPeer,0);
			int rtx = rtr->rt->getRtx(comt,dest);
			if (rtx == 0) rtr->rt->addEntry(comt,dest,cLnk);
		}
	}

	// allocate queue and bind it to lnk and comtree link
	int qid = qm->allocQ(lnk);
	if (qid == 0) {
		rtr->ctt->removeLink(ctx,cLnk);
		cp.fmtError("add comtree link: no queues "
					"available for link";
		return;
	}
	cli.qnum = qid;

	// adjust rates for link comtree and queue
	RateSpec minRates(Forest::MINBITRATE,Forest::MINBITRATE,
		    	  Forest::MINPKTRATE,Forest::MINPKTRATE);
	if (!minRates.leq(lte.availRates)) {
		cp.fmtError("add comtree link: request "
			       "exceeds link capacity";
		return;
	}
	lte.availRates.subtract(minRates);
	cli.rates = minRates;

	qm->setQRates(qid,minRates);
	if (isRtr) qm->setQLimits(qid,500,1000000);
	else	   qm->setQLimits(qid,500,1000000);
	sm->clearQuStats(qid);
	cp.fmtAddComtreeLinkReply(lnk,lte.availRates);
	return;
}

void RouterControl::dropComtreeLink(CtlPkt& cp) {
	comt_t comt; int lnk; ipa_t peerIp; ipp_t peerPort; fAdr_t peerAdr;
	xtrDropComtreeLink(comt, lnk, peerIp, peerPort, peerAdr);

	unique_lock  ltLock(rtr->ltMtx,defer_lock);
	unique_lock cttLock(rtr->cttMtx,defer_lock);
	unique_lock  rtLock(rtr->rtMtx,defer_lock);
	lock(ltLock, cttLock, rtLock);
	int ctx = rtr->ctt->getComtIndex(comt);
	if (ctx == 0) {
		cp.fmtError("drop comtree link: invalid comtree";
		return;
	}
	if (lnk == 0 && cp.ip1 != 0 && cp.port1 != 0) {
		lnk = rtr->lt->lookup(cp.ip1, cp.port1);
	} else if (lnk == 0 && peerAdr != 0) {
		lnk = rtr->lt->lookup(cp.adr1);
	}
	if (!rtr->lt->valid(lnk)) {
		cp.fmtError("drop comtree link: invalid link "
			       "or peer IP and port";
		return;
	}
	LinkTable::Entry& lte = rtr->lt->getEntry(lnk);
	int cLnk = rtr->ctt->getComtLink(comt,lnk);
	if (cLnk != 0) {
		// remove all routes involving this comtree
		rtr->lt->purge(comt,cLnk);

		// return assigned link bandwidth to link
		ComtreeTable::ClnkInfo& cli = rtr->ctt->getClnkInfo(ctx,cLnk);
		lte.availRates.add(cli.rates);
		
		// de-allocate queue
		qm->freeQ(cli.qnum);
		
		rtr->ctt->removeLink(ctx,cLnk);
	}
	cp.fmtDropComtreeLinkReply(lte.availRates);
	return;
}

void RouterControl::modComtreeLink(CtlPkt& cp) {
	comt_t comt; int lnk; RateSpec rates;
	if (!cp.xtrModComtreeLink(comt,lnk,rates)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock  ltLock(rtr->ltMtx,defer_lock);
	unique_lock cttLock(rtr->cttMtx,defer_lock);
	lock(ltLock, cttLock);
	int ctx = rtr->ctt->getComtIndex(comt);
	if (ctx == 0) {
		cp.fmtError("modify comtree link: invalid comtree";
		return;
	}
	if (!rtr->lt->valid(lnk)) {
		cp.fmtError("modify comtree link: invalid link number";
		return;
	}
	int cLnk = rtr->ctt->getComtLink(comt,lnk);
	if (cLnk == 0) {
		cp.fmtError("modify comtree link: specified link "
			       "not defined in specified comtree";
		return;
	}
	LinkTable::Entry& lte = rtr->lt->getEntry(lnk);
	ComtreeTable::Entry& ctte = rtr->ctt->getEntry(ctx);
	ComtreeTable::ClnkInfo& cli = rtr->ctt->getClnkInfo(ctx,cLnk);

	cli.dest = dest;
	RateSpec diff = rates; diff.subtract(cli.rates);
	if (!diff.leq(lte.availRates)) {
		cp.fmtError("modify comtree link: new rate spec "
				"exceeds available link capacity";
		return;
	}
	lte.availRates.subtract(diff);
	cli.rates = rates;
	cp.fmtModComtreeLink(lte.availRates); // return available rate on link
	return;
}

void RouterControl::getComtreeLink(CtlPkt& cp) {
	comt_t comt; int lnk;
	if (!cp.xtrGetComtreeLink(comt,lnk)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock  ltLock(rtr->ltMtx,defer_lock);
	unique_lock cttLock(rtr->cttMtx,defer_lock);
	lock(ltLock, cttLock);
	int ctx = rtr->ctt->getComtIndex(comt);
	if (ctx == 0) {
		cp.fmtError("get comtree link: invalid comtree";
		return;
	}
	if (!rtr->lt->valid(lnk)) {
		cp.fmtError("get comtree link: invalid link number";
		return;
	}
	int cLnk = rtr->ctt->getClnkNum(comt,lnk);
	if (cLnk == 0) {
		cp.fmtError("getComtreeLink: specified link "
			  	"not defined in specified comtree";
		return;
	}
	ComtreeTable::ClnkInfo& cli = rtr->ctt->getClnkInfo(ctx,cLnk);
	cp.fmtGetComtreeLinkReply(comt,lnk,cli.rates,qid,cli.dest);
	return;
}

void RouterControl::addRoute(CtlPkt& cp) {
	comt_t comt; fAdr_t destAdr; int lnk;
	if (!cp.xtrAddRoute(comt, destAdr, lnk)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock cttLock(rtr->cttMtx,defer_lock);
	unique_lock  rtLock(rtr->rtMtx,defer_lock);
	lock(cttLock, rtLock);
	if (!rtr->ctt->validComtree(comt)) {
		cp.fmtError("comtree not defined at this router\n";
		return;
	}
	if (!Forest::validUcastAdr(destAdr) && !Forest::mcastAdr(destAdr)) {
		cp.fmtError("invalid address\n";
		return;
	}
	int cLnk = rtr->ctt->getComtLink(comt,lnk);
	int rtx = rtr->rt->getRteIndex(comt,destAdr);
	if (rtx != 0) {
		cp.fmtError("add route: requested route "
			        "conflicts with existing route";
		return;
	} else if (rtr->rt->addEntry(comt, destAdr, lnk)) {
		cp.fmtAddRouteReply();
		return;
	}
	cp.fmtError("add route: cannot add route";
	return;
}

void RouterControl::dropRoute(CtlPkt& cp) {
	comt_t comt; fAdr_t destAdr;
	if (!cp.xtrDropRoute(comt, destAdr)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock cttLock(rtr->cttMtx,defer_lock);
	unique_lock  rtLock(rtr->rtMtx,defer_lock);
	lock(cttLock, rtLock);
	if (!rtr->ctt->validComtree(comt)) {
		cp.fmtError("comtree not defined at this router\n";
		return;
	}
	if (!Forest::validUcastAdr(destAdr) && !Forest::mcastAdr(destAdr)) {
		cp.fmtError("invalid address\n";
		return;
	}
	rtr->rt->removeEntry(rtr->rt->getRtx(comt,destAdr));
	cp.fmtDropRouteReply();
	return;
}

void RouterControl::getRoute(CtlPkt& cp) {
	comt_t comt; fAdr_t destAdr;
	if (!cp.xtrGetRoute(comt, destAdr)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock cttLock(rtr->cttMtx,defer_lock);
	unique_lock  rtLock(rtr->rtMtx,defer_lock);
	lock(cttLock, rtLock);
	if (!rtr->ctt->validComtree(comt)) {
		cp.fmtError("comtree not defined at this router\n";
		return;
	}
	if (!Forest::validUcastAdr(destAdr) && !Forest::mcastAdr(destAdr)) {
		cp.fmtError("invalid address\n";
		return;
	}
	int rtx = rtr->rt->getRtx(comt,destAdr);
	if (rtx != 0) {
		int lnk = (Forest::validUcastAdr(destAdr) ?
			   rtr->ctt->getLink(rtr->rt->firstComtLink(rtx)) : 0);
		cp.fmtGetRoute(comt,destAdr,lnk,rtr->rt->getLinkCount(rtx));
		return;
	}
	cp.fmtError("get route: no route for specified address";
	return;
}
        
void RouterControl::modRoute(CtlPkt& cp) {
	comt_t comt; fAdr_t destAdr; int lnk;
	if (!cp.xtrModRoute(comt, destAdr, lnk)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	unique_lock cttLock(rtr->cttMtx,defer_lock);
	unique_lock  rtLock(rtr->rtMtx,defer_lock);
	lock(cttLock, rtLock);
	if (!rtr->ctt->validComtree(comt)) {
		cp.fmtError("comtree not defined at this router\n";
		return;
	}
	if (!Forest::validUcastAdr(destAdr) && !Forest::mcastAdr(destAdr)) {
		cp.fmtError("invalid address\n";
		return;
	}
	int rtx = rtr->rt->getRtx(comt,destAdr);
	if (rtx != 0) {
		if (cp.link != 0) {
			if (Forest::mcastAdr(destAdr)) {
				cp.fmtError("modify route: cannot "
					    "set link in multicast route";
				return;
			}
			rtr->rt->setLink(rtx,cp.link);
		}
		cp.fmtReply();
		return;
	}
	cp.fmtError("modify route: invalid route";
	return;
}

/** Respond to a get route set control packet.
 *  Control packet includes the first route in the set to be retrieved plus
 *  a count of the number of route to be returned; reply includes the
 *  first route in the set, the count of the number of routes returned and
 *  the next route in the table, following the last one in the returned set.
 *  @param cp is a reference to a received get route set control packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return on success, false on failure
 */
void RouterControl::getRouteSet(CtlPkt& cp) {
	int rtx, count;
	xtrGetRouteSet(rtx, count);

	unique_lock cttLock(rtr->cttMtx,defer_lock);
	unique_lock  rtLock(rtr->rtMtx,defer_lock);
	lock(cttLock, rtLock);
// think about re-doing this to use (comt,dest) pairs rather than rtx values
	if (rtx == 0) {
		rtx = rtr->rt->firstRtx(); // 0 means first route
	} else if (!rtr->rt->validRtx(rtx)) {
		cp.fmtError("get route set: invalid route number";
		return;
	}
	count = min(10,count);
	int i = 0;
	while (i < count && rtx != 0) {
		rcp.stringData.append(rtr->rt->entry2string(rtx)); 
		if (rcp.stringData.length() > 1300) {
			cp.fmtError( "get route set: error while formatting "
					"reply";
			return;
		}
		i++; rtx = rtr->rt->nextRtx(rIndex);
	}
	cp.fmtGetRouteSetReply(count, rtx);
	return;
}

/** Handle an add filter control packet.
 *  Adds the specified interface and prepares a reply packet.
 *  @param cp is the control packet structure (already unpacked)
 *  @param rcp is a control packet structure for the reply, which
 *  the cpType has been set to match cp, the rrType is POS_REPLY
 *  and the seqNum has been set to match cp.
 *  @param return on success, false on failure; on a successful
 *  return, all appropriate attributes in rcp are set; on a failure
 *  return, the errMsg field of rcp is set.
 */
void RouterControl::addFilter(CtlPkt& cp) {
	if (!cp.xtrAddFilter()) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	fltx fx = pktLog->addFilter();
	if (fx == 0) {
		cp.fmtError("add filter: cannot add filter";
		return;
	}
	cp.fmtAddFilterReply(fx);
	return;
}

void RouterControl::dropFilter(CtlPkt& cp) {
	int fx;
	if (!cp.xtrDropFilter(fx)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	pktLog->dropFilter(fx);
	cp.fmtReply();
	return;
}

void RouterControl::getFilter(CtlPkt& cp) {
	fltx fx;
	if (!cp.xtrGetFilter(fx)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	if (!pktLog->validFilter(fx)) {
		cp.fmtError("get filter: invalid filter index";
		return;
	}
	PacketFilter& f = pktLog->getFilter(fx);
	cp.fmtGetFilterReply(f.toString);
	return;
}

void RouterControl::modFilter(CtlPkt& cp) {
	fltx fx; string s;
	if (!cp.xtrModFilter(fx,s)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	if (!pktLog->validFilter(fx)) {
		cp.fmtError("mod filter: invalid filter index";
		return;
	}
	PacketFilter& f = pktLog->getFilter(fx);
	f.fromString(s);
	cp.fmtReply();
	return;
}

/** Respond to a get filter set control packet.
 *  Control packet includes the first filter in the set to be retrieved plus
 *  a count of the number of filters to be returned; reply includes the
 *  first filters in the set, the count of the number of filters returned and
 *  the next filters in the table, following the last one in the returned set.
 *  @param cp is a reference to a received get filter set control packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return on success, false on failure
 */
void RouterControl::getFilterSet(CtlPkt& cp) {
	fltx fx; int count;
	if (!cp.xtrGetFilterSet(fx, count)) { 
		cp.fmtError("unable to unpack control packet"); return;
	}

	if (fx == 0) {
		fx = pktLog->firstFilter(); // 0 means start with first filter
	} else if (!pktLog->validFilter(fx)) {
		cp.fmtError("get filter set: invalid filter index";
		return;
	}
	count = min(10,count);
	int i = 0;
	stringstream ss;
	while (i < count && fx != 0) {
		PacketFilter& f = pktLog->getFilter(fx);
		ss << fx << " " << f.toString() << "\n";

		if (ss.str().length() > 1300) {
			cp.fmtError("get filter set: error while "
					"formatting reply";
			return;
		}
		i++; fx = pktLog->nextFilter(fx);
	}
	cp.fmtGetFilterSetReply(fx, count, ss.str());
	return;
}

/** Respond to a get logged packets control packet.
 *  @param cp is a reference to a received get logged packets control packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return on success, false on failure
 */
void RouterControl::getLoggedPackets(CtlPkt& cp) {
	if (!cp.xtrGetLoggedPackets()) { 
		cp.fmtError("unable to unpack control packet"); return;
	}
	string s;
	int count = pktLog->extract(1300, s);
	cp.fmtGetLoggedPacketsReply(count,s);
	return;
}

/** Handle an incoming join request from a client.
 *  @param cp is a reference to the received request packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return on success, false on failure
 */
void RouterControl::joinComtree(CtlPkt& cp) {
	// if this client has another op in progress, add request to list
	//   of pending operations and return
	// if comtree exists at this router
	//    if client already in comtree, send positive reply and return
	//    if any non-local ops in progress, add request to list of
	//      pending operations and return
	//    add client to comtree, allocating rates appropriately
	//    add operation to set of pending local operations
	//    send addNode report to comtree controller and return
}

/** Handle an incoming leave request from a client.
 *  @param cp is a reference to the received request packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return on success, false on failure
 */
void RouterControl::leaveComtree(CtlPkt& cp) {
} 

/** Handle an incoming addBranch request from a router.
 *  @param cp is a reference to the received request packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return on success, false on failure
 */
void RouterControl::addBranch(CtlPkt& cp) {
} 

/** Handle an incoming prune request from a router.
 *  @param cp is a reference to the received request packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return on success, false on failure
 */
void RouterControl::prune(CtlPkt& cp) {
} 

/** Handle an incoming confirm request from a router.
 *  @param cp is a reference to the received request packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return on success, false on failure
 */
void RouterControl::confirm(CtlPkt& cp) {
} 

/** Handle an incoming abort request from a router.
 *  @param cp is a reference to the received request packet
 *  @param rcp is a reference to the reply packet with fields to be
 *  filled in
 *  @return on success, false on failure
 */
void RouterControl::abort(CtlPkt& cp) {
} 


} // ends namespace
