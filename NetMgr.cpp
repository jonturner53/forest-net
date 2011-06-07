/** @file NetMgr.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "NetMgr.h"

/** usage:
 *       NetMgr extIp intIp rtrIp myAdr rtrAdr
 * 
 *  Command line arguments include two ip addresses for the
 *  NetMgr. The first is the IP address that a remote UI can
 *  use to connect to the NetMgr. If this is specified as 127.0.0.1
 *  the NetMgr listens on the default IP address. The second is the
 *  IP address used by the NetMgr within the Forest overlay. RtrIp
 *  is the router's IP address, myAdr is the NetMgr's Forest
 *  address, rtrAdr is the Forest address of the router.
 */
main(int argc, char *argv[]) {
	ipa_t intIp, extIp, rtrIp; fAdr_t myAdr, rtrAdr;
	int comt, finTime;

	if (argc != 6 ||
  	    (extIp  = Np4d::ipAddress(argv[1])) == 0 ||
  	    (intIp  = Np4d::ipAddress(argv[2])) == 0 ||
	    (rtrIp  = Np4d::ipAddress(argv[3])) == 0 ||
	    (myAdr  = Forest::forestAdr(argv[4])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[5])) == 0 ||
	     sscanf(argv[6],"%d", &finTime) != 1)
		fatal("usage: NetMgr extIp intIp rtrIpAdr myAdr rtrAdr");

	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress(); 
	if (extIp == 0) fatal("can't retrieve default IP address");

	NetMgr mon(extIp,intIp,rtrIp,myAdr,rtrAdr);
	if (!mon.init()) {
		perror("perror");
		fatal("NetMgr: initialization failure");
	}
	mon.run();
	exit(0);
}

// Constructor for NetMgr, allocates space and initializes private data
NetMgr::NetMgr(ipa_t xipa, ipa_t iipa, ipa_t ripa, fAdr_t ma, fAdr_t ra)
		: extIp(xipa), intIp(iipa), rtrIp(ripa), myAdr(ma), rtrAdr(ra) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);

	connSock = -1;
}

NetMgr::~NetMgr() {
	cout.flush();
	if (extSock > 0) close(extSock);
	if (intSock > 0) close(intSock);
	delete ps;
}

/** Initialize sockets.
 *  @return true on success, false on failure
 */
bool NetMgr::init() {
	intSock = Np4d::datagramSocket();
	if (intSock < 0 || 
	    !Np4d::bind4d(intSock,intIp,0) ||
	    !Np4d::nonblock(intSock))
		return false;

	connect(); 		// send initial connect packet
	usleep(1000000);	// 1 second delay provided for use in SPP
				// delay gives SPP linecard the time it needs
				// to setup NAT filters before second packet

	// setup external TCP socket for use by remote GUI
	extSock = Np4d::streamSocket();
	if (extSock < 0) return false;
	return	Np4d::bind4d(extSock,extIp,NM_PORT) &&
		Np4d::listen4d(extSock) && 
		Np4d::nonblock(extSock);
}

/** Run the NetMgr until killed by some external signal.
 */
void NetMgr::run(int finishTime) {

	int p; int idleCount = 0;

	uint32_t now;    	// free-running microsecond time

	now = Misc::getTime();

	while (now <= finishTime) {

		if ("packet" received from remote UI) {
			fill in fields and forward it into forest net
			idleCount = 0;
		} else if (packet received from forest net) {
			if (it is a reply to an earlier UI request) {
				return it to remote UI
			} else if (it is a reply to an earlier autonomous req) {
				update local state
			}
			idleCount = 0;
		} else {
			idleCount++;
		}

		if (idleCount >= 10) {
			struct timespec sleeptime, rem;
                        sleeptime.tv_sec = 0; sleeptime.tv_nsec = 1000000;
                        nanosleep(&sleeptime,&rem);
		}

		now = Misc::getTime();
	}
}

/** Check for next packet from the Forest network.
 *  @return next report packet or 0, if no report has been received.
 */
int NetMgr::rcvFromForest() { 
	int p = ps->alloc();
	if (p == 0) return 0;
        buffer_t& b = ps->getBuffer(p);

        int nbytes = Np4d::recv4d(intSock,&b[0],1500);
        if (nbytes < 0) { ps->free(p); return 0; }
	ps->unpack(p);

	if (it's a control packet reply) {
		match it to an outgoing request and clear timer
	}
	if (there are expired timers) {
		re-send packets with expired timers up to 5 times
	}

        return p;
}

