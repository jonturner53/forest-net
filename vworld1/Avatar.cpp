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
#include <string>
#include <algorithm>
/** usage:
 *       Avatar myIpAdr cliMgrIpAdr walls firstComt lastComt uname pword finTime
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
int main(int argc, char *argv[]) {
	ipa_t myIpAdr, cliMgrIpAdr;
	comt_t firstComt, lastComt;
	uint32_t finTime;
	if (!argc == 9 ||
	    (myIpAdr  = Np4d::ipAddress(argv[1])) == 0 ||
	    (cliMgrIpAdr   = Np4d::ipAddress(argv[2])) == 0 ||
	    sscanf(argv[4],"%d", &firstComt) != 1 ||
	    sscanf(argv[5],"%d", &lastComt) != 1 ||
	    sscanf(argv[8],"%d", &finTime) != 1)
		fatal("usage: Avatar myIpAdr cliMgrIpAdr walls "
		      "firstComt lastComt uname pword finTime");
	Avatar avatar(myIpAdr,firstComt,lastComt);
	string uname = string(argv[6]);
	string pword = string(argv[7]);
	char* wallsFile = argv[3];
	if (!avatar.init(cliMgrIpAdr,uname,pword,wallsFile))
		fatal("Avatar:: initialization failure");
	avatar.run(1000000*finTime);
}

/** Constructor allocates space and initializes private data.
 * 
 *  @param mipa is this host's IP address
 *  @param fc is the first comtree in the range used by the avatar
 *  @param lc is the last comtree in the range used by the avatar
 */
Avatar::Avatar(ipa_t mipa, comt_t fc, comt_t lc)
		: myIpAdr(mipa), firstComt(fc), lastComt(lc) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);
	mySubs = new set<int>();
	nearAvatars = new HashSet(MAXNEAR);
	visibleAvatars = new HashSet(MAXNEAR);

	numNear = numVisible = 0;
	seqNum = 1;
}

Avatar::~Avatar() {
	delete mySubs; delete nearAvatars; delete ps; delete [] walls;
	if (sock >= 0) close(sock);
	if (extSock >= 0) close(extSock);
}

/** Perform all required initialization.
 *  @return true on success, false on failure
 */
bool Avatar::init(ipa_t cmIpAdr, string& uname, string& pword,
		  char *wallsFile) {
	Misc::getTime(); // starts clock running

	// open and configure forest socket
	sock = Np4d::datagramSocket();
	if (sock<0 || !Np4d::bind4d(sock,myIpAdr,0) || !Np4d::nonblock(sock)) {
		cerr << "Avatar::init: could not open/configure forest socket "
			"sockets\n";
		return false;
	}

	// open and configure external socket
	extSock = Np4d::streamSocket();
	if (extSock < 0 || !Np4d::bind4d(extSock,Np4d::myIpAddress(),0) ||
	    !Np4d::listen4d(extSock) || !Np4d::nonblock(extSock)) {
		cerr << "Avatar::init: could not open/configure external "
			"socket\n";
		return false;
	}
	connSock = -1; // connection socket for extSock; -1 means not connected
	string s;
	cout << "external socket: " << Np4d::ip2string(Np4d::myIpAddress(),s)
	     << "/" << Np4d::getSockPort(extSock) << endl;
	cout.flush();

	// login and setup walls 
	if (!login(cmIpAdr, uname, pword)) return false;
	if (!setupWalls(wallsFile)) return false;

	// initialize avatar to a random position
	srand(myAdr);
	x = randint(0,GRID*worldSize-1); y = randint(0,GRID*worldSize-1);
	direction = (double) randint(0,359);
	deltaDir = 0;
	speed = MEDIUM;

	return true;
}

/** Send username and password to the Client manager, and receive.
 *  @param cmIpAdr is the IP address of the client manager
 *  @param uname is the username to log in with
 *  @param pword is the password to log in with
 *  @return true on success, false on failure
 */
