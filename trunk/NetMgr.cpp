/** @file NetMgr.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

/**
 *  NetMgr provides a basic network management interface to a
 *  forest network. It uses a simple command line interpreter
 *  that reads commands from stdin and prints results on stdout.
 *
 *  usage:
 *       NetMgr config
 */

#include "stdinc.h"
#include "forest.h"

main(int argc, char *argv[]) {
}

/*
Notes
Two basic enumerations, CpType and CpAttr.
For each, have an array indexed by the enum value.
Each array is a struct with an integer code,
a name and an abbreviation.

Enum can be defined without use of codes to facilitate
iteration through the array. Is there an iterator for
enums? Otherwise, relying on the automatic mechanisms
would allow simple integer based iteration.

Note that switch statements must use enum values, not codes.
So, we would need to extract code from packet and get the
enum value for that code before doing a switch.
*/


	CLIENT_ADD_COMTREE(10,"client add comtree","cac"),
	CLIENT_DROP_COMTREE(11,"client drop comtree","cdc"),
	CLIENT_GET_COMTREE(12,"client get comtree","cgc"),
	CLIENT_MOD_COMTREE(13,"client modify comtree","cmc"),
	CLIENT_JOIN_COMTREE(17,"client join comtree","cjc"),
	CLIENT_LEAVE_COMTREE(18,"client leave comtree","clc"),
	CLIENT_RESIZE_COMTREE(16,"client autosize comtree","crc"),
	CLIENT_GET_LEAF_RATE(14,"client get leaf rate","cglf"),
	CLIENT_MOD_LEAF_RATE(15,"client modify leaf rate","cmlf"),

	// internal control packet types
	ADD_IFACE(50,"add interface","ai"),
	DROP_IFACE(51,"drop interface","di"),
	GET_IFACE(52,"get interface","gi"),
	MOD_IFACE(53,"modify interface","mi"),

	ADD_LINK(60,"add link","al"),
	DROP_LINK(61,"drop link","dl),
	GET_LINK(62,"get link","gl"),
	MOD_LINK(63,"modify link","ml"),

	ADD_COMTREE(70,"add comtree","ac"),
	DROP_COMTREE(71,"drop comtree","dc"),
	GET_COMTREE(74,"get comtree","gc"),
	MOD_COMTREE(75,"modify comtree","mc"),
	ADD_COMTREE_LINK(72,"add comtree link","acl"),
	DROP_COMTREE_LINK(73,"drop comtree link","dcl"),
	RESIZE_COMTREE_LINK(76,"resize comtree link","rcl"),

	ADD_ROUTE(90,"add route","ar"),
	DROP_ROUTE(91,"drop route","dr"),
	GET_ROUTE(94,"get route","gr"),
	MOD_ROUTE(95,"modify route","mr"),
	ADD_ROUTE_LINK(92,"add route link","arl"),
	DROP_ROUTE_LINK(93,"drop route link","drl"),
	
	NO_MATCH(110,"no match","nm")


/** Command line interface
 *  Each command is entered on a new line and consists of a
 *  "command phrase" such as "add comtree" and zero or more
 *  (attribute,value) pairs, which take the form
 *  attributeName=value.
 */


// Constructor for Avatar, allocates space and initializes private data
Avatar::Avatar(ipa_t mipa, ipa_t ripa, fAdr_t ma, fAdr_t ra, comt_t ct)
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

	// initialize avatar to random position
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0)
		fatal("Avatar::Avatar: gettimeofday failure");
	//srand(tv.tv_usec);
	srand(myAdr);
	x = randint(0,SIZE-1); y = randint(0,SIZE-1);
	direction = (double) randint(0,359);
	deltaDir = 0;
	speed = MEDIUM;

	mcGroups = new dlist(MAXGROUPS);
	nearAvatars = new hashTbl(MAXNEAR);
	numNear = 0;
	nextAv = 1;
}

Avatar::~Avatar() { delete mcGroups; delete nearAvatars; delete ps; }

bool Avatar::init() {
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

int Avatar::receive() { 
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
                fatal("Avatar::receive: error in recvfrom call");
        }

	ps->unpack(p);
        h.ioBytes() = nbytes;
        h.tunSrcIp() = ntohl(ssa.sin_addr.s_addr);
        h.tunSrcPort() = ntohs(ssa.sin_port);

        return p;
}

void Avatar::send(int p) {
// Send packet on specified link and recycle storage.
	dsa.sin_addr.s_addr = htonl(rtrIpAdr);
	dsa.sin_port = htons(FOREST_PORT);
	header& h = ps->hdr(p);
	ps->pack(p);
	int rv = sendto(sock,(void *) ps->buffer(p),h.leng(),0,
		    	(struct sockaddr *) &dsa, sizeof(dsa));
	if (rv == -1) fatal("Avatar::send: failure in sendto");
}

int Avatar::getTime() {
// Return time expressed as a free-running microsecond clock
	static uint32_t prevTime = 0;
	static struct timeval prevTimeval = { 0, 0 };
	static uint32_t now;
	struct timeval nowTimeval;

	if (prevTimeval.tv_sec == 0 && prevTimeval.tv_usec == 0) {
		// first call, initialize and return 0
		if (gettimeofday(&prevTimeval, NULL) < 0)
			fatal("Avatar::getTime: gettimeofday failure");
		prevTime = 0;
		return 0;
	}
	// normal case
	if (gettimeofday(&nowTimeval, NULL) < 0)
		fatal("Avatar::getTime: gettimeofday failure");
	now = prevTime +
		(nowTimeval.tv_sec == prevTimeval.tv_sec ?
                 nowTimeval.tv_usec - prevTimeval.tv_usec :
                 nowTimeval.tv_usec + (1000000 - prevTimeval.tv_usec));
	prevTime = now; prevTimeval = nowTimeval;
	return now;
}

void Avatar::updateStatus(int now) {
// Update status of avatar
	const double PI = 3.141519625;
	double r;

	// update position
	double dist = (speed * UPDATE_PERIOD)/1000;
	double dirRad = direction * (2*PI/360);
	x += (int) (dist * sin(dirRad));
	y += (int) (dist * cos(dirRad));
	x = max(x,0); x = min(x,SIZE-1);
	y = max(y,0); y = min(y,SIZE-1);

	// adjust direction and deltaDir
	if (x == 0)		direction = 90;
	else if (x == SIZE-1)	direction = 270;
	else if (y == 0)	direction = 0;
	else if (y == SIZE-1)	direction = 180;
	else {
		direction += deltaDir;
		if (direction < 0) direction += 360;
		if ((r = randfrac()) < 0.1) {
			if (r < .05) deltaDir -= 0.2 * randfrac();
			else         deltaDir += 0.2 * randfrac();
			deltaDir = min(1.0,deltaDir);
			deltaDir = max(-1.0,deltaDir);
		}
	}

	// update speed
	if ((r = randfrac()) <= 0.1) {
		if (speed == SLOW || speed == FAST) speed = MEDIUM;
		else if (r < 0.05) 		    speed = SLOW;
		else 				    speed = FAST;
	}
}

int Avatar::groupNum(int x1, int y1) {
// Return the multicast group number associated with position (x,y)
// Assumes that SIZE is an integer multiple of GRID
	return 1 + (x1/GRID) + (y1/GRID)*(SIZE/GRID);
}

void Avatar::updateSubscriptions() {
	const double SQRT2 = 1.41421356;
	const int GRANGE = VISRANGE + (4*FAST*UPDATE_PERIOD)/1000;

	int myGroup = groupNum(x,y);
	dlist *newGroups = new dlist(MAXGROUPS);
	(*newGroups) &= myGroup;

	int x1, y1, g;
	x1 = min(SIZE-1, x+GRANGE);
	if (!newGroups->mbr(g = groupNum(x1,y)))  (*newGroups) &= g;
	x1 = max(0, x-GRANGE);
	if (!newGroups->mbr(g = groupNum(x1,y)))  (*newGroups) &= g;

	y1 = min(SIZE-1, y+GRANGE);
	if (!newGroups->mbr(g = groupNum(x,y1)))  (*newGroups) &= g;
	y1 = max(0, y-GRANGE);
	if (!newGroups->mbr(g = groupNum(x,y1)))  (*newGroups) &= g;

	x1 = min(SIZE-1, (int) (x+GRANGE/SQRT2));
	y1 = min(SIZE-1, (int) (y+GRANGE/SQRT2));
	if (!newGroups->mbr(g = groupNum(x1,y1)))  (*newGroups) &= g;
	y1 = min(SIZE-1, (int) (y-GRANGE/SQRT2));
	if (!newGroups->mbr(g = groupNum(x1,y1)))  (*newGroups) &= g;
	x1 = min(SIZE-1, (int) (x-GRANGE/SQRT2));
	if (!newGroups->mbr(g = groupNum(x1,y1)))  (*newGroups) &= g;
	y1 = min(SIZE-1, (int) (y+GRANGE/SQRT2));
	if (!newGroups->mbr(g = groupNum(x1,y1)))  (*newGroups) &= g;

	packet p = ps->alloc();
	header& h = ps->hdr(p);
	uint32_t *pp = ps->payload(p);

	int nsub = 0; int nunsub = 0;
	g = (*newGroups)[1];
	while (g != Null) {
		if (!mcGroups->mbr(g)) 
			pp[1+nsub++] = htonl(-g);
		g = newGroups->suc(g);
	}

	g = (*mcGroups)[1];
	while (g != Null) {
		if (!newGroups->mbr(g)) 
			pp[2+nsub+nunsub++] = htonl(-g);
		g = mcGroups->suc(g);
	}

	if (nsub + nunsub == 0) { ps->free(p); delete newGroups; return; }

	delete mcGroups; mcGroups = newGroups;

	pp[0] = htonl(nsub);
	pp[1+nsub] = htonl(nunsub);

	h.ptype() = SUB_UNSUB; h.flags() = 0;
	h.comtree() = comt;
	h.srcAdr() = myAdr;
	h.dstAdr() = rtrAdr;
	h.leng() = 4*(8+nsub+nunsub);

	send(p); ps->free(p);
}

void Avatar::updateNearby(int p) {
// If the given packet is a status report, check to see if the
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
	int x1 = ntohl(pp[2]); int y1 = ntohl(pp[3]);
	double dx = x - x1; double dy = y - y1;

	uint64_t key = h.srcAdr(); key = (key << 32) | h.srcAdr();
	if (sqrt(dx*dx + dy*dy) <= VISRANGE) {
		if (nearAvatars->lookup(key) == 0) {
			if (nextAv <= MAXNEAR) {
				nearAvatars->insert(key, nextAv++);
				numNear++;
			}
		}
	} else {
		if (nearAvatars->lookup(key) != 0)  {
			nearAvatars->remove(key);
			numNear--;
		}
	}
	return;
}

void Avatar::sendStatus(int now) {
// Send a status packet
	packet p = ps->alloc();

	header& h = ps->hdr(p);
	h.leng() = 4*(5+8); h.ptype() = CLIENT_DATA; h.flags() = 0;
	h.comtree() = comt; h.srcAdr() = myAdr; h.dstAdr() = -groupNum(x,y);

	uint32_t *pp = ps->payload(p);
	pp[0] = htonl(STATUS_REPORT);
	pp[1] = htonl(now);
	pp[2] = htonl(x);
	pp[3] = htonl(y);
	pp[4] = htonl((uint32_t) direction);
	pp[5] = htonl((uint32_t) speed);
	pp[6] = htonl(numNear);

	send(p); ps->free(p);
}

void Avatar::connect() {
// Send initial connect packet, using comtree 1 (the signalling comtree)
	packet p = ps->alloc();

	header& h = ps->hdr(p);
	h.leng() = 4*(5+1); h.ptype() = CONNECT; h.flags() = 0;
	h.comtree() = 1; h.srcAdr() = myAdr; h.dstAdr() = rtrAdr;

	send(p); ps->free(p);
}

void Avatar::disconnect() {
// Send final disconnect packet
	packet p = ps->alloc();
	header& h = ps->hdr(p);

	h.leng() = 4*(5+1); h.ptype() = DISCONNECT; h.flags() = 0;
	h.comtree() = 1; h.srcAdr() = myAdr; h.dstAdr() = rtrAdr;

	send(p); ps->free(p);
}

void Avatar::run(int finishTime) {
// Run the avatar, stopping after finishTime
// Operate on a cycle with a period of UPDATE_PERIOD milliseconds,
// Each cycle, update my current position, direction, speed;
// issue new SUB_UNSUB packet if necessary;
// read all incoming status reports and update my set of nearby
// avatars; finally, send new status report

	int p;

	connect(); 		// send initial connect packet

	uint32_t now;    	// free-running microsecond time
	uint32_t nextTime;	// time of next operational cycle

	now = nextTime = 0;

	while (now <= finishTime) {
		now = getTime();
		updateStatus(now);
		updateSubscriptions();
		while ((p = receive()) != Null) {
			updateNearby(p);
			ps->free(p);
		}
		sendStatus(now);

		nextTime += 1000*UPDATE_PERIOD;
		useconds_t delay = nextTime - now;
		usleep(delay);
	}

	disconnect(); 		// send final disconnect packet
}