/** Check for next message from the remote UI.
 *  @return a packet number with a formatted control packet on success,
 *  0 on failure
 */
int NetMgr::readFromUi() { 
	if (connSock < 0) {
		connSock = Np4d::accept4d(extSock);
		if (connSock < 0) return;
	}

	if (!Np4d::hadData(connSock)) return;

	uint32_t numPairs;
	if (!Np4d::readWord32(connSock, numPairs))
		fatal("NetMgr::readFromUi: cannot process message from UI");
	uint32_t target;
	if (!Np4d::readWord32(connSock, target))
		fatal("NetMgr::readFromUi: cannot process message from UI");
	uint32_t cpTypCode;
	if (!Np4d::readWord32(connSock, cpTypCode))
		fatal("NetMgr::readFromUi: cannot process message from UI");
	if (!Forest::ucastAdr(target)) 
		fatal("NetMgr::readFromUi: misformatted target address");
	CpTypeIndex cpTypIndex = CpType::getIndexByCode(cpTypCode);
	if (cpTypIndex == 0) 
		fatal("NetMgr::readFromUi: invalid control message code");
	
	int p = ps->alloc();
	if (p == 0) fatal("NetMgr::readFromUi: out of packets");

	PacketHeader& h = ps->getHeader(p);
	buffer_t& b = ps->getBuffer(p);
	uint32_t* payload = ps->getPayload(p);
	CtlPkt cp(payload);

	h.setLength(Forest::HDR_LENG + 4 + 4*(4+2*numPairs));
	h.setType(NET_SIG); h.setFlags(0);
	h.setComtree(100); h.setDestAdr(target); h.setSrcAdr(myAdr);

	cp.setRrType(REQUEST); cp.setCpType(cpTypIndex);
	cp.setSeqNum(nextSeqNum++);

	for (int i = 0; i < numPairs; i++) {
		uint32_t attr, val;
		if (!Np4d::readWord32(connSock, attr))
			fatal("NetMgr::readFromUi: cannot read attribute");
		if (!Np4d::readWord32(connSock, val))
			fatal("NetMgr::readFromUi: cannot read value");
		CpAttrIndex attrIndex;
	

        int nbytes = read(connSock, (char *) &length, sizeof(uint32_t));
        if (nbytes < 0) {
                if (errno == EAGAIN) return;
                fatal("NetMgr::readFromUi: error in read call");
        } else if (nbytes < sizeof(uint32_t)) {
		fatal("NetMgr::readFromUi: misformated message");
	}


        return;
}

/** Send packet to Forest router (connect, disconnect, sub_unsub).
 */
void NetMgr::send2router(int p) {
	static sockaddr_in sa;
	int leng = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(intSock,(void *) ps->getBuffer(p),leng,
		    	      	rtrIp,Forest::ROUTER_PORT);
	if (rv == -1) fatal("NetMgr::send2router: failure in sendto");
}

/** Send status packet to remote GUI, assuming the comt value is non-zero.
 */
void NetMgr::send2gui() {

	if (comt == 0) return;

	int nbytes = repCnt*NUMITEMS*sizeof(uint32_t);
	char* p = (char *) statPkt;
	while (nbytes > 0) {
		int n = write(connSock, (void *) p, nbytes);
		if (n < 0) fatal("NetMgr::send2gui: failure in write");
		p += n; nbytes -= n;
	}
}

/** Return the multicast group number associated with a virtual world positon.
 *  Assumes that SIZE is an integer multiple of GRID
 *  @param x1 is x coordinate
 *  @param y1 is y coordinate
 *  @return the group number for the grid square containing (x1,y1)
 */
int NetMgr::groupNum(int x1, int y1) {
	return 1 + (x1/GRID) + (y1/GRID)*(SIZE/GRID);
}

/** If oldcomt != 0, send a packet to unsubscribe from all multicasts
 *  in oldcomt. if newcomt != 0 send a packet to subscribe to all
 *  multicasts in newcomt.
 */
void NetMgr::updateSubscriptions(comt_t oldcomt, comt_t newcomt) {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	// unsubscriptions
	if (oldcomt != 0) {
		int nunsub = 0;
		for (int x = 0; x < SIZE; x += GRID) {
			for (int y = 0; y < SIZE; y += GRID) {
				if (nunsub > 350) fatal("too many subscriptions");
				pp[++nunsub+1] = htonl(-groupNum(x,y));
			}
		}
		pp[0] = 0; pp[1] = htonl(nunsub);
	
		h.setLength(4*(8+nunsub)); h.setPtype(SUB_UNSUB); h.setFlags(0);
		h.setComtree(oldcomt); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
	
		send2router(p);
	}

	// subscriptions
	if (newcomt != 0) {
		int nsub = 0;
		for (int x = 0; x < SIZE; x += GRID) {
			for (int y = 0; y < SIZE; y += GRID) {
				if (nsub > 350) fatal("too many subscriptions");
				pp[++nsub] = htonl(-groupNum(x,y));
			}
		}
		pp[0] = htonl(nsub); pp[nsub+1] = 0;
	
		h.setLength(4*(8+nsub)); h.setPtype(SUB_UNSUB); h.setFlags(0);
		h.setComtree(newcomt); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
	
		send2router(p);
	}
	ps->free(p);
}

/** If the given packet is a status report, track the sender.
 *  Add to our set of watchedAvatars if not already there.
 *  sending avatar is visible. If it is visible, but not in our
 *  set of nearby avatars, then add it. If it is not visible
 *  but is in our set of nearby avatars, then delete it.
 *  Note: we assume that the visibility range and avatar speeds
 *  are such that we will get at least one report from a newly
 *  invisible avatar.
 */
void NetMgr::updateStatus(int p, int now) {

	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	uint64_t key = h.getSrcAdr(); key = (key << 32) | h.getSrcAdr();
	int avNum = watchedAvatars->lookup(key);
	if (avNum == 0) {
		watchedAvatars->insert(key, nextAvatar);
		avNum = nextAvatar++;
	}

	avData[avNum].adr = h.getSrcAdr();
	avData[avNum].ts = ntohl(pp[1]);
	avData[avNum].x = ntohl(pp[2]);
	avData[avNum].y = ntohl(pp[3]);
	avData[avNum].dir = ntohl(pp[4]);
	avData[avNum].speed = ntohl(pp[5]);
	avData[avNum].numVisible = ntohl(pp[6]);
	avData[avNum].numNear = ntohl(pp[7]);
	avData[avNum].comt = h.getComtree();

	if (h.getComtree() != comt) return;

	// if no room in statusPkt buffer for another report,
	// send current buffer
	if (repCnt + 1 > MAX_REPORTS) {
		send2gui(); repCnt = 0;
	}
	// add new report to the outgoing status packet
	statPkt[NUMITEMS*repCnt]   = htonl(now);
	statPkt[NUMITEMS*repCnt+1] = htonl(avData[avNum].adr);
	statPkt[NUMITEMS*repCnt+2] = htonl(avData[avNum].x);
	statPkt[NUMITEMS*repCnt+3] = htonl(avData[avNum].y);
	statPkt[NUMITEMS*repCnt+4] = htonl(avData[avNum].dir);
	statPkt[NUMITEMS*repCnt+5] = htonl(avData[avNum].speed);
	statPkt[NUMITEMS*repCnt+6] = htonl(avData[avNum].numVisible);
	statPkt[NUMITEMS*repCnt+7] = htonl(avData[avNum].numNear);
	statPkt[NUMITEMS*repCnt+8] = htonl(h.getComtree());
	repCnt++;

	return;
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
void NetMgr::connect() {
	packet p = ps->alloc();

	PacketHeader& h = ps->getHeader(p);
	h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send2router(p); ps->free(p);
}

/** Send final disconnect packet to forest router, after unsubscribing.
 */
void NetMgr::disconnect() {
	updateSubscriptions(comt, 0);
	comt = 0;

	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send2router(p); ps->free(p);
}

void NetMgr::writeStatus(int now) {
	if (comt == 0 || logging == false) return;
	for (int i = 1; i < nextAvatar; i++) {
		struct avatarData *ad = & avData[i];
		if (ad->comt != comt) continue;
		string s; Forest::addFadr2string(s,ad->adr);
		logFile << now << " " << s << " " << ad->ts << " "
			<< ad->x << " " << ad->y << " "
			<< ad->dir << " " << ad->speed << " "
			<< ad->numNear << endl;
	}
}

