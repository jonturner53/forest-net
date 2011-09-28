/** @file ClientAvatar.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "stdinc.h"
#include "CommonDefs.h"
#include "ClientAvatar.h"
#include <string>
#include <algorithm>
/** usage:
 *       Avatar myIpAdr cliMgrIpAdr finTime gridSize walls uname pword
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
	ipa_t myIpAdr, cliMgrIpAdr; ipp_t port;
	int comt, comt1, comt2, finTime, gridSize;
	if (!argc==12 ||
	    (myIpAdr  = Np4d::ipAddress(argv[1])) == 0 ||
	    (cliMgrIpAdr   = Np4d::ipAddress(argv[2])) == 0 ||
	    sscanf(argv[3],"%d", &finTime) != 1 ||
	    sscanf(argv[4],"%d", &gridSize) != 1 ||
	    sscanf(argv[5],"%d", &comt) != 1 ||
	    sscanf(argv[6],"%d", &comt1) != 1 ||
	    sscanf(argv[7],"%d", &comt2) != 1 ||
	    sscanf(argv[8],"%d", &port) != 1)
		fatal("usage: Avatar myIpAdr cliMgrIpAdr finTime gridSize walls uname pword");
	Avatar avatar(myIpAdr,cliMgrIpAdr,comt1,comt2,port);
	if(!avatar.init()) fatal("Avatar:: initialization failure");
	avatar.login(string(argv[10]),string(argv[11]),false);
	avatar.setup(argv[9]);
	avatar.run(1000000*finTime);
}

/** Constructor allocates space and initializes private data.
 * 
 *  @param mipa is this host's IP address
 *  @param cmipa is the IP address of the client manager
 *  @param gridSize is width and height of the number of grid squares in the virtual world
 *  @param walls is a hex representation of the walls in the maze
 *  @param cmt is the comtree used for the virtual world
 *  @param cmt1 is the lower bound of comtrees in the virtual world
 *  @param cmt2 is the upper bound of comtrees in the virtual world
 *  @param prt is the port on which to listen for the MazeWorld Controller
 */
Avatar::Avatar(ipa_t mipa, ipa_t cmipa, comt_t cmt1, comt_t cmt2, ipp_t prt)
		: myIpAdr(mipa), cliMgrIpAdr(cmipa),
		  comt1(cmt1), comt2(cmt2), port(prt) {
}

/** setup performs initialization of variables that couldn't be done
 *  until after information is received from the ClientManager.
 */
void Avatar::setup(const char *wallsFile) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);
	setupWalls(wallsFile);
	srand(myAdr);
	// initialize avatar to random position
	x = randint(0,SIZE-1); y = randint(0,SIZE-1);
	direction = (double) randint(0,359);
	deltaDir = 0;
	speed = MEDIUM;
	controllerConnSock = -1;
	mcGroups = new UiDlist((SIZE/GRID)*(SIZE/GRID));
	nearAvatars = new UiHashTbl(MAXNEAR);
	visibleAvatars = new UiHashTbl(MAXNEAR);
	statPkt = new uint32_t[10];

	visibility = new bool*[(SIZE/GRID)*(SIZE/GRID)];
	for(int i = 0; i < (SIZE/GRID)*(SIZE/GRID); i++)
		visibility[i] = new bool[(SIZE/GRID)*(SIZE/GRID)];
	for(int i = 0; i < (SIZE/GRID)*(SIZE/GRID); i++) {
		for(int j = i; j < (SIZE/GRID)*(SIZE/GRID); j++) {
			if (i == j) {
				visibility[i][j] = true;
			} else {
				visibility[i][j] = isVis(i,j);
				visibility[j][i] = visibility[i][j];
			}
		}
	}
	numVisible = numNear = stableNumNear = stableNumVisible = 0;
	// print some visibility statistics for the log
	int n = SIZE/GRID;
	int maxVis; int totVis = 0;
	for (int h = 0; h < n*n; h++) {
		int vis = 0;
		for (int k = 0; k < n*n; k++) {
			if (k != h && visibility[h][k]) vis++;
		}
		maxVis = max(vis,maxVis); totVis += vis;
	}
	cout << "avg visible: " << totVis/(n*n)
	     << " max visibile: " << maxVis << endl;
}