bool Avatar::login(ipa_t cmIpAdr, string uname, string pword) {
	// open socket to client manager
	int cmSock = Np4d::streamSocket();
        if (cmSock < 0 ||
	    !Np4d::bind4d(cmSock, myIpAdr, 0) ||
	    !Np4d::connect4d(cmSock,cmIpAdr,CLIMGR_PORT)) {
		cerr << "Avatar::login: cannot open/configure socket to "
			"ClientMgr\n";
		return false;
	}
	// send login string to Client Manager
	stringstream ss;
	ss << uname << " " << pword << " " << Np4d::getSockPort(sock);
	Np4d::sendBufBlock(cmSock,(char *) ss.str().c_str(),
			   ss.str().size()+1);

	//receive rtrAdr, myAdr, rtrIp, comtCtlAdr
	Np4d::recvIntBlock(cmSock,(uint32_t&) rtrAdr);
	if (rtrAdr == -1) {
		cerr << "Avatar::login: negative reply from ClientMgr\n";
		return false;
	}
	Np4d::recvIntBlock(cmSock,(uint32_t&) myAdr);
	Np4d::recvIntBlock(cmSock,(uint32_t&) rtrIpAdr);
	Np4d::recvIntBlock(cmSock,(uint32_t&) comtCtlAdr);
	close(cmSock);
	string s;
	cout << "avatar address=" << Forest::fAdr2string(myAdr,s);
	cout << " router address=" << Forest::fAdr2string(rtrAdr,s);
	cout << " comtree controller address="
	     << Forest::fAdr2string(comtCtlAdr,s) << endl;
	return true;
}

/** Setup the internal representation of the walls.
 *  @param wallsFile contains the specification of the walls in the
 *  virtual world
 *  @return true on success, false on failure
 */
