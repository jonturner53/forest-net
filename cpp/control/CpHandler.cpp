/** @file CpHandler.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "CpHandler.h"

namespace forest {

/** Send a client add comtree request.
 *  @param dest is the destination address for the packet
 *  @param zipCode is the number of the interface to be added
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientAddComtree(fAdr_t dest, int zipCode, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::CLIENT_ADD_COMTREE,CtlPkt::REQUEST,0);
	reqCp.zipCode = zipCode;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a client drop comtree request.
 *  @param dest is the destination address for the packet
 *  @param comt is the number of the comtree to be dropped
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientDropComtree(fAdr_t dest, comt_t comt,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::CLIENT_DROP_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comt;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a client join comtree request.
 *  @param dest is the destination address for the packet
 *  @param comt is the number of the comtree to be dropped
 *  @param clientIp is the client IP address
 *  @param clientPort is the client IP port number
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientJoinComtree(fAdr_t dest, comt_t comt,
				  ipa_t clientIp, ipp_t clientPort,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::CLIENT_JOIN_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comt;
	reqCp.ip1 = clientIp; reqCp.port1 = clientPort;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a client leave comtree request.
 *  @param dest is the destination address for the packet
 *  @param comt is the number of the comtree to be dropped
 *  @param clientIp is the client IP address
 *  @param clientPort is the client IP port number
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientLeaveComtree(fAdr_t dest, comt_t comt,
				   ipa_t clientIp, ipp_t clientPort,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::CLIENT_LEAVE_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comt;
	reqCp.ip1 = clientIp; reqCp.port1 = clientPort;
	return sendRequest(reqCp,dest,repCp);
}

/** Send an add interface request packet.
 *  @param dest is the destination address for the packet
 *  @param iface is the number of the interface to be added
 *  @param ifip is the IP address to be associated with the interface
 *  @param rates is the rate spec for the interface
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::addIface(fAdr_t dest, int iface, ipa_t ifip, RateSpec& rates,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::ADD_IFACE,CtlPkt::REQUEST,0);
	reqCp.iface = iface;
	reqCp.ip1 = ifip;
	reqCp.rspec1 = rates;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a drop interface request packet.
 *  @param dest is the destination address for the packet
 *  @param iface is the number of the interface to be dropped
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::dropIface(fAdr_t dest, int iface,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::DROP_IFACE,CtlPkt::REQUEST,0);
	reqCp.iface = iface;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a modify interface request packet.
 *  @param dest is the destination address for the packet
 *  @param iface is the number of the interface to be modified
 *  @param ifip is the IP address to be associated with the interface
 *  @param rates is the rate spec for the interface
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::modIface(fAdr_t dest, int iface, ipa_t ifip, RateSpec& rates,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::MOD_IFACE,CtlPkt::REQUEST,0);
	reqCp.iface = iface;
	reqCp.ip1 = ifip;
	reqCp.rspec1 = rates;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get interface request packet.
 *  @param dest is the destination address for the packet
 *  @param iface is the number of the interface
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getIface(fAdr_t dest, int iface,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_IFACE,CtlPkt::REQUEST,0);
	reqCp.iface = iface;
	return sendRequest(reqCp,dest,repCp);
}

/** Send an add link request packet.
 *  @param dest is the destination address for the packet
 *  @param peerType is the node type of the peer of the new link
 *  @param iface is the number of the interface for the link
 *  @param link is the link number of the link to be added
 *  @param peerIp is the ip address of the peer
 *  @param peerPort is the ip port of the peer
 *  @param peerAdr is the address for the peer
 *  @param nonce is the nonce that must be included in client connect
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */

pktx CpHandler::addLink(fAdr_t dest,Forest::ntyp_t peerType, int iface, int lnk,
			ipa_t peerIp, ipp_t peerPort, fAdr_t peerAdr,
			uint64_t nonce, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::ADD_LINK,CtlPkt::REQUEST,0);
	reqCp.nodeType = peerType; reqCp.iface = iface; reqCp.link = lnk;
	reqCp.ip1 = peerIp; reqCp.port1 = peerPort; reqCp.adr1 = peerAdr;
	reqCp.nonce = nonce;
	return sendRequest(reqCp,dest,repCp);
}