void Avatar::setupWalls(const char *wallsFile) {
	ifstream ifs(wallsFile);
	if (ifs.fail()) fatal("setupWalls: cannot open walls file");
	int lineCnt = 1;
	WALLS = NULL;
	while(ifs.good()) {
		string line;
		getline(ifs,line);
		if (WALLS == NULL) {
			gridSize = line.size();
			SIZE = gridSize*GRID;
			WALLS = new int[gridSize*gridSize];
		} else if (line.size() < gridSize) {
			fatal("setupWalls: format error, all lines must have "
			      "same length");
		}
		for(int i = 0; i < gridSize; i++) {
			if(line[i] == '+') {
				WALLS[(gridSize-lineCnt)*gridSize+i] = 3;
			} else if(line[i] == '-') {
				WALLS[(gridSize-lineCnt)*gridSize+i] = 2;
			} else if(line[i] == '|') {
				WALLS[(gridSize-lineCnt)*gridSize+i] = 1;
			} else if(line[i] == ' ') {
				WALLS[(gridSize-lineCnt)*gridSize+i] = 0;
			} else {
				fatal("Unrecognized symbol in map file!");
			}
		}
		if (lineCnt == gridSize) break;
		lineCnt++;
	}
}

Avatar::~Avatar() { delete mcGroups; delete nearAvatars; delete ps; }

/** Initialize a datagram socket for nonblocking IO.
 *  @return true on success, false on failure
 */
bool Avatar::init() {
	CM_sock = Np4d::streamSocket();
	Controller_sock = Np4d::streamSocket();
	sock = Np4d::datagramSocket();
	if(CM_sock < 0 || sock < 0 || Controller_sock < 0) return false;
	//if(!Np4d::bind4d(Controller_sock,Np4d::myIpAddress(),port))
	//	return false;
	return	Np4d::bind4d(sock,myIpAdr,0) &&
		Np4d::nonblock(sock) &&
	//	Np4d::listen4d(Controller_sock) &&
	//	Np4d::nonblock(Controller_sock) &&
        	Np4d::bind4d(CM_sock, myIpAdr, 0) &&
		Np4d::connect4d(CM_sock,cliMgrIpAdr,CLIMGR_PORT);
}

/** Send username and password to the Client manager, and receive
 *@param uname is the username to log in with
 *@param pword is the password to log in with
 *@param newuser is whether the username has logged in before
 *@return true on success, false on failure
 */