bool Avatar::setupWalls(const char *wallsFile) {
	ifstream ifs(wallsFile);
	if (ifs.fail()) {
		cerr << "setupWalls: cannot open walls file\n";
		return false;
	}
	int y = 0; walls = NULL;
	while(ifs.good()) {
		string line;
		getline(ifs,line);
		if (walls == NULL) {
			worldSize = line.size();
			y = worldSize-1;
			walls = new int[worldSize*worldSize];
		} else if ((int) line.size() != worldSize) {
			cerr << "setupWalls: format error, all lines must have "
			        "same length\n";
			return false;
		}
		for(int x = 0; x < worldSize; x++) {
			if(line[x] == '+') {
				walls[y*worldSize + x] = 3;
			} else if(line[x] == '-') {
				walls[y*worldSize + x] = 2;
			} else if(line[x] == '|') {
				walls[y*worldSize + x] = 1;
			} else if(line[x] == ' ') {
				walls[y*worldSize + x] = 0;
			} else {
				cerr << "setupWalls: unrecognized symbol in "
					"map file!\n";
				return false;
			}
		}
		if (--y < 0) break;
	}

	// now, compute visibility of squares relative to each other
	// and store results in visibility sets
	visSet = new set<int>[worldSize*worldSize+1];
	for (int x1 = 0; x1 < worldSize; x1++) {
	for (int y1 = 0; y1 < worldSize; y1++) {
		int g1 = 1 + x1 + y1*worldSize;
		// start with upper right quadrant
		for (int d = 1; d < worldSize; d++) {
			// check squares at rectilinear distance d from (x1,y1)
			bool done = true;
			for (int x2 = x1; x2 <= min(x1+d,worldSize-1); x2++) {
				int y2 = d + y1 - (x2 - x1);
				if (y2 >= worldSize) continue;
				int g2 = 1 + x2 + y2*worldSize;
if (g1 == 1) cerr << "checking " << g2;
				if (isVis(g1,g2)) {
if (g1 == 1) cerr << " is visible";
					(visSet[g1]).insert(g2);
					done = false;
				}
if (g1 == 1) cerr << endl;
			}
			if (done) break;
		}
		// now process upper left quadrant
		for (int d = 1; d < worldSize; d++) {
			bool done = true;
			for (int x2 = x1; x2 >= max(0,x1-d); x2--) {
				int y2 = d + y1 - (x1 - x2);
				if (y2 >= worldSize) continue;
				int g2 = 1 + x2 + y2*worldSize;
				if (isVis(g1,g2)) {
					(visSet[g1]).insert(g2);
					done = false;
				}
			}
			if (done) break;
		}
		// and lower left quadrant
		for (int d = 1; d < worldSize; d++) {
			bool done = true;
			for (int x2 = x1; x2 >= max(0,x1-d); x2--) {
				int y2 = (x1 - x2) + y1 - d;
				if (y2 < 0) continue;
				int g2 = 1 + x2 + y2*worldSize;
				if (isVis(g1,g2)) {
					(visSet[g1]).insert(g2);
					done = false;
				}
			}
			if (done) break;
		}
		// and finally, lower right
		for (int d = 1; d < worldSize; d++) {
			bool done = true;
			for (int x2 = x1; x2 <= min(x1+d,worldSize-1); x2++) {
				int y2 = (x2 - x1) + y1 - d;
				if (y2 < 0) continue;
				int g2 = 1 + x2 + y2*worldSize;
				if (isVis(g1,g2)) {
					(visSet[g1]).insert(g2);
					done = false;
				}
			}
			if (done) break;
		}
	}}

	// print some visibility statistics for the log
	int maxVis = 0;; int totVis = 0;
	for (int g = 1; g <= worldSize*worldSize; g++) {
if (g == 1) {
cerr << "visSet[" << g << "]=";
set<int>::iterator p;
for (p = visSet[g].begin(); p != visSet[g].end(); p++)
cerr << *p << " ";
cerr << endl;
}
		int vis = visSet[g].size();
		maxVis = max(vis,maxVis); totVis += vis;
	}
			
/*

	visibility = new bool*[worldSize*worldSize];
	for(int i = 0; i < worldSize*worldSize; i++)
		visibility[i] = new bool[worldSize*worldSize];
	for(int i = 0; i < worldSize*worldSize; i++) {
		for(int j = i; j < worldSize*worldSize; j++) {
			if (i == j) {
				visibility[i][j] = true;
			} else {
				visibility[i][j] = isVis(i,j);
				visibility[j][i] = visibility[i][j];
			}
		}
	}

	// print some visibility statistics for the log
	int maxVis; int totVis = 0;
	for (int h = 0; h < worldSize*worldSize; h++) {
		int vis = 0;
		for (int k = 0; k < worldSize*worldSize; k++) {
			if (k != h && visibility[h][k]) vis++;
		}
		maxVis = max(vis,maxVis); totVis += vis;
	}
*/
	cout << "avg visible: " << totVis/(worldSize*worldSize)
	     << " max visible: " << maxVis << endl;
	return true;
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
void Avatar::run(uint32_t finishTime) {
	connect(); 			// send initial connect packet
	uint32_t now;			// free-running us clock
	uint32_t nextTime;		// time of next operational cycle
	uint32_t lastComtSwitch;	// last time a comt switched

	now = nextTime = lastComtSwitch = Misc::getTime();
	comt = randint(firstComt, lastComt);
	uint32_t comtSwitchTime = (uint32_t) randint(10,15);
	send2comtCtl(CLIENT_JOIN_COMTREE); // join initial comtree
	bool waiting4comtCtl = true;	// true when waiting for reply to a join

	comt_t newcomt = 0;
	while (now <= finishTime) {
		//reset hashtables and report
		numNear = nearAvatars->size(); nearAvatars->clear();
		numVisible = visibleAvatars->size(); visibleAvatars->clear();

		now = Misc::getTime();
		if (!waiting4comtCtl) updateSubs();
		int p;
		while ((p = receive()) != 0) {
			PacketHeader& h = ps->getHeader(p);
			int ptyp = h.getPtype();
			
			if (!waiting4comtCtl && ptyp == CLIENT_DATA) {
				updateNearby(p);
			} else if (waiting4comtCtl && ptyp == CLIENT_SIG) {
				// currently not checking for missing reply
				// should really fix this
				CtlPkt cp;
				cp.unpack(ps->getPayload(p),
					  h.getLength() - Forest::OVERHEAD);
				if (cp.getCpType() == CLIENT_JOIN_COMTREE &&
				    cp.getRrType() == POS_REPLY) {
					waiting4comtCtl = false;
				} else if (cp.getCpType() ==
					    CLIENT_LEAVE_COMTREE &&
                                    	   cp.getRrType() == POS_REPLY) {
					comt = newcomt;
					send2comtCtl(CLIENT_JOIN_COMTREE);
				}
			}
			if (connSock >= 0) {
				// send status of sending avatar to driver
				// for this avatar
				uint64_t key = h.getSrcAdr();
				key = (key << 32) | h.getSrcAdr();
				bool isVis = visibleAvatars->member(key);
				forwardReport(now, (isVis ? 2 : 3), p);
			}
			ps->free(p);
		}
		if (!waiting4comtCtl) {
			check4command();	// check for command from driver
			updateStatus(now);	// update Avatar status
			// send status of this avatar
			if (connSock >= 0) forwardReport(now, 1);
			sendStatus(now);
		}

		// now, switch comtrees if not connected to driver
		if (connSock  < 0 &&
		    (now-lastComtSwitch) > (1000000*comtSwitchTime) &&
		    !waiting4comtCtl) {
			lastComtSwitch = now;
			newcomt = randint(firstComt, lastComt);
			if (comt != newcomt) {
				unsubscribeAll();
				send2comtCtl(CLIENT_LEAVE_COMTREE);
				waiting4comtCtl = true;
			}
			comtSwitchTime = randint(10,15);
		}

		// update time and wait for next cycle
		nextTime += 1000*UPDATE_PERIOD;
		now = Misc::getTime();
		useconds_t delay = nextTime - now;
		if (delay < ((uint32_t) 1) << 31) usleep(delay);
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
	pp[6] = htonl(numVisible);
	pp[7] = htonl(numNear);
	send(p);
}

/** Send a status report to the remote controller for this avatar.
 *  @param now is the current time
 *  @param avType is the type code for the avatar whose status is
 *  being sent: 1 means the Avatar for "this" object, 2 a visible avatar
 *  and 3 an avatar that we can "hear" but not see
 *  @param p is an optional packet number; if present and non-zero,
 *  its payload is unpacked to produce the report to send to the avatar
 */
void Avatar::forwardReport(uint32_t now, int avType, int p) {
	uint32_t buf[10];
	buf[0] = htonl((uint32_t) now);
	buf[8] = htonl((uint32_t) comt);
	buf[9] = htonl(avType);
	if (avType == 1) { // send report for this avatar
		buf[1] = htonl((uint32_t) myAdr);
		buf[2] = htonl((uint32_t) x);
		buf[3] = htonl((uint32_t) y);
		buf[4] = htonl((uint32_t) direction);
		buf[5] = htonl((uint32_t) speed);
		buf[6] = htonl((uint32_t) numVisible);
		buf[7] = htonl((uint32_t) numNear);
	} else if (p != 0) {
		PacketHeader& h = ps->getHeader(p);
		uint32_t *pp = ps->getPayload(p);
		if (h.getComtree() != comt) return;

		buf[1] = htonl(h.getSrcAdr());
		buf[2] = pp[2];
		buf[3] = pp[3];
		buf[4] = pp[4];
		buf[5] = pp[5];
		buf[6] = pp[6];
		buf[7] = pp[7];
	} else {
		return;
	}
	int nbytes = NUM_ITEMS*sizeof(uint32_t);
	char *bp= (char *) buf;
	while (nbytes > 0) {
		int n = write(connSock, (void *) bp, nbytes);
		if (n < 0) fatal("Avatar::forwardReport: failure in write");
		bp += n; nbytes -= n;
	}
}

/** Send join or leave packet packet to the ComtreeController.
 *  @param cpx is either CLIENT_JOIN_COMTREE or CLIENT_LEAVE_COMTREE,
 *  depending on whether we want to join or leave the comtree
 */
void Avatar::send2comtCtl(CpTypeIndex cpx) {
        packet p = ps->alloc();
        if (p == 0)
		fatal("Avatar::send2comtCtl: no packets left to allocate");
        CtlPkt cp(cpx,REQUEST,seqNum++);;
        cp.setAttr(COMTREE_NUM,comt);
        cp.setAttr(CLIENT_IP,myIpAdr);
        cp.setAttr(CLIENT_PORT,Np4d::getSockPort(sock));
        int len = cp.pack(ps->getPayload(p));
	if (len == 0) 
		fatal("Avatar::send2comtCtl: control packet packing error");
        PacketHeader& h = ps->getHeader(p);
	h.setLength(Forest::OVERHEAD + len);
        h.setPtype(CLIENT_SIG); h.setFlags(0);
        h.setComtree(Forest::CLIENT_SIG_COMT);
	h.setSrcAdr(myAdr); h.setDstAdr(comtCtlAdr);
        h.pack(ps->getBuffer(p));
	send(p);
}

/** Check for a new command from remote display program.
 *  Check to see if the remote display has sent a comand and
 *  respond appropriately. Commands take the form of pairs
 *  (c,i) where c is a character that identifies a specific
 *  command, and i is an integer that provides an optional
 *  parameter value for the command. The current commands
 *  are defined below.
 *
 *  j steer the Avatar to the left
 *  l steer the Avatar to the right
 *  i speed up the Avatar
 *  k slow down the Avatar
 *
 *  Note that the integer parameter is sent even when
 *  the command doesn't use it
 */
void Avatar::check4command() { 
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
	char buf[5];
        int nbytes = read(connSock, buf, 5);
        if (nbytes < 0) {
                if (errno == EAGAIN) return;
                fatal("Avatar::check4command: error in read call");
	} else if (nbytes == 0) { // remote display closed connection
		close(connSock); connSock = -1;
		unsubscribeAll();
		return;
        } else if (nbytes < 5) {
		fatal("Avatar::check4command: incomplete command");
	}
	char cmd = buf[0];
	uint32_t param = ntohl(*((int*) &buf[1]));
cerr << "got command " << cmd << " " << param << endl;
	switch (cmd) {
	case 'j': direction -= 10;
		  if (direction < 0) direction += 360;
		  break;
	case 'l': direction += 10;
		  if (direction > 360) direction -= 360;
		  break;
	case 'i': if (speed == SLOW) speed = MEDIUM;
	          else if (speed == MEDIUM) speed = FAST;
		  break;
	case 'k': if (speed == FAST) speed = MEDIUM;
	          else if (speed == MEDIUM) speed = SLOW;
		  break;
	}
}

/** Send initial connect packet, using comtree 1 (the signalling comtree).
 */
void Avatar::connect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
	h.setComtree(Forest::CLIENT_CON_COMT);
	h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send(p);
}


/** Send final disconnect packet.
 */
void Avatar::disconnect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
	h.setComtree(Forest::CLIENT_CON_COMT);
	h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	send(p);
}

