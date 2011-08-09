/** @file McmAvatar.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "stdinc.h"
#include "CommonDefs.h"
#include "McmAvatar.h"
/** usage:
 *       McmAvatar myIpAdr rtrIpAdr myAdr rtrAdr ccAdr comt finTime gridSize comt1 comt2 walls
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
	ipa_t myIpAdr, rtrIpAdr; fAdr_t myAdr, rtrAdr, ccAdr;
	int comt, comt1, comt2, finTime, gridSize;

	if (argc != 12 ||
	    (myIpAdr  = Np4d::ipAddress(argv[1])) == 0 ||
	    (rtrIpAdr = Np4d::ipAddress(argv[2])) == 0 ||
	    (myAdr  = Forest::forestAdr(argv[3])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[4])) == 0 ||
	    (ccAdr = Forest::forestAdr(argv[5])) == 0 ||
	    sscanf(argv[6],"%d", &comt) != 1 ||
	    sscanf(argv[7],"%d", &finTime) != 1 ||
	    sscanf(argv[8],"%d", &gridSize) != 1 ||
	    sscanf(argv[9],"%d", &comt1) != 1 ||
	    sscanf(argv[10], "%d", &comt2) != 1)
		fatal("usage: McmAvatar myIpAdr rtrIpAdr myAdr rtrAdr "
		      		    "comtree finTime");
	
	char * walls = argv[11];
	McmAvatar avatar(myIpAdr,rtrIpAdr,ccAdr,myAdr,rtrAdr, comt, comt1, comt2, gridSize, walls);
	if (!avatar.init()) fatal("McmAvatar:: initialization failure");
	avatar.run(1000000*finTime);
}

/** Constructor allocates space and initializes private data.
 * 
 *  @param mipa is this host's IP address
 *  @param ripa is the IP address of the access router for this host
 *  @param ma is the forest address for this host
 *  @param ra is the forest address for the access router
 *  @param ct is the comtree used for the virtual world
 *  @param ct1 is the lower bound on the range of comtrees to jump to
 *  @param ct2 is the upper bound on the range of comtrees to jump to
 *  @param gridSize is the size of one grid space
 *  @param walls a hex representation of the walls in the maze
 */
McmAvatar::McmAvatar(ipa_t mipa, ipa_t ripa, fAdr_t ccAdr, fAdr_t ma, fAdr_t ra, comt_t ct, comt_t ct1, comt_t ct2, int gridSize, char * walls)
		: myIpAdr(mipa), rtrIpAdr(ripa), CC_Adr(ccAdr), myAdr(ma), rtrAdr(ra),
		  comt(ct), comt1(ct1), comt2(ct2), SIZE(GRID*gridSize), WALLS(walls) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);

	// initialize avatar to random position
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0)
		fatal("McmAvatar::McmAvatar: gettimeofday failure");
	//srand(tv.tv_usec);
	srand(myAdr);
	x = randint(0,SIZE-1); y = randint(0,SIZE-1);
	direction = (double) randint(0,359);
	deltaDir = 0;
	speed = MEDIUM;

	mcGroups = new UiDlist((SIZE/GRID)*(SIZE/GRID));
	nearMcmAvatars = new UiHashTbl(MAXNEAR);
	visibleMcmAvatars = new UiHashTbl(MAXNEAR);
	
	//set up bitset
	if((SIZE/GRID)*(SIZE/GRID) >= 10000)
		fatal("McmAvatarController::McmAvatarController bitset too small; hex string too large");
	for(int i = 0; i < (SIZE/GRID)*(SIZE/GRID); i++) {
		char s = WALLS[i];
		int x;
		sscanf(&s,"%x",&x);
		wallsSet[i*4] = x&8;
		wallsSet[i*4+1] = x&4;
		wallsSet[i*4+2] = x&2;
		wallsSet[i*4+3] = x&1;
	}
	
	visibility = new bool*[(SIZE/GRID)*(SIZE/GRID)];
	for(int i = 0; i < (SIZE/GRID)*(SIZE/GRID); i++)
		visibility[i] = new bool[(SIZE/GRID)*(SIZE/GRID)];
	for(int i = 0; i < (SIZE/GRID)*(SIZE/GRID); i++) {
		for(int j = i; j < (SIZE/GRID)*(SIZE/GRID); j++) {
			if(i==j) {
				visibility[i][j] = true;
				visibility[j][i] = true;
			} else {
				visibility[i][j] = isVis(i,j);
				visibility[j][i] = visibility[i][j];
			}
		}
	}
	numVisible = numNear = stableNumNear = stableNumVisible = 0;
	nextAv = 1;
}