/** Send an add link request packet (short form for dynamic leaf nodes).
 *  @param dest is the destination address for the packet
 *  @param peerType is the node type of the peer of the new link
 *  @param iface is the number of the interface for the link
 *  @param nonce is the nonce that must be included in client connect
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response;
 *  the reply packet should include the link number assigned by the
 *  router and the Forest address assigned to the peer by the router.
 */
pktx CpHandler::addLink(fAdr_t dest, Forest::ntyp_t peerType, int iface,
			uint64_t nonce, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::ADD_LINK,CtlPkt::REQUEST,0);
	reqCp.nodeType = peerType; reqCp.iface = iface; reqCp.nonce = nonce;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a drop link request packet.
 *  @param dest is the destination address for the packet
 *  @param link is the number of the link to be dropped (may be 0)
 *  @param peerAdr is the address of the peer at the end of the link (may be 0)
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::dropLink(fAdr_t dest, int link, fAdr_t peerAdr, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::DROP_LINK,CtlPkt::REQUEST,0);
	if (link != 0) reqCp.link = link;
	if (peerAdr != 0) reqCp.adr1 = peerAdr;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a modify link request packet.
 *  @param dest is the destination address for the packet
 *  @param link is the number of the link to be modified
 *  @param rates is the rate spec for the link
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::modLink(fAdr_t dest, int link, RateSpec& rates,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::MOD_LINK,CtlPkt::REQUEST,0);
	reqCp.link = link; reqCp.rspec1 = rates;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get link request packet.
 *  @param dest is the destination address for the packet
 *  @param link is the number of the link 
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getLink(fAdr_t dest, int link,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_LINK,CtlPkt::REQUEST,0);
	reqCp.link = link;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get link set request packet.
 *  @param dest is the destination address for the packet
 *  @param firstLink is the table index for the first link to be returned;
 *  if zero, start with the first link in the table
 *  @param count is the number of links whose table entries are requested
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getLinkSet(fAdr_t dest, int link, int count, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_LINK_SET,CtlPkt::REQUEST,0);
	reqCp.index1 = link; reqCp.count = count;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get comtree set request packet.
 *  @param dest is the destination address for the packet
 *  @param firstLink is the table index for the first comtree to be returned;
 *  if zero, start with the first comtree in the table
 *  @param count is the number of comtrees whose table entries are requested
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getComtreeSet(fAdr_t dest, int link, int count, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_COMTREE_SET,CtlPkt::REQUEST,0);
	reqCp.index1 = link; reqCp.count = count;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get iface set request packet.
 *  @param dest is the destination address for the packet
 *  @param firstIface is the table index for the first iface to be returned;
 *  if zero, start with the first iface in the table
 *  @param count is the number of ifaces whose table entries are requested
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getIfaceSet(fAdr_t dest, int iface, int count, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_IFACE_SET,CtlPkt::REQUEST,0);
	reqCp.index1 = iface; reqCp.count = count;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get route set request packet.
 *  @param dest is the destination address for the packet
 *  @param firstRoute is the table index for the first route to be returned;
 *  if zero, start with the first route in the table
 *  @param count is the number of routes whose table entries are requested
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getRouteSet(fAdr_t dest, int route, int count, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_ROUTE_SET,CtlPkt::REQUEST,0);
	reqCp.index1 = route; reqCp.count = count;
	return sendRequest(reqCp,dest,repCp);
}