/** Send packet to forest router and recycle storage.
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
	if ((h.getPtype() == CLIENT_SIG &&
	     h.getComtree() != Forest::CLIENT_CON_COMT) &&
	    h.getComtree() != comt) {
		ps->free(p);
		return 0;
	}
        h.setIoBytes(nbytes);
        h.setTunSrcIp(remoteIp);
        h.setTunSrcPort(remotePort);

        return p;
}

/** Update status of avatar based on passage of time.
 *  @param now is the reference time for the simulated update
 *  @param controlInput is the input code from the controller,
 *  or -1 if no controller is connected, or 0 if no current input
 */
void Avatar::updateStatus(uint32_t now) {
	const double PI = 3.141519625;
	double r;

	// update position
	double dist = speed;
	double dirRad = direction * (2*PI/360);
	int prevRegion = groupNum(x,y)-1;
	x += (int) (dist * sin(dirRad));
	y += (int) (dist * cos(dirRad));
	x = max(x,0); x = min(x,GRID*worldSize-1);
	y = max(y,0); y = min(y,GRID*worldSize-1);
	int postRegion = groupNum(x,y)-1;

	//bounce off walls
        if (x == 0)        	direction = -direction;
        else if (x == GRID*worldSize-1)   direction = -direction;
        else if (y == 0)        direction = 180 - direction;
        else if (y == GRID*worldSize-1)   direction = 180 - direction;
        else if (postRegion != prevRegion) {
                if (prevRegion == postRegion + 1 &&
			(walls[prevRegion]==1 || walls[prevRegion]==3)) {
			// going east to west and hitting wall
                        direction = -direction;
                        x = (prevRegion%worldSize)*GRID + 1;
                } else if (prevRegion == postRegion - 1 &&
			(walls[postRegion]==1 || walls[postRegion]==3)) {
			// going west to east and hitting wall
                        direction = -direction;
                        x = (postRegion%worldSize)*GRID - 1;
                } else if (prevRegion == postRegion+worldSize &&
			(walls[postRegion] == 2 || walls[postRegion] == 3)) {
			// going north to south and hitting wall
                        direction = 180-direction;
                        y = (prevRegion/worldSize)*GRID + 1;
                } else if (prevRegion == postRegion - worldSize &&
			(walls[prevRegion] == 2 || walls[prevRegion] == 3)) {
			// going south to north and hitting wall
                        direction = 180-direction;
                        y = (postRegion/worldSize)*GRID - 1;
                } else if (prevRegion == postRegion - (worldSize-1)) {
			// going southeast to northwest
			if (walls[prevRegion] == 3) {
				direction = direction - 180;
				x = (prevRegion%worldSize)*GRID + 1;
				y = (postRegion/worldSize)*GRID - 1;
			} else if (walls[prevRegion] == 1) {
				direction = -direction;
                        	x = (prevRegion%worldSize)*GRID + 1;
			} else if (walls[prevRegion] == 2 ||
				   walls[prevRegion-1]&2) {
				direction = 180 - direction;
                        	y = (postRegion/worldSize)*GRID - 1;
			} 
                } else if (prevRegion == postRegion - (worldSize+1)) {
			// going southwest to northeast
			if (walls[prevRegion]&2 && walls[prevRegion+1]&1) {
				direction = direction - 180;
				x = (postRegion%worldSize)*GRID - 1;
				y = (postRegion/worldSize)*GRID - 1;
			} else if (walls[prevRegion]&2) {
				direction = 180 - direction;
                        	y = (postRegion/worldSize)*GRID - 1;
			} else if (walls[prevRegion+1]&1 ||
				   walls[postRegion]&1) {
				direction = -direction;
                        	x = (postRegion%worldSize)*GRID - 1;
			} 
                } else if (prevRegion == postRegion - (worldSize-1)) {
			// going northeast to southwest
			if (walls[prevRegion]&1 &&
			    walls[postRegion+1]&2) {
				direction = direction - 180;
				if (direction < 0) direction += 360;
				x = (prevRegion%worldSize)*GRID + 1;
				y = (prevRegion/worldSize)*GRID + 1;
			} else if (walls[prevRegion]&1) {
				direction = -direction;
                        	x = (prevRegion%worldSize)*GRID + 1;
			} else if (walls[postRegion+1]&2 ||
				   walls[postRegion]&2) {
				direction = 180 - direction;
                        	y = (prevRegion/worldSize)*GRID + 1;
			} 
                } else if (prevRegion == postRegion - (worldSize+1)) {
			// going northwest to southeast
			if (walls[postRegion-1]&2 &&
			    walls[prevRegion+1]&1) {
				direction = direction - 180;
				x = (postRegion%worldSize)*GRID - 1;
				y = (prevRegion/worldSize)*GRID + 1;
			} else if (walls[postRegion-1]&2) {
				direction = 180 - direction;
                        	y = (prevRegion/worldSize)*GRID + 1;
			} else if (walls[prevRegion+1]&1 ||
				   walls[postRegion]&1) {
				direction = -direction;
                        	x = (postRegion%worldSize)*GRID - 1;
			} 
		}
        } else if (connSock < 0) {
		// no controller connected, so just make random
		// changes to direction and speed
		direction += deltaDir;
		if (direction < 0) direction += 360;
		if ((r = randfrac()) < 0.1) {
			if (r < .05) deltaDir -= 0.2 * randfrac();
			else         deltaDir += 0.2 * randfrac();
			deltaDir = min(1.0,deltaDir);
			deltaDir = max(-1.0,deltaDir);
		}
		// update speed
		if ((r = randfrac()) <= 0.1) {
			if (speed == SLOW || speed == FAST) speed = MEDIUM;
			else if (r < 0.05) 		    speed = SLOW;
			else 				    speed = FAST;
		}
	}
	if (direction < 0) direction += 360;
}