McmAvatar::~McmAvatar() { delete mcGroups; delete nearMcmAvatars; delete ps; }

/** Initialize a datagram socket for nonblocking IO.
 *  @return true on success, false on failure
 */
bool McmAvatar::init() {
	sock = Np4d::datagramSocket();

	return (sock >= 0 &&
        	Np4d::bind4d(sock, myIpAdr, 0) &&
        	Np4d::nonblock(sock));
}

/** Main McmAvatar processing loop.
 *  Operates on a cycle with a period of UPDATE_PERIOD milliseconds,
 *  Each cycle, update the current position, direction, speed;
 *  issue new SUB_UNSUB packet if necessary; read all incoming 
 *  status reports and update the set of nearby avatars;
 *  finally, send new status report
 * 
 *  @param finishTime is the number of microseconds to 
 *  to run before halting.
 */
void McmAvatar::run(int finishTime) {
	connect(); 		// send initial connect packet

	uint32_t now;    	// free-running microsecond time
	uint32_t nextTime;	// time of next operational cycle
	now = nextTime = 0;
	while (now <= finishTime) {
		//reset hashtables and report
		nearMcmAvatars->clear();
		visibleMcmAvatars->clear();
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
		if(nextTime % 10000000 == 0) {
			sendCtlPkt(false,comt); //leave current
			comt = comt==comt1 ? comt2 : comt1;
			sendCtlPkt(true,comt); //join other
		}
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
void McmAvatar::sendStatus(int now) {
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
/*Send a control packet to the ComtreeController indicating joining or leaving a comtree
 *@param join is true if the avatar wishes to join the comtree, false otherwise
 *@param comtree is the comtree number that the avatar wishes to join or leave
 */
void McmAvatar::sendCtlPkt(bool join, int comtree) {
	packet p = ps->alloc();
	if(p==0)
		fatal("McmAvatar::sendCtlPkt: Not enough space to alloc packet");
	CtlPkt cp;
	cp.setAttr(COMTREE_NUM,comtree);
	if(join)
		cp.setCpType(CLIENT_JOIN_COMTREE);
	else
		cp.setCpType(CLIENT_LEAVE_COMTREE);
	cp.setRrType(REQUEST);
	cp.setSeqNum(1);
	int len = cp.pack(ps->getPayload(p));
	PacketHeader& h = ps->getHeader(p); h.setLength(Forest::HDR_LENG + len + sizeof(uint32_t));
	h.setPtype(CLIENT_SIG);	h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr);
	h.setDstAdr(CC_Adr);
	h.pack(ps->getBuffer(p));

	send2CC(p);
}
/*sends a packet to the ComtreeController
 *@param p is the packet to be sent
 */
void McmAvatar::send2CC(int p) {
	int length = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(sock,(void *) ps->getBuffer(p),length,rtrIpAdr, Forest::ROUTER_PORT);
	if (rv == -1) fatal("McmAvatar::send: failure in sendto");
	ps->free(p);
}
/** Send initial connect packet, using comtree 1 (the signalling comtree).
 */
void McmAvatar::connect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send(p);
}


/** Send final disconnect packet.
 */
void McmAvatar::disconnect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send(p);
}

/** Send packet and recycle storage.
 */
void McmAvatar::send(int p) {
	int length = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(sock,(void *) ps->getBuffer(p),length,
		    		rtrIpAdr, Forest::ROUTER_PORT);
	if (rv == -1) fatal("McmAvatar::send: failure in sendto");
	ps->free(p);
}

/** Return next waiting packet or 0 if there is none. 
 */
