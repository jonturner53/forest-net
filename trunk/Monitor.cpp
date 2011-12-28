/** @file Monitor.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Monitor.h"
/** usage:
 *       Monitor extIp intIp rtrIp myAdr rtrAdr gridSize finTime [logfile]
 * 
 *  Monitor is a program that monitors a virtual world, tracking
 *  the motion of the avatars within it. The monitored data is
 *  sent to a remote GUI which can display it visually. The remote
 *  GUI "connects" to the Monitor by sending it a datagram with
 *  a magic number in the first word (1234567). The second word
 *  of the connection packet identifies the comtree which should
 *  be displayed. The remote GUI can switch to another comtree
 *  by sending another connection packet. To sever the connection,
 *  the GUI sends a disconnect packet with another magic number
 *  (7654321) in the first word.
 * 
 *  Data is forwarded in binary form with 40 reports per packet.
 *  If an optional log file is specified, the reports are also
 *  written to a local log file in binary form.
 * 
 *  Command line arguments include two ip addresses for the
 *  Monitor. The first is the IP address that the GUI can
 *  use to connect to the Monitor. If this is specified as 0.0.0.0,
 *  the Monitor listens on the default IP address. The second is the
 *  IP address used by the Monitor within the Forest overlay. RtrIp
 *  is the route's IP address, myAdr is the Monitor's Forest
 *  address, rtrAdr is the Forest address of the router, 
 *  gridSize is the number of squares in the virtual world's
 *  basic grid (actually, the grid is gridSizeXgridSize) and
 *  finTime is the number of seconds to run before terminating.
 */
main(int argc, char *argv[]) {
	ipa_t intIp, extIp, rtrIp; fAdr_t myAdr, rtrAdr;
	int comt, finTime, gridSize;

	if (argc !=8 ||
  	    (extIp  = Np4d::ipAddress(argv[1])) == 0 ||
  	    (intIp  = Np4d::ipAddress(argv[2])) == 0 ||
	    (rtrIp  = Np4d::ipAddress(argv[3])) == 0 ||
	    (myAdr  = Forest::forestAdr(argv[4])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[5])) == 0 ||
	    (sscanf(argv[6],"%d", &gridSize) != 1) ||
	     sscanf(argv[7],"%d", &finTime) != 1)
		fatal("usage: Monitor extIp intIp rtrIpAdr myAdr rtrAdr "
		      		    "gridSize finTime");
	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress(); 
	if (extIp == 0) fatal("can't retrieve default IP address");

	Monitor mon(extIp,intIp,rtrIp,myAdr,rtrAdr,gridSize);
	if (!mon.init()) {
		fatal("Monitor: initialization failure");
	}
	mon.run(1000000*(finTime-1)); // -1 to compensate for delay in init
	exit(0);
}

// Constructor for Monitor, allocates space and initializes private data
Monitor::Monitor(ipa_t xipa, ipa_t iipa, ipa_t ripa, fAdr_t ma, fAdr_t ra,
		 int gs)
		 : extIp(xipa), intIp(iipa), rtrIp(ripa), myAdr(ma), rtrAdr(ra),
		   SIZE(GRID*gs) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);

	watchedAvatars = new UiHashTbl(MAX_AVATARS);
	avData = new avatarData[MAX_AVATARS+1];
	nextAvatar = 1;
	cornerX = 0;
	cornerY = 0;
	comt = 0;
	connSock = -1;
}

Monitor::~Monitor() {
	cout.flush();
	if (extSock > 0) close(extSock);
	if (intSock > 0) close(intSock);
	delete watchedAvatars; delete ps;
}

/** Initialize sockets and open log file for writing.
 *  @return true on success, false on failure
 */
bool Monitor::init() {
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
	bool status;
	status = Np4d::bind4d(extSock,extIp,MON_PORT);
	if (!status) {
		return false;
	}
	return Np4d::listen4d(extSock) && Np4d::nonblock(extSock);
}

/** Run the monitor, stopping after finishTime
 *  Operate on a cycle with a period of UPDATE_PERIOD milliseconds,
 *  Each cycle, process all the packets that came in during
 *  that period and store results.
 *  Print a record of all avatar's status, once per second. 
 */
void Monitor::run(int finishTime) {
	int p;

	uint32_t now;    	// free-running microsecond time
	uint32_t nextTime;	// time of next operational cycle
	now = nextTime = Misc::getTime(); 

	while (now <= finishTime) {
		check4comtree();
		while ((p = receiveReport()) != 0) {
			updateStatus(p,now);
			ps->free(p);
		}

		nextTime += 1000*UPDATE_PERIOD;
		useconds_t delay = nextTime - Misc::getTime();
		if (0 < delay && delay <= 1000*UPDATE_PERIOD) usleep(delay);
		now = Misc::getTime();
	}

	disconnect(); 		// send final disconnect packet
}

/** Check for next Avatar report packet and return it if there is one.
 *  @return next report packet or 0, if no report has been received.
 */