/** Return the multicast group number associated with given position.
 *  Assumes that SIZE is an integer multiple of GRID
 *  @param x1 is the x coordinate of the position of interest
 *  @param y1 is the y coordinate of the position of interest
 */
int Avatar::groupNum(int x1, int y1) {
	return 1 + (x1/GRID) + (y1/GRID)*worldSize;
}

/** Determine if two squares are visible from each other.
 *  @param g1 is the group number of the first square
 *  @param g2 is the group number of the second square
 */
bool Avatar::isVis(int g1, int g2) {
	int region1 = g1-1; int region2 = g2-1;
	int region1xs[4]; int region1ys[4];
	int region2xs[4]; int region2ys[4];

	int row1 = region1/worldSize; int col1 = region1%worldSize;
	int row2 = region2/worldSize; int col2 = region2%worldSize;

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

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			bool canSee = true;
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
				int k = i*worldSize+j; // k=region index
				if (walls[k] == 1 || walls[k] == 3) {
					// region k has a left side wall
					// check if ab intersects it
					double dx = cx; double dy = cy-GRID;
					if (linesIntersect(ax,ay,bx,by,
							   cx,cy,dx,dy)) {
						canSee = false; break;
					} else {
					}
				}
				if (walls[k] == 2 || walls[k] == 3) {
					// region k has a top wall
					// check if ab intersects it
					double dx = cx+GRID; double dy = cy;
					if (linesIntersect(ax,ay,bx,by,
							   cx,cy,dx,dy)) {
						canSee = false; break;
					} else {
					}
				}
			}
			if (!canSee) break;
			}
			if (canSee) return true;
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
}

