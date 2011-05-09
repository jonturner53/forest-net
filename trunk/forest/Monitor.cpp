// usage:
//      Monitor myIp1 myIp2 rtrIp myAdr rtrAdr finTime [logfile]
//
// Monitor is a program that monitors a virtual world, tracking
// the motion of the avatars within it. The monitored data is
// sent to a remote GUI which can display it visually. The remote
// GUI "connects" to the Monitor by sending it a datagram with
// a magic number in the first word (1234567). The second word
// of the connection packet identifies the comtree which should
// be displayed. The remote GUI can switch to another comtree
// by sending another connection packet. To sever the connection,
// the GUI sends a disconnect packet with another magic number
// (7654321) in the first word.
//
// Data is forwarded in binary form with 40 reports per packet.
// If an optional log file is specified, the reports are also
// written to a local log file in binary form.
//
// Command line arguments include two ip addresses for the
// Monitor. The first is the IP address that the GUI can
// use to connect to the Monitor. The second is the IP address
// used by the Monitor with the Forest overlay. RtrIp
// is the route's IP address, myAdr is the Monitor's Forest
// address, rtrAdr is the Forest address of the router, and
// finTime is the number of seconds to run before terminating.
//

#include "stdinc.h"
#include "forest.h"
#include "Monitor.h"

main(int argc, char *argv[]) {
	ipa_t myIp1, myIp2, rtrIp; fAdr_t myAdr, rtrAdr;
	int comt, finTime;

	if (argc < 7 || argc > 8 ||
  	    (myIp1  = misc::ipAddress(argv[1])) == 0 ||
  	    (myIp2  = misc::ipAddress(argv[2])) == 0 ||
	    (rtrIp  = misc::ipAddress(argv[3])) == 0 ||
	    (myAdr  = forest::forestAdr(argv[4])) == 0 ||
	    (rtrAdr = forest::forestAdr(argv[5])) == 0 ||
	     sscanf(argv[6],"%d", &finTime) != 1)
		fatal("usage: Monitor myIp1 myIp2 rtrIpAdr myAdr rtrAdr "
		      		    "finTime [logfile]");

	Monitor mon(myIp1,myIp2,rtrIp,myAdr,rtrAdr);
	char logFileName[101];
	strncpy(logFileName,(argc == 8 ? argv[7] : ""),100);
	if (!mon.init(logFileName))
		fatal("Monitor:: initialization failure");
	mon.run(1000000*finTime);
	exit(0);
}

// Constructor for Monitor, allocates space and initializes private data
Monitor::Monitor(ipa_t mipa1, ipa_t mipa2, ipa_t ripa, fAdr_t ma, fAdr_t ra)
		: myIp1(mipa1), myIp2(mipa2), rtrIp(ripa), myAdr(ma), rtrAdr(ra) {
	int nPkts = 10000;
	ps = new pktStore(nPkts+1, nPkts+1);

	watchedAvatars = new hashTbl(MAX_AVATARS);
	nextAvatar = 1;

	avData = new avatarData[MAX_AVATARS+1];
	statPkt = new uint32_t[500];

	comt = 0; repCnt = 0;
}

Monitor::~Monitor() {
	fflush(stdout);
	if (sock1 > 0) close(sock1);
	if (sock2 > 0) close(sock2);
	delete watchedAvatars; delete ps;
}

bool Monitor::init(char *logFileName) {
// Initialize sockets and open log file for writing.
// Return true on success, false on failure.
	sock1 = misc::setupTcpSock(myIp1,MON_PORT);
	sock2 = misc::setupSock(myIp2,0);
	if (sock1 < 0 || sock2 < 0) return false;

	if (logFileName[0] == EOS) {
		logging = false;
        } else {
		logFile.open(logFileName);
		if (logFile.fail()) {
			cerr << "Monitor::init: can't open log file\n";
			return false;
		}
		logging = true;
	}
	return true;
}

int Monitor::receiveReport() { 
// Return next report packet from Forest network or Null if there is none. 
	int nbytes;	  // number of bytes in received packet
	sockaddr_in ssa;  // socket address of sender
	socklen_t ssaLen; // length of sender's socket address structure

	int p = ps->alloc();
	if (p == Null) return Null;
	header& h = ps->hdr(p);
        buffer_t& b = ps->buffer(p);

        ssaLen = sizeof(ssa); 
        nbytes = recvfrom(sock2, &b[0], 1500, 0,
                          (struct sockaddr *) &ssa, &ssaLen);
        if (nbytes < 0) {
                if (errno == EWOULDBLOCK) {
                        ps->free(p); return Null;
                }
                fatal("Monitor::receive: error in recvfrom call");
        }

	ps->unpack(p);
        return p;
}

