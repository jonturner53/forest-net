/** @file ComtreeController.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ComtreeController.h"

/** usage:
 *       ComtreeController extIp intIp rtrIp myAdr rtrAdr topology
 * 
 *  Command line arguments include two ip addresses for the
 *  ComtreeController. The first is the IP address that a remote display can
 *  use to connect to the ComtreeController. If this is specified as 127.0.0.1
 *  the ComtreeController listens on the default IP address. The second is the
 *  IP address used by the ComtreeController within the Forest overlay. RtrIp
 *  is the router's IP address, myAdr is the ComtreeController's Forest
 *  address, rtrAdr is the Forest address of the router.
 *
 *  Topology is the name of a file that defines the backbone topology of the
 *  forest network and the topologies of one or more comtrees.
 */
main(int argc, char *argv[]) {
}

// Constructor for ComtreeController, allocates space and initializes private data
ComtreeController::ComtreeController() {
}

ComtreeController::~ComtreeController() {
}

/** Initialize sockets.
 *  @return true on success, false on failure
 */
bool ComtreeController::init(char* topology) {
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

	// Read the topology file and store information
	// in internal data structures
}

/** Run the ComtreeController.
 */
void ComtreeController::run(int finishTime) {

	uint32_t now = Misc::getTime();

	while (true) {

		// check for control packets from Forest side
		// and process them

		// check for requests from remote display
		// and process them

		now = Misc::getTime();
	}
}

/** Check for next message from the remote UI.
 *  @return a packet number with a formatted control packet on success,
 *  0 on failure
 */
int ComtreeController::readFromDisplay() { 
	if (connSock < 0) {
		connSock = Np4d::accept4d(extSock);
		if (connSock < 0) return 0;
	}

	if (!Np4d::hasData(connSock)) return 0;

	uint32_t length;
	if (!Np4d::readWord32(connSock, length))
		fatal("ComtreeController::readFromDisplay: cannot read "
		      "packet length from remote display");

	int p = ps->alloc();
	if (p == 0) fatal("ComtreeController::readFromDisplay: out of packets");

	PacketHeader& h = ps->getHeader(p);
	buffer_t& b = ps->getBuffer(p);

	int nbytes = read(connSock, (char *) &b, length);
	if (nbytes != length)
		fatal("ComtreeController::readFromDisplay: cannot read "
		      "message from remote display");

	h.setSrcAdr(myAdr);

        return p;
}

/** Write a packet to the socket for the user interface.
 */
void ComtreeController::writeToDisplay(int p) {
	buffer_t& buf = ps->getBuffer(p);
	int length = ps->getHeader(p).getLength();
	ps->pack(p);

	int nbytes = write(connSock, (void *) &length, sizeof(uint32_t));
	if (nbytes != sizeof(uint32_t))
		fatal("ComtreeController::writeToDisplay: can't write packet "
		      "length to remote display");
	nbytes = write(connSock, (void *) &buf, length);
	if (nbytes != length)
		fatal("ComtreeController::writeToDisplay: can't write packet "
		      "to remote display");
	ps->free(p);
}

/** Check for next packet from the Forest network.
 *  @return next report packet or 0, if no report has been received.
 */
int ComtreeController::rcvFromForest() { 
	int p = ps->alloc();
	if (p == 0) return 0;
        buffer_t& b = ps->getBuffer(p);

        int nbytes = Np4d::recv4d(intSock,&b[0],1500);
        if (nbytes < 0) { ps->free(p); return 0; }
	ps->unpack(p);

	PacketHeader& h = ps->getHeader(p);
	CtlPkt cp(ps->getPayload(p));
	cp.unpack(h.getLength() - (Forest::HDR_LENG + 4));

        return p;
}

/** Send packet to Forest router.
 */
void ComtreeController::sendToForest(int p) {
	buffer_t& buf = ps->getBuffer(p);
	int leng = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(intSock,(void *) &buf,leng,
		    	      	rtrIp,Forest::ROUTER_PORT);
	if (rv == -1) fatal("ComtreeController::sendToForest: failure in sendto");

	/* add later
	save packet in re-send heap with timer set to
	retransmission time
	*/
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
void ComtreeController::connect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	sendToForest(p);
}

/** Send final disconnect packet to forest router.
 */
void ComtreeController::disconnect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	sendToForest(p);
}
