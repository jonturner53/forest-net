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
#include "Avatar2Controller.h"
#include <netinet/tcp.h>
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
	ipa_t myIpAdr, extIpAdr, rtrIpAdr; fAdr_t myAdr, rtrAdr;
	int comt, finTime, gridSize;
	char * walls = argv[9];

	if (argc != 10 || 
	    (extIpAdr = Np4d::ipAddress(argv[1])) == 0 ||
	    (myIpAdr  = Np4d::ipAddress(argv[2])) == 0 ||
	    (rtrIpAdr = Np4d::ipAddress(argv[3])) == 0 ||
	    (myAdr  = Forest::forestAdr(argv[4])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[5])) == 0 ||
	    sscanf(argv[6],"%d", &comt) != 1 ||
	    sscanf(argv[7],"%d", &finTime) != 1 ||
	    sscanf(argv[8],"%d", &gridSize) != 1)
		fatal("usage: AvatarController myIpAdr rtrIpAdr myAdr rtrAdr "
		      		    "comtree finTime");
	if (extIpAdr == Np4d::ipAddress("127.0.0.1")) extIpAdr = Np4d::myIpAddress();
	Avatar avatar(extIpAdr,myIpAdr,rtrIpAdr,myAdr,rtrAdr, comt,gridSize, walls);
	if (!avatar.init()) fatal("AvatarController:: initialization failure");
	avatar.run(1000000*finTime);
}

/** Constructor allocates space and initializes private data.
 * 
 *  @param eipa is the IP address of the controller
 *  @param mipa is this host's IP address
 *  @param ripa is the IP address of the access router for this host
 *  @param ma is the forest address for this host
 *  @param ra is the forest address for the access router
 *  @param ct is the comtree used for the virtual world
 */
Avatar::Avatar(ipa_t eipa, ipa_t mipa, ipa_t ripa, fAdr_t ma, fAdr_t ra, comt_t ct, int gridSize, char * walls)
		: extIpAdr(eipa), intIp(mipa), rtrIpAdr(ripa), myAdr(ma), rtrAdr(ra),
		  comt(ct), SIZE(GRID*gridSize), WALLS(walls) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);

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
	connSock = -1;
	repCnt = 0;
	statPkt = new uint32_t[500];	
	
	//set up bitset
	if((SIZE/GRID)*(SIZE/GRID) >= 10000)
		fatal("AvatarController::AvatarController bitset too small; hex string too large");
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
	mcGroups = new UiDlist((SIZE/GRID)*(SIZE/GRID));
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
	intSock = Np4d::datagramSocket();

	if (intSock < 0 || 
	    !Np4d::bind4d(intSock,intIp,0) ||
	    !Np4d::nonblock(intSock)) {
		return false;
	}
	extSock = Np4d::streamSocket();
	if (extSock < 0)  return false;
	bool status = Np4d::bind4d(extSock,extIpAdr,AV_PORT);
	if (!status)  return false;
	return Np4d::listen4d(extSock) && Np4d::nonblock(extSock);
}



