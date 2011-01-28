// usage:
//      Monitor myIpAdr rtrIpAdr myAdr rtrAdr comt finTime
//
// Monitor is a program that monitors a virtual world, tracking
// the motion of the avatars within it. This version just logs
// results to a file once per second for debugging purposes.
// The final version will connect to a remote GUI and relay
// updates for display.
//
// Command line arguments include the ip address of the
// Montir's machine, its router's IP address, the forest
// address of the monitor, the forest address of the router,
// the comtree to be used in the packet and the number of
// seconds to run before terminating.
//

#include "stdinc.h"
#include "forest.h"
#include "Monitor.h"

main(int argc, char *argv[]) {
	ipa_t myIpAdr, rtrIpAdr; fAdr_t myAdr, rtrAdr;
	int comt, finTime;

	if (argc != 7 ||
  	    (myIpAdr  = misc::ipAddress(argv[1])) == 0 ||
	    (rtrIpAdr = misc::ipAddress(argv[2])) == 0 ||
	    (myAdr  = forest::forestAdr(argv[3])) == 0 ||
	    (rtrAdr = forest::forestAdr(argv[4])) == 0 ||
	    sscanf(argv[5],"%d", &comt) != 1 ||
	    sscanf(argv[6],"%d", &finTime) != 1)
		fatal("usage: Monitor myIpAdr rtrIpAdr myAdr rtrAdr "
		      		    "comtree finTime");

	Monitor avatar(myIpAdr,rtrIpAdr,myAdr,rtrAdr, comt);
	if (!avatar.init()) fatal("Monitor:: initialization failure");
	avatar.run(1000000*finTime);
}


// Constructor for Monitor, allocates space and initializes private data
Monitor::Monitor(ipa_t mipa, ipa_t ripa, fAdr_t ma, fAdr_t ra, comt_t ct)
		: myIpAdr(mipa), rtrIpAdr(ripa), myAdr(ma), rtrAdr(ra),
		  comt(ct) {
	int nPkts = 10000;
	ps = new pktStore(nPkts+1, nPkts+1);

	// initialize socket address structures
	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = 0; // let system select address
	sa.sin_addr.s_addr = htonl(myIpAdr);
	bzero(&dsa, sizeof(dsa));
	dsa.sin_family = AF_INET;

	watchedAvatars = new hashTbl(MAX_AVATARS);
	nextAvatar = 1;
}

Monitor::~Monitor() { delete watchedAvatars; delete ps; }

bool Monitor::init() {
// Initialize IO. Return true on success, false on failure.
// Configure socket for non-blocking access, so that we don't
// block when there are no input packets available.
	// create datagram socket
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                return false;
        }
	// bind it to the socket address structure
        if (bind(sock, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
                return false;
        }
	// make socket nonblocking
	int flags;
	if ((flags = fcntl(sock, F_GETFL, 0)) < 0)
		return false;
	flags |= O_NONBLOCK;
	if ((flags = fcntl(sock, F_SETFL, flags)) < 0)
		return false;
	return true;
}

int Monitor::receive() { 
// Return next waiting packet or Null if there is none. 
	int nbytes;	  // number of bytes in received packet
	sockaddr_in ssa;  // socket address of sender
	socklen_t ssaLen; // length of sender's socket address structure
	int lnk;	  // # of link on which packet received
	ipa_t sIpAdr; ipp_t sPort;

	int p = ps->alloc();
	if (p == Null) return Null;
	header& h = ps->hdr(p);
        buffer_t& b = ps->buffer(p);

        ssaLen = sizeof(ssa); 
        nbytes = recvfrom(sock, &b[0], 1500, 0,
                          (struct sockaddr *) &ssa, &ssaLen);
        if (nbytes < 0) {
                if (errno == EWOULDBLOCK) {
                        ps->free(p); return Null;
                }
                fatal("Monitor::receive: error in recvfrom call");
        }

        h.ioBytes() = nbytes;
        h.tunSrcIp() = ntohl(ssa.sin_addr.s_addr);
        h.tunSrcPort() = ntohs(ssa.sin_port);

        return p;
}

