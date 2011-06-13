/** @file NetMgr.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "NetMgr.h"

/** usage:
 *       NetMgr extIp intIp rtrIp myAdr rtrAdr
 * 
 *  Command line arguments include two ip addresses for the
 *  NetMgr. The first is the IP address that a remote UI can
 *  use to connect to the NetMgr. If this is specified as 127.0.0.1
 *  the NetMgr listens on the default IP address. The second is the
 *  IP address used by the NetMgr within the Forest overlay. RtrIp
 *  is the router's IP address, myAdr is the NetMgr's Forest
 *  address, rtrAdr is the Forest address of the router.
 */
main(int argc, char *argv[]) {
	ipa_t intIp, extIp, rtrIp; fAdr_t myAdr, rtrAdr;
	int comt, finTime;

	if (argc != 6 ||
  	    (extIp  = Np4d::ipAddress(argv[1])) == 0 ||
  	    (intIp  = Np4d::ipAddress(argv[2])) == 0 ||
	    (rtrIp  = Np4d::ipAddress(argv[3])) == 0 ||
	    (myAdr  = Forest::forestAdr(argv[4])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[5])) == 0 ||
	     sscanf(argv[6],"%d", &finTime) != 1)
		fatal("usage: NetMgr extIp intIp rtrIpAdr myAdr rtrAdr");

	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress(); 
	if (extIp == 0) fatal("can't retrieve default IP address");

	NetMgr mon(extIp,intIp,rtrIp,myAdr,rtrAdr);
	if (!mon.init()) {
		perror("perror");
		fatal("NetMgr: initialization failure");
	}
	mon.run();
	exit(0);
}

// Constructor for NetMgr, allocates space and initializes private data
NetMgr::NetMgr(ipa_t xipa, ipa_t iipa, ipa_t ripa, fAdr_t ma, fAdr_t ra)
		: extIp(xipa), intIp(iipa), rtrIp(ripa), myAdr(ma), rtrAdr(ra) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);

	connSock = -1;
}

NetMgr::~NetMgr() {
	cout.flush();
	if (extSock > 0) close(extSock);
	if (intSock > 0) close(intSock);
	delete ps;
}

/** Initialize sockets.
 *  @return true on success, false on failure
 */
bool NetMgr::init() {
	intSock = Np4d::datagramSocket();
	if (intSock < 0 || 
	    !Np4d::bind4d(intSock,intIp,0) ||
	    !Np4d::nonblock(intSock))
		return false;

	connect(); 		// send initial connect packet
	usleep(1000000);	// 1 second delay provided for use in SPP
				// delay gives SPP linecard the time it needs
				// to setup NAT filters before second packet

	// setup external TCP socket for use by remote GUI
	extSock = Np4d::streamSocket();
	if (extSock < 0) return false;
	return	Np4d::bind4d(extSock,extIp,NM_PORT) &&
		Np4d::listen4d(extSock) && 
		Np4d::nonblock(extSock);
}

/** Run the NetMgr.
 *  @param finishTime is the number of seconds to run before halting;
 *  if zero, run forever
 */
void NetMgr::run(int finishTime) {

	int idleCount = 0;

	uint32_t now = Misc::getTime();

	while (finishTime == 0 || now <= finishTime) {

		idleCount++;

		int p = readFromUi();

		if (p != 0) {
			sendToForest(p);
			idleCount = 0;
		}

		p = rcvFromForest();

		if (p != 0) {
			writeToUi(p);
			idleCount = 0;
		} 

		/* add later
		checkTimers(); // if any expired timers, re-send
		*/

		if (idleCount >= 10) { // sleep for 1 ms
			struct timespec sleeptime, rem;
                        sleeptime.tv_sec = 0; sleeptime.tv_nsec = 1000000;
                        nanosleep(&sleeptime,&rem);
		}

		now = Misc::getTime();
	}
}

/** Check for next message from the remote UI.
 *  @return a packet number with a formatted control packet on success,
 *  0 on failure
 */
int NetMgr::readFromUi() { 
	if (connSock < 0) {
		connSock = Np4d::accept4d(extSock);
		if (connSock < 0) return 0;
		if (!nonblock(connSock))
			fatal("can't make connection socket nonblocking");
	}

	uint32_t length;
	if (!Np4d::readWord32(connSock, length))
		fatal("NetMgr::readFromUi: cannot read packet length from UI");

	int p = ps->alloc();
	if (p == 0) fatal("NetMgr::readFromUi: out of packets");

	PacketHeader& h = ps->getHeader(p);
	buffer_t& b = ps->getBuffer(p);

	int nbytes = read(connSock, (char *) &b, length);
	if (nbytes != length)
		fatal("NetMgr::readFromUi: cannot read message from UI");

	h.setSrcAdr(myAdr);

        return p;
}

/** Write a packet to the socket for the user interface.
 */
void NetMgr::writeToUi(int p) {
	buffer_t& buf = ps->getBuffer(p);
	int length = ps->getHeader(p).getLength();
	ps->pack(p);

	int nbytes = write(connSock, (void *) &length, sizeof(uint32_t));
	if (nbytes != sizeof(uint32_t))
		fatal("NetMgr::writeToUi: can't write packet length to UI");
	nbytes = write(connSock, (void *) &buf, length);
	if (nbytes != length)
		fatal("NetMgr::writeToUi: can't write packet to UI");
	ps->free(p);
}

/** Check for next packet from the Forest network.
 *  @return next report packet or 0, if no report has been received.
 */
int NetMgr::rcvFromForest() { 
	int p = ps->alloc();
	if (p == 0) return 0;
        buffer_t& b = ps->getBuffer(p);

        int nbytes = Np4d::recv4d(intSock,&b[0],1500);
        if (nbytes < 0) { ps->free(p); return 0; }
	ps->unpack(p);

	PacketHeader& h = ps->getHeader(p);
	CtlPkt cp(ps->getPayload(p));
	cp.unpack(h.getLength() - (Forest::HDR_LENG + 4));

	/* add later
	if (it's a control packet reply) {
		match it to an outgoing request and clear timer
		use hash table with sequence number as key
		packets from UI are required to have sequence
		numbers with a high order bit or zero;
		those that originate with the NetMgr have
		a seq# with a high bit of 1;
		or, we distinguish in some other way.
	}
	*/

        return p;
}

/** Send packet to Forest router.
 */
void NetMgr::sendToForest(int p) {
	buffer_t& buf = ps->getBuffer(p);
	int leng = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(intSock,(void *) &buf,leng,
		    	      	rtrIp,Forest::ROUTER_PORT);
	if (rv == -1) fatal("NetMgr::sendToForest: failure in sendto");

	/* add later
	save packet in re-send heap with timer set to
	retransmission time
	*/
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
void NetMgr::connect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	sendToForest(p);
}

/** Send final disconnect packet to forest router.
 */
void NetMgr::disconnect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	sendToForest(p);
}
