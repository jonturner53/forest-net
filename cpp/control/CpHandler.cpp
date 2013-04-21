/** @file CpHandler.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "CpHandler.h"

namespace forest {


bool CpHandler::getCp(pktx px, CtlPkt& cp) {
	Packet& p = ps->getPacket(px);
	cp.reset(p.payload(), p.length - Forest::OVERHEAD);
	cp.unpack();
	return cp.mode == CtlPkt::POS_REPLY;
}

/** Send a client add comtree request.
 *  @param dest is the destination address for the packet
 *  @param zipCode is the number of the interface to be added
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientAddComtree(fAdr_t dest, int zipCode) {
	CtlPkt reqCp(CtlPkt::CLIENT_ADD_COMTREE,CtlPkt::REQUEST,0);
	reqCp.zipCode = zipCode;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a client drop comtree request.
 *  @param dest is the destination address for the packet
 *  @param comt is the number of the comtree to be dropped
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientDropComtree(fAdr_t dest, comt_t comt) {
	CtlPkt reqCp(CtlPkt::CLIENT_DROP_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comt;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a client join comtree request.
 *  @param dest is the destination address for the packet
 *  @param comt is the number of the comtree to be dropped
 *  @param clientIp is the client IP address
 *  @param clientPort is the client IP port number
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientJoinComtree(fAdr_t dest, comt_t comt,
				  ipa_t clientIp, ipp_t clientPort) {
	CtlPkt reqCp(CtlPkt::CLIENT_JOIN_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comt;
	reqCp.ip1 = clientIp; reqCp.port1 = clientPort;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a client leave comtree request.
 *  @param dest is the destination address for the packet
 *  @param comt is the number of the comtree to be dropped
 *  @param clientIp is the client IP address
 *  @param clientPort is the client IP port number
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientLeaveComtree(fAdr_t dest, comt_t comt,
				   ipa_t clientIp, ipp_t clientPort) {
	CtlPkt reqCp(CtlPkt::CLIENT_LEAVE_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comt;
	reqCp.ip1 = clientIp; reqCp.port1 = clientPort;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send an add interface request packet.
 *  @param dest is the destination address for the packet
 *  @param iface is the number of the interface to be added
 *  @param ifip is the IP address to be associated with the interface
 *  @param rates is the rate spec for the interface
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::addIface(fAdr_t dest, int iface, ipa_t ifip, RateSpec& rates) {
	CtlPkt reqCp(CtlPkt::ADD_IFACE,CtlPkt::REQUEST,0);
	reqCp.iface = iface;
	reqCp.ip1 = ifip;
	reqCp.rspec1 = rates;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a drop interface request packet.
 *  @param dest is the destination address for the packet
 *  @param iface is the number of the interface to be dropped
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::dropIface(fAdr_t dest, int iface) {
	CtlPkt reqCp(CtlPkt::DROP_IFACE,CtlPkt::REQUEST,0);
	reqCp.iface = iface;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a modify interface request packet.
 *  @param dest is the destination address for the packet
 *  @param iface is the number of the interface to be modified
 *  @param ifip is the IP address to be associated with the interface
 *  @param rates is the rate spec for the interface
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::modIface(fAdr_t dest, int iface, ipa_t ifip, RateSpec& rates) {
	CtlPkt reqCp(CtlPkt::MOD_IFACE,CtlPkt::REQUEST,0);
	reqCp.iface = iface;
	reqCp.ip1 = ifip;
	reqCp.rspec1 = rates;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a get interface request packet.
 *  @param dest is the destination address for the packet
 *  @param iface is the number of the interface
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getIface(fAdr_t dest, int iface) {
	CtlPkt reqCp(CtlPkt::GET_IFACE,CtlPkt::REQUEST,0);
	reqCp.iface = iface;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send an add link request packet.
 *  @param dest is the destination address for the packet
 *  @param peerType is the node type of the peer of the new link
 *  @param peerIp is the ip address of the peer
 *  @param peerPort is the ip port of the peer (may be 0)
 *  @param iface is the number of the interface for the link (may be 0)
 *  @param link is the link number of the link to be added (may be 0)
 *  @param peerAdr is the address for the peer (may be 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::addLink(fAdr_t dest, ntyp_t peerType, ipa_t peerIp,
			ipp_t peerPort, int iface, int link, fAdr_t peerAdr) {
	CtlPkt reqCp(CtlPkt::ADD_LINK,CtlPkt::REQUEST,0);
	reqCp.nodeType = peerType;
	reqCp.ip1 = peerIp; reqCp.port1 = peerPort;
	reqCp.iface = iface; reqCp.link = link;
	reqCp.adr1 = peerAdr;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a drop link request packet.
 *  @param dest is the destination address for the packet
 *  @param link is the number of the link to be dropped
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::dropLink(fAdr_t dest, int link) {
	CtlPkt reqCp(CtlPkt::DROP_LINK,CtlPkt::REQUEST,0);
	reqCp.link = link;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a modify link request packet.
 *  @param dest is the destination address for the packet
 *  @param link is the number of the link to be modified
 *  @param rates is the rate spec for the link
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::modLink(fAdr_t dest, int link, RateSpec& rates) {
	CtlPkt reqCp(CtlPkt::MOD_LINK,CtlPkt::REQUEST,0);
	reqCp.link = link; reqCp.rspec1 = rates;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a get link request packet.
 *  @param dest is the destination address for the packet
 *  @param link is the number of the link 
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getLink(fAdr_t dest, int link) {
	CtlPkt reqCp(CtlPkt::GET_COMTREE,CtlPkt::REQUEST,0);
	reqCp.link = link;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send an add comtree request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree to be added (may be 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::addComtree(fAdr_t dest, comt_t comtree) {
	CtlPkt reqCp(CtlPkt::ADD_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a drop comtree request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the number of the comtree to be dropped
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::dropComtree(fAdr_t dest, comt_t comtree) {
	CtlPkt reqCp(CtlPkt::DROP_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a modify comtree request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the number of the comtree to be modified
 *  @param pLink is the parent link for the comtree
 *  @param coreFlag is the core flag for the comtree
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::modComtree(fAdr_t dest, comt_t comtree, int pLink,
			   int coreFlag) {
	CtlPkt reqCp(CtlPkt::MOD_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	reqCp.link = pLink; reqCp.coreFlag = coreFlag;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a get comtree request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the number of the comtree
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getComtree(fAdr_t dest, comt_t comtree) {
	CtlPkt reqCp(CtlPkt::GET_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send an add comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree to be added
 *  @param link is the link number of the link to be added
 *  @param peerCoreFlag is the core flag of the peer for this link (may be -1)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::addComtreeLink(fAdr_t dest, comt_t comtree, int link,
			       int peerCoreFlag) {
	CtlPkt reqCp(CtlPkt::ADD_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	reqCp.link = link;
	reqCp.coreFlag = peerCoreFlag;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send an add comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree to be added
 *  @param peerCoreFlag is the core flag of the peer for this link (may be -1)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::addComtreeLink(fAdr_t dest, comt_t comtree,
			       ipa_t peerIp, ipp_t peerPort, int peerCoreFlag) {
	CtlPkt reqCp(CtlPkt::ADD_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	reqCp.ip1 = peerIp; reqCp.port1 = peerPort;
	reqCp.coreFlag = peerCoreFlag;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a drop comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree
 *  @param link is the link number of the link to be added
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::dropComtreeLink(fAdr_t dest, comt_t comtree, int link) {
	CtlPkt reqCp(CtlPkt::DROP_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	reqCp.link = link;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a drop comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree
 *  @param peerIp is the ip address of the peer
 *  @param peerPort is the ip port of the peer
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::dropComtreeLink(fAdr_t dest, comt_t comtree,
				ipa_t peerIp, ipp_t peerPort) {
	CtlPkt reqCp(CtlPkt::DROP_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	reqCp.ip1 = peerIp; reqCp.port1 = peerPort;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a modify comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the number of the comtree to be modified
 *  @param link is the link number for the comtree
 *  @param rates is the rate spec for the comtree link
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::modComtreeLink( fAdr_t dest, comt_t comtree, int link,
			   	RateSpec& rates) {
	CtlPkt reqCp(CtlPkt::MOD_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	reqCp.link = link; reqCp.rspec1 = rates;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a get comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the number of the comtree
 *  @param link is the number of the link
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getComtreeLink( fAdr_t dest, comt_t comtree, int link) {
	CtlPkt reqCp(CtlPkt::GET_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree; reqCp.link = link;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a new client request packet.
 *  @param dest is the destination address for the packet
 *  @param clientIp is the ip address of the client
 *  @param clientPort is the port number of the client
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::newClient(fAdr_t dest, ipa_t clientIp, ipp_t clientPort) {
	CtlPkt reqCp(CtlPkt::NEW_CLIENT,CtlPkt::REQUEST,0);
	reqCp.ip1 = clientIp; reqCp.port1 = clientPort;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a client connect request packet.
 *  @param dest is the destination address for the packet
 *  @param clientAdr is the address of the client
 *  @param rtrAdr is the address of the client's access router
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientConnect(fAdr_t dest, fAdr_t clientAdr, fAdr_t rtrAdr) {
	CtlPkt reqCp(CtlPkt::CLIENT_CONNECT,CtlPkt::REQUEST,0);
	reqCp.adr1 = clientAdr; reqCp.adr2 = rtrAdr;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a client disconnect request packet.
 *  @param dest is the destination address for the packet
 *  @param clientAdr is the address of the client
 *  @param rtrAdr is the address of the client's access router
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientDisconnect(fAdr_t dest, fAdr_t clientAdr, fAdr_t rtrAdr) {
	CtlPkt reqCp(CtlPkt::CLIENT_DISCONNECT,CtlPkt::REQUEST,0);
	reqCp.adr1 = clientAdr; reqCp.adr2 = rtrAdr;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a boot request packet.
 *  @param dest is the destination address for the packet
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::bootRequest(fAdr_t dest) {
	CtlPkt reqCp(CtlPkt::BOOT_REQUEST,CtlPkt::REQUEST,0);
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a boot reply packet.
 *  @param dest is the destination address for the packet
 *  @param first is the first in the router's range of assigned addresses
 *  @param last is the last in the router's range of assigned addresses
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::bootReply(fAdr_t dest, fAdr_t first, fAdr_t last) {
	CtlPkt reqCp(CtlPkt::BOOT_REQUEST,CtlPkt::REQUEST,0);
	reqCp.adr1 = first; reqCp.adr2 = last;
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a boot complete packet.
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::bootComplete(fAdr_t dest) {
	CtlPkt reqCp(CtlPkt::BOOT_COMPLETE,CtlPkt::REQUEST,0);
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a boot abort packet.
 *  @param dest is the destination address for the packet
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::bootAbort(fAdr_t dest) {
	CtlPkt reqCp(CtlPkt::BOOT_ABORT,CtlPkt::REQUEST,0);
	pktx reply = sendCtlPkt(reqCp,dest);
	if (reply == 0) return 0;
	Packet& pr = ps->getPacket(reply);
	CtlPkt repCp(pr.payload(), pr.length - Forest::OVERHEAD);
	repCp.unpack();
	return reply;
}

/** Send a control packet back through the main thread.
 *  The control packet object is assumed to be already initialized.
 *  If it is a reply packet, it is packed into a packet object whose index is
 *  then placed in the outq. If it is a request packet, it is sent similarly
 *  and then the method waits for a reply. If the reply is not received
 *  after one second, it tries again. After three attempts, it gives up.
 *  @param cp is the pre-formatted control packet
 *  @param dest is the destination address to which it is to be sent
 *  @return the packet index of the reply, when there is one, and 0
 *  on an error, or if there is no reply.
 */
