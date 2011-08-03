/** @file NetMgr.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "NetMgr.h"

/** usage:
 *       NetMgr extIp intIp rtrIp myAdr rtrAdr finTime clientInfo
 * 
 *  Command line arguments include two ip addresses for the
 *  NetMgr. The first is the IP address that a remote UI can
 *  use to connect to the NetMgr. If this is specified as 127.0.0.1
 *  the NetMgr listens on the default IP address. The second is the
 *  IP address used by the NetMgr within the Forest overlay. RtrIp
 *  is the router's IP address, myAdr is the NetMgr's Forest
 *  address, rtrAdr is the Forest address of the router.
 *  ClientInfo is a file containing address information relating
 *  clients and routers. This is a temporary expedient.
 */
main(int argc, char *argv[]) {
	ipa_t intIp, extIp, rtrIp; fAdr_t myAdr, rtrAdr, cliMgrAdr;
	int comt, finTime;

	if (argc != 9 ||
  	    (extIp  = Np4d::ipAddress(argv[1])) == 0 ||
  	    (intIp  = Np4d::ipAddress(argv[2])) == 0 ||
	    (rtrIp  = Np4d::ipAddress(argv[3])) == 0 ||
	    (myAdr  = Forest::forestAdr(argv[4])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[5])) == 0 ||
	    (cliMgrAdr = Forest::forestAdr(argv[6])) == 0 ||
	     sscanf(argv[7],"%d", &finTime) != 1)
		fatal("usage: NetMgr extIp intIp rtrIpAdr myAdr rtrAdr "
		      "clientInfo");


	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress(); 
	if (extIp == 0) fatal("can't retrieve default IP address");

	NetMgr nm(extIp,intIp,rtrIp,myAdr,rtrAdr,cliMgrAdr);
	if (!nm.init()) {
		fatal("NetMgr: initialization failure");
	}
	if (!nm.readPrefixInfo(argv[8]))
		fatal("can't read prefix address info");
	nm.run(finTime*1000000);
	exit(0);
}

