/** @file Avatar.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "stdinc.h"
#include "CommonDefs.h"
#include "Avatar.h"

/** usage:
 *       Avatar myIpAdr rtrIpAdr myAdr rtrAdr comt finTime
 * 
 *  Command line arguments include the ip address of the
 *  avatar's machine, the router's IP address, the forest
 *  address of the avatar, the forest address of the router,
 *  the comtree to be used in the packet and the number of
 *  seconds to run before terminating.
 * 
 *  The status reports include the current time (in us),
 *  the avatar's position, the  direction it is facing,
 *  the speed at which it is moving and the number 
 *  of nearby avatars it is tracking.
 *
 *  The type of the status packet is CLIENT_DATA, the first
 *  word of the payload is the constant STATUS_REPORT=1,
 *  and successive words of the payload contain a timestamp,
 *  the avatar's x position, its y position, its direction, its speed
 *  and the number of nearby avatars. This gives a total
 *  payload length of 6 words (24 bytes), giving a total
 *  packet length of 52 bytes. 
 */
main(int argc, char *argv[]) {
	ipa_t myIpAdr, rtrIpAdr; fAdr_t myAdr, rtrAdr;
	int comt, finTime;

	if (argc != 7 ||
	    (myIpAdr  = Np4d::ipAddress(argv[1])) == 0 ||
	    (rtrIpAdr = Np4d::ipAddress(argv[2])) == 0 ||
	    (myAdr  = Forest::forestAdr(argv[3])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[4])) == 0 ||
	    sscanf(argv[5],"%d", &comt) != 1 ||
	    sscanf(argv[6],"%d", &finTime) != 1)
		fatal("usage: Avatar myIpAdr rtrIpAdr myAdr rtrAdr "
		      		    "comtree finTime");

	Avatar avatar(myIpAdr,rtrIpAdr,myAdr,rtrAdr, comt);
	if (!avatar.init()) fatal("Avatar:: initialization failure");
	avatar.run(1000000*finTime);
}

/** Constructor allocates space and initializes private data.
 * 
 *  @param mipa is this host's IP address
 *  @param ripa is the IP address of the access router for this host
 *  @param ma is the forest address for this host
 *  @param ra is the forest address for the access router
 *  @param ct is the comtree used for the virtual world
 */
Avatar::Avatar(ipa_t mipa, ipa_t ripa, fAdr_t ma, fAdr_t ra, comt_t ct)
		: myIpAdr(mipa), rtrIpAdr(ripa), myAdr(ma), rtrAdr(ra),
		  comt(ct) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);

	// initialize avatar to random position
	srand(myAdr);
	x = randint(0,SIZE-1); y = randint(0,SIZE-1);
	direction = (double) randint(0,359);
	deltaDir = 0;
	speed = MEDIUM;

	mcGroups = new UiDlist(MAXGROUPS);
	nearAvatars = new UiHashTbl(MAXNEAR);
	visibleAvatars = new UiHashTbl(MAXNEAR);
	numVisible = numNear = stableNumNear = stableNumVisible = 0;
	nextAv = 1;
}

Avatar::~Avatar() { delete mcGroups; delete nearAvatars; delete ps; }

/** Initialize a datagram socket for nonblocking IO.
 *  @return true on success, false on failure
 */
bool Avatar::init() {
	sock = Np4d::datagramSocket();

	return	sock >= 0 &&
        	Np4d::bind4d(sock, myIpAdr, 0) &&
        	Np4d::nonblock(sock);
}

/** Main Avatar processing loop.
 *  Operates on a cycle with a period of UPDATE_PERIOD milliseconds,
 *  Each cycle, update the current position, direction, speed;
 *  issue new SUB_UNSUB packet if necessary; read all incoming 
 *  status reports and update the set of nearby avatars;
 *  finally, send new status report
 * 
 *  @param finishTime is the number of microseconds to 
 *  to run before halting.
 */