void Avatar::subscribe(list<int>& glist) {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	int nsub = 0;
	list<int>::iterator gp;
	for (gp = glist.begin(); gp != glist.end(); gp++) {
		int g = *gp; nsub++;
		if (nsub > 350) {
			pp[0] = htonl(nsub-1); pp[nsub] = 0;
			h.setLength(Forest::OVERHEAD + 4*(2+nsub));
			h.setPtype(SUB_UNSUB); h.setFlags(0);
			h.setComtree(comt);
			h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
			send(p);
			nsub = 1;
		}
		pp[nsub] = htonl(-g);
	}
	pp[0] = htonl(nsub); pp[nsub+1] = 0;

	h.setLength(Forest::OVERHEAD + 4*(2+nsub));
	h.setPtype(SUB_UNSUB); h.setFlags(0);
	h.setComtree(comt); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
	send(p);
	ps->free(p);
}

/** Unsubscribe from a list of multicast groups.
 *  @param glist is a reference to a list of multicast group numbers.
 */
void Avatar::unsubscribe(list<int>& glist) {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);
	uint32_t *pp = ps->getPayload(p);

	int nunsub = 0;
	list<int>::iterator gp;
	for (gp = glist.begin(); gp != glist.end(); gp++) {
		int g = *gp; nunsub++;
		if (nunsub > 350) {
			pp[0] = 0; pp[1] = htonl(nunsub-1);
			h.setLength(Forest::OVERHEAD + 4*(2+nunsub));
			h.setPtype(SUB_UNSUB); h.setFlags(0);
			h.setComtree(comt);
			h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
			send(p);
			nunsub = 1;
		}
		pp[nunsub+1] = htonl(-g);
	}
	pp[0] = 0; pp[1] = htonl(nunsub);

	h.setLength(Forest::OVERHEAD + 4*(2+nunsub));
	h.setPtype(SUB_UNSUB); h.setFlags(0);
	h.setComtree(comt); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
	send(p);
	ps->free(p);
}