int Monitor::receiveReport() { 
	int p = ps->alloc();
	if (p == 0) return 0;
        buffer_t& b = ps->getBuffer(p);

        int nbytes = Np4d::recv4d(intSock,&b[0],1500);
        if (nbytes < 0) { ps->free(p); return 0; }

	ps->unpack(p);
        return p;
}

/** Check for a new comtree number from the GUI and update subscriptions.
 *  If the remote GUI has connected and sent a comtree number,
 *  read the comtree number and store it. If the new comtree is
 *  different from the previous one, unsubscribe from all multicasts
 *  in the old one and subscribe to all multicasts in the new one.
 */
void Monitor::check4comtree() { 
	if (connSock < 0) {
		connSock = Np4d::accept4d(extSock);
		if (connSock < 0) return;
		if (!Np4d::nonblock(connSock))
			fatal("can't make connection socket nonblocking");
		bool status; int ndVal = 1;
		status = setsockopt(extSock,IPPROTO_TCP,TCP_NODELAY,
			    (void *) &ndVal,sizeof(int));
		if (status != 0) {
			cerr << "setsockopt for no-delay failed\n";
			perror("");
		}
	}
	uint64_t input;
        int nbytes = read(connSock, (char *) &input, sizeof(uint64_t));
        if (nbytes < 0) {
                if (errno == EAGAIN) return;
                fatal("Monitor::check4comtree: error in read call");
	} else if (nbytes == 0) { // remote gui closed connection
		close(connSock); connSock = -1;
		updateSubscriptions(comt, 0);
		watchedAvatars->clear(); nextAvatar = 1;
		return;
        } else if (nbytes < sizeof(uint32_t)) {
		fatal("Monitor::check4comtree: incomplete comtree number");
	}
	uint32_t lowerBits = (uint32_t) (input);
	comt_t newComt = (comt_t) (input >> 32);
	lowerBits = ntohl(lowerBits);
	newComt = ntohl(newComt);
	if(lowerBits != 0) { //not a comtree, but a movement instruction
		if(lowerBits == 1) cornerX--;
		else if(lowerBits == 2) cornerY++;
		else if(lowerBits == 3) cornerX++;
		else if(lowerBits == 4) cornerY--;
		else if(lowerBits == 5) viewSize--;
		else if(lowerBits == 6) viewSize++;
		else if(lowerBits == 7) viewSize = newComt;
		else fatal("unrecognized number from remote client");

		updateSubscriptions();
		return;
	}
	if (newComt == comt) return;
	updateSubscriptions(comt, newComt);
	comt = newComt;
	watchedAvatars->clear(); 
	nextAvatar = 1;

        return;
}

/** Send packet to Forest router (connect, disconnect, sub_unsub).
 */
void Monitor::send2router(int p) {
	static sockaddr_in sa;
	int leng = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(intSock,(void *) ps->getBuffer(p),leng,
		    	      	rtrIp,Forest::ROUTER_PORT);
	if (rv == -1) fatal("Monitor::send2router: failure in sendto");
}

/** Send status report to remote GUI, assuming the comt value is non-zero.
 *  @param now is the current time, used to timestamp the report
 *  @param avNum is the number of the avatar for which we are sending
 *  a report
 */
void Monitor::send2gui(uint32_t now, int avNum) {

	if (comt == 0 || connSock == -1) return;

	uint32_t buf[9];
	buf[0] = htonl(now);
	buf[1] = htonl(avData[avNum].adr);
	buf[2] = htonl(avData[avNum].x);
	buf[3] = htonl(avData[avNum].y);
	buf[4] = htonl(avData[avNum].dir);
	buf[5] = htonl(avData[avNum].speed);
	buf[6] = htonl(avData[avNum].numVisible);
	buf[7] = htonl(avData[avNum].numNear);
	buf[8] = htonl(avData[avNum].comt);

	int nbytes = 9*sizeof(uint32_t);
	char* p = (char *) buf;
	while (nbytes > 0) {
		int n = write(connSock, (void *) p, nbytes);
		if (n < 0) return;//fatal("Monitor::send2gui: failure in write");
		p += n; nbytes -= n;
	}
}

/** Return the multicast group number associated with a virtual world positon.
 *  Assumes that SIZE is an integer multiple of GRID
 *  @param x1 is x coordinate
 *  @param y1 is y coordinate
 *  @return the group number for the grid square containing (x1,y1)
 */
int Monitor::groupNum(int x1, int y1) {
	return 1 + (x1/GRID) + (y1/GRID)*(SIZE/GRID);
}
/** If oldcomt != 0, send a packet to unsubscribe from all multicasts
 *  in oldcomt. if newcomt != 0 send a packet to subscribe to all
 *  multicasts in newcomt.
 */