pktx CpHandler::sendCtlPkt(CtlPkt& cp, fAdr_t dest) {
	pktx px = ps->alloc();
	if (px == 0) {
		logger->log("CpHandler::sendCtlPkt: no packets "
		            "left in packet store\n",4,cp);
		// terminates
	}
	Packet& p = ps->getPacket(px);
	// set default seq# for requests - tells main thread to use "next" seq#
	if (cp.mode == CtlPkt::REQUEST) cp.seqNum = 0;
	cp.payload = p.payload();
	int plen = cp.pack();
	if (plen == 0) {
		logger->log("CpHandler::sendCtlPkt: packing error\n",4,cp);
		// terminates
	}
	p.length = plen + Forest::OVERHEAD;
	if (cp.type < CtlPkt::CLIENT_NET_SIG_SEP) {
		p.type = CLIENT_SIG; p.comtree = Forest::CLIENT_SIG_COMT;
	} else {
		p.type = NET_SIG; p.comtree = Forest::NET_SIG_COMT;
	}
	p.flags = 0; p.dstAdr = dest; p.srcAdr = myAdr;
	p.tunIp = tunIp; p.tunPort = tunPort;
	p.pack();

	if (cp.mode != CtlPkt::REQUEST) {
		outq->enq(px); return 0;
	}
	pktx reply = sendAndWait(px,cp);
	ps->free(px);
	return reply;
}