/** Subscribe to all multicasts for regions that are currently visible
 *  and that we're not already subscribed to.
 */ 
void Avatar::subscribeAll() {
	list<int> glist;
	set<int>::iterator gp;
	int g = groupNum(x,y);
	for (gp = visSet[g].begin(); gp != visSet[g].end(); gp++) {
		if (mySubs->find(*gp) == mySubs->end()) {
			mySubs->insert(*gp); glist.push_back(*gp);
		}
	}
	subscribe(glist);
}

/** Unsubscribe from all multicasts that we're currently subscribed to.
 */
void Avatar::unsubscribeAll() {
	list<int> glist;
	set<int>::iterator gp;
	for (gp = mySubs->begin(); gp != mySubs->end(); gp++)
		glist.push_back(*gp);
	unsubscribe(glist);
	mySubs->clear();
}

/** Update subscriptions without changing comtrees.
 *  Unsubscribe from all multicasts for squares that are not visible from
 *  our current position, then subscribe to all missing multicasts that
 *  are visible.
 */
void Avatar::updateSubs() {	
	// identify subscriptions for squares that are not currently visible
	int myGroup = groupNum(x,y);
	list<int> glist;
	set<int>::iterator gp;
	for (gp = mySubs->begin(); gp != mySubs->end(); gp++) {
		int g = *gp;
		if ((visSet[myGroup]).find(g) == (visSet[myGroup]).end())
			glist.push_back(g); 
	}
	// remove them from subscription set and unsubscribe
	list<int>::iterator p;
	for (p = glist.begin(); p != glist.end(); p++)
		mySubs->erase(*p);
	unsubscribe(glist);

	// identify squares that are now visible
	// add their multicast groups to subscription set and subscribe
	glist.clear();
	set<int>::iterator vp;
	for (vp = visSet[myGroup].begin(); vp != visSet[myGroup].end(); vp++) {
		int g = *vp;
		if (mySubs->find(g) == mySubs->end()) {
			mySubs->insert(g);
			glist.push_back(g);
		}
	}
	subscribe(glist);
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
	if (ntohl(pp[0]) != (int) STATUS_REPORT) return;

	// add the Avatar that sent this status report to the nearAvatars set
	uint64_t avId = h.getSrcAdr(); avId = (avId << 32) | h.getSrcAdr();
	if (nearAvatars->size() < MAXNEAR)
		nearAvatars->insert(avId);

	// determine if we can see the Avatar that sent this report
	int x1 = ntohl(pp[2]); int y1 = ntohl(pp[3]);
	int g1 = groupNum(x1,y1);
	int myGroup = groupNum(x,y);
	if (visSet[myGroup].find(g1) == visSet[myGroup].end()) {
		visibleAvatars->remove(avId);
		return;
	}

	// still possible that (x,y) can see (x1,y1)
	// check for walls that intersect the connecting line
	// it sufficies to check walls that belong to the squares
	// that are visible from (x,y)
	bool canSee = true;
	set<int>::iterator vp;
	for (vp = visSet[myGroup].begin(); vp != visSet[myGroup].end(); vp++) {
		int i = (*vp)-1; // i is region index for a visible square
		if (walls[i] == 1 || walls[i] == 3) {
			int l2x1 = (i % worldSize) * GRID; 
			int l2x2 = l2x1;
			int l2y1 = (i / worldSize) * GRID;
			int l2y2 = l2y1 + GRID;
			if(linesIntersect(x1,y1,x,y,
					  l2x1,l2y1,l2x2,l2y2)) {
				canSee = false; break;
			}
		}
		if (walls[i] == 2 || walls[i] == 3) {
			int l2x1 = (i % worldSize) * GRID; 
			int l2x2 = l2x1 + GRID;
			int l2y1 = (i / worldSize) * GRID + GRID;
			int l2y2 = l2y1;
			if(linesIntersect(x1,y1,x,y,
					  l2x1,l2y1,l2x2,l2y2)) {
				canSee = false; break;
			}
		}
	}
	if (canSee && visibleAvatars->size() < MAXNEAR)
		visibleAvatars->insert(avId);
}
