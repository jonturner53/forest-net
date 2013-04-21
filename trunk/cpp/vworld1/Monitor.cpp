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
 *       Monitor extIp intIp rtrIp myAdr rtrAdr worldSize finTime [logfile]
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
 *  If an optional log file is specified, the reports are also
 *  written to a local log file in binary form.
 * 
 *  Command line arguments include two ip addresses for the
 *  Monitor. The first is the IP address that the GUI can
 *  use to connect to the Monitor. If this is specified as 0.0.0.0,
 *  the Monitor listens on the default IP address. The second is the
 *  IP address used by the Monitor within the Forest overlay. RtrIp
 *  is the route's IP address, myAdr is the Monitor's Forest
 *  address, rtrAdr is the Forest address of the router, 
 *  worldSize is the number of squares in the virtual world's
 *  basic grid (actually, the grid is worldSizeXworldSize) and
 *  finTime is the number of seconds to run before terminating.
 */
int main(int argc, char *argv[]) {
	ipa_t intIp, extIp, rtrIp;
	fAdr_t myAdr, rtrAdr, comtCtlAdr;
	int finTime, worldSize;

	if (argc != 9 ||
  	    (extIp  = Np4d::ipAddress(argv[1])) == 0 ||
  	    (intIp  = Np4d::ipAddress(argv[2])) == 0 ||
	    (rtrIp  = Np4d::ipAddress(argv[3])) == 0 ||
	    (myAdr  = Forest::forestAdr(argv[4])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[5])) == 0 ||
	    (comtCtlAdr = Forest::forestAdr(argv[6])) == 0 ||
	    (sscanf(argv[7],"%d", &worldSize) != 1) ||
	     sscanf(argv[8],"%d", &finTime) != 1)
		fatal("usage: Monitor extIp intIp rtrIpAdr myAdr rtrAdr "
		      		    "comtCtlAdr worldSize finTime");
	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress(); 
	if (extIp == 0) fatal("can't retrieve default IP address");

	Monitor mon(extIp,intIp,rtrIp,myAdr,rtrAdr,comtCtlAdr,worldSize);
	if (!mon.init()) {
		fatal("Monitor: initialization failure");
	}
	mon.run(1000000*(finTime-1)); // -1 to compensate for delay in init
	exit(0);
}

namespace forest {

// Constructor for Monitor, allocates space and initializes private data
Monitor::Monitor(ipa_t xipa, ipa_t iipa, ipa_t ripa, fAdr_t ma,
		 fAdr_t ra, fAdr_t cca, int ws)
		 : extIp(xipa), intIp(iipa), rtrIp(ripa), myAdr(ma),
		   rtrAdr(ra), comtCtlAdr(cca), worldSize(min(ws,MAX_WORLD)) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);
	mySubs = new set<int>;

	cornerX = 0; cornerY = 0;
	viewSize = min(10,worldSize);
	comt = 0;
	switchState = IDLE;
	seqNum = 0;
	connSock = -1;
}

Monitor::~Monitor() {
	cout.flush();
	if (extSock > 0) close(extSock);
	if (intSock > 0) close(intSock);
	delete ps; delete mySubs;
}

/** Initialize sockets and open log file for writing.
 *  @return true on success, false on failure
 */
bool Monitor::init() {
	intSock = Np4d::datagramSocket();
	if (intSock < 0 || 
	    !Np4d::bind4d(intSock,intIp,MON_PORT) ||
	    !Np4d::nonblock(intSock))
		return false;

	connect(); 		// send initial connect packet
	usleep(1000000);	// 1 second delay provided for use in SPP
				// delay gives SPP linecard the time it needs
				// to setup NAT filters before second packet

	// setup external TCP socket for use by remote GUI
	extSock = Np4d::streamSocket();
	if (extSock < 0) return false;
	if (!Np4d::bind4d(extSock,extIp,MON_PORT)) return false;
	return Np4d::listen4d(extSock) && Np4d::nonblock(extSock);
}

/** Run the monitor, stopping after finishTime
 *  Operate on a cycle with a period of UPDATE_PERIOD milliseconds,
 *  Each cycle, process all the packets that came in during
 *  that period and store results.
 *  Print a record of all avatar's status, once per second. 
 */