void Avatar::login(string uname, string pword, bool newuser) {
	string buf;
	string portString;
	Misc::getTime(); // this starts clock
	Misc::num2string(Np4d::getSockPort(sock),portString);
	if(newuser) {
		buf = "n " + uname + " " + pword + " " + portString;
	} else {
		buf = "o " + uname + " " + pword + " " + portString;
	}
	Np4d::sendBufBlock(CM_sock,(char *) buf.c_str(),buf.size()+1);
	//receive rtrAdr, myAdr,rtrIp, CC_Adr
	Np4d::recvIntBlock(CM_sock,(uint32_t&)rtrAdr);
	Np4d::recvIntBlock(CM_sock,(uint32_t&)myAdr);
	Np4d::recvIntBlock(CM_sock,(uint32_t&)rtrIpAdr);
	Np4d::recvIntBlock(CM_sock,(uint32_t&)CC_Adr);
	close(CM_sock);
	cerr << "assigned address ";
	Forest::writeForestAdr(cerr,myAdr); cerr << endl;
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
	uint32_t lastComtSwitch;// time of the last time a comt switched
	now = nextTime = lastComtSwitch = Misc::getTime();
	comt = randint(comt1,comt2);
	int comtSwitchTime = randint(60,80);
	sendCtlPkt2CC(true,comt); // join initial comtree
	bool replyPending = true;	// true when waiting for reply to a join
	while (now <= finishTime) {
		//reset hashtables and report
		nearAvatars->clear();
		visibleAvatars->clear();
		stableNumNear = numNear; stableNumVisible = numVisible;
		numVisible = numNear = 0;

		now = Misc::getTime();
		if (!replyPending) updateSubscriptions();
		int p;
		while ((p = receive()) != 0) {
			PacketHeader& h = ps->getHeader(p);
			updateNearby(p);
			if (h.getPtype() == CLIENT_SIG) {
				// should really check cp fields too,
				// but this will do for now
				replyPending = false;
			}

			if(controllerConnSock > 0) {
				uint32_t * pp = ps->getPayload(p);
				uint64_t key = h.getSrcAdr(); key = (key << 32) | h.getSrcAdr();
				statPkt[0] = htonl(now);
				statPkt[1] = htonl(h.getSrcAdr());
				statPkt[2] = pp[2];
				statPkt[3] = pp[3];
				statPkt[4] = pp[4];
				statPkt[5] = pp[5];
				statPkt[6] = pp[6];
				statPkt[7] = pp[7];
				statPkt[8] = htonl(h.getComtree());
				statPkt[9] = visibleAvatars->lookup(key) == 0 ? htonl(2) : htonl(3);
				send2Controller();
			}
			ps->free(p);
		}
		check4input(now);
		sendStatus(now);

		nextTime += 1000*UPDATE_PERIOD;
		if(controllerConnSock  < 0 && (now-lastComtSwitch) > (1000000*comtSwitchTime)) {
			lastComtSwitch = now;
			int newcomt = randint(comt1,comt2);
			if(comt != newcomt) {
				unsubAll();
				switchComtree(newcomt);
				replyPending = true;
			}
			comtSwitchTime = randint(60,100);
		}
		now = Misc::getTime();
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

/*Switch to comtree and send control packet to ComtreeController
 *@param comtree to switch to
 */
void Avatar::switchComtree(int comtree) {
	sendCtlPkt2CC(false,comt);
	comt = comtree;
	sendCtlPkt2CC(true,comt);
}


void Avatar::send2Controller() {
	int nbytes = 10*sizeof(uint32_t);
	char * p = (char *) statPkt;
	while(nbytes > 0) {
		int n = write(controllerConnSock, (void *) p, nbytes);
		if (n < 0) fatal("Avatar::send2Controller: failure in write");
		p += n; nbytes -= n;
	}
}

/*Send a control packet to the ComtreeController indicating joining or leaving a comtree
 *@param join is true if the avatar wishes to join the comtree, false otherwise
 *@param comtree is the comtree number that the avatar wishes to join or leave
 */
void Avatar::sendCtlPkt2CC(bool join, int comtree) {
        packet p = ps->alloc();
        if(p==0)
                fatal("Avatar::sendCtlPkt2CC: no packets left to allocate");
        CtlPkt cp;
        cp.setAttr(COMTREE_NUM,comtree);
        cp.setAttr(PEER_IP,myIpAdr);
        cp.setAttr(PEER_PORT,Np4d::getSockPort(sock));
        if(join)
                cp.setCpType(CLIENT_JOIN_COMTREE);
        else
                cp.setCpType(CLIENT_LEAVE_COMTREE);
        cp.setRrType(REQUEST);
        cp.setSeqNum(1);
        int len = cp.pack(ps->getPayload(p));
        PacketHeader& h = ps->getHeader(p);
	h.setLength(Forest::OVERHEAD + len);
        h.setPtype(CLIENT_SIG); h.setFlags(0);
        h.setComtree(1); h.setSrcAdr(myAdr);
        h.setDstAdr(CC_Adr);
        h.pack(ps->getBuffer(p));
	send(p);
}


/** Update status of avatar based on passage of time.
 *  @param now is the reference time for the simulated update
 *  @param input is the input from the user; 1 - left, 2 - up, 3 - right, 4 - down
 */
void Avatar::updateStatus(int now,int input) {
	const double PI = 3.141519625;
        double r;

        double dist = speed;
        double dirRad = direction * (2*PI/360);
        int prevRegion = groupNum(x,y);
        x += (int) (dist * sin(dirRad));
        y += (int) (dist * cos(dirRad));
        x = max(x,0); x = min(x,SIZE-1);
        y = max(y,0); y = min(y,SIZE-1);
        int postRegion = groupNum(x,y);

        if(postRegion!=prevRegion) {
                if(prevRegion==postRegion+1 &&
			(WALLS[prevRegion-1]==1 || WALLS[prevRegion-1]==3)) {
                        direction = -direction;
                        x = (prevRegion-1)%(SIZE/GRID)*GRID+1;
                } else if(prevRegion==postRegion-1 &&
			(WALLS[postRegion-1]==1 || WALLS[postRegion-1]==3)) {
                        direction = -direction;
                        x = (postRegion-1)%(SIZE/GRID)*GRID-1;
                } else if(prevRegion==postRegion+(SIZE/GRID) &&
			(WALLS[prevRegion-1]==2 || WALLS[prevRegion-1]==3)) {
                        direction = 180-direction;
                        y = ((prevRegion-1)/(SIZE/GRID))*GRID+1;
                } else if(prevRegion==postRegion-(SIZE/GRID) &&
			(WALLS[postRegion-1]==2 || WALLS[postRegion-1]==3)) {
                        direction = 180-direction;
                        y = ((postRegion-1)/(SIZE/GRID))*GRID-1;
                }
        }
        else if (x == 0)        direction = -direction;
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
	
	statPkt[0] = htonl((uint32_t)now);
	statPkt[1] = htonl((uint32_t)myAdr);
	statPkt[2] = htonl((uint32_t)x);
	statPkt[3] = htonl((uint32_t)y);
	statPkt[4] = htonl((uint32_t)direction);
	statPkt[5] = htonl((uint32_t)speed);
	statPkt[6] = htonl((uint32_t)stableNumVisible);
	statPkt[7] = htonl((uint32_t)stableNumNear);
	statPkt[8] = htonl((uint32_t)comt);
	statPkt[9] = htonl(1);
	send2Controller();
}

/*
 * Check for input from the Controller
 *@param now is the current time as given by Misc::now()
 *
 */

void Avatar::check4input(int now) {
	if(controllerConnSock < 0) {
		controllerConnSock = Np4d::accept4d(Controller_sock);
		if(controllerConnSock < 0) {
			updateStatus(now);
			return;
		}
		if(!Np4d::nonblock(controllerConnSock))
			fatal("can't make connection socket nonblocking");
	}
	uint32_t direction;
	int nbytes = read(controllerConnSock, (char *) &direction,
			  sizeof(uint32_t));
	if(nbytes < 0) {
		if(errno == EAGAIN) {
			updateStatus(now,0);
			return;
		}
		fatal("Avatar::check4input: error in read call");
	} else if(nbytes < sizeof(uint32_t)) {
		fatal("Avatar::check4input: incomplete number");
	}
	direction = ntohl(direction);
	updateStatus(now,direction);
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
	if(h.getComtree()!=comt) {
		ps->free(p);
		return 0;
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
	double dist = speed;
	double dirRad = direction * (2*PI/360);
	int prevRegion = groupNum(x,y);
	x += (int) (dist * sin(dirRad));
	y += (int) (dist * cos(dirRad));
	x = max(x,0); x = min(x,SIZE-1);
	y = max(y,0); y = min(y,SIZE-1);
	int postRegion = groupNum(x,y);

	//bounce off walls
	if (postRegion != prevRegion) {
		if (prevRegion == postRegion+1 &&
			(WALLS[prevRegion-1]==1 || WALLS[prevRegion-1]==3)) {
			direction = -direction;
			x = (prevRegion-1)%(SIZE/GRID)*GRID+1;
		} else if (prevRegion == postRegion-1 &&
			(WALLS[postRegion-1]==1 || WALLS[postRegion-1]==3)) {
			direction = -direction;
			x = (postRegion-1)%(SIZE/GRID)*GRID-1;
		} else if (prevRegion == postRegion+(SIZE/GRID) &&
			(WALLS[postRegion-1]==2 || WALLS[postRegion-1]==3)) {
			direction = 180-direction;
			y = ((prevRegion-1)/(SIZE/GRID))*GRID+1;
		} else if (prevRegion == postRegion-(SIZE/GRID) &&
			(WALLS[prevRegion-1]==2 || WALLS[prevRegion-1]==3)) {
			direction = 180-direction;
			y = ((postRegion-1)/(SIZE/GRID))*GRID-1;
		}
	}
	else if (x == 0)        direction = -direction;
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

/**Returns true if region 1 has any visibility to region 2
 * @param region1 is the region from which we are interested in visiblity
 * @param region2 is the region to which we are interested in visibility
 */
bool Avatar::isVis(int region1, int region2) {
	int region1xs[4]; int region1ys[4];
	int region2xs[4]; int region2ys[4];

	int n = SIZE/GRID; // we have nxn regions
	int row1 = region1/n; int col1 = region1%n;
	int row2 = region2/n; int col2 = region2%n;

	region1xs[0] = region1xs[2] = col1 * GRID + 1;
	region1ys[0] = region1ys[1] = (row1+1) * GRID - 1;
	region1xs[1] = region1xs[3] = (col1+1) * GRID - 1;
	region1ys[2] = region1ys[3] = row1 * GRID + 1;
	region2xs[0] = region2xs[2] = col2 * GRID + 1;
	region2ys[0] = region2ys[1] = (row2+1) * GRID - 1;
	region2xs[1] = region2xs[3] = (col2+1) * GRID - 1;
	region2ys[2] = region2ys[3] = row2 * GRID + 1;

	int minRow = min(row1,row2); int maxRow = max(row1,row2);
	int minCol = min(col1,col2); int maxCol = max(col1,col2);

	for(size_t i = 0; i < 4; i++) {
		for(size_t j = 0; j < 4; j++) {
			bool temp = true;
			double ax = (double) region1xs[i];
			double ay = (double) region1ys[i];
			double bx = (double) region2xs[j];
			double by = (double) region2ys[j];
			// line segment ab connects a corner of region1
			// to a corner of region2
			for (int i = minRow; i <= maxRow; i++) {
			for (int j = minCol; j <= maxCol; j++) {
				double cx = (double) j*GRID;
				double cy = (double) (i+1)*GRID;
				// (cx,cy) is top left corner of region (i,j)
				int k = i*n+j; // k=region index
				if (WALLS[k] == 1 || WALLS[k] == 3) {
					// region k has a left side wall
					// check if ab intersects it
					double dx = cx; double dy = cy-GRID;
					if (linesIntersect(ax,ay,bx,by,
							   cx,cy,dx,dy)) {
						temp = false; break;
					} else {
					}
				}
				if (WALLS[k] == 2 || WALLS[k] == 3) {
					// region k has a top wall
					// check if ab intersects it
					double dx = cx+GRID; double dy = cy;
					if (linesIntersect(ax,ay,bx,by,
							   cx,cy,dx,dy)) {
						temp = false; break;
					} else {
					}
				}
			}
			if (!temp) break;
			}
			if (temp) return true;
		}
	}
	return false;
}

bool Avatar::linesIntersect(double ax, double ay, double bx, double by,
			    double cx ,double cy, double dx, double dy) {
	double epsilon = .0001; // two lines are considered vertical if
				// x values differ by less than epsilon
	// first handle special case of two vertical lines
	if (abs(ax-bx) < epsilon && abs(cx-dx) < epsilon) {
		return abs(ax-cx) < epsilon && max(ay,by) >= min(cy,dy)
				  	    && min(ay,by) <= max(cy,dy);
	}
	// now handle cases of one vertical line
	if (abs(ax-bx) < epsilon) {
		double s2 = (dy-cy)/(dx-cx); // slope of second line
		double i2 = cy - s2*cx; // y-intercept of second line
		double y = s2*ax + i2;	// lines intersect at (ax,y)
		return  (y >= min(ay,by) && y <= max(ay,by) &&
			 y >= min(cy,dy) && y <= max(cy,dy));
	}
	if (abs(cx-dx) < epsilon) {
		double s1 = (by-ay)/(bx-ax); // slope of first line
		double i1 = ay - s1*ax; // y-intercept of first line
		double y = s1*cx + i1;	// lines intersect at (cx,y)
		return  (y >= min(ay,by) && y <= max(ay,by) &&
			 y >= min(cy,dy) && y <= max(cy,dy));
	}
	double s1 = (by-ay)/(bx-ax);	// slope of first line
	double i1 = ay - s1*ax; 	// y-intercept of first line
	double s2 = (dy-cy)/(dx-cx);	// slope of second line
	double i2 = cy - s2*cx; 	// y-intercept of second line

	// handle special case of lines with equal slope
	// treat the slopes as equal if both slopes have very small
	// magnitude, or if their relative difference is small
	if (abs(s1)+abs(s2) <= epsilon ||
	    abs(s1-s2)/(abs(s1)+abs(s2)) < epsilon) {
		return (abs(i1-i2) < epsilon &&
			min(ax,bx) <= max(cx,dx) && max(ax,bx) >= min(cx,dx));
	}
	// now, to the common case
	double x = (i2-i1)/(s1-s2);	// x value where the lines interesect

	return (x >= min(ax,bx) && x <= max(ax,bx) &&
		x >= min(cx,dx) && x <= max(cx,dx));

/*
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
*/
}
/** Unsubscribe from all groups before switching comtrees
 */
void Avatar::unsubAll() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);
	int nunsub = 0;
	int g = mcGroups->get(1);
	while(g != 0) {
		pp[2+nunsub++] = htonl(-g);
		g = mcGroups->next(g);
	}
	pp[0] = htonl(0);
	pp[1] = htonl(nunsub);
	h.setLength(4*(8+nunsub)); h.setPtype(SUB_UNSUB); h.setFlags(0);
	h.setComtree(comt); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
	mcGroups->clear();
	send(p); ps->free(p);
}

/** Update the set of multicast subscriptions based on current position.
 */
void Avatar::updateSubscriptions() {
	const double SQRT2 = 1.41421356;
	
	int myGroup = groupNum(x,y);
	UiDlist *newGroups = new UiDlist((SIZE/GRID)*(SIZE/GRID));
	newGroups->addLast(myGroup);
	for (int i = 1; i <= (SIZE/GRID)*(SIZE/GRID); i++) {
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

	uint64_t key = h.getSrcAdr(); key = (key << 32) | h.getSrcAdr();
	if(nearAvatars->lookup(key) == 0) {
		if (numNear <= MAXNEAR)
			nearAvatars->insert(key, ++numNear);
	}
	bool canSee = true;
	for (int i = 0; i < (SIZE/GRID)*(SIZE/GRID); i++) {
		if (WALLS[i] == 1 || WALLS[i] == 3) {
			int l2x1 = (i % (SIZE/GRID)) * GRID; 
			int l2x2 = l2x1;
			int l2y1 = (i / (SIZE/GRID)) * GRID;
			int l2y2 = l2y1 + GRID;
			if(linesIntersect(x1,y1,x,y,
					  l2x1,l2y1,l2x2,l2y2)) {
				canSee = false; break;
			}
		}
		if (WALLS[i] == 2 || WALLS[i] == 3) {
			int l2x1 = (i % (SIZE/GRID)) * GRID; 
			int l2x2 = l2x1 + GRID;
			int l2y1 = (i / (SIZE/GRID)) * GRID + GRID;
			int l2y2 = l2y1;
			if(linesIntersect(x1,y1,x,y,
					  l2x1,l2y1,l2x2,l2y2)) {
				canSee = false; break;
			}
		}
	}
	if (canSee) {
		if (visibleAvatars->lookup(key) == 0) {
			if (numVisible <= MAXNEAR)
				visibleAvatars->insert(key, ++numVisible);
		}
	}
}