void Monitor::updateSubscriptions() {	
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	// unsubscriptions
	int nunsub = 0;
	for (int x = 0; x < SIZE; x += GRID) {
		for (int y = 0; y < SIZE; y += GRID) {
			nunsub++;
			if (nunsub > 350) {
				pp[0] = 0; pp[1] = htonl(nunsub-1);
				h.setLength(Forest::OVERHEAD +
					    4*(1+nunsub));
				h.setPtype(SUB_UNSUB); h.setFlags(0);
				h.setComtree(comt);
				h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
				send2router(p);
				nunsub = 1;
			}
			pp[nunsub+1] = htonl(-groupNum(x,y));
		}
	}
	pp[0] = 0; pp[1] = htonl(nunsub);
	h.setLength(Forest::OVERHEAD + 4*(1+nunsub));
	h.setPtype(SUB_UNSUB); h.setFlags(0);
	h.setComtree(comt); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
	send2router(p);	
	// subscriptions
	int nsub = 0;
	for (int x = GRID*cornerX; x < GRID*(viewSize+cornerX); x += GRID) {
		for (int y = GRID*cornerY; y < GRID*(viewSize+cornerX); y += GRID) {
			nsub++;
			if (nsub > 350) {
				pp[0] = htonl(nsub-1); pp[nsub] = 0;
				h.setLength(Forest::OVERHEAD +
					    4*(1+nsub));
				h.setPtype(SUB_UNSUB); h.setFlags(0);
				h.setComtree(comt);
				h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
				send2router(p);
				nsub = 1;
			}
			pp[nsub] = htonl(-groupNum(x,y));
		}
	}
	pp[0] = htonl(nsub); pp[nsub+1] = 0;

	h.setLength(Forest::OVERHEAD + 4*(2+nsub));
	h.setPtype(SUB_UNSUB); h.setFlags(0);
	h.setComtree(comt); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
	send2router(p);
	ps->free(p);
}
/** If oldcomt != 0, send a packet to unsubscribe from all multicasts
 *  in oldcomt. if newcomt != 0 send a packet to subscribe to all
 *  multicasts in newcomt.
 */
void Monitor::updateSubscriptions(comt_t oldcomt, comt_t newcomt) {
cerr << "updateSubscriptions(comt_t,comt_t)" << endl;
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	// unsubscriptions
	if (oldcomt != 0) {
		int nunsub = 0;
		for (int x = 0; x < SIZE; x += GRID) {
			for (int y = 0; y < SIZE; y += GRID) {
				nunsub++;
				if (nunsub > 350) {
					pp[0] = 0; pp[1] = htonl(nunsub-1);
					h.setLength(Forest::OVERHEAD +
						    4*(1+nunsub));
					h.setPtype(SUB_UNSUB); h.setFlags(0);
					h.setComtree(oldcomt);
					h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
					send2router(p);
					nunsub = 1;
				}
				pp[nunsub+1] = htonl(-groupNum(x,y));
			}
		}
		pp[0] = 0; pp[1] = htonl(nunsub);
		h.setLength(Forest::OVERHEAD + 4*(1+nunsub));
		h.setPtype(SUB_UNSUB); h.setFlags(0);
		h.setComtree(oldcomt); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
		send2router(p);
	}

	// subscriptions
	if (newcomt != 0) {
		int nsub = 0;
		for (int x = 0; x < SIZE; x += GRID) {
			for (int y = 0; y < SIZE; y += GRID) {
				nsub++;
				if (nsub > 350) {
					pp[0] = htonl(nsub-1); pp[nsub] = 0;
					h.setLength(Forest::OVERHEAD +
						    4*(1+nsub));
					h.setPtype(SUB_UNSUB); h.setFlags(0);
					h.setComtree(newcomt);
					h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
					send2router(p);
					nsub = 1;
				}
				pp[nsub] = htonl(-groupNum(x,y));
			}
		}
		pp[0] = htonl(nsub); pp[nsub+1] = 0;
	
		h.setLength(Forest::OVERHEAD + 4*(2+nsub));
		h.setPtype(SUB_UNSUB); h.setFlags(0);
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
void Monitor::updateStatus(int p, int now) {
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	if (h.getComtree() != comt || h.getPtype() != CLIENT_DATA ||
	    ntohl(pp[0]) != Avatar::STATUS_REPORT) {
		// ignore packets for other comtrees, or that are not
		// avatar status reports
		ps->free(p); return;
	}
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
	avData[avNum].comt = comt;

	send2gui(now,avNum);

	return;
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
void Monitor::connect() {
	packet p = ps->alloc();

	PacketHeader& h = ps->getHeader(p);
	h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
	h.setComtree(Forest::CLIENT_CON_COMT);
	h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send2router(p); ps->free(p);
}

/** Send final disconnect packet to forest router, after unsubscribing.
 */
void Monitor::disconnect() {
	updateSubscriptions(comt, 0);
	comt = 0;

	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
	h.setComtree(Forest::CLIENT_CON_COMT);
	h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send2router(p); ps->free(p);
}