/** Send a control request packet multiple times before giving up.
 *  This method makes a copy of the original and sends the copy
 *  back to the main thread. If no reply is received after 1 second,
 *  it tries again; it makes a total of three attempts before giving up.
 *  @param px is the packet index of the packet to be sent; the header
 *  for px is assumed to be unpacked
 *  @param cp is the control packet structure for p (already unpacked)
 *  @return the packet number for the reply packet, or 0 if there
 *  was an error or no reply
 */
int CpHandler::sendAndWait(pktx px, CtlPkt& cp) {
	Packet& p = ps->getPacket(px);
	p.srcAdr = myAdr; p.pack();

	// make copy of packet and send the copy
	pktx copy = ps->fullCopy(px);
	if (copy == 0) {
		logger->log("CpHandler::sendAndWait: no packets "
		            "left in packet store\n",4,p);
		// terminates
	}

	outq->enq(copy);

	for (int i = 1; i < 3; i++) {
		pktx reply = inq->deq(1000000000); // 1 sec timeout
		if (reply == Queue::TIMEOUT) {
			// no reply, make new copy and send
			pktx retry = ps->fullCopy(px);
			if (retry == 0) {
				logger->log("CpHandler::sendAndWait: no "
					    "packets left in packet store\n",
					    4,p);
				// terminates
			}
			Packet& pr = ps->getPacket(retry);
			cp.payload = pr.payload();
			cp.seqNum = 1;	// tag retry as a repeat
			cp.pack();
			pr.payErrUpdate();
			outq->enq(retry);
		} else {
			Packet& pr = ps->getPacket(reply);
			CtlPkt repCp(pr.payload(),pr.length-Forest::OVERHEAD);
			repCp.unpack();
			if (repCp.type == CtlPkt::NEG_REPLY) {
				logger->log("CpHandler::sendAndWait: negative "
					    "reply (" + repCp.errMsg +
					    ") to control packet",1,pr);
			}
			return reply;
		}
	}
	logger->log("CpHandler::sendAndWait: no response to control packet",
		    2,p);

	return 0;
}

/** Build and send error reply packet for p.
 *  @param p is the packet number of the request packet
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param msg is the error message to be sent.
 */
void CpHandler::errReply(pktx px, CtlPkt& cp, const char* msg) {
	Packet& p = ps->getPacket(px);

	pktx px1 = ps->alloc();
	Packet& p1 = ps->getPacket(px1);
	CtlPkt cp1(cp.type,CtlPkt::NEG_REPLY,cp.seqNum);
	cp1.errMsg = msg;
	cp1.payload = p1.payload();

	p1.length = Forest::OVERHEAD + cp1.pack();
	p1.type = p.type; p1.flags = 0; p1.comtree = p.comtree;
	p1.dstAdr = p.srcAdr; p1.srcAdr = myAdr;
	p1.pack();

	outq->enq(px1);
}

} // ends namespace