void Monitor::check4comtree() { 
// Check for a new comtree number from the GUI.
	int nbytes;	  // number of bytes in received packet
	uint32_t newComt;

        nbytes = read(sock1, (char *) &newComt, sizeof(uint32_t));
        if (nbytes < 0) {
                if (errno == EWOULDBLOCK) return;
                fatal("Monitor::check4comtree: error in recvfrom call");
        } else if (nbytes < sizeof(uint32_t)) {
		fatal("Monitor::check4comtree: incomplete comtree number");
	}
	newComt = ntohl(newComt);

	if (newComt == comt) return;
	updateSubscriptions(comt, newComt);
	comt = newComt;
	repCnt = 0;

        return;
}

void Monitor::send2router(int p) {
// Send packet to Forest router (connect, disconnect, sub_unsub)
	static bool initialize = true;
	static sockaddr_in sa;
	if (initialize) {
		misc::initSockAdr(rtrIp,FOREST_PORT,&sa);
		initialize = false;
	}
	header& h = ps->hdr(p);
	ps->pack(p);
	int rv = sendto(sock2,(void *) ps->buffer(p),h.leng(),0,
		    	(struct sockaddr *) &sa, sizeof(sa));
	if (rv == -1) fatal("Monitor::send2router: failure in sendto");
}

void Monitor::send2gui() {
// Send status packet to remote GUI, assuming the comt value is non-zero

	if (comt == 0) return;

	int nbytes = repCnt*8*sizeof(uint32_t);
	char* p = (char *) statPkt;
	while (nbytes > 0) {
		int n = write(sock1, (void *) p, nbytes);
		if (n < 0) fatal("Monitor::send2gui: failure in write");
		p += n; nbytes -= n;
	}
}

int Monitor::getTime() {
// Return time expressed as a free-running microsecond clock
	static uint32_t prevTime = 0;
	static struct timeval prevTimeval = { 0, 0 };
	static uint32_t now;
	struct timeval nowTimeval;

	if (prevTimeval.tv_sec == 0 && prevTimeval.tv_usec == 0) {
		// first call, initialize and return 0
		if (gettimeofday(&prevTimeval, NULL) < 0)
			fatal("Monitor::getTime: gettimeofday failure");
		prevTime = 0;
		return 0;
	}
	// normal case
	if (gettimeofday(&nowTimeval, NULL) < 0)
		fatal("Monitor::getTime: gettimeofday failure");
	now = prevTime +
		(nowTimeval.tv_sec == prevTimeval.tv_sec ?
                 nowTimeval.tv_usec - prevTimeval.tv_usec :
                 nowTimeval.tv_usec + (1000000 - prevTimeval.tv_usec));
	prevTime = now; prevTimeval = nowTimeval;
	return now;
}

int Monitor::groupNum(int x1, int y1) {
// Return the multicast group number associated with position (x,y)
// Assumes that SIZE is an integer multiple of GRID
	return 1 + (x1/GRID) + (y1/GRID)*(SIZE/GRID);
}

void Monitor::updateSubscriptions(comt_t oldcomt, comt_t newcomt) {
// If oldcomt != 0, send a packet to unsubscribe from all multicasts
// in oldcomt. if newcomt != 0 send a packet to subscribe to all
// multicasts in newcomt.
	packet p = ps->alloc();
	header& h = ps->hdr(p);
	uint32_t *pp = ps->payload(p);

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
	
		h.leng() = 4*(8+nunsub); h.ptype() = SUB_UNSUB; h.flags() = 0;
		h.comtree() = oldcomt; h.srcAdr() = myAdr; h.dstAdr() = rtrAdr;
	
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
	
		h.leng() = 4*(8+nsub); h.ptype() = SUB_UNSUB; h.flags() = 0;
		h.comtree() = newcomt; h.srcAdr() = myAdr; h.dstAdr() = rtrAdr;
	
		send2router(p);
	}
	ps->free(p);
}

void Monitor::updateStatus(int p, int now) {
// If the given packet is a status report, track the sender.
// Add to our set of watchedAvatars if not already there.
// sending avatar is visible. If it is visible, but not in our
// set of nearby avatars, then add it. If it is not visible
// but is in our set of nearby avatars, then delete it.
// Note: we assume that the visibility range and avatar speeds
// are such that we will get at least one report from a newly
// invisible avatar.

	header& h = ps->hdr(p);
	uint32_t *pp = ps->payload(p);

	uint64_t key = h.srcAdr(); key = (key << 32) | h.srcAdr();
	int avNum = watchedAvatars->lookup(key);
	if (avNum == 0) {
		watchedAvatars->insert(key, nextAvatar);
		avNum = nextAvatar++;
	}

	avData[avNum].adr = h.srcAdr();
	avData[avNum].ts = ntohl(pp[1]);
	avData[avNum].x = ntohl(pp[2]);
	avData[avNum].y = ntohl(pp[3]);
	avData[avNum].dir = ntohl(pp[4]);
	avData[avNum].speed = ntohl(pp[5]);
	avData[avNum].numNear = ntohl(pp[6]);
	avData[avNum].comt = h.comtree();

	if (h.comtree() != comt) return;

	// if no room in statusPkt buffer for another report,
	// send current buffer
	if (repCnt + 1 > MAX_REPORTS) {
		send2gui(); repCnt = 0;
	}
	// add new report to the outgoing status packet
	statPkt[8*repCnt]   = htonl(now);
	statPkt[8*repCnt+1] = htonl(avData[avNum].adr);
	statPkt[8*repCnt+2] = htonl(avData[avNum].x);
	statPkt[8*repCnt+3] = htonl(avData[avNum].y);
	statPkt[8*repCnt+4] = htonl(avData[avNum].dir);
	statPkt[8*repCnt+5] = htonl(avData[avNum].speed);
	statPkt[8*repCnt+6] = htonl(avData[avNum].numNear);
	statPkt[8*repCnt+7] = htonl(h.comtree());
	repCnt++;

	return;
}

void Monitor::connect() {
// Send initial connect packet to forest router
// Uses comtree 1, which is for user signalling.
	packet p = ps->alloc();

	header& h = ps->hdr(p);
	h.leng() = 4*(5+1); h.ptype() = CONNECT; h.flags() = 0;
	h.comtree() = 1; h.srcAdr() = myAdr; h.dstAdr() = rtrAdr;

	send2router(p); ps->free(p);
}

void Monitor::disconnect() {
// Send final disconnect packet to forest router, after unsubscribing.
	updateSubscriptions(comt, 0);
	comt = 0;

	packet p = ps->alloc();
	header& h = ps->hdr(p);

	h.leng() = 4*(5+1); h.ptype() = DISCONNECT; h.flags() = 0;
	h.comtree() = 1; h.srcAdr() = myAdr; h.dstAdr() = rtrAdr;

	send2router(p); ps->free(p);
}

void Monitor::printStatus(int now) {
	if (comt == 0 || logging == false) return;
	for (int i = 1; i < nextAvatar; i++) {
		struct avatarData *ad = & avData[i];
		if (ad->comt != comt) continue;
		char *adrStr = forest::forestStr(ad->adr);
		logFile << now << " " << adrStr << " " << ad->ts << " "
			<< ad->x << " " << ad->y << " "
			<< ad->dir << " " << ad->speed << " "
			<< ad->numNear << endl;
		delete [] adrStr;
	}
}

void Monitor::run(int finishTime) {
// Run the monitor, stopping after finishTime
// Operate on a cycle with a period of UPDATE_PERIOD milliseconds,
// Each cycle, process all the packets that came in during
// that period and store results.
// Print a record of all avatar's status, once per second. 

	int p;

	connect(); 		// send initial connect packet
	usleep(1000000);	// 1 second delay provided for use in SPP
				// delay gives SPP linecard needs time to
				// setup NAT filters before second packet

	uint32_t now;    	// free-running microsecond time
	uint32_t nextTime;	// time of next operational cycle
	uint32_t printTime;	// next time to print status

	now = nextTime = 0; printTime = 1000*UPDATE_PERIOD;

	while (now <= finishTime) {
		now = getTime();
		check4comtree();
		while ((p = receiveReport()) != Null) {
			updateStatus(p,now);
			ps->free(p);
		}
		if (now > printTime) {
			printStatus(now); printTime += 1000*UPDATE_PERIOD;
		}

		nextTime += 1000*UPDATE_PERIOD;
		useconds_t delay = nextTime - getTime();
		if (delay <= 1000*UPDATE_PERIOD) usleep(delay);
	}

	disconnect(); 		// send final disconnect packet
}