/** Send an add comtree request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree to be added (may be 0)
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::addComtree(fAdr_t dest, comt_t comtree,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::ADD_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a drop comtree request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the number of the comtree to be dropped
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::dropComtree(fAdr_t dest, comt_t comtree,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::DROP_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a modify comtree request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the number of the comtree to be modified
 *  @param pLink is the parent link for the comtree
 *  @param coreFlag is the core flag for the comtree
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::modComtree(fAdr_t dest, comt_t comtree, int pLink,
			   int coreFlag,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::MOD_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	reqCp.link = pLink; reqCp.coreFlag = coreFlag;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get comtree request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the number of the comtree
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getComtree(fAdr_t dest, comt_t comtree,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_COMTREE,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	return sendRequest(reqCp,dest,repCp);
}

/** Send an add comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree to be added
 *  @param link is the link number of the link to be added
 *  @param peerCoreFlag is the core flag of the peer for this link (may be -1)
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::addComtreeLink(fAdr_t dest, comt_t comtree, int link,
			       int peerCoreFlag,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::ADD_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree; reqCp.link = link;
	reqCp.coreFlag = peerCoreFlag;
	return sendRequest(reqCp,dest,repCp);
}

/** Send an add comtree link request packet.
 *  This version is used only when the peer is a leaf node.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree to be added
 *  @param peerAdr is the forest address of the peer
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::addComtreeLink(fAdr_t dest, comt_t comtree, fAdr_t peerAdr,
			       CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::ADD_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree; reqCp.adr1 = peerAdr;
	return sendRequest(reqCp,dest,repCp);
}

/** Send an add comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree to be added
 *  @param peerCoreFlag is the core flag of the peer for this link (may be -1)
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
pktx CpHandler::addComtreeLink(fAdr_t dest, comt_t comtree,
			       ipa_t peerIp, ipp_t peerPort, int peerCoreFlag,
				CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::ADD_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	reqCp.ip1 = peerIp; reqCp.port1 = peerPort;
	reqCp.coreFlag = peerCoreFlag;
	return sendRequest(reqCp,dest,repCp);
}
 */

/** Send a drop comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree
 *  @param link is the link number of the link to be added (may be 0)
 *  @param peerAdr is the address of the peer node (must be non-zero if link=0)
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::dropComtreeLink(fAdr_t dest, comt_t comtree, int link,
				fAdr_t peerAdr, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::DROP_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	if (link == 0 && peerAdr == 0) {
		logger->log("CpHandler::dropLink: link, peerAdr both 0", 2);
		return false;
	}
	if (link != 0) reqCp.link = link;
	else if (peerAdr != 0) reqCp.adr1 = peerAdr;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a drop comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the comtree number of the comtree
 *  @param peerIp is the ip address of the peer
 *  @param peerPort is the ip port of the peer
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
pktx CpHandler::dropComtreeLink(fAdr_t dest, comt_t comtree,
				ipa_t peerIp, ipp_t peerPort,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::DROP_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	reqCp.ip1 = peerIp; reqCp.port1 = peerPort;
	return sendRequest(reqCp,dest,repCp);
}
 */

/** Send a modify comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the number of the comtree to be modified
 *  @param link is the link number for the comtree
 *  @param rates is the rate spec for the comtree link
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::modComtreeLink( fAdr_t dest, comt_t comtree, int link,
			   	RateSpec& rates,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::MOD_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree;
	reqCp.link = link; reqCp.rspec1 = rates;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get comtree link request packet.
 *  @param dest is the destination address for the packet
 *  @param comtree is the number of the comtree
 *  @param link is the number of the link
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getComtreeLink( fAdr_t dest, comt_t comtree, int link,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_COMTREE_LINK,CtlPkt::REQUEST,0);
	reqCp.comtree = comtree; reqCp.link = link;
	return sendRequest(reqCp,dest,repCp);
}

/** Send an add filter request packet.
 *  @param dest is the destination address for the packet
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response;
 *  the reply packet should include the link number assigned by the
 *  router and the Forest address assigned to the peer by the router.
 */
pktx CpHandler::addFilter(fAdr_t dest, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::ADD_FILTER,CtlPkt::REQUEST,0);
	return sendRequest(reqCp,dest,repCp);
}