int McmAvatar::receive() { 
	int p = ps->alloc();
	if (p == 0) return 0;
	PacketHeader& h = ps->getHeader(p);
	//check if this is a control packet response
	if(h.getComtree()==1) return 0;
        buffer_t& b = ps->getBuffer(p);

	ipa_t remoteIp; ipp_t remotePort;
        int nbytes = Np4d::recvfrom4d(sock, &b[0], 1500,
				      remoteIp, remotePort);
        if (nbytes < 0) {
                if (errno == EWOULDBLOCK) {
                        ps->free(p); return 0;
                }
                fatal("McmAvatar::receive: error in recvfrom call");
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
void McmAvatar::updateStatus(int now) {
	const double PI = 3.141519625;
	double r;

	// update position
	double dist = (speed * UPDATE_PERIOD)/1000;
	double dirRad = direction * (2*PI/360);
	int prevRegion = groupNum(x,y);
	x += (int) (dist * sin(dirRad));
	y += (int) (dist * cos(dirRad));
	x = max(x,0); x = min(x,SIZE-1);
	y = max(y,0); y = min(y,SIZE-1);
	int postRegion = groupNum(x,y);
	//bounce off walls
	if(postRegion!=prevRegion) {
		if(prevRegion==postRegion+1 && wallsSet[prevRegion-1]==1) {
			direction = -direction;
			x = (prevRegion-1)%(SIZE/GRID)*GRID+1;
		} else if(prevRegion==postRegion-1 && wallsSet[postRegion-1]==1) {
			direction = -direction;
			x = (postRegion-1)%(SIZE/GRID)*GRID-1;
		} else if(prevRegion==postRegion+(SIZE/GRID) && wallsSet[prevRegion-1]==0) {
			direction = 180-direction;
			y = ((prevRegion-1)/(SIZE/GRID))*GRID+1;
		} else if(prevRegion==postRegion-(SIZE/GRID) && wallsSet[postRegion-1]==0) {
			direction = 180-direction;
			y = ((postRegion-1)/(SIZE/GRID))*GRID-1;
		}
	}

	// adjust direction and deltaDir
	else if (x == 0)             direction = -direction;
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
int McmAvatar::groupNum(int x1, int y1) {
	return 1 + (x1/GRID) + (y1/GRID)*(SIZE/GRID);
}

/**Returns true if region 1 has any visibility to region 2
 * @param region1 is the region from which we are interested in visiblity
 * @param region2 is the region to which we are interested in visibility
 */
bool McmAvatar::isVis(int region1, int region2) {
	int region1xs[4];
	int region1ys[4];
	int region2xs[4];
	int region2ys[4];
	region1xs[0] = region1xs[2] = (region1 % (SIZE/GRID)) * GRID+1;
	region1ys[0] = region1ys[1] = ((region1/(SIZE/GRID))+1) * GRID-1;
	region1xs[1] = region1xs[3] = (region1 % (SIZE/GRID)+1) * GRID-1;
	region1ys[2] = region1ys[3] = (region1/(SIZE/GRID)) * GRID+1;
	region2xs[0] = region2xs[2] = (region2 % (SIZE/GRID)) * GRID+1;
	region2ys[0] = region2ys[1] = ((region2/(SIZE/GRID))+1) * GRID-1;
	region2xs[1] = region2xs[3] = (region2 % (SIZE/GRID)+1) * GRID-1;
	region2ys[2] = region2ys[3] = (region2/(SIZE/GRID)) * GRID+1;
	
	for(size_t i = 0; i < 4; i++) {
		for(size_t j = 0; j < 4; j++) {
			bool temp = true;
			for(size_t k = 0; k < (SIZE/GRID)*(SIZE/GRID); k++) {
				if(wallsSet[k] == 0) {
					if(linesIntersect((double)region1xs[i],(double)region1ys[i],(double)region2xs[j],(double)region2ys[j],(double)(k % (SIZE/GRID)) * GRID,(double)(k/(SIZE/GRID)) * GRID,(double)(k % (SIZE/GRID)) * GRID + GRID,(double) (k/(SIZE/GRID)) * GRID))
						temp = false;
				} else {
					if(linesIntersect((double)region1xs[i],(double)region1ys[i],(double)region2xs[j],(double)region2ys[j],(double)(k % (SIZE/GRID)) * GRID,(double)(k/(SIZE/GRID)) * GRID,(double)(k % (SIZE/GRID)) * GRID,(double) ((k/(SIZE/GRID)) + 1) * GRID))
						temp = false;
				}
			}
			if(temp)
				return true;
		}
	}
	return false;
}
/*Returns true if lines intersect
 *@param ax is the x of the first point of the first line
 *@param ay is the y of the first point of the first line
 *@param bx is the x of the second point of the first line
 *@param by is the y of the second point of the first line
 *@param cx is the x of the first point of the second line
 *@param cy is the y of the first point of the second line
 *@param dx is the x of the second point of the second line
 *@param dy is the y of the seocnd point of the second line
 */
bool McmAvatar::linesIntersect(double ax, double ay, double bx, double by , double cx ,double cy, double dx,double dy) {
	double distAB;
	double theCos;
	double theSin;
	double newX;
	double posAB;
	if (ax==bx && ay==by || cx==dx && cy==dy) return false;
	if (ax == cx && ay == cy || bx == cx && by == cy) return true;
	if (ax==dx && ay==dy || bx==dx && by==dy) return true;
	bx-=ax; by-=ay;
	cx-=ax; cy-=ay;
	dx-=ax; dy-=ay;
	distAB=sqrt(bx*bx+by*by);
	theCos = bx / distAB;            
	theSin = by / distAB;            
	newX = cx * theCos + cy * theSin;            
	cy  = cy * theCos - cx * theSin; cx = newX;
	newX = dx * theCos + dy * theSin;
	dy  = dy * theCos - dx * theSin; dx = newX;
	if (cy<0. && dy<0. || cy>=0. && dy>=0.) return false;
	posAB=dx+(cx-dx)*dy/(dy-cy);
	if (posAB<0. || posAB>distAB) return false;
	return true;
}

/** Update the set of multicast subscriptions based on current position.
 */
void McmAvatar::updateSubscriptions() {
	const double SQRT2 = 1.41421356;
	
	int myGroup = groupNum(x,y);
	UiDlist *newGroups = new UiDlist((SIZE/GRID)*(SIZE/GRID));
	newGroups->addLast(myGroup);
	for(size_t i = 1; i <= (SIZE/GRID)*(SIZE/GRID); i++) {
		if(visibility[myGroup-1][i-1] && !newGroups->member(i)) {
			newGroups->addLast(i);
		}
	}

	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	int nsub = 0; int nunsub = 0;
	int g = newGroups->get(1);
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

	send(p); ps->free(p);
}

/** Update the set of nearby McmAvatars.
 *  If the given packet is a status report, check to see if the
 *  sending avatar is visible. If it is visible, but not in our
 *  set of nearby avatars, then add it. If it is not visible
 *  but is in our set of nearby avatars, then delete it.
 *  Note: we assume that the visibility range and avatar speeds
 *  are such that we will get at least one report from a newly
 *  invisible avatar.
 */
void McmAvatar::updateNearby(int p) {
	ps->unpack(p);
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);
	if (ntohl(pp[0]) != STATUS_REPORT) return;

	int x1 = ntohl(pp[2]); int y1 = ntohl(pp[3]);
	double dx = x - x1; double dy = y - y1;

	uint64_t key = h.getSrcAdr(); key = (key << 32) | h.getSrcAdr();
	if(nearMcmAvatars->lookup(key) == 0) {
		numNear++;
		nearMcmAvatars->insert(key, nextAv++);
	}
	bool canSee = true;
	for(size_t i = 0; i < (SIZE/GRID)*(SIZE/GRID); i++) {
		int l2x1 = (i % (SIZE/GRID)) * GRID;
		int l2x2 = wallsSet[i] == 0 ? l2x1 + GRID : l2x1;
		int l2y1 = (i / (SIZE/GRID)) * GRID;
		int l2y2 = wallsSet[i] == 1 ? l2y1 + GRID : l2y1;
		if(linesIntersect(x1,y1,x,y,l2x1,l2y1,l2x2,l2y2))
			canSee = false;
	}
	if(canSee) {
		if (visibleMcmAvatars->lookup(key) == 0) {
			if (nextAv <= MAXNEAR) {
				visibleMcmAvatars->insert(key, nextAv++);
				numVisible++;
			}
		}
	}
}