void Avatar::run(int finishTime) {
	connect(); 		// send initial connect packet

	uint32_t now;    	// free-running microsecond time
	uint32_t nextTime;	// time of next operational cycle
	now = nextTime = 0;
	while (finishTime == 0 || now <= finishTime) {
		//reset hashtables and report
		nearAvatars->clear();
		visibleAvatars->clear();
		stableNumNear = numNear; stableNumVisible = numVisible;
		numVisible = numNear = 0;
		nextAv = 1;

		now = Misc::getTime();
		updateStatus(now);
		updateSubscriptions();
		int p;
		while ((p = receive()) != 0) {
			updateNearby(p);
			ps->free(p);
		}
		sendStatus(now);

		nextTime += 1000*UPDATE_PERIOD;
		useconds_t delay = nextTime - now;
		if (delay < (1 << 31)) usleep(delay);
		else nextTime = now + 1000*UPDATE_PERIOD;
	}
	disconnect(); 		// send final disconnect packet
}

/** Send a status packet on the multicast group for the current location.
 *  
 *  @param now is the reference time for the status report
 */
void Avatar::sendStatus(int now) {
	packet p = ps->alloc();

	PacketHeader& h = ps->getHeader(p);
	h.setLength(4*(5+8)); h.setPtype(CLIENT_DATA); h.setFlags(0);
	h.setComtree(comt); h.setSrcAdr(myAdr); h.setDstAdr(-groupNum(x,y));

	uint32_t *pp = ps->getPayload(p);
	pp[0] = htonl(STATUS_REPORT);
	pp[1] = htonl(now);
	pp[2] = htonl(x);
	pp[3] = htonl(y);
	pp[4] = htonl((uint32_t) direction);
	pp[5] = htonl((uint32_t) speed);
	pp[6] = htonl(stableNumVisible);
	pp[7] = htonl(stableNumNear);

	send(p);
}

/** Send initial connect packet, using comtree 1 (the signalling comtree).
 */
void Avatar::connect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send(p);
}

/** Send final disconnect packet.
 */
void Avatar::disconnect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send(p);
}

/** Send packet and recycle storage.
 */
void Avatar::send(int p) {
	int length = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(sock,(void *) ps->getBuffer(p),length,
		    		rtrIpAdr, Forest::ROUTER_PORT);
	if (rv == -1) fatal("Avatar::send: failure in sendto");
	ps->free(p);
}

/** Return next waiting packet or 0 if there is none. 
 */
int Avatar::receive() { 
	int p = ps->alloc();
	if (p == 0) return 0;
	PacketHeader& h = ps->getHeader(p);
        buffer_t& b = ps->getBuffer(p);

	ipa_t remoteIp; ipp_t remotePort;
        int nbytes = Np4d::recvfrom4d(sock, &b[0], 1500,
				      remoteIp, remotePort);
        if (nbytes < 0) {
                if (errno == EWOULDBLOCK) {
                        ps->free(p); return 0;
                }
                fatal("Avatar::receive: error in recvfrom call");
        }

	ps->unpack(p);
        h.setIoBytes(nbytes);
        h.setTunSrcIp(remoteIp);
        h.setTunSrcPort(remotePort);

        return p;
}

/** Update status of avatar based on passage of time.
 *  @param now is the reference time for the simulated update
 */
void Avatar::updateStatus(int now) {
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
	if (x == 0)             direction = -direction;
        else if (x == SIZE-1)   direction = -direction;
        else if (y == 0)        direction = 180 - direction;
        else if (y == SIZE-1)   direction = 180 - direction;
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
	if (direction < 0) direction += 360;

	// update speed
	if ((r = randfrac()) <= 0.1) {
		if (speed == SLOW || speed == FAST) speed = MEDIUM;
		else if (r < 0.05) 		    speed = SLOW;
		else 				    speed = FAST;
	}
}

/** Return the multicast group number associated with given position.
 *  Assumes that SIZE is an integer multiple of GRID
 *  @param x1 is the x coordinate of the position of interest
 *  @param y1 is the y coordinate of the position of interest
 */