void Avatar::check4input(int now) { 
	if (connSock < 0) {
		connSock = Np4d::accept4d(extSock);
		if (connSock < 0) {
			updateStatus(now);
			return;
		}
		int on = 1;
		int tcpnodelayret = setsockopt(connSock,IPPROTO_TCP,TCP_NODELAY,&on,sizeof(int));
		if(tcpnodelayret == -1) {
			fatal("can't make connection socket nodelay");	
		}
		if (!Np4d::nonblock(connSock))
			fatal("can't make connection socket nonblocking");
	}
	uint32_t direction;
        int nbytes = read(connSock, (char *) &direction, sizeof(uint32_t));
        if (nbytes < 0) {
                if (errno == EAGAIN) {
			updateStatus(now,0);
			return;
		}
                fatal("Avatar::check4input: error in read call");
        } else if (nbytes < sizeof(uint32_t)) {
		fatal("Avatar::check4input: incomplete number");
	}
	direction = ntohl(direction);
	updateStatus(now,direction);
        return;
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
	while (now <= finishTime) {
		//reset hashtables and report
		nearAvatars->clear();
		visibleAvatars->clear();
		stableNumNear = numNear; stableNumVisible = numVisible;
		numVisible = numNear = 0;
		nextAv = 1;
		now = Misc::getTime();
		updateSubscriptions();
		int p;
		while ((p = receive()) != 0) {
			updateNearby(p);
			if(connSock > 0)
				updateStatus2cont(p,now);
			ps->free(p);
		}
		check4input(now);
		sendStatus(now);
		nextTime += 1000*UPDATE_PERIOD;
		useconds_t delay = nextTime - now;
		if (delay > 0) usleep(delay);
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
/** Send packet to controller.
 */
void Avatar::send2cont() {
	if (comt == 0) return;

	int nbytes = repCnt*NUMITEMS*sizeof(uint32_t);
	char* p = (char *) statPkt;
	while (nbytes > 0) {
		int n = write(connSock, (void *) p, nbytes);
		if (n < 0) fatal("AvatarController::send2cont: failure in write");
		p += n; nbytes -= n;
	}
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
void Avatar::updateStatus2cont(int p, int now) {
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	uint64_t key = h.getSrcAdr(); key = (key << 32) | h.getSrcAdr();

	if (h.getComtree() != comt) return;

	// if no room in statusPkt buffer for another report,
	// send current buffer
	if (repCnt > MAX_REPORTS) {
		send2cont(); repCnt = 0;
	}
	// add new report to the outgoing status packet
	statPkt[NUMITEMS*repCnt]   = htonl(now);
	statPkt[NUMITEMS*repCnt+1] = htonl(h.getSrcAdr());
	statPkt[NUMITEMS*repCnt+2] = pp[2];
	statPkt[NUMITEMS*repCnt+3] = pp[3];
	statPkt[NUMITEMS*repCnt+4] = pp[4];
	statPkt[NUMITEMS*repCnt+5] = pp[5];
	statPkt[NUMITEMS*repCnt+6] = pp[6];
	statPkt[NUMITEMS*repCnt+7] = pp[7];
	statPkt[NUMITEMS*repCnt+8] = htonl(h.getComtree());
	statPkt[NUMITEMS*repCnt+9] = visibleAvatars->lookup(key) == 0 ? htonl(2) : htonl(3);
	repCnt++;
}

/** Send packet and recycle storage.
 */
void Avatar::send(int p) {
	int length = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(intSock,(void *) ps->getBuffer(p),length,
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
        int nbytes = Np4d::recvfrom4d(intSock, &b[0], 1500,
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
 *  @param input is the input from the user; 1 - left, 2 - up, 3 - right, 4 - down
 */
void Avatar::updateStatus(int now,int input) {
	const double PI = 3.141519625;
        double r;

	double dist = (speed * UPDATE_PERIOD)/1000;
        double dirRad = direction * (2*PI/360);
        int prevRegion = groupNum(x,y);
        x += (int) (dist * sin(dirRad));
        y += (int) (dist * cos(dirRad));
        x = max(x,0); x = min(x,SIZE-1);
        y = max(y,0); y = min(y,SIZE-1);
        int postRegion = groupNum(x,y);

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
	else if (x == 0)             direction = -direction;
        else if (x == SIZE-1)   direction = -direction;
        else if (y == 0)        direction = 180 - direction;
        else if (y == SIZE-1)   direction = 180 - direction;

	// use input
	if(input==2) {
		if(speed == SLOW) speed = MEDIUM;
		else if(speed == MEDIUM) speed = FAST;
	} else if(input==4) {
		if(speed == FAST) speed = MEDIUM;
		else if(speed == MEDIUM) speed = SLOW;
	} else if(input==1) {
		direction -= 10;
	} else if(input==3) {
		direction += 10;
	}
	if (direction < 0) direction += 360;
	if (direction > 360) direction -= 360;
	//add self before sending
	if (input !=0 || repCnt > MAX_REPORTS) {
		send2cont(); repCnt = 0;
	}
	statPkt[NUMITEMS*repCnt]   = htonl((uint32_t)now);
	statPkt[NUMITEMS*repCnt+1] = htonl((uint32_t)myAdr);
	statPkt[NUMITEMS*repCnt+2] = htonl((uint32_t)x);
	statPkt[NUMITEMS*repCnt+3] = htonl((uint32_t)y);
	statPkt[NUMITEMS*repCnt+4] = htonl((uint32_t)direction);
	statPkt[NUMITEMS*repCnt+5] = htonl((uint32_t)speed);
	statPkt[NUMITEMS*repCnt+6] = htonl((uint32_t)stableNumVisible);
	statPkt[NUMITEMS*repCnt+7] = htonl((uint32_t)stableNumNear);
	statPkt[NUMITEMS*repCnt+8] = htonl((uint32_t)comt);
	statPkt[NUMITEMS*repCnt+9] = htonl(1);
	repCnt++;
}




/** Update status of avatar based on passage of time.
 *  @param now is the reference time for the simulated update
 */
void Avatar::updateStatus(int now) {
	const double PI = 3.141519625;
        double r;
	//update position
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
	return;
}

/** Return the multicast group number associated with given position.
 *  Assumes that SIZE is an integer multiple of GRID
 *  @param x1 is the x coordinate of the position of interest
 *  @param y1 is the y coordinate of the position of interest
 */
int Avatar::groupNum(int x1, int y1) {
	return 1 + (x1/GRID) + (y1/GRID)*(SIZE/GRID);
}


bool Avatar::isVis(int region1, int region2) {
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

bool Avatar::linesIntersect(double ax, double ay, double bx, double by , double cx ,double cy, double dx,double dy) {
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
void Avatar::updateSubscriptions() {
	const double SQRT2 = 1.41421356;
	
	int myGroup = groupNum(x,y);
	UiDlist *newGroups = new UiDlist((SIZE/GRID)*(SIZE/GRID));
	newGroups->addLast(myGroup);

	for(size_t i = 1; i <= (SIZE/GRID)*(SIZE/GRID); i++) {
                if(visibility[myGroup-1][i-1] && !newGroups->member(i))
                        newGroups->addLast(i);
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
		nearAvatars->insert(key, ++numNear);
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
                if (visibleAvatars->lookup(key) == 0) {
			visibleAvatars->insert(key, ++numVisible);
                }
        }

}