void Monitor::run(uint32_t finishTime) {
	pktx px;
	uint32_t now;    	// free-running microsecond time
	uint32_t nextTime;	// time of next operational cycle
	now = nextTime = Misc::getTime(); 

	bool waiting4switch = false;
	while (now <= finishTime) {
		comt_t newComt = check4command();
		if (newComt != 0 && newComt != comt) {
			startComtSwitch(newComt,now);
			waiting4switch = true;
		}
		while ((px = receiveReport()) != 0) {
			Packet& p = ps->getPacket(px);
			if (!waiting4switch) {
				forwardReport(px,now);
				ps->free(px); continue;
			}
			// discard non-signalling packets and pass
			// signalling packets to completeComtSwitch
			if (p.type == CLIENT_DATA) {
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
        cp.ip1 = intIp;
        cp.port1 = Np4d::getSockPort(intSock);
        int len = cp.pack();
	if (len == 0) 
		fatal("Monitor::send2comtCtl: control packet packing error");
	p.length = Forest::OVERHEAD + len;
        p.type = CLIENT_SIG; p.flags = 0;
        p.comtree = Forest::CLIENT_SIG_COMT;
	p.srcAdr = myAdr; p.dstAdr = comtCtlAdr;
        p.pack();
	send2router(px);
}

/** Send packet to Forest router (connect, disconnect, sub_unsub).
 */
void Monitor::send2router(pktx px) {
	Packet& p = ps->getPacket(px);
	p.pack();
	int rv = Np4d::sendto4d(intSock,(void *) p.buffer,p.length,
		    	      	rtrIp,Forest::ROUTER_PORT);
	if (rv == -1) fatal("Monitor::send2router: failure in sendto");
}

/** Check for next Avatar report packet and return it if there is one.
 *  @return next report packet or 0, if no report has been received.
 */
int Monitor::receiveReport() { 
	pktx px = ps->alloc();
	if (px == 0) return 0;
        Packet& p = ps->getPacket(px);

        int nbytes = Np4d::recv4d(intSock,p.buffer,1500);
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
		connSock = Np4d::accept4d(extSock);
		if (connSock < 0) return 0;
		if (!Np4d::nonblock(connSock))
			fatal("can't make connection socket nonblocking");
		bool status; int ndVal = 1;
		status = setsockopt(extSock,IPPROTO_TCP,TCP_NODELAY,
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
			p.type = SUB_UNSUB; p.flags = 0;
			p.comtree = comt;
			p.srcAdr = myAdr; p.dstAdr = rtrAdr;
			send2router(px);
			nsub = 1;
		}
		pp[nsub] = htonl(-g);
	}
	pp[0] = htonl(nsub); pp[nsub+1] = 0;

	p.length = Forest::OVERHEAD + 4*(2+nsub);
	p.type = SUB_UNSUB; p.flags = 0; p.comtree = comt;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;
	send2router(px);

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
			p.type = SUB_UNSUB; p.flags = 0;
			p.comtree = comt;
			p.srcAdr = myAdr; p.dstAdr = rtrAdr;
			send2router(px);
			nunsub = 1;
		}
		pp[nunsub+1] = htonl(-g);
	}
	pp[0] = 0; pp[1] = htonl(nunsub);

	p.length = Forest::OVERHEAD + 4*(2+nunsub);
	p.type = SUB_UNSUB; p.flags = 0; p.comtree = comt;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;
	send2router(px);

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

	if (p.comtree != comt || p.type != CLIENT_DATA ||
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
void Monitor::connect() {
	pktx px = ps->alloc();

	Packet& p = ps->getPacket(px);
	p.length = 4*(5+1); p.type = CONNECT; p.flags = 0;
	p.comtree = Forest::CLIENT_CON_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	send2router(px); ps->free(px);
}

/** Send final disconnect packet to forest router, after unsubscribing.
 */
void Monitor::disconnect() {
	pktx px = ps->alloc();

	Packet& p = ps->getPacket(px);
	p.length = 4*(5+1); p.type = DISCONNECT; p.flags = 0;
	p.comtree = Forest::CLIENT_CON_COMT;
	p.srcAdr = myAdr; p.dstAdr = rtrAdr;

	send2router(px); ps->free(px);
}

} // ends namespace