int Avatar::groupNum(int x1, int y1) {
	return 1 + (x1/GRID) + (y1/GRID)*(SIZE/GRID);
}

/** Update the set of multicast subscriptions based on current position.
 */
void Avatar::updateSubscriptions() {
	const double SQRT2 = 1.41421356;
	
	int myGroup = groupNum(x,y);
	UiDlist *newGroups = new UiDlist(MAXGROUPS);
	newGroups->addLast(myGroup);

	int x1, y1, g;
	x1 = min(SIZE-1, x+VISRANGE);
	if (!newGroups->member(g = groupNum(x1,y)))  newGroups->addLast(g);
	x1 = max(0, x-VISRANGE);
	if (!newGroups->member(g = groupNum(x1,y)))  newGroups->addLast(g);

	y1 = min(SIZE-1, y+VISRANGE);
	if (!newGroups->member(g = groupNum(x,y1)))  newGroups->addLast(g);
	y1 = max(0, y-VISRANGE);
	if (!newGroups->member(g = groupNum(x,y1)))  newGroups->addLast(g);

	x1 = min(SIZE-1, (int) (x+VISRANGE/SQRT2));
	y1 = min(SIZE-1, (int) (y+VISRANGE/SQRT2));
	if (!newGroups->member(g = groupNum(x1,y1)))  newGroups->addLast(g);
	y1 = min(SIZE-1, (int) (y-VISRANGE/SQRT2));
	if (!newGroups->member(g = groupNum(x1,y1)))  newGroups->addLast(g);
	x1 = min(SIZE-1, (int) (x-VISRANGE/SQRT2));
	if (!newGroups->member(g = groupNum(x1,y1)))  newGroups->addLast(g);
	y1 = min(SIZE-1, (int) (y+VISRANGE/SQRT2));
	if (!newGroups->member(g = groupNum(x1,y1)))  newGroups->addLast(g);

	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	int nsub = 0; int nunsub = 0;
	g = newGroups->get(1);
	while (g != 0) {
		if (!mcGroups->member(g)) 
			pp[1+nsub++] = htonl(-g);
		g = newGroups->next(g);
	}

	g = mcGroups->get(1);
	while (g != 0) {
		if (!newGroups->member(g)) 
			pp[2+nsub+nunsub++] = htonl(-g);
		g = mcGroups->next(g);
	}

	if (nsub + nunsub == 0) { ps->free(p); delete newGroups; return; }

	delete mcGroups; mcGroups = newGroups;

	pp[0] = htonl(nsub);
	pp[1+nsub] = htonl(nunsub);

	h.setLength(4*(8+nsub+nunsub)); h.setPtype(SUB_UNSUB); h.setFlags(0);
	h.setComtree(comt); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send(p); 
}

/** Update the set of nearby Avatars.
 *  If the given packet is a status report, check to see if the
 *  sending avatar is visible. If it is visible, but not in our
 *  set of nearby avatars, then add it. If it is not visible
 *  but is in our set of nearby avatars, then delete it.
 *  Note: we assume that the visibility range and avatar speeds
 *  are such that we will get at least one report from a newly
 *  invisible avatar.
 */
void Avatar::updateNearby(int p) {
	ps->unpack(p);
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);
	if (ntohl(pp[0]) != STATUS_REPORT) return;

	int x1 = ntohl(pp[2]); int y1 = ntohl(pp[3]);
	double dx = x - x1; double dy = y - y1;

	uint64_t key = h.getSrcAdr(); key = (key << 32) | h.getSrcAdr();
	if(nearAvatars->lookup(key) == 0) {
		numNear++;
		nearAvatars->insert(key, nextAv++);
	}
	if (sqrt(dx*dx + dy*dy) <= VISRANGE) {
		if (visibleAvatars->lookup(key) == 0) {
			if (nextAv <= MAXNEAR) {
				visibleAvatars->insert(key, nextAv++);
				numVisible++;
			}
		}
	}
	return;
}