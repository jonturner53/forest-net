/** @file Monitor.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Monitor.h"

using namespace forest;

/** usage:
 *       Monitor cmIp myIp worldSize uname pword finTime
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
 * 
 *  Command line arguments include
 *
 *  cmIp	the IP address of the client manager
 *  myIp	the IP address to bind to the local socket
 *  worldSize   the number of squares in the virtual world's
 *  		basic grid (actually, the grid is worldSizeXworldSize)
 *  uname	the user name to use for logging in
 *  pword	the corresponding password
 *  finTime	the number of seconds to run before terminating.
 */
int main(int argc, char *argv[]) {
	ipa_t cmIp, myIp; int finTime, worldSize;

	if (argc != 7 ||
  	    (cmIp  = Np4d::ipAddress(argv[1])) == 0 ||
  	    (myIp  = Np4d::ipAddress(argv[2])) == 0 ||
	    (sscanf(argv[3],"%d", &worldSize) != 1) ||
	     sscanf(argv[6],"%d", &finTime) != 1)
		fatal("usage: Monitor myIp worldSize uname pword finTime");

	Monitor mon(cmIp, myIp,worldSize);
	if (!mon.init(argv[4],argv[5])) {
		fatal("Monitor: initialization failure");
	}
	mon.run(finTime);
	exit(0);
}

namespace forest {

// Constructor for Monitor, allocates space and initializes private data
Monitor::Monitor(ipa_t cmIp1, ipa_t myIp1, int worldSize1) : cmIp(cmIp1),
		 myIp(myIp1), worldSize(min(worldSize1,MAX_WORLD)) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);
	mySubs = new set<int>;

	cornerX = 0; cornerY = 0;
	viewSize = min(10,worldSize);
	comt = 0;
	switchState = IDLE;
	seqNum = 0;
}

Monitor::~Monitor() {
	cout.flush();
	if (listenSock > 0) close(listenSock);
	delete ps; delete mySubs;
}

/** Initialize sockets and open log file for writing.
 *  @return true on success, false on failure
 */
bool Monitor::init(const string& uname, const string& pword) {
	dgSock = Np4d::datagramSocket();
	if (dgSock < 0 || 
	    !Np4d::bind4d(dgSock,myIp,MON_PORT) ||
	    !Np4d::nonblock(dgSock))
		return false;

	// login through client manager
	if (!login(uname, pword)) return false;

	// setup external TCP socket for use by remote GUI
	listenSock = Np4d::streamSocket();
	if (listenSock < 0) return false;
	if (!Np4d::bind4d(listenSock,myIp,MON_PORT)) return false;
	return Np4d::listen4d(listenSock) && Np4d::nonblock(listenSock);
}

/** Send username and password to the Client manager, and receive.
 *  @param cmIpAdr is the IP address of the client manager
 *  @param uname is the username to log in with
 *  @param pword is the password to log in with
 *  @return true on success, false on failure
 */
bool Monitor::login(const string& uname, const string& pword) {
	// open socket to client manager
	int loginSock = Np4d::streamSocket();
        if (loginSock < 0 ||
	    !Np4d::bind4d(loginSock, myIp, 0) ||
	    !Np4d::connect4d(loginSock,cmIp,Forest::CM_PORT)) {
		cerr << "Avatar::login: cannot open/configure socket to "
			"ClientMgr\n";
		return false;
	}
	// start login dialog
	string s = "login: " + uname + "\npassword: " + pword + "\nover\n";
	Np4d::sendString(loginSock,s);
	NetBuffer buf(loginSock,1024);
	string s0, s1, s2;
	if (!buf.readLine(s0) || s0 != "login successful" ||
	    !buf.readLine(s1) || s1 != "over")
		return false;
	
	// proceed to new session dialog
	s = "newSession\nover\n";
	Np4d::sendString(loginSock,s);

	// process reply - expecting my forest address
	if (!buf.readAlphas(s0) || s0 != "yourAddress" || !buf.verify(':') ||
	    !buf.readForestAddress(s1) || !buf.nextLine()) 
		return false;
	myAdr = Forest::forestAdr(s1.c_str());

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

	// continuing - expecting address of comtree controller
	if (!buf.readAlphas(s0) || s0 != "comtCtlAddress" || !buf.verify(':') ||
	    !buf.readForestAddress(s1) || !buf.nextLine()) 
		return false;
	ccAdr = Forest::forestAdr(s1.c_str());
	
	// continuing - expecting connection nonce
	if (!buf.readAlphas(s0) || s0 != "connectNonce" || !buf.verify(':') ||
	    !buf.readInt(nonce) || !buf.nextLine())
		return false;
	if (!buf.readLine(s0) || s0 != "overAndOut")
		return false;
	
	close(loginSock);

	cout << "avatar address=" << Forest::fAdr2string(myAdr,s) << endl;
	cout << "router info= (" << Np4d::ip2string(rtrIp,s) << ",";
	cout << rtrPort << "," << Forest::fAdr2string(rtrAdr,s) << ")\n";
	cout << "comtCtl address=" << Forest::fAdr2string(ccAdr,s) << "\n";
	cout << "nonce=" << nonce << endl;
	return true;
}

/** Run the monitor, stopping after finishTime
 *  Operate on a cycle with a period of UPDATE_PERIOD milliseconds,
 *  Each cycle, process all the packets that came in during
 *  that period and store results.
 *  Print a record of all avatar's status, once per second. 
 */
void Monitor::run(uint32_t finishTime) {
	uint32_t now;    	// free-running microsecond time
	uint32_t nextTime;	// time of next operational cycle
	now = nextTime = Misc::getTime(); 

	connSock = -1;
	bool waiting4switch = false;
	finishTime *= 1000000; // from seconds to microseconds
	while (now <= finishTime) {
		comt_t newComt = check4command();
		if (newComt != 0 && newComt != comt) {
			startComtSwitch(newComt,now);
			waiting4switch = true;
		}
		pktx px;
		while ((px = receiveFromRouter()) != 0) {
			Packet& p = ps->getPacket(px);
			if (!waiting4switch) {
				forwardReport(px,now);
				ps->free(px); continue;
			}
			// discard non-signalling packets and pass
			// signalling packets to completeComtSwitch
			if (p.type == Forest::CLIENT_DATA) {
				ps->free(px); continue;
			}
			waiting4switch = !completeComtSwitch(px,now);
			ps->free(px);
		}
		// check for timeout
		waiting4switch = !completeComtSwitch(0,now);

		nextTime += 1000*UPDATE_PERIOD;
		useconds_t delay = nextTime - Misc::getTime();
		if (0 < delay && delay <= 1000*UPDATE_PERIOD) usleep(delay);
		now = Misc::getTime();
	}

	unsubscribeAll();
	disconnect(); 		// send final disconnect packet
}

/** Start the process of switching to a new comtree.
 *  Unsubscribe to the current comtree and leave send signalling
 *  message to leave the current comtree. 
 *  @param newComt is the number of the comtree we're switching to
 *  @param now is the current time
 */
void Monitor::startComtSwitch(comt_t newComt, uint32_t now) {
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
 *  @return true if the switch has been completed, or if the switch
 *  has failed (either because the ComtCtl sent a negative response or
 *  because it never responded after repeated attempts); otherwise,
 *  return false
 */
bool Monitor::completeComtSwitch(pktx px, uint32_t now) {
	if (switchState == IDLE) return true;
	if (px == 0 && now - switchTimer < SWITCH_TIMEOUT)
		return false; // nothing to do in this case
	switch (switchState) {
	case LEAVING: {
		if (px == 0) {
			if (switchCnt > 3) { // give up
				switchState = IDLE; return true;
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
				switchState = IDLE; return true;
			} 
		}
		return false; // ignore anything else
	}
	case JOINING: {
		if (px == 0) {
			if (switchCnt > 3) { // give up
				switchState = IDLE; return true;
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
				switchState = IDLE; return true;
			} 
		}
		return false; // ignore anything else
	}
	default: break;
	}
	return false;
}

/** Send join or leave packet packet to the ComtreeController.
 *  @param joinLeave is either CLIENT_JOIN_COMTREE or CLIENT_LEAVE_COMTREE,
 *  depending on whether we want to join or leave the comtree
 */
void Monitor::send2comtCtl(CtlPkt::CpType joinLeave, bool retry) {
        pktx px = ps->alloc();
        if (px == 0)
		fatal("Monitor::send2comtCtl: no packets left to allocate");
	if (!retry) seqNum++;
        Packet& p = ps->getPacket(px);
        CtlPkt cp(joinLeave,CtlPkt::REQUEST,seqNum,p.payload());
        cp.comtree = comt;
        cp.ip1 = myIp;
        cp.port1 = Np4d::getSockPort(dgSock);
        int len = cp.pack();
	if (len == 0) 
		fatal("Monitor::send2comtCtl: control packet packing error");
	p.length = Forest::OVERHEAD + len;
        p.type = Forest::CLIENT_SIG; p.flags = 0;
        p.comtree = Forest::CLIENT_SIG_COMT;
	p.srcAdr = myAdr; p.dstAdr = ccAdr;
        p.pack();
	sendToRouter(px);
}

/** Send packet to Forest router (connect, disconnect, sub_unsub).
 */
void Monitor::sendToRouter(pktx px) {
	Packet& p = ps->getPacket(px);
	p.pack();
	int rv = Np4d::sendto4d(dgSock,(void *) p.buffer,p.length,
		    	      	rtrIp,rtrPort);
	if (rv == -1) fatal("Monitor::sendToRouter: failure in sendto");
}

/** Check for next Avatar report packet and return it if there is one.
 *  @return next report packet or 0, if no report has been received.
 */
int Monitor::receiveFromRouter() { 
	pktx px = ps->alloc();
	if (px == 0) return 0;
        Packet& p = ps->getPacket(px);

        int nbytes = Np4d::recv4d(dgSock,p.buffer,1500);
        if (nbytes < 0) { ps->free(px); return 0; }

	p.unpack();
        return px;
}

/** Check for a new command from remote display program.
 *  Check to see if the remote display has sent a comand and
 *  respond appropriately. Commands take the form of pairs
 *  (c,i) where c is a character that identifies a specific
 *  command, and i is an integer that provides an optional
 *  parameter value for the command. The current commands
 *  are defined below.
 *
 *  x set x coordinate of lower left corner of current view to paramater value
 *  y set y coordinate of lower left corner of current view to paramater value
 *  v set view size to value specified by paramater value
 *  c switch to comtree identified by parameter value
 *
 *  For the aswd commands, there are corresponding ASWD commands that
 *  shift by a larger amount. Also, there are OP commands that half/double
 *  the window size.
 *
 *  Note that the integer parameter must be sent even when
 *  the command doesn't use it
 *
 *  @return the requested comtree number if the 'c' command
 *  is received; else 0
 */
comt_t Monitor::check4command() { 
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
                fatal("Monitor::check4command: error in read call");
	} else if (nbytes == 0) { // remote display closed connection
		close(connSock); connSock = -1;
		unsubscribeAll();
		return 0;
        } else if (nbytes < 5) {
		fatal("Monitor::check4command: incomplete command");
	}
	char cmd = buf[0];
	uint32_t param = ntohl(*((int*) &buf[1]));
	switch (cmd) {
	case 'x': cornerX = max(0,min(worldSize-viewSize,param)); break;
	case 'y': cornerY = max(0,min(worldSize-viewSize,param)); break;
	case 'v': viewSize = param;
		  if (viewSize < 1) viewSize = 1;
		  if (viewSize > worldSize) viewSize = worldSize;
		  if (viewSize > MAX_VIEW) viewSize = MAX_VIEW;
		  if (viewSize+cornerX > worldSize)
			viewSize = worldSize-cornerX;
		  if (viewSize+cornerY > worldSize)
			viewSize = worldSize-cornerY;
		  break;
	case 'c': return param;
	default: fatal("unrecognized command from remote display");
	}
	updateSubs();
        return 0;
}


/** Return the multicast group number associated with a virtual world position.
 *  @param x1 is x coordinate
 *  @param y1 is y coordinate
 *  @return the group number for the grid square containing (x1,y1)
 */
int Monitor::groupNum(int x1, int y1) {
	return 1 + (x1/GRID) + (y1/GRID)*worldSize;
}

/** Switch from the current comtree to newComt.
 *  This requires unsubscribing from all current multicasts
 *  and subscribing to multicasts in new comtree. Currently,
 *  Monitor is statically configured as a member of all comtrees.
 */
void Monitor::switchComtrees(int newComt) {
	unsubscribeAll(); comt = newComt; subscribeAll();
}

/** Subscribe to all multicasts for regions in our current view,
 *  that we're not already subscribed to.
 */ 
void Monitor::subscribeAll() {
	list<int> glist;
	for (int xi = cornerX; xi < cornerX + viewSize; xi++) {
	//for (int xi = 0; xi < worldSize; xi++) {
		for (int yi = cornerY; yi < cornerY + viewSize; yi++) {
		//for (int yi = 0; yi < worldSize; yi++) {
			int g = groupNum(xi*GRID, yi*GRID);
			if (mySubs->find(g) == mySubs->end()) {
				mySubs->insert(g); glist.push_back(g);
			}
		}
	}
	subscribe(glist);
}

/** Unsubscribe from all multicasts that we're currently subscribed to.
 */
void Monitor::unsubscribeAll() {
	list<int> glist;
	set<int>::iterator gp;
	for (gp = mySubs->begin(); gp != mySubs->end(); gp++)
		glist.push_back(*gp);
	unsubscribe(glist);
	mySubs->clear();
}

/** Subscribe to a list of multicast groups.
 *  @param glist is a reference to a list of multicast group numbers.
 */
void Monitor::subscribe(list<int>& glist) {
	if (glist.size() == 0) return;

	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);
	uint32_t *pp = p.payload();

	int nsub = 0;
	list<int>::iterator gp;
	for (gp = glist.begin(); gp != glist.end(); gp++) {
		int g = *gp; nsub++;
		if (nsub > 350) {
			pp[0] = htonl(nsub-1); pp[nsub] = 0;
			p.length = Forest::OVERHEAD + 4*(2+nsub);
			p.type = Forest::SUB_UNSUB; p.flags = 0;
			p.comtree = comt;
			p.srcAdr = myAdr; p.dstAdr = rtrAdr;
			sendToRouter(px);
			nsub = 1;
		}
		pp[nsub] = htonl(-g);
	}
	pp[0] = htonl(nsub); pp[nsub+1] = 0;

	p.length = Forest::OVERHEAD + 4*(2+nsub);
	p.type = Forest::SUB_UNSUB; p.flags = 0; p.comtree = comt;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;
	sendToRouter(px);

	ps->free(px);
}

/** Unsubscribe from a list of multicast groups.
 *  @param glist is a reference to a list of multicast group numbers.
 */
void Monitor::unsubscribe(list<int>& glist) {
	if (glist.size() == 0) return;

	pktx px = ps->alloc();
	Packet& p = ps->getPacket(px);
	uint32_t *pp = p.payload();

	int nunsub = 0;
	list<int>::iterator gp;
	for (gp = glist.begin(); gp != glist.end(); gp++) {
		int g = *gp; nunsub++;
		if (nunsub > 350) {
			pp[0] = 0; pp[1] = htonl(nunsub-1);
			p.length = Forest::OVERHEAD + 4*(2+nunsub);
			p.type = Forest::SUB_UNSUB; p.flags = 0;
			p.comtree = comt;
			p.srcAdr = myAdr; p.dstAdr = rtrAdr;
			sendToRouter(px);
			nunsub = 1;
		}
		pp[nunsub+1] = htonl(-g);
	}
	pp[0] = 0; pp[1] = htonl(nunsub);

	p.length = Forest::OVERHEAD + 4*(2+nunsub);
	p.type = Forest::SUB_UNSUB; p.flags = 0; p.comtree = comt;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;
	sendToRouter(px);

	ps->free(px);
}

/** Update subscriptions without changing comtrees.
 *  Unsubscribe to all multicasts that are not in current view,
 *  then subscribe to all missing multicasts that are in current view.
 */
void Monitor::updateSubs() {	
	if (comt == 0) return;
	// identify subscriptions that are not in current view
	list<int> glist;
	set<int>::iterator gp;
	for (gp = mySubs->begin(); gp != mySubs->end(); gp++) {
		int g = *gp;
		int xi = (g-1) % worldSize;
		int yi = (g-1) / worldSize;
		if (xi >= cornerX && xi < cornerX + viewSize &&
		    yi >= cornerY && yi < cornerY + viewSize)
			continue; // keep this subscription
		glist.push_back(g); 
	}
	// remove them from subscription set and unsubscribe
	list<int>::iterator p;
	for (p = glist.begin(); p != glist.end(); p++)
		mySubs->erase(*p);
	unsubscribe(glist);

	// identify subscriptions that are now in our current view,
	// add them to subscription set and subscribe
	glist.clear();
	for (int xi = cornerX; xi < cornerX + viewSize; xi++) {
		for (int yi = cornerY; yi < cornerY + viewSize; yi++) {
			int g = groupNum(xi*GRID, yi*GRID);
			if (mySubs->find(g) == mySubs->end()) {
				mySubs->insert(g); glist.push_back(g);
			}
		}
	}
	subscribe(glist);
}

/** If the given packet is a status report and there is a remote display
 *  connected, then forward the status info to the remote display.
 *  Filter out packets that are not in our current "view".
 *  @param p is the packet number for a packet from the Forest network
 *  @param now is the current time
 */
void Monitor::forwardReport(pktx px, int now) {
	if (comt == 0 || connSock == -1) return;

	Packet& p = ps->getPacket(px);
	uint32_t *pp = p.payload();

	if (p.comtree != comt || p.type != Forest::CLIENT_DATA ||
	    ntohl(pp[0]) != (uint32_t) Avatar::STATUS_REPORT) {
		// ignore packets for other comtrees, or that are not
		// avatar status reports
		ps->free(px); return;
	}
	uint32_t buf[NUMITEMS];
	for (int i = 0; i < NUMITEMS; i++) buf[i] = pp[i];
	buf[0] = htonl(now); buf[1] = htonl(p.srcAdr);
	buf[8] = htonl(comt);

	int nbytes = NUMITEMS*sizeof(uint32_t);
	char* bp = (char *) buf;
	while (nbytes > 0) {
		int n = write(connSock, (void *) bp, nbytes);
		if (n < 0) break;
		bp += n; nbytes -= n;
	}
	ps->free(px); return;
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
bool Monitor::connect() {
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
			sendToRouter(px);
			resendTime += 1000000;
			resendCount++;
		}
		int rx = receiveFromRouter();
		if (rx == 0) { sleep(100000); continue; }
		Packet& reply = ps->getPacket(rx);
		bool status =  (reply.type == Forest::CONNECT &&
		    		reply.flags == Forest::ACK_FLAG);
		ps->free(px); ps->free(rx);
		return status;
	}
}

/** Send final disconnect packet to forest router.
 */
bool Monitor::disconnect() {
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
			sendToRouter(px);
			resendTime += 1000000;
			resendCount++;
		}
		int rx = receiveFromRouter();
		if (rx == 0) { sleep(100000); continue; }
		Packet& reply = ps->getPacket(rx);
		bool status =  (reply.type == Forest::DISCONNECT &&
		    		reply.flags == Forest::ACK_FLAG);
		ps->free(px); ps->free(rx);
		return status;
	}
}

} // ends namespace