// Constructor for NetMgr, allocates space and initializes private data
NetMgr::NetMgr(ipa_t xipa, ipa_t iipa, ipa_t ripa, fAdr_t ma, fAdr_t ra, fAdr_t cma)
		: extIp(xipa), intIp(iipa), rtrIp(ripa), myAdr(ma), rtrAdr(ra), cliMgrAdr(cma) {
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1, nPkts+1);
	
	connSock = -1;
	packetsSent = new map<uint32_t,clientInfo>;
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

		int p = recvFromUi();
		if (p != 0) {
			PacketHeader& h = ps->getHeader(p);
			h.setSrcAdr(myAdr);
			ps->pack(p);
			cerr << "sending packet to router" << endl;
			sendToForest(p);
			idleCount = 0;
		}

		p = rcvFromForest();
		PacketHeader& h = ps->getHeader(p);
		if (p != 0 && (h.getPtype() == NET_SIG ||
			       h.getPtype() == CLIENT_SIG)) {
			// this is temporary version
			// ultimately, we'll need to send control packets
			// to configure router
			CtlPkt cp;
			cp.unpack(ps->getPayload(p),
				  h.getLength() - Forest::OVERHEAD);
			if (cp.getCpType() == NEW_CLIENT &&
			    cp.getRrType() == REQUEST) {
				//send ADD_LINK to appropriate router
				ipa_t cliIp = cp.getAttr(CLIENT_IP);
				uint32_t highLvlSeq = cp.getSeqNum() & 0xffffffff;
				uint64_t lowLvlSeq = cp.getSeqNum() >> 32;
				(*packetsSent)[highLvlSeq].cliIp = cliIp;
				cp.setSeqNum((++lowLvlSeq) << 32 | (highLvlSeq & 0xffffffff));
				cp.setRrType(REQUEST);
				cp.setCpType(ADD_LINK);
				cp.setAttr(PEER_IP,cliIp);
				cp.setAttr(PEER_TYPE,CLIENT);
				ipa_t rtrIp; fAdr_t rtrAdr;
				getIpByPrefix(cliIp,rtrAdr,rtrIp);
				int plen = cp.pack(ps->getPayload(p));
				h.setPtype(NET_SIG);
				h.setLength(plen + Forest::OVERHEAD);
				h.setDstAdr(rtrAdr);
				h.setSrcAdr(myAdr);
				ps->pack(p);
				sendToForest(p);
			} else if(cp.getCpType()==ADD_LINK &&
			    cp.getRrType()==POS_REPLY) { // it's a reply to an add link request
				//send ADD_COMTREE_LINK to router
				uint32_t highLvlSeq = cp.getSeqNum() & 0xffffffff;
				uint64_t lowLvlSeq = cp.getSeqNum() >> 32;
				fAdr_t cliAdr = cp.getAttr(PEER_ADR);
				fAdr_t rtrAdr = h.getSrcAdr();
				ipa_t rtrIp = rtrIpFromAdr(rtrAdr);
				(*packetsSent)[highLvlSeq].cliAdr = cliAdr;
				(*packetsSent)[highLvlSeq].rtrAdr = rtrAdr;
				(*packetsSent)[highLvlSeq].rtrIp = rtrIp;
				cp.setSeqNum((++lowLvlSeq << 32)|(highLvlSeq & 0xffffffff));
				cp.setRrType(REQUEST);
				cp.setCpType(ADD_COMTREE_LINK);
				cp.setAttr(COMTREE_NUM,1);
				cp.setAttr(PEER_ADR,cliAdr);
				int plen = cp.pack(ps->getPayload(p));
				h.setPtype(NET_SIG);
				h.setLength(plen + Forest::OVERHEAD);
				h.setDstAdr(rtrAdr);
				h.setSrcAdr(myAdr);
				h.setComtree(100);
				ps->pack(p);
				sendToForest(p);

			} else if(cp.getCpType()==ADD_COMTREE_LINK &&
			    cp.getRrType()==POS_REPLY) {
				//send reply to CliMgr
				uint32_t highLvlSeq = cp.getSeqNum() & 0xffffffff;
				uint64_t lowLvlSeq = cp.getSeqNum() >> 32;
				cp.setSeqNum((++lowLvlSeq << 32)|(highLvlSeq & 0xffffffff));
				cp.setCpType(NEW_CLIENT);
				cp.setRrType(POS_REPLY);
				cp.setAttr(CLIENT_ADR,(*packetsSent)[highLvlSeq].cliAdr);
				cp.setAttr(RTR_IP,(*packetsSent)[highLvlSeq].rtrIp);
				cp.setAttr(RTR_ADR,(*packetsSent)[highLvlSeq].rtrAdr);
				int plen = cp.pack(ps->getPayload(p));
				h.setPtype(NET_SIG);
				h.setLength(plen + Forest::OVERHEAD);
			        h.setDstAdr(cliMgrAdr);
        			h.setSrcAdr(myAdr);
				ps->pack(p);
				sendToForest(p);

			} else if((cp.getCpType() == CLIENT_CONNECT ||
			    cp.getCpType() == CLIENT_DISCONNECT) &&
			    cp.getRrType() == REQUEST) {
				//forward to CliMgr
				uint32_t highLvlSeq = cp.getSeqNum() & 0xfffffff;
				uint64_t lowLvlSeq = cp.getSeqNum() >> 32;
				cp.setSeqNum((++lowLvlSeq << 32)|(highLvlSeq & 0xffffffff));
				h.setDstAdr(cliMgrAdr);
				h.setSrcAdr(myAdr);
				ps->pack(p);
				sendToForest(p);	
			} else if((cp.getCpType() == CLIENT_CONNECT ||
			    cp.getCpType() == CLIENT_DISCONNECT) &&
			    cp.getRrType() == POS_REPLY) {
				uint32_t highLvlSeq = cp.getSeqNum() & 0xffffffff;
				uint64_t lowLvlSeq = cp.getSeqNum() >> 32;
				cp.setSeqNum((++lowLvlSeq << 32)|(highLvlSeq & 0xffffffff));
				h.setDstAdr(cp.getAttr(RTR_ADR));
				h.setSrcAdr(myAdr);
				ps->pack(p);
				sendToForest(p);
			} else { // it's a reply to previous request from CLI
				cerr << "got a reply from router" << endl;
				sendToUi(p);
				idleCount = 0;
			}
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

ipa_t NetMgr::rtrIpFromAdr(fAdr_t rtrAdr) {
	for(size_t i = 0; i < numPrefixes; ++i)
		if(prefixes[i].rtrAdr == rtrAdr)
			return prefixes[i].rtrIp;
	fatal("NetMgr::rtrIpFromAdr: rtrAdr not found");
}
/*
 *
 */
bool NetMgr::getIpByPrefix(ipa_t cliIp, fAdr_t& rtrAdr, ipa_t& rtrIp) {
	string cip;
	Np4d::addIp2string(cip,cliIp);
	for(size_t i = 0; i < numPrefixes; ++i) {
		string ip = prefixes[i].prefix;
		for(size_t j = 0; j < ip.size(); ++j) {
			if(ip[j]=='*') {
				rtrAdr = prefixes[i].rtrAdr;
				rtrIp = prefixes[i].rtrIp;
				return true;
			} else if(cip[j]!=ip[j]) {
				break;
			}
		}
	}
	return false;
}

/*
bool NetMgr::setupClient(ipa_t  cliIp, fAdr_t& cliAdr,
			 ipa_t& rtrIp, fAdr_t& rtrAdr) {
	for (int i = 0; i < numClients; i++) {
		if (cliIp == clientInfo[i].cliIp &&
		    clientInfo[i].inUse == false) {
			cliAdr = clientInfo[i].cliAdr;
			rtrIp  = clientInfo[i].rtrIp;
			rtrAdr = clientInfo[i].rtrAdr;
			clientInfo[i].inUse = true;
			return true;
		}
	}
	return false;
}*/

bool NetMgr::readPrefixInfo(char filename[]) {
	ifstream ifs; ifs.open(filename);
	if(ifs.fail()) return false;
	Misc::skipBlank(ifs);
	int i = 0;
	while(!ifs.eof()) {
		string pfix; fAdr_t rtrAdr; ipa_t rtrIp;
		ifs >> pfix;
		if(!Forest::readForestAdr(ifs,rtrAdr) ||
		   !Np4d::readIpAdr(ifs,rtrIp))
			break;
		prefixes[i].prefix = pfix;
		prefixes[i].rtrAdr = rtrAdr;
		prefixes[i].rtrIp = rtrIp;
		Misc::skipBlank(ifs);
		i++;
	}
	numPrefixes = i;
	cout << "read address info for " << numPrefixes << " clients" << endl;
	return true;
}
/*
bool NetMgr::readClientInfo(char fileName[]) {
	ifstream in; in.open(fileName);
	if (in.fail()) return false;
	Misc::skipBlank(in);
	int i = 0;
	while (!in.eof()) {
		ipa_t  cliIp,  rtrIp;
		fAdr_t cliAdr, rtrAdr;
		if (!Np4d::readIpAdr(in, cliIp) || 
		    !Forest::readForestAdr(in, cliAdr) ||
		    !Np4d::readIpAdr(in, rtrIp) || 
		    !Forest::readForestAdr(in, rtrAdr))
			break;
		if (i >= MAX_CLIENTS) break;
		clientInfo[i].cliIp  = cliIp;
		clientInfo[i].cliAdr = cliAdr;
		clientInfo[i].rtrIp  = rtrIp;
		clientInfo[i].rtrAdr = rtrAdr;
		clientInfo[i].inUse = false;
		Misc::skipBlank(in);
		i++;
	}
	numClients = i;
	cout << "read address information for " << numClients << " clients\n";
	return true;
}*/

/** Check for next packet from the remote UI.
 *  @return a packet number with a formatted control packet on success,
 *  0 on failure
 */
int NetMgr::recvFromUi() { 
	if (connSock < 0) {
		connSock = Np4d::accept4d(extSock);
		if (connSock < 0) return 0;
		if (!Np4d::nonblock(connSock))
			fatal("can't make connection socket nonblocking");
	}

	int p = ps->alloc();
	if (p == 0) return 0;
	PacketHeader& h = ps->getHeader(p);
	buffer_t& b = ps->getBuffer(p);
	
	int nbytes = Np4d::recvBuf(connSock, (char *) &b, Forest::BUF_SIZ);
	if (nbytes == -1) { ps->free(p); return 0; }
	if (nbytes < Forest::HDR_LENG)
		fatal("NetMgr:: recvFromUi: misformatted packet from UI");
	h.unpack(b);
	if (h.getVersion() != 1 || h.getLength() != nbytes ||
	    (h.getPtype() != CLIENT_SIG && h.getPtype() != NET_SIG))
		fatal("NetMgr:: recvFromUi: misformatted packet from UI");
        return p;
}

/** Write a packet to the socket for the user interface.
 */
void NetMgr::sendToUi(int p) {
	buffer_t& buf = ps->getBuffer(p);
	int length = ps->getHeader(p).getLength();
	ps->pack(p);
	Np4d::sendBuf(connSock, (char *) &buf, length);
	ps->free(p);
}

/** Check for next packet from the Forest network.
 *  @return next packet or 0, if no report has been received.
 */
int NetMgr::rcvFromForest() { 
	int p = ps->alloc();
	if (p == 0) return 0;
        buffer_t& b = ps->getBuffer(p);

        int nbytes = Np4d::recv4d(intSock,&b[0],1500);
        if (nbytes < 0) { ps->free(p); return 0; }
	ps->unpack(p);

	PacketHeader& h = ps->getHeader(p);

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