void Monitor::send(int p) {
// Send packet on specified link.
	dsa.sin_addr.s_addr = htonl(rtrIpAdr);
	dsa.sin_port = htons(FOREST_PORT);
	header& h = ps->hdr(p);
	int rv = sendto(sock,(void *) ps->buffer(p),h.leng(),0,
		    	(struct sockaddr *) &dsa, sizeof(dsa));
	if (rv == -1) fatal("Monitor::send: failure in sendto");
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

void Monitor::subscribeAll() {
	packet p = ps->alloc();
	header& h = ps->hdr(p);
	uint32_t *pp = ps->payload(p);

	int nsub = 0;
	for (int x = 0; x < SIZE; x += GRID) {
		for (int y = 0; y < SIZE; y += GRID) {
			if (nsub > 350) fatal("too many subscriptions");
			pp[++nsub] = htonl(-groupNum(x,y));
		}
	}
	pp[0] = htonl(nsub); pp[nsub+1] = 0;

	h.leng() = 4*(8+nsub); h.ptype() = SUB_UNSUB; h.flags() = 0;
	h.comtree() = comt; h.srcAdr() = myAdr; h.dstAdr() = rtrAdr;

	ps->pack(p); send(p); ps->free(p);
}

void Monitor::updateStatus(int p) {
// If the given packet is a status report, track the sender.
// Add to our set of watchedAvatars if not already there.
// sending avatar is visible. If it is visible, but not in our
// set of nearby avatars, then add it. If it is not visible
// but is in our set of nearby avatars, then delete it.
// Note: we assume that the visibility range and avatar speeds
// are such that we will get at least one report from a newly
// invisible avatar.

	ps->unpack(p);
	header& h = ps->hdr(p);

	uint32_t *pp = ps->payload(p);
	if (ntohl(pp[0]) != STATUS_REPORT) return;

	uint64_t key = h.srcAdr(); key = (key << 32) | h.srcAdr();
	int avNum = watchedAvatars->lookup(key);
//cout << "a " << h.srcAdr() << " " << nextAvatar << " " << avNum << "\n";
	if (avNum == 0) {
//cout << "c " << h.srcAdr() << " " << nextAvatar << " " << avNum << "\n";
		watchedAvatars->insert(key, nextAvatar);
//cout << "b " << h.srcAdr() << " " << nextAvatar << " " << avNum << "\n";
		avNum = nextAvatar++;
	}
//cout << "d " << h.srcAdr() << " " << nextAvatar << " " << avNum << "\n";


	avData[avNum].adr = h.srcAdr();
	avData[avNum].ts = ntohl(pp[1]);
	avData[avNum].x = ntohl(pp[2]);
	avData[avNum].y = ntohl(pp[3]);
	avData[avNum].dir = ntohl(pp[4]);
	avData[avNum].speed = ntohl(pp[5]);
	avData[avNum].numNear = ntohl(pp[6]);

	return;
}

void Monitor::connect() {
// Send initial connect packet
	packet p = ps->alloc();

	header& h = ps->hdr(p);
	h.leng() = 4*(5+1); h.ptype() = CONNECT; h.flags() = 0;
	h.comtree() = comt; h.srcAdr() = myAdr; h.dstAdr() = rtrAdr;

	ps->pack(p); send(p); ps->free(p);
}

void Monitor::disconnect() {
// Send final disconnect packet
	packet p = ps->alloc();
	header& h = ps->hdr(p);

	h.leng() = 4*(5+1); h.ptype() = DISCONNECT; h.flags() = 0;
	h.comtree() = comt; h.srcAdr() = myAdr; h.dstAdr() = rtrAdr;

	ps->pack(p); send(p); ps->free(p);
}

void Monitor::printStatus(int now) {
	for (int i = 1; i < nextAvatar; i++) {
		struct avatarData *ad = & avData[i];
		char *adrStr = forest::forestStr(ad->adr);
		printf("%10d %s %10d %8d %8d %4d %8d %4d\n",
			now, adrStr, ad->ts, ad->x, ad->y,
			ad->dir, ad->speed, ad->numNear);
		delete adrStr;
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
	subscribeAll();		// subscribe to all groups

	uint32_t now;    	// free-running microsecond time
	uint32_t nextTime;	// time of next operational cycle
	uint32_t printTime;	// next time to print status

	now = nextTime = 0; printTime = 1000*UPDATE_PERIOD;

	while (now <= finishTime) {
		now = getTime();

//cout << "x " << now << endl;
		while ((p = receive()) != Null) {
			updateStatus(p);
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
