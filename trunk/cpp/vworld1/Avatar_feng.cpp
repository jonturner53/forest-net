/** @file Avatar.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "stdinc.h"
#include "Forest.h"
#include "Avatar.h"
#include <string>
#include <algorithm>

using namespace forest;

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
	if (argc != 9 ||
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

namespace forest {

/** Constructor allocates space and initializes private data.
 * 
 *  @param mipa is this host's IP address
 *  @param fc is the first comtree in the range used by the avatar
 *  @param lc is the last comtree in the range used by the avatar
 */
Avatar::Avatar(ipa_t mipa, comt_t fc, comt_t lc)
		: myIp(mipa), firstComt(fc), lastComt(lc) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);
	mySubs = new set<int>();
	nearAvatars = new HashSet(MAXNEAR);
	visibleAvatars = new HashSet(MAXNEAR);
	myVisSet = new set<int>;

	numNear = numVisible = 0;
	seqNum = 0;
	subSeqNum = 0;
	switchState = IDLE;
}

Avatar::~Avatar() {
	delete mySubs; delete nearAvatars; delete ps; delete [] walls;
	delete myVisSet;
	if (sock >= 0) close(sock);
	if (listenSock >= 0) close(listenSock);
}

/** Perform all required initialization.
 *  @return true on success, false on failure
 */
bool Avatar::init(ipa_t cmIpAdr, string& uname, string& pword,
		  char *wallsFile) {
	Misc::getTime(); // starts clock running

	// open and configure forest socket
	sock = Np4d::datagramSocket();
	if (sock<0 || !Np4d::bind4d(sock,myIp,0) || !Np4d::nonblock(sock)) {
		cerr << "Avatar::init: could not open/configure forest "
			"socket\n";
		return false;
	}

	// open and configure listen socket
	listenSock = Np4d::streamSocket();
	if (listenSock < 0 || !Np4d::bind4d(listenSock,myIp,0) ||
	    !Np4d::listen4d(listenSock) || !Np4d::nonblock(listenSock)) {
		cerr << "Avatar::init: could not open/configure external "
			"socket\n";
		return false;
	}
	connSock = -1;	// connection socket for listenSock
			// -1 means not connected
	string s;
	cout << "listen socket: " << Np4d::ip2string(myIp,s)
	     << "/" << Np4d::getSockPort(listenSock) << endl;
	cout.flush();

	// login and setup walls 
	if (!login(cmIpAdr, uname, pword)) return false;
	if (!setupWalls(wallsFile)) return false;
	// initialize avatar to a random position
	srand(myAdr);
	while (true) {
		x = randint(0,GRID*worldSize-1);
		y = randint(0,GRID*worldSize-1);
		if (!(walls[groupNum(x,y)-1]&4)) break;
	}
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
cerr << "logging in\n";
	int loginSock = Np4d::streamSocket();
        if (loginSock < 0 ||
	    !Np4d::bind4d(loginSock, myIp, 0) ||
	    !Np4d::connect4d(loginSock,cmIpAdr,Forest::CM_PORT)) {
		cerr << "Avatar::login: cannot open/configure socket to "
			"ClientMgr\n";
		return false;
	}
	// start login dialog
	string s = "Forest-login-v1\nlogin: " + uname +
		   "\npassword: " + pword + "\nover\n";
cerr << "login string " << s << endl;
	Np4d::sendString(loginSock,s);
	NetBuffer buf(loginSock,1024);
	string s0, s1, s2;
	if (!buf.readLine(s0) || s0 != "success" ||
	    !buf.readLine(s1) || s1 != "over") {
		return false;
	}
	
	// proceed to new session dialog
	s = "newSession\nover\n";
	Np4d::sendString(loginSock,s);

cerr << "a\n";
	// process reply - expecting my forest address
	if (!buf.readAlphas(s0) || s0 != "yourAddress" || !buf.verify(':') ||
	    !buf.readForestAddress(s1) || !buf.nextLine()) 
		return false;
	myAdr = Forest::forestAdr(s1.c_str());

cerr << "b\n";
	// continuing - expecting info for my forest access router
	int port;
	if (!buf.readAlphas(s0) || s0 != "yourRouter" || !buf.verify(':') ||
	    !buf.verify('(') || !buf.readIpAddress(s1) || !buf.verify(',') ||
	    !buf.readInt(port) || !buf.verify(',') || 
	    !buf.readForestAddress(s2) || !buf.verify(')') || !buf.nextLine())
		return false;
	rtrIp = Np4d::getIpAdr(s1.c_str());
	rtrPort = (ipp_t) port;
	rtrAdr = Forest::forestAdr(s2.c_str());

cerr << "c\n";
	// continuing - expecting address of comtree controller
	if (!buf.readAlphas(s0) || s0 != "comtCtlAddress" || !buf.verify(':') ||
	    !buf.readForestAddress(s1) || !buf.nextLine()) 
		return false;
	ccAdr = Forest::forestAdr(s1.c_str());
	
cerr << "d\n";
	// continuing - expecting connection nonce
	if (!buf.readAlphas(s0) || s0 != "connectNonce" || !buf.verify(':') ||
	    !buf.readInt(nonce) || !buf.nextLine())
		return false;
	if (!buf.readLine(s0) || (s0 != "over" && s0 != "overAndOut"))
		return false;
	
cerr << "e\n";
	close(loginSock);

	cout << "avatar address=" << Forest::fAdr2string(myAdr,s) << endl;
	cout << "router info= (" << Np4d::ip2string(rtrIp,s) << ",";
	cout << rtrPort << "," << Forest::fAdr2string(rtrAdr,s) << ")\n";
	cout << "comtCtl address=" << Forest::fAdr2string(ccAdr,s) << "\n";
	cout << "nonce=" << nonce << endl;
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
	bool horizRow = true;
	while(ifs.good()) {
		string line;
		getline(ifs,line);
		if (walls == NULL) {
			worldSize = line.size()/2;
			y = worldSize-1;
			try {
				walls = new char[worldSize*worldSize];
			} catch(exception& e) {
				cerr << "setupWalls: could not allocate space "
					"for walls array (worldSize="
				     << worldSize << ")\n" << endl;
				perror("");
				exit(1);
			}
		} else if ((int) line.size()/2 != worldSize) {
			cerr << "setupWalls: format error, all lines must have "
			        "same length\n";
			return false;
		}
		for(int xx = 0; xx < 2*worldSize; xx++) {
			int pos = y * worldSize + xx/2;
			if (horizRow) {
				if (!(xx&1)) continue;
				if (line[xx] == '-') walls[pos] |= 2;
				continue;
			}
			if (xx&1) {
				if (line[xx] == 'x') walls[pos] |= 4;
			} else if (line[xx] == '|') walls[pos] |= 1;
		}
		horizRow = !horizRow;
		if (horizRow) y--;
		if (y < 0) break;
	}
	return true;
}