/** Send a drop filter request packet.
 *  @param dest is the destination address for the packet
 *  @param filter is the number of the filter to be dropped
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::dropFilter(fAdr_t dest, int filter, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::DROP_FILTER,CtlPkt::REQUEST,0);
	reqCp.index1 = filter;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a modify filter request packet.
 *  @param dest is the destination address for the packet
 *  @param filter is the number of the filter to be modified
 *  @param filterString is a string representation fo the filter
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::modFilter(fAdr_t dest, int filter, string& filterString,
			  CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::MOD_LINK,CtlPkt::REQUEST,0);
	reqCp.index1 = filter; reqCp.stringData.assign(filterString);
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get filter request packet.
 *  @param dest is the destination address for the packet
 *  @param filter is the number of the filter 
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getFilter(fAdr_t dest, int filter, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_FILTER,CtlPkt::REQUEST,0);
	reqCp.index1 = filter;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get filter set request packet.
 *  @param dest is the destination address for the packet
 *  @param firstFilter is the table index for the first filter to be returned;
 *  if zero, start with the first link in the table
 *  @param count is the number of links whose table entries are requested
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getFilterSet(fAdr_t dest, int link, int count, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_FILTER_SET,CtlPkt::REQUEST,0);
	reqCp.index1 = link; reqCp.count = count;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a get logged packets request packet.
 *  @param dest is the destination address for the packet
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::getLoggedPackets(fAdr_t dest, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::GET_LOGGED_PACKETS,CtlPkt::REQUEST,0);
	return sendRequest(reqCp,dest,repCp);
}

/** Send a new session request packet.
 *  @param dest is the destination address for the packet
 *  @param clientIp is the ip address of the client
 *  @param rates is the rate spec for this session
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::newSession(fAdr_t dest, ipa_t clientIp, RateSpec& rates, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::NEW_SESSION,CtlPkt::REQUEST,0);
	reqCp.ip1 = clientIp; reqCp.rspec1 = rates;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a cancel session request packet.
 *  @param dest is the destination address for the packet
 *  @param clientAdr is the forest address of the client
 *  @param rtAdr is the forest address of the client's router
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::cancelSession(fAdr_t dest, fAdr_t clientAdr, fAdr_t rtrAdr,
			      CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::CANCEL_SESSION,CtlPkt::REQUEST,0);
	reqCp.adr1 = clientAdr; reqCp.adr2 = rtrAdr;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a client connect request packet.
 *  @param dest is the destination address for the packet
 *  @param clientAdr is the address of the client
 *  @param rtrAdr is the address of the client's access router
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientConnect(fAdr_t dest, fAdr_t clientAdr, fAdr_t rtrAdr,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::CLIENT_CONNECT,CtlPkt::REQUEST,0);
	reqCp.adr1 = clientAdr; reqCp.adr2 = rtrAdr;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a client disconnect request packet.
 *  @param dest is the destination address for the packet
 *  @param clientAdr is the address of the client
 *  @param rtrAdr is the address of the client's access router
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::clientDisconnect(fAdr_t dest, fAdr_t clientAdr, fAdr_t rtrAdr,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::CLIENT_DISCONNECT,CtlPkt::REQUEST,0);
	reqCp.adr1 = clientAdr; reqCp.adr2 = rtrAdr;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a boot router packet.
 *  @param dest is the destination address for the packet
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::bootRouter(fAdr_t dest,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::BOOT_ROUTER,CtlPkt::REQUEST,0);
	return sendRequest(reqCp,dest,repCp);
}

/** Send a leaf config packet.
 *  @param dest is the destination address for the packet
 *  @param leafAdr is the forest address assigned to the leaf
 *  @param rtrAdr is the forest address of the leaf's access router
 *  @param rtrIp is the IP of the leaf's access router
 *  @param rtrPort is the port number of the leaf's access router
 *  @param nonce is a nonce that the leaf can use to connect to router
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::configLeaf(fAdr_t dest, fAdr_t leafAdr, fAdr_t rtrAdr,
			   ipa_t rtrIp, ipp_t rtrPort, uint64_t nonce,
			   CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::CONFIG_LEAF,CtlPkt::REQUEST,0);
	reqCp.adr1 = leafAdr; reqCp.adr2 = rtrAdr;
	reqCp.ip1 = rtrIp; reqCp.port1 = rtrPort;
	reqCp.nonce = nonce;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a set leaf range packet to a router
 *  @param dest is the destination address for the packet
 *  @param first is the first forest address in the target router's leaf range
 *  @param last is the last forest address in the target router's leaf range
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::setLeafRange(fAdr_t dest, fAdr_t first, fAdr_t last,
			     CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::SET_LEAF_RANGE,CtlPkt::REQUEST,0);
	reqCp.adr1 = first; reqCp.adr2 = last;
	return sendRequest(reqCp,dest,repCp);
}

/** Send a boot leaf packet.
 *  @param dest is the destination address for the packet
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::bootLeaf(fAdr_t dest, CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::BOOT_LEAF,CtlPkt::REQUEST,0);
	return sendRequest(reqCp,dest,repCp);
}

/** Send a boot complete packet.
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::bootComplete(fAdr_t dest,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::BOOT_COMPLETE,CtlPkt::REQUEST,0);
	return sendRequest(reqCp,dest,repCp);
}

/** Send a boot abort packet.
 *  @param dest is the destination address for the packet
 *  @param repCp is a reference to a control packet in which the control
 *  packet in the response is returned (if response is != 0)
 *  @return the index of the response packet or 0 if there is no response
 */
pktx CpHandler::bootAbort(fAdr_t dest,CtlPkt& repCp) {
	CtlPkt reqCp(CtlPkt::BOOT_ABORT,CtlPkt::REQUEST,0);
	return sendRequest(reqCp,dest,repCp);
}

/** Send a control packet request back through the main thread.
 *  The control packet object is assumed to be already initialized.
 *  It is packed into a packet object whose index is then placed in the outq.
 *  It then waits for a reply. If the reply is not received after one second,
 *  it tries again. After three failed attempts, it gives up.
 *  @param cp is the pre-formatted control packet
 *  @param dest is the destination address to which it is to be sent
 *  @param repCp is a reference to a control packet in which the response
 *  control packet is returned.
 *  @return the packet index of the reply, when there is one, and 0
 *  on an error, or if there is no reply.
 */
pktx CpHandler::sendRequest(CtlPkt& cp, fAdr_t dest, CtlPkt& repCp) {
	pktx px = ps->alloc();
	if (px == 0) {
		logger->log("CpHandler::sendRequest: no packets "
		            "left in packet store\n",4,cp);
		// terminates
	}
	Packet& p = ps->getPacket(px);
	// set default seq# for requests - tells main thread to use "next" seq#
	if (cp.mode == CtlPkt::REQUEST) cp.seqNum = 0;
	cp.payload = p.payload();
	int plen = cp.pack();
	if (plen == 0) {
		logger->log("CpHandler::sendRequest: packing error\n",4,cp);
		// terminates
	}
	p.length = plen + Forest::OVERHEAD;
	if (cp.type < CtlPkt::CLIENT_NET_SIG_SEP) {
		p.type = Forest::CLIENT_SIG; p.comtree =Forest::CLIENT_SIG_COMT;
	} else {
		p.type = Forest::NET_SIG; p.comtree = Forest::NET_SIG_COMT;
	}
	p.flags = 0; p.dstAdr = dest; p.srcAdr = myAdr;
	p.tunIp = tunIp; p.tunPort = tunPort;
	p.pack();

	if (cp.mode != CtlPkt::REQUEST) {
		outq->enq(px); return 0;
	}
	pktx reply = sendAndWait(px,cp);
	if (reply != 0) repCp.reset(ps->getPacket(reply));
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
			if (repCp.mode == CtlPkt::NEG_REPLY) {
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



/** Send a control packet reply back through the main thread.
 *  The control packet object is assumed to be already initialized.
 *  @param cp is the pre-formatted control packet
 *  @param dest is the destination address to which it is to be sent
 */
void CpHandler::sendReply(CtlPkt& cp, fAdr_t dest) {
	pktx px = ps->alloc();
	if (px == 0) {
		logger->log("CpHandler::sendRequest: no packets "
		            "left in packet store\n",4,cp);
		// terminates
	}
	Packet& p = ps->getPacket(px);
	cp.payload = p.payload();
	int plen = cp.pack();
	if (plen == 0) {
		logger->log("CpHandler::sendRequest: packing error\n",4,cp);
		// terminates
	}
	p.length = plen + Forest::OVERHEAD;
	if (cp.type < CtlPkt::CLIENT_NET_SIG_SEP) {
		p.type = Forest::CLIENT_SIG; p.comtree =Forest::CLIENT_SIG_COMT;
	} else {
		p.type = Forest::NET_SIG; p.comtree = Forest::NET_SIG_COMT;
	}
	p.flags = 0; p.dstAdr = dest; p.srcAdr = myAdr;
	p.tunIp = tunIp; p.tunPort = tunPort;
	p.pack();
	outq->enq(px);
}

/** Build and send error reply packet for ps.
 *  @param px is the packet index of the request packet we are replying to
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param msg is the error message to be sent.
 */
void CpHandler::errReply(pktx px, CtlPkt& cp, const string& msg) {
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