/** Compute a visbility set for a given square in the virtual world.
 *  @param g1 is the multicast group number for a square in the virtual world
 *  @param vSet is a reference to a visibility set in which the
 *  result is to be returned
 */
void Avatar::computeVisSet(int g1, set<int>& vSet) {
	int x1 = (g1-1)%worldSize; int y1 = (g1-1)/worldSize;
	vSet.clear();
	vSet.insert(g1);

	bool *visVec = new bool[worldSize];
	bool *prevVisVec = new bool[worldSize];

	// start with upper right quadrant
	prevVisVec[0] = true;
	int dlimit = min(worldSize,MAX_VIS);
	for (int d = 1; d <= dlimit; d++) {
		bool done = true;
		for (int x2 = x1; x2 <= min(x1+d,worldSize-1); x2++) {
			int y2 = d + y1 - (x2 - x1);
			if (y2 >= worldSize) continue;
			int dx = x2-x1;
			visVec[dx] = false;
			if ((x1==x2 && !prevVisVec[dx]) ||
			    (x1!=x2 && y1==y2 && !prevVisVec[dx-1]) ||
			    (x1!=x2 && y1!=y2 && !prevVisVec[dx-1] &&
						 !prevVisVec[dx])) {
				continue;
			}
			int g2 = 1 + x2 + y2*worldSize;

			if (isVis(g1,g2)) {
				visVec[dx] = true;
				vSet.insert(g2);
				done = false;
			}
		}
		if (done) break;
		for (int x2 = x1; x2 <= min(x1+d,worldSize-1); x2++) {
			prevVisVec[x2-x1] = visVec[x2-x1];
		}
	}
	// now process upper left quadrant
	prevVisVec[0] = true;
	for (int d = 1; d <= dlimit; d++) {
		bool done = true;
		for (int x2 = x1; x2 >= max(x1-d,0); x2--) {
			int dx = x1-x2;
			int y2 = d + y1 - dx;
			if (y2 >= worldSize) continue;
			visVec[dx] = false;
			if ((x1==x2 && !prevVisVec[dx]) ||
			    (x1!=x2 && y1==y2 && !prevVisVec[dx-1]) ||
			    (x1!=x2 && y1!=y2 && !prevVisVec[dx-1] &&
						 !prevVisVec[dx])) {
				continue;
			}
			int g2 = 1 + x2 + y2*worldSize;
			if (isVis(g1,g2)) {
				visVec[dx] = true;
				vSet.insert(g2);
				done = false;
			}
		}
		if (done) break;
		for (int x2 = x1; x2 >= max(x1-d,0); x2--) {
			int dx = x1-x2;
			prevVisVec[dx] = visVec[dx];
		}
	}
	// now process lower left quadrant
	prevVisVec[0] = true;
	for (int d = 1; d <= dlimit; d++) {
		bool done = true;
		for (int x2 = x1; x2 >= max(x1-d,0); x2--) {
			int dx = x1-x2;
			int y2 = (y1 - d) + dx;
			if (y2 < 0) continue;
			visVec[dx] = false;
			if ((x1==x2 && !prevVisVec[dx]) ||
			    (x1!=x2 && y1==y2 && !prevVisVec[dx-1]) ||
			    (x1!=x2 && y1!=y2 && !prevVisVec[dx-1] &&
						 !prevVisVec[dx])) {
				continue;
			}
			int g2 = 1 + x2 + y2*worldSize;
			if (isVis(g1,g2)) {
				visVec[dx] = true;
				vSet.insert(g2);
				done = false;
			}
		}
		if (done) break;
		for (int x2 = x1; x2 >= max(x1-d,0); x2--) {
			int dx = x1-x2;
			prevVisVec[dx] = visVec[dx];
		}
	}
	// finally, process lower right quadrant
	prevVisVec[0] = true;
	for (int d = 1; d <= dlimit; d++) {
		bool done = true;
		for (int x2 = x1; x2 <= min(x1+d,worldSize-1); x2++) {
			int dx = x2-x1;
			int y2 = (y1 - d) + dx;
			if (y2 < 0) continue;
			visVec[dx] = false;
			if ((x1==x2 && !prevVisVec[dx]) ||
			    (x1!=x2 && y1==y2 && !prevVisVec[dx-1]) ||
			    (x1!=x2 && y1!=y2 && !prevVisVec[dx-1] &&
						 !prevVisVec[dx])) {
				continue;
			}
			int g2 = 1 + x2 + y2*worldSize;
			if (isVis(g1,g2)) {
				visVec[dx] = true;
				vSet.insert(g2);
				done = false;
			}
		}
		if (done) break;
		for (int x2 = x1; x2 <= min(x1+d,worldSize-1); x2++) {
			int dx = x2-x1;
			prevVisVec[dx] = visVec[dx];
		}
	}
	delete [] visVec; delete [] prevVisVec;
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

	now = nextTime = Misc::getTime();
	uint32_t comtSwitchTime = now+1;
	comt = 0;

	bool waiting4switch = false;
	while (finishTime == 0 || now <= finishTime) {
		//reset hashtables and report
		numNear = nearAvatars->size(); nearAvatars->clear();
		numVisible = visibleAvatars->size(); visibleAvatars->clear();

		// check for a command from driver and process it
		// if not yet connected, check4command attempts an accept
		comt_t newComt = check4command();
		if (newComt != 0 && newComt != comt) {
			startComtSwitch(newComt,now);
			waiting4switch = true;
		}

		now = Misc::getTime();
		pktx px;
		while ((px = receive()) != 0) {
			Packet& p = ps->getPacket(px);
			Forest::ptyp_t ptyp = p.type;
			if (waiting4switch) {
				// pass client signalling packets to
				// completeComtSwitch and discard all others
				if (ptyp == Forest::CLIENT_SIG) {
					waiting4switch =
						!completeComtSwitch(px,now);
				}
				ps->free(px); continue;
			}
			if (ptyp != Forest::CLIENT_DATA) { // ignore other packets
				ps->free(px); continue;
			}
			// process status reports from other avatars
			// and forward to driver for this avatar
			updateNearby(px);
			if (connSock >= 0) {
				uint64_t key = p.srcAdr;
				key = (key << 32) | p.srcAdr;
				bool isVis = visibleAvatars-> member(key);
				forwardReport(now, (isVis ? 2 : 3), px);
			}
			ps->free(px);
		}
		// check for timeout
		waiting4switch = !completeComtSwitch(0,now);

		if (!waiting4switch) {
			updateStatus(now);	// update Avatar status
			sendStatus(now);	// send status on comtree
			updateSubs();		// update subscriptions
			if (connSock >= 0) { 
				// send "my" status to driver
				forwardReport(now, 1);

			} else if (comt == 0 ||
				   now-comtSwitchTime < ((unsigned) 1)<<31) {
				// switch comtrees if not connected to driver
				lastComtSwitch = now;
				comt_t newComt = randint(firstComt, lastComt);
				if (comt != newComt) {
					startComtSwitch(newComt,now);
					waiting4switch = true;
				}
				comtSwitchTime = now + randint(10,30)*1000000;
			}
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

/** Start the process of switching to a new comtree.
 *  Unsubscribe to the current comtree and leave send signalling
 *  message to leave the current comtree. 
 *  @param newComt is the number of the comtree we're switching to
 *  @param now is the current time
 */
void Avatar::startComtSwitch(comt_t newComt, uint32_t now) {
	nextComt = newComt;
	if (comt != 0) {
		unsubscribeAll();
		send2comtCtl(CtlPkt::CLIENT_LEAVE_COMTREE);
		switchState = LEAVING;
		switchTimer = now; switchCnt = 1;
	} else {
		comt = nextComt;
		send2comtCtl(CtlPkt::CLIENT_JOIN_COMTREE);
		switchState = JOINING;
		switchTimer = now; switchCnt = 1;
	}
}

/** Attempt to complete the process of switching to a new comtree.
 *  This involves completing signalling interactions with ComtCtl,
 *  including re-sending signalling packets when timeouts occur.
 *  @param p is a packet number, or 0 if the caller just wants to
 *  check for a timeout
 *  @param now is the current time
 *  @return false so long as the switch is in progress; return true
 *  if the switch completes successfully, or if it fails (either 
 *  because the ComtCtl sent a negative response or because it
 *  never responded after repeated attempts); in the latter case,
 *  set comt to 0
 */
bool Avatar::completeComtSwitch(pktx px, uint32_t now) {
	if (switchState == IDLE) return true;
	if (px == 0 && now - switchTimer < SWITCH_TIMEOUT)
		return false; // nothing to do in this case
	switch (switchState) {
	case LEAVING: {
		if (px== 0) {
			if (switchCnt > 3) { // give up
				cerr << "completeSwitch: failed while "
					"attempting to leave " << comt
				     << " (timeout)" << endl;
				comt = 0; switchState = IDLE; return true;
			}
			// try again
			send2comtCtl(CtlPkt::CLIENT_LEAVE_COMTREE,RETRY);
			switchTimer = now; switchCnt++;
			return false;
		}
		Packet& p = ps->getPacket(px); 
		CtlPkt cp(p.payload(),p.length - Forest::OVERHEAD);
		cp.unpack();
		if (cp.type == CtlPkt::CLIENT_LEAVE_COMTREE) {
			if (cp.mode == CtlPkt::POS_REPLY) {
				comt = nextComt;
				send2comtCtl(CtlPkt::CLIENT_JOIN_COMTREE);
				switchState = JOINING;
				switchTimer = now; switchCnt = 1;
				return false;
			} else if (cp.mode == CtlPkt::NEG_REPLY) { // give up
				cerr << "completeSwitch: failed while "
					"attempting to leave " << comt
				     << " (request rejected)" << endl;
				comt = 0; switchState = IDLE; return true;
			} 
		}
		return false; // ignore anything else
	}
	case JOINING: {
		if (px == 0) {
			if (switchCnt > 3) { // give up
				cerr << "completeSwitch: failed while "
					"attempting to join " << comt
				     << " (timeout)" << endl;
				comt = 0; switchState = IDLE; return true;
			}
			// try again
			send2comtCtl(CtlPkt::CLIENT_JOIN_COMTREE,RETRY);
			switchTimer = now; switchCnt++;
			return false;
		}
		Packet& p = ps->getPacket(px); 
		CtlPkt cp(p.payload(),p.length - Forest::OVERHEAD);
		cp.unpack();
		if (cp.type == CtlPkt::CLIENT_JOIN_COMTREE) {
			if (cp.mode == CtlPkt::POS_REPLY) {
				subscribeAll();
				switchState = IDLE; return true;
			} else if (cp.mode == CtlPkt::NEG_REPLY) { // give up
				cerr << "completeSwitch: failed while "
					"attempting to join " << comt
				     << " (request rejected)" << endl;
				comt = 0; switchState = IDLE; return true;
			} 
		}
		return false; // ignore anything else
	}
	default: break;
	}
	return false;
}

/** Send a status packet on the multicast group for the current location.
 *  
 *  @param now is the reference time for the status report
 */
void Avatar::sendStatus(int now) {
	if (comt == 0) return;
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);
	p.length = 4*(5+8); p.type = Forest::CLIENT_DATA; p.flags = 0;
	p.comtree = comt; p.srcAdr = myAdr; p.dstAdr = -groupNum(x,y);

	uint32_t *pp = p.payload();
	pp[0] = htonl(STATUS_REPORT);
	pp[1] = htonl(now);
	pp[2] = htonl(x);
	pp[3] = htonl(y);
	pp[4] = htonl((uint32_t) direction);
	pp[5] = htonl((uint32_t) speed);
	pp[6] = htonl(numVisible);
	pp[7] = htonl(numNear);
	send(px);
}

/** Send a status report to the remote controller for this avatar.
 *  @param now is the current time
 *  @param avType is the type code for the avatar whose status is
 *  being sent: 1 means the Avatar for "this" object, 2 a visible avatar
 *  and 3 an avatar that we can "hear" but not see
 *  @param p is an optional packet number; if present and non-zero,
 *  its payload is unpacked to produce the report to send to the avatar
 */
void Avatar::forwardReport(uint32_t now, int avType, pktx px) {
	if (comt == 0) return;
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
	} else if (px != 0) {
		Packet& p = ps->getPacket(px);
		uint32_t *pp = p.payload();
		if (p.comtree != comt) return;

		buf[1] = htonl(p.srcAdr);
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
 *  @param joinLeave is either CLIENT_JOIN_COMTREE or CLIENT_LEAVE_COMTREE,
 *  depending on whether we want to join or leave the comtree
 */
void Avatar::send2comtCtl(CtlPkt::CpType joinLeave, bool retry) {
        pktx px = ps->alloc();
        if (px == 0)
		fatal("Avatar::send2comtCtl: no packets left to allocate");
        Packet& p = ps->getPacket(px);
	if (!retry) seqNum++;
        CtlPkt cp(joinLeave,CtlPkt::REQUEST,seqNum,p.payload());
        cp.comtree = comt;
        cp.ip1 = myIp;
        cp.port1 = Np4d::getSockPort(sock);
        int len = cp.pack();
	if (len == 0) 
		fatal("Avatar::send2comtCtl: control packet packing error");
	p.length = Forest::OVERHEAD + len;
        p.type = Forest::CLIENT_SIG; p.flags = 0;
        p.comtree = Forest::CLIENT_SIG_COMT;
	p.srcAdr = myAdr; p.dstAdr = ccAdr;
        p.pack();
	send(px);
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
 *  c change comtree to the given parameter value
 *
 *  Note that the integer parameter is sent even when
 *  the command doesn't use it
 *  @return the specified comtree value when a 'c' command is received
 *  or 0 in all other cases
 */
comt_t Avatar::check4command() { 
	if (connSock < 0) {
		connSock = Np4d::accept4d(listenSock);
		if (connSock < 0) return 0;
		if (!Np4d::nonblock(connSock))
			fatal("can't make connection socket nonblocking");
		bool status; int ndVal = 1;
		status = setsockopt(listenSock,IPPROTO_TCP,TCP_NODELAY,
			    (void *) &ndVal,sizeof(int));
		if (status != 0) {
			cerr << "setsockopt for no-delay failed\n";
			perror("");
			exit(1);
		}
	}
	char buf[5];
        int nbytes = read(connSock, buf, 5);
        if (nbytes < 0) {
                if (errno == EAGAIN) return 0;
                fatal("Avatar::check4command: error in read call");
	} else if (nbytes == 0) { // remote display closed connection
		close(connSock); connSock = -1;
		unsubscribeAll();
		return 0;
        } else if (nbytes < 5) {
		fatal("Avatar::check4command: incomplete command");
	}
	char cmd = buf[0];
	uint32_t param = ntohl(*((int*) &buf[1]));
	switch (cmd) {
	case 'j': direction -= 10;
		  if (direction < 0) direction += 360;
		  break;
	case 'l': direction += 10;
		  if (direction > 360) direction -= 360;
		  break;
	case 'i': if (speed == STOPPED) speed = SLOW;
	          else if (speed == SLOW) speed = MEDIUM;
		  else if (speed == MEDIUM) speed = FAST;
		  break;
	case 'k': if (speed == FAST) speed = MEDIUM;
	          else if (speed == MEDIUM) speed = SLOW;
		  else if (speed == SLOW) speed = SLOW;
		  break;
	case 'c': return param; 
	}
	return 0;
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
bool Avatar::connect() {
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);

	p.payload()[0] = htonl((uint32_t) (nonce >> 32));
	p.payload()[1] = htonl((uint32_t) (nonce & 0xffffffff));
	p.length = Forest::OVERHEAD + 8; p.type = Forest::CONNECT; p.flags = 0;
	p.comtree = Forest::CONNECT_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	int resendTime = Misc::getTime();
	int resendCount = 1;
	while (true) {
		int now = Misc::getTime();
		if (now > resendTime) {
			if (resendCount > 3) { ps->free(px); return false; }
			send(px);
			resendTime += 1000000;
			resendCount++;
		}
		int rx = receive();
		if (rx == 0) { usleep(100000); continue; }
		Packet& reply = ps->getPacket(rx);
		bool status =  (reply.type == Forest::CONNECT &&
		    		reply.flags == Forest::ACK_FLAG);
		ps->free(px); ps->free(rx);
		return status;
	}
}

/** Send final disconnect packet to forest router.
 */
bool Avatar::disconnect() {
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);

	p.payload()[0] = htonl((uint32_t) (nonce >> 32));
	p.payload()[1] = htonl((uint32_t) (nonce & 0xffffffff));
	p.length = Forest::OVERHEAD + 8; p.type = Forest::DISCONNECT;
	p.flags = 0; p.comtree = Forest::CONNECT_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	int resendTime = Misc::getTime();
	int resendCount = 1;
	while (true) {
		int now = Misc::getTime();
		if (now > resendTime) {
			if (resendCount > 3) { ps->free(px); return false; }
			send(px);
			resendTime += 1000000;
			resendCount++;
		}
		int rx = receive();
		if (rx == 0) { usleep(100000); continue; }
		Packet& reply = ps->getPacket(rx);
		bool status =  (reply.type == Forest::DISCONNECT &&
		    		reply.flags == Forest::ACK_FLAG);
		ps->free(px); ps->free(rx);
		return status;
	}
}

/** Send packet to forest router and recycle storage.
 */
void Avatar::send(pktx px) {
	Packet& p = ps->getPacket(px);
	p.pack();
//string s;
//cerr << "sending to (" << Np4d::ip2string(rtrIp,s) << "," << rtrPort << ")\n";
//cerr << p.toString(s);
	int rv = Np4d::sendto4d(sock,(void *) p.buffer, p.length,
		    		rtrIp, rtrPort);
	if (rv == -1) fatal("Avatar::send: failure in sendto");
	ps->free(px);
}

/** Return next waiting packet or 0 if there is none. 
 */
int Avatar::receive() { 
	pktx px = ps->alloc();
	if (px == 0) return 0;
	Packet& p = ps->getPacket(px);

	ipa_t remoteIp; ipp_t remotePort;
        int nbytes = Np4d::recvfrom4d(sock, p.buffer, 1500,
				      remoteIp, remotePort);
        if (nbytes < 0) {
                if (errno == EWOULDBLOCK) {
                        ps->free(px); return 0;
                }
                fatal("Avatar::receive: error in recvfrom call");
        }
	p.unpack();
//string s;
//cerr << "received from (" << Np4d::ip2string(remoteIp,s) << "," << remotePort << ")\n";
//cerr << p.toString(s);
	if ((p.type == Forest::CLIENT_SIG &&
	     p.comtree != Forest::CLIENT_SIG_COMT) &&
	     p.comtree != comt) {
		ps->free(px);
		return 0;
	}
        p.bufferLen = nbytes;
        p.tunIp = remoteIp;
        p.tunPort = remotePort;

        return px;
}

/** Update status of avatar based on passage of time.
 *  @param now is the reference time for the simulated update
 *  @param controlInput is the input code from the controller,
 *  or -1 if no controller is connected, or 0 if no current input
 */
void Avatar::updateStatus(uint32_t now) {
	const double PI = 3.141519625;

	int prevRegion = groupNum(x,y)-1;

	if (connSock >= 0) { // if there's a driver, stop on collision
		double dist = speed;
		double dirRad = direction * (2*PI/360);
		int x1 = x + (int) (dist * sin(dirRad));
		int y1 = y + (int) (dist * cos(dirRad));
		int postRegion = groupNum(x1,y1)-1;
		if (x1 <= 0 || x1 >= GRID*worldSize-1 ||
		    y1 <= 0 || y1 >= GRID*worldSize-1 ||
		    (prevRegion != postRegion &&
		     (walls[postRegion]&4 ||
		      separated(prevRegion,postRegion)))) {
			speed = STOPPED;
		} else {
			x = x1; y = y1;
			if (postRegion != prevRegion) updateVisSet();
		}
		return;
	}

	// remainder for case of no driver
	bool atLeft  = (prevRegion%worldSize == 0);
	bool atRight = (prevRegion%worldSize == worldSize-1);
	bool atBot   = (prevRegion/worldSize == 0);
	bool atTop   = (prevRegion/worldSize == worldSize-1);
	int xd = x%GRID; int yd = y%GRID;
	if (xd < .25*GRID && (atLeft || walls[prevRegion]&1 ||
		walls[prevRegion-1]&4)) {
		// near left edge of prevRegion
		if (direction >= 270 || direction < 20) direction += 20;
		if (160 < direction && direction < 270) direction -= 20;
		speed = SLOW; deltaDir = 0;
	} else if (xd > .75*GRID && (atRight || walls[prevRegion+1]&1 ||
		walls[prevRegion+1]&4)) {
		// near right edge of prevRegion
		if (340 < direction || direction <= 90) direction -= 20;
		if (90 < direction && direction < 200) direction += 20;
		speed = SLOW; deltaDir = 0;
	} else if (yd < .25*GRID && (atBot || walls[prevRegion-worldSize]&2 ||
		walls[prevRegion-worldSize]&4)) {
		// near bottom of prevRegion
		if (70 < direction && direction <= 180) direction -= 20;
		if (180 < direction && direction < 290) direction += 20;
		speed = SLOW; deltaDir = 0;
	} else if (yd > .75*GRID && (atTop || walls[prevRegion]&2 ||
		walls[prevRegion+worldSize]&4)) {
		// near top of prevRegion
		if (0 <= direction && direction < 110) direction += 20;
		if (250 < direction && direction <= 359) direction -= 20;
		speed = SLOW; deltaDir = 0;
	} else {
		// no walls to avoid, so just make random
		// changes to direction and speed
		direction += deltaDir;
		double r;
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
	// update position using adjusted direction and speed
	if (direction < 0) direction += 360;
	if (direction >= 360) direction -= 360;
	double dist = speed;
	double dirRad = direction * (2*PI/360);
	x += (int) (dist * sin(dirRad));
	y += (int) (dist * cos(dirRad));
	int postRegion = groupNum(x,y)-1;
	if (postRegion != prevRegion) updateVisSet();
}

/** Determine if two adjacent squares are separated by a wall.
 *  @param c0 is the index of a square
 *  @param c0 is the index of an adjacent square (may be diagonal)
 *  @return true if there are walls separating c0 and c1, else false
 */
bool Avatar::separated(int c0, int c1) {
	if (c0 > c1) { int temp = c1; c1 = c0; c0 = temp; }
	if (c0/worldSize == c1/worldSize) // same row
		return walls[c1]&1;
	else if (c0%worldSize == c1%worldSize) // same column
		return walls[c0]&2;
	else if (c0%worldSize > c1%worldSize) { // se/nw diagonal
		if ((walls[c0]&3) == 3 ||
		    (walls[c0]&1 && walls[c1+1]&1) ||
		    (walls[c0]&2 && walls[c0-1]&2) ||
		    (walls[c1+1]&1 && walls[c0-1]&2))
			return true;
		else 
			return false;
	} else { // sw/ne diagonal
		if ((walls[c0]&2 && walls[c0+1]&1) ||
		    (walls[c0+1]&1 && walls[c1]&1) ||
		    (walls[c0]&2 && walls[c0+1]&2) ||
		    (walls[c0+1]&2 && walls[c1]&1))
			return true;
		else 
			return false;
	}
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
	int sq1 = g1-1; int sq2 = g2-1;

	int x1 = sq1%worldSize; int y1 = sq1/worldSize;
	int x2 = sq2%worldSize; int y2 = sq2/worldSize;

	if (x1 > x2) { // swap so x1 <= x2
		int x = x1; x1 = x2; x2 = x;
		int y = y1; y1 = y2; y2 = y;
	}

	if (x1 == x2) {
		int lo = min(y1, y2); int hi = max(y1, y2);
		for (int y = lo; y < hi; y++) {
			if (walls[x1 + y*worldSize] & 2) return false;
		}
		return true;
	} else if (y1 == y2) {
		for (int x = x1+1; x <= x2; x++) {
			if (walls[x + y1*worldSize] & 1) return false;
		}
		return true;
	}

	const double eps = .001;
	double sq1xs[] = {x1+eps,      x1+(1.0-eps),x1+eps,x1+(1.0-eps)};
	double sq1ys[] = {y1+(1.0-eps),y1+(1.0-eps),y1+eps,y1+eps      };
	double sq2xs[] = {x2+eps,      x2+(1.0-eps),x2+eps,x2+(1.0-eps)};
	double sq2ys[] = {y2+(1.0-eps),y2+(1.0-eps),y2+eps,y2+eps      };

	int miny = min(y1,y2); int maxy = max(y1,y2);
	double slope = (y2-y1)/((double) x2-x1);

	for (int i = 0; i < 4; i++) {
	for (int j = 0; j < 4; j++) {
		bool canSee = true;
		double ax = sq1xs[i]; double ay = sq1ys[i];
		double bx = sq2xs[j]; double by = sq2ys[j];
		// line segment ab connects a corner of sq1 to a corner of sq2
		for (int x = x1; x <= x2 && canSee; x++) {
		int lo = miny; int hi = maxy;
		if (y2 > y1) {
			lo = (x==x1 ? y1 : (int) ((x-(x1+1))*slope + y1));
			hi = ((x+1)-x1)*slope + (y1+1);
			lo = max(lo,y1); hi = min(hi,y2);
		} else { // y2 < y1
			lo = ((x+1)-x1)*slope + y1;
			hi = (x==x1 ? y1-1 : (int) ((x-(x1+1))*slope + (y1+1)));
			lo = max(lo,y2); hi = min(hi,y1);
		}
		for (int y = lo; y <= hi; y++) {
			double cx = (double) x;
			double cy = (double) (y+1);
			// (cx,cy) is top left corner of square (x,y)
			int k = x+y*worldSize; // k=square index
			if (walls[k] & 1) {
				// square k has a left side wall
				// check if ab intersects it
				double dx = cx; double dy = cy-1;
				if (linesIntersect(ax,ay,bx,by,cx,cy,dx,dy)) {
					canSee = false; break;
				}
			}
			if (walls[k] & 2) {
				// square k has a top wall
				// check if ab intersects it
				double dx = cx+1; double dy = cy;
				if (linesIntersect(ax,ay,bx,by,cx,cy,dx,dy)) {
					canSee = false; break;
				}
			}
		} }
		if (canSee) return true;
	} }
	return false;
}

bool Avatar::linesIntersect(double ax, double ay, double bx, double by,
		    double cx ,double cy, double dx, double dy) {
	double epsilon = .001;  // two lines are considered vertical if
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
	if (comt == 0 || glist.size() == 0) return;
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);
	uint32_t *pp = p.payload();

	int nsub = 0;
	list<int>::iterator gp;
	for (gp = glist.begin(); gp != glist.end(); gp++) {
		int g = *gp; nsub++;
		if (nsub > 350) {
			pp[0] = htonl((uint32_t) (subSeqNum >> 32));
        		pp[1] = htonl((uint32_t) (subSeqNum & 0xffffffff));
			subSeqNum++;
			pp[2] = htonl(nsub-1); pp[nsub+2] = 0;
			p.length = Forest::OVERHEAD + 4*(4+nsub);
			p.type = Forest::SUB_UNSUB; p.flags = 0;
			p.comtree = comt;
			p.srcAdr = myAdr; p.dstAdr = rtrAdr;
			send(px);
			nsub = 1;
		}
		pp[nsub+2] = htonl(-g);
	}
	pp[0] = htonl((uint32_t) (subSeqNum >> 32));
        pp[1] = htonl((uint32_t) (subSeqNum & 0xffffffff));
	subSeqNum++;
	pp[2] = htonl(nsub); pp[nsub+3] = 0;
	p.length = Forest::OVERHEAD + 4*(4+nsub);
	p.type = Forest::SUB_UNSUB; p.flags = 0;
	p.comtree = comt;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;
	send(px);
	ps->free(px);
}

/** Unsubscribe from a list of multicast groups.
 *  @param glist is a reference to a list of multicast group numbers.
 */
void Avatar::unsubscribe(list<int>& glist) {
	if (comt == 0 || glist.size() == 0) return;
	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);
	uint32_t *pp = p.payload();

	int nunsub = 0;
	list<int>::iterator gp;
	for (gp = glist.begin(); gp != glist.end(); gp++) {
		int g = *gp; nunsub++;
		if (nunsub > 350) {
			pp[0] = htonl((uint32_t) (subSeqNum >> 32));
        		pp[1] = htonl((uint32_t) (subSeqNum & 0xffffffff));
			pp[2] = 0; pp[3] = htonl(nunsub-1);
			p.length = Forest::OVERHEAD + 4*(4+nunsub);
			p.type = Forest::SUB_UNSUB; p.flags = 0;
			p.comtree = comt;
			p.srcAdr = myAdr; p.dstAdr = rtrAdr;
			send(px);
			nunsub = 1;
		}
		pp[nunsub+3] = htonl(-g);
	}
	pp[0] = htonl((uint32_t) (subSeqNum >> 32));
        pp[1] = htonl((uint32_t) (subSeqNum & 0xffffffff));
	subSeqNum++;
	pp[2] = 0; pp[3] = htonl(nunsub);
	p.length = Forest::OVERHEAD + 4*(4+nunsub);
	p.type = Forest::SUB_UNSUB; p.flags = 0; p.comtree = comt;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;
	send(px);
	ps->free(px);
}

/** Subscribe to all multicasts for regions that are currently visible
 *  and that we're not already subscribed to.
 */ 
void Avatar::subscribeAll() {
	list<int> glist;
	set<int>::iterator gp;
	for (gp = myVisSet->begin(); gp != myVisSet->end(); gp++) {
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
	list<int> glist;
	set<int>::iterator gp;
	for (gp = mySubs->begin(); gp != mySubs->end(); gp++) {
		int g = *gp;
		if (myVisSet->find(g) == myVisSet->end())
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
	for (vp = myVisSet->begin(); vp != myVisSet->end(); vp++) {
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
void Avatar::updateNearby(pktx px) {
	Packet& p = ps->getPacket(px);
	p.unpack();
	uint32_t *pp = p.payload();
	if (ntohl(pp[0]) != (int) STATUS_REPORT) return;

	// add the Avatar that sent this status report to the nearAvatars set
	uint64_t avId = p.srcAdr; avId = (avId << 32) | p.srcAdr;
	if (nearAvatars->size() < MAXNEAR)
		nearAvatars->insert(avId);

	// determine if we can see the Avatar that sent this report
	int x1 = ntohl(pp[2]); int y1 = ntohl(pp[3]);
	int g1 = groupNum(x1,y1);
	if (myVisSet->find(g1) == myVisSet->end()) {
		visibleAvatars->remove(avId);
		return;
	}

	// still possible that (x,y) can see (x1,y1)
	// check for walls that intersect the connecting line
	// it sufficies to check walls that belong to the squares
	// that are visible from (x,y)
	bool canSee = true;
	set<int>::iterator vp;
	int minx = min(x,x1)/GRID; int maxx = max(x,x1)/GRID;
	int miny = min(y,y1)/GRID; int maxy = max(y,y1)/GRID;
	for (vp = myVisSet->begin(); vp != myVisSet->end(); vp++) {
		int i = (*vp)-1; // i is index of a visible square
		int xi = i%worldSize; int yi = i/worldSize;
		if (xi < minx || xi > maxx || yi < miny || yi > maxy)
			continue;
		xi *= GRID; yi *= GRID;
		int ax = xi; int ay = yi + GRID;
		if (walls[i] & 2) { // i has top wall
			if(linesIntersect(x,y,x1,y1,ax,ay,ax+GRID,ay)) {
				canSee = false; break;
			}
		}
		if (walls[i] & 1) { // i has left wall
			if(linesIntersect(x,y,x1,y1,ax,ay,ax,ay-GRID)) {
				canSee = false; break;
			}
		}
	}
	if (canSee && visibleAvatars->size() < MAXNEAR)
		visibleAvatars->insert(avId);
}

/* incremental computation of visibility sets
 *
 * Setup a cache of visibility sets
 * When this avatar moves between squares, update pointer myVisSet to
 * point to the visibility set for the current square.
 * If the visibility set for the current square is not in the cache,
 * add it to the cache, possibly evicting the least-recently used
 * thing in the cache.
 * The current visSet goes to the front of the lruList for the cache
 *
 * Where does this happen? In updateStatus, I detect if I am in a new square
 * or not, so this is the place to update myVisSet.
 *
 * Do I need a cache? Given current speeds it takes 16, 40 or 100 time
 * steps to cross a square, so 50 on average. I can do visibility calculation
 * in <1ms so can have 50 avatars doing an update concurrently before
 * that swamps the machine and so this allows maybe 2500 avatars on
 * a machine. Other things will limit it well before this.
 *
 * So, perhaps it's best to start just doing visibility calculation
 * online and defer the cache option. Can always add it later should
 * it be necessary. With this approach, the only big data structure is
 * walls array and it's a byte per region, so a million regions is fine.
 * 
 * Add a maximum visibility distance as well. In practice, 50 squares
 * should be ample. Might get by with 10-20, depending on how we scale
 * things in the world.
 *
 */

void Avatar::updateVisSet() {
	computeVisSet(groupNum(x,y),*myVisSet);
}

} // ends namespace

