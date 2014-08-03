/** @file CtlPkt.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

/** The CtlPkt class is used to pack and unpack forest control messages.
 *  The class has a public field for every field that can be used in
 *  in a control packet. To create a control packet, the user
 *  constructs a CtlPkt object. The user then specifies
 *  selected fields and calls pack, which packs the specified
 *  fields into the packet's payload and returns the length of
 *  the payload.
 *  
 *  To unpack a buffer, the user constructs a CtlPkt object
 *  and then calls unpack, specifying the length of the payload.
 *  The control packet fields can then be retrieved from the
 *  corresponding fields of the CtlPkt object.
 */

#include "CtlPkt.h"

namespace forest {

/** Constructor for CtlPkt with no initialization.
 */
CtlPkt::CtlPkt() {
	payload = 0; paylen = 0;
}

/** Construct a new control packet from the payload of a packet.
 *  @param p is a reference to a packet that contains a control packet
 */
CtlPkt::CtlPkt(const Packet& p) { reset(p); }

/** Destructor for CtlPkt. */
CtlPkt::~CtlPkt() { } 

/** Reset control packet, from a given packet's payload.
 *  The payload pointer and paylen fields are initialized and
 *  the fixed fields (type, mode, seqNum) are extracted from the packet.
 *  Also, the internal pointer used to track the position of the first
 *  unread or unwritten payload byte is initialzed.
 *  @param p is a reference to a packet.
 */
void CtlPkt::reset(const Packet& p) {
	payload = (char*) p.payload();
	paylen = p.length-Forest::OVERHEAD;
	xtrBase();
} 

/** Format an error reply packet.
 *  The type field is assumed to be already set.
 *  @param msg is an error message to be included in the body of the packet
 *  @param snum is the sequence number to be included in the packet
 */
void CtlPkt::fmtError(const string& msg, int64_t snum) {
	mode = NEG_REPLY; if (snum != 0) seqNum = snum;
	fmtBase();
	put(msg); paylen = next - payload;
}

/** Extract an error reply packet.
 *  The type field is assumed to be already set.
 *  @param msg is an error message to be included in the body of the packet
 *  @param true if the control packet passes basic checks
 */
bool CtlPkt::xtrError(string& msg) {
	return	mode == NEG_REPLY && get(msg) && paylen == (next - payload);
}

/** Format a generic control packet reply (no information fields).
 *  Retains the type and sequence number field in the control packet.
 */
void CtlPkt::fmtReply() {
	mode = POS_REPLY; fmtBase();
}

/** Format a ADD_IFACE control packet (request).
 *  @param iface is the number of the interface
 *  @param ip is the IP address of the interface
 *  @param rates is the rate spec for the interface
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtAddIface(int iface, ipa_t ip, RateSpec rates, int64_t snum) {
	type = ADD_IFACE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(iface); put(ip); put(rates); 
	paylen = next - payload;
}

/** Extract a ADD_IFACE control packet (request).
 *  @param iface is the number of the interface
 *  @param ip is the IP address of the interface
 *  @param rates is the rate spec for the interface
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddIface(int& iface, ipa_t& ip, RateSpec& rates) {
	return	type == ADD_IFACE && mode == REQUEST
		&& get(iface) && get(ip) && get(rates) 
		&& paylen >= (next - payload);
}

/** Format a ADD_IFACE control packet reply.
 *  @param ip is the IP address of the interface
 *  @param port is the port number of the interface
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtAddIfaceReply(ipa_t ip, ipp_t port, int64_t snum) {
	type = ADD_IFACE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(ip); put(port); 
	paylen = next - payload;
}

/** Extract a ADD_IFACE control packet reply.
 *  @param ip is the IP address of the interface
 *  @param port is the port number of the interface
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddIfaceReply(ipa_t& ip, ipp_t& port) {
	return	type == ADD_IFACE && mode == REQUEST
		&& get(ip) && get(port) 
		&& paylen >= (next - payload);
}

/** Format a DROP_IFACE control packet (request).
 *  @param iface is the number of the interface to be dropped
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtDropIface(int iface, int64_t snum) {
	type = DROP_IFACE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(iface); 
	paylen = next - payload;
}

/** Extract a DROP_IFACE control packet (request).
 *  @param iface is the number of the interface to be dropped
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropIface(int& iface) {
	return	type == DROP_IFACE && mode == REQUEST
		&& get(iface) 
		&& paylen >= (next - payload);
}

/** Format a DROP_IFACE control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtDropIfaceReply(int64_t snum) {
	type = DROP_IFACE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a DROP_IFACE control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropIfaceReply() {
	return	type == DROP_IFACE && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a MOD_IFACE control packet (request).
 *  @param iface is the number of the interface to be modified
 *  @param rates is the new interface rate spec
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtModIface(int iface, RateSpec rates, int64_t snum) {
	type = MOD_IFACE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(iface); put(rates); 
	paylen = next - payload;
}

/** Extract a MOD_IFACE control packet (request).
 *  @param iface is the number of the interface to be modified
 *  @param rates is the new interface rate spec
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModIface(int& iface, RateSpec& rates) {
	return	type == MOD_IFACE && mode == REQUEST
		&& get(iface) && get(rates) 
		&& paylen >= (next - payload);
}

/** Format a MOD_IFACE control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtModIfaceReply(int64_t snum) {
	type = MOD_IFACE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a MOD_IFACE control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModIfaceReply() {
	return	type == MOD_IFACE && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a GET_IFACE control packet (request).
 *  @param iface is the number of the interface to be retrieved
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetIface(int iface, int64_t snum) {
	type = GET_IFACE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(iface); 
	paylen = next - payload;
}

/** Extract a GET_IFACE control packet (request).
 *  @param iface is the number of the interface to be retrieved
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetIface(int& iface) {
	return	type == GET_IFACE && mode == REQUEST
		&& get(iface) 
		&& paylen >= (next - payload);
}

/** Format a GET_IFACE control packet reply.
 *  @param iface is the number of the interface
 *  @param ip is the IP address of the interface
 *  @param port is the port number of the interface
 *  @param rates is the interface rate spec
 *  @param availRates is the rate spec for the available rates at the interface
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetIfaceReply(int iface, ipa_t ip, ipp_t port, RateSpec rates, RateSpec availRates, int64_t snum) {
	type = GET_IFACE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(iface); put(ip); put(port); put(rates); put(availRates); 
	paylen = next - payload;
}

/** Extract a GET_IFACE control packet reply.
 *  @param iface is the number of the interface
 *  @param ip is the IP address of the interface
 *  @param port is the port number of the interface
 *  @param rates is the interface rate spec
 *  @param availRates is the rate spec for the available rates at the interface
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetIfaceReply(int& iface, ipa_t& ip, ipp_t& port, RateSpec& rates, RateSpec& availRates) {
	return	type == GET_IFACE && mode == REQUEST
		&& get(iface) && get(ip) && get(port) && get(rates) && get(availRates) 
		&& paylen >= (next - payload);
}

/** Format a GET_IFACE_SET control packet (request).
 *  @param iface is the number of the first interface to be retrieved (if zero, start with first defined interface)
 *  @param count is the number of entries to retrieve (at most 10)
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetIfaceSet(int iface, int count, int64_t snum) {
	type = GET_IFACE_SET; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(iface); put(count); 
	paylen = next - payload;
}

/** Extract a GET_IFACE_SET control packet (request).
 *  @param iface is the number of the first interface to be retrieved (if zero, start with first defined interface)
 *  @param count is the number of entries to retrieve (at most 10)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetIfaceSet(int& iface, int& count) {
	return	type == GET_IFACE_SET && mode == REQUEST
		&& get(iface) && get(count) 
		&& paylen >= (next - payload);
}

/** Format a GET_IFACE_SET control packet reply.
 *  @param count is the number of entries actually retirieved
 *  @param nexti is the number of the next interface following the
 *  last one retrieved (or 0 if no more)
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetIfaceSetReply(int count, int nexti, string s, int64_t snum) {
	type = GET_IFACE_SET; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(count); put(nexti); put(s);
	paylen = next - payload;
}

/** Extract a GET_IFACE_SET control packet reply.
 *  @param count is the number of entries actually retirieved
 *  @param next is the number of the next interface following the last
 *  one retrieved (or 0 if no more)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetIfaceSetReply(int& count, int& nexti, string& s) {
	return	type == GET_IFACE_SET && mode == REQUEST
		&& get(count) && get(nexti) && get(s)
		&& paylen >= (next - payload);
}

/** Format a ADD_LINK control packet (request).
 *  @param nodeType is the type of the peer node
 *  @param iface is the interface used by the link
 *  @param lnk is the number of the link to be added
 *  (if zero, router selects link number)
 *  @param peerIp is the IP address of the peer node (may be zero)
 *  @param peerPort is the port number used by the peer node (may be zero)
 *  @param peerAdr is the Forest address of the peer node (may be zero)
 *  @param nonce is the nonce used by the peer when connecting
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtAddLink(Forest::ntyp_t nodeType, int iface, int lnk, ipa_t peerIp, ipp_t peerPort, fAdr_t peerAdr, uint64_t nonce, int64_t snum) {
	type = ADD_LINK; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(nodeType); put(iface); put(lnk); put(peerIp); put(peerPort);
	put(peerAdr); put(nonce); 
	paylen = next - payload;
}

/** Extract a ADD_LINK control packet (request).
 *  @param nodeType is the type of the peer node
 *  @param iface is the interface used by the link
 *  @param lnk is the number of the link to be added
 *  (if zero, router selects link number)
 *  @param peerIp is the IP address of the peer node (may be zero)
 *  @param peerPort is the port number used by the peer node (may be zero)
 *  @param peerAdr is the Forest address of the peer node (may be zero)
 *  @param nonce is the nonce used by the peer when connecting
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddLink(Forest::ntyp_t& nodeType, int& iface, int& lnk,
	ipa_t& peerIp, ipp_t& peerPort, fAdr_t& peerAdr, uint64_t& nonce) {
	return	type == ADD_LINK && mode == REQUEST
		&& get(nodeType) && get(iface) && get(lnk) && get(peerIp)
		&& get(peerPort) && get(peerAdr) && get(nonce) 
		&& paylen >= (next - payload);
}

/** Format a ADD_LINK control packet reply.
 *  @param lnk is the link number (possibly assigned by the router)
 *  @param peerAdr is the peer's Forest address
 *  (possibly assigned by the router)
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtAddLinkReply(int lnk, fAdr_t peerAdr, int64_t snum) {
	type = ADD_LINK; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(lnk); put(peerAdr); 
	paylen = next - payload;
}

/** Extract a ADD_LINK control packet reply.
 *  @param lnk is the link number (possibly assigned by the router)
 *  @param peerAdr is the peer's Forest address
 *  (possibly assigned by the router)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddLinkReply(int& lnk, fAdr_t& peerAdr) {
	return	type == ADD_LINK && mode == REQUEST
		&& get(lnk) && get(peerAdr) 
		&& paylen >= (next - payload);
}

/** Format a DROP_LINK control packet (request).
 *  @param lnk is the number of the link to be dropped (may be zero)
 *  @param peerAdr is the Forest address of the peer node,
 *  and is used to identify the link if lnk==0 (may be zero)
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtDropLink(int lnk, fAdr_t peerAdr, int64_t snum) {
	type = DROP_LINK; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(lnk); put(peerAdr); 
	paylen = next - payload;
}

/** Extract a DROP_LINK control packet (request).
 *  @param lnk is the number of the link to be dropped (may be zero)
 *  @param peerAdr is the Forest address of the peer node,
 *  and is used to identify the link if lnk==0 (may be zero)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropLink(int& lnk, fAdr_t& peerAdr) {
	return	type == DROP_LINK && mode == REQUEST
		&& get(lnk) && get(peerAdr) 
		&& paylen >= (next - payload);
}

/** Format a DROP_LINK control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtDropLinkReply(int64_t snum) {
	type = DROP_LINK; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	paylen = next - payload;
}

/** Extract a DROP_LINK control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropLinkReply() {
	return	type == DROP_LINK && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a MOD_LINK control packet (request).
 *  @param lnk is the number of the link to be modified
 *  @param rates is the new link rate spec
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtModLink(int lnk, RateSpec rates, int64_t snum) {
	type = MOD_LINK; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(lnk); put(rates); 
	paylen = next - payload;
}

/** Extract a MOD_LINK control packet (request).
 *  @param lnk is the number of the link to be modified
 *  @param rates is the new link rate spec
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModLink(int& lnk, RateSpec& rates) {
	return	type == MOD_LINK && mode == REQUEST
		&& get(lnk) && get(rates) 
		&& paylen >= (next - payload);
}

/** Format a MOD_LINK control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtModLinkReply(int64_t snum) {
	type = MOD_LINK; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a MOD_LINK control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModLinkReply() {
	return	type == MOD_LINK && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a GET_LINK control packet (request).
 *  @param lnk is the number of the link to be retrieved
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetLink(int lnk, int64_t snum) {
	type = GET_LINK; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(lnk); 
	paylen = next - payload;
}

/** Extract a GET_LINK control packet (request).
 *  @param lnk is the number of the link to be retrieved
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetLink(int& lnk) {
	return	type == GET_LINK && mode == REQUEST
		&& get(lnk) 
		&& paylen >= (next - payload);
}

/** Format a GET_LINK control packet reply.
 *  @param lnk is the link number for the retrieved entry
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetLinkReply(int lnk, int iface, Forest::ntyp_t ntyp,
		ipa_t peerIp, ipp_t peerPort, fAdr_t peerAdr,
		RateSpec& rates, RateSpec& availRates,int64_t snum) {
	type = GET_LINK; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(lnk);  put(iface); put(ntyp); put(peerIp); put(peerPort); 
	put(peerAdr); put(rates); put(availRates);
	paylen = next - payload;
}

/** Extract a GET_LINK control packet reply.
 *  @param lnk is the link number for the retrieved entry
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetLinkReply(int& lnk, int& iface, Forest::ntyp_t& ntyp,
		ipa_t& peerIp, ipp_t& peerPort, fAdr_t& peerAdr,
		RateSpec& rates, RateSpec& availRates) {
	return	type == GET_LINK && mode == REQUEST
		&& get(lnk) && get(iface) && get(ntyp) && get(peerIp)
		&& get(peerPort) && get(peerAdr)
		&& get(rates) && get(availRates)
		&& paylen >= (next - payload);
}

/** Format a GET_LINK_SET control packet (request).
 *  @param lnk is the number of the first link to be retrieved (if zero, start with first defined link)
 *  @param count is the number of entries to retrieve (at most 10)
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetLinkSet(int lnk, int count, int64_t snum) {
	type = GET_LINK_SET; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(lnk); put(count); 
	paylen = next - payload;
}

/** Extract a GET_LINK_SET control packet (request).
 *  @param lnk is the number of the first link to be retrieved (if zero, start with first defined link)
 *  @param count is the number of entries to retrieve (at most 10)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetLinkSet(int& lnk, int& count) {
	return	type == GET_LINK_SET && mode == REQUEST
		&& get(lnk) && get(count) 
		&& paylen >= (next - payload);
}

/** Format a GET_LINK_SET control packet reply.
 *  @param count is the number of entries actually retirieved
 *  @param next is the number of the next link following the last one retrieved (or 0 if no more)
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetLinkSetReply(int count, int nexti, string s, int64_t snum) {
	type = GET_LINK_SET; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(count); put(nexti); put(s);
	paylen = next - payload;
}

/** Extract a GET_LINK_SET control packet reply.
 *  @param count is the number of entries actually retirieved
 *  @param next is the number of the next link following the last one retrieved (or 0 if no more)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetLinkSetReply(int& count, int& nexti, string& s) {
	return	type == GET_LINK_SET && mode == REQUEST
		&& get(count) && get(nexti) && get(s)
		&& paylen >= (next - payload);
}

/** Format a ADD_COMTREE control packet (request).
 *  @param comt is the number of the comtree to be added
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtAddComtree(comt_t comt, int64_t snum) {
	type = ADD_COMTREE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); 
	paylen = next - payload;
}

/** Extract a ADD_COMTREE control packet (request).
 *  @param comt is the number of the comtree to be added
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddComtree(comt_t& comt) {
	return	type == ADD_COMTREE && mode == REQUEST
		&& get(comt) 
		&& paylen >= (next - payload);
}

/** Format a ADD_COMTREE control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtAddComtreeReply(int64_t snum) {
	type = ADD_COMTREE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a ADD_COMTREE control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddComtreeReply() {
	return	type == ADD_COMTREE && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a DROP_COMTREE control packet (request).
 *  @param comt is the number of the comtree to be dropped
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtDropComtree(comt_t comt, int64_t snum) {
	type = DROP_COMTREE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); 
	paylen = next - payload;
}

/** Extract a DROP_COMTREE control packet (request).
 *  @param comt is the number of the comtree to be dropped
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropComtree(comt_t& comt) {
	return	type == DROP_COMTREE && mode == REQUEST
		&& get(comt) 
		&& paylen >= (next - payload);
}

/** Format a DROP_COMTREE control packet reply.
 *  @param plnkAvailRates specifies the rates now available on the link that connected the target router to its parent in the comtree
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtDropComtreeReply(RateSpec plnkAvailRates, int64_t snum) {
	type = DROP_COMTREE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(plnkAvailRates); 
	paylen = next - payload;
}

/** Extract a DROP_COMTREE control packet reply.
 *  @param plnkAvailRates specifies the rates now available on the link that connected the target router to its parent in the comtree
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropComtreeReply(RateSpec& plnkAvailRates) {
	return	type == DROP_COMTREE && mode == REQUEST
		&& get(plnkAvailRates) 
		&& paylen >= (next - payload);
}

/** Format a MOD_COMTREE control packet (request).
 *  @param comt is the number of the comtree to be modified
 *  @param coreFlag is 1 if the target router is a core node in the comtree, else 0,
 *  @param plnk is the link number for the link connecting to the comtree parent
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtModComtree(comt_t comt, int coreFlag, int plnk, int64_t snum) {
	type = MOD_COMTREE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(coreFlag); put(plnk); 
	paylen = next - payload;
}

/** Extract a MOD_COMTREE control packet (request).
 *  @param comt is the number of the comtree to be modified
 *  @param coreFlag is 1 if the target router is a core node in the comtree,
 *  else 0
 *  @param plnk is the link number for the link connecting to the comtree parent
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModComtree(comt_t& comt, int& coreFlag, int& plnk) {
	return	type == MOD_COMTREE && mode == REQUEST
		&& get(comt) && get(coreFlag) && get(plnk) 
		&& paylen >= (next - payload);
}

/** Format a MOD_COMTREE control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtModComtreeReply(int64_t snum) {
	type = MOD_COMTREE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a MOD_COMTREE control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModComtreeReply() {
	return	type == MOD_COMTREE && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a GET_COMTREE control packet (request).
 *  @param comt is the number of the comtree to be retrieved
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetComtree(comt_t comt, int64_t snum) {
	type = GET_COMTREE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); 
	paylen = next - payload;
}

/** Extract a GET_COMTREE control packet (request).
 *  @param comt is the number of the comtree to be retrieved
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetComtree(comt_t& comt) {
	return	type == GET_COMTREE && mode == REQUEST
		&& get(comt) 
		&& paylen >= (next - payload);
}

/** Format a GET_COMTREE control packet reply.
 *  @param comt is the comtree number for the retrieved entry
 *  @param coreFlag is 1 if the router is a core node in the comtree, else 0
 *  @param plnk is the number of the link connecting to the comtree parent
 *  @param lnkCount is the number of comtree links at the router
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetComtreeReply(comt_t comt, int coreFlag, int plnk, int lnkCount, int64_t snum) {
	type = GET_COMTREE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(comt); put(coreFlag); put(plnk); put(lnkCount); 
	paylen = next - payload;
}

/** Extract a GET_COMTREE control packet reply.
 *  @param comt is the comtree number for the retrieved entry
 *  @param coreFlag is 1 if the router is a core node in the comtree, else 0
 *  @param plnk is the number of the link connecting to the comtree parent
 *  @param lnkCount is the number of comtree links at the router
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetComtreeReply(comt_t& comt, int& coreFlag, int& plnk, int& lnkCount) {
	return	type == GET_COMTREE && mode == REQUEST
		&& get(comt) && get(coreFlag) && get(plnk) && get(lnkCount) 
		&& paylen >= (next - payload);
}

/** Format a GET_COMTREE_SET control packet (request).
 *  @param comt is the number of the first comtree to be retrieved (if zero, start with first defined comtree)
 *  @param count is the number of entries to retrieve (at most 10)
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetComtreeSet(comt_t comt, int count, int64_t snum) {
	type = GET_COMTREE_SET; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(count); 
	paylen = next - payload;
}

/** Extract a GET_COMTREE_SET control packet (request).
 *  @param comt is the number of the first comtree to be retrieved (if zero, start with first defined comtree)
 *  @param count is the number of entries to retrieve (at most 10)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetComtreeSet(comt_t& comt, int& count) {
	return	type == GET_COMTREE_SET && mode == REQUEST
		&& get(comt) && get(count) 
		&& paylen >= (next - payload);
}

/** Format a GET_COMTREE_SET control packet reply.
 *  @param count is the number of entries actually retirieved
 *  @param next is the number of the next comtree following the last
 *  one retrieved (or 0 if no more)
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetComtreeSetReply(int count, comt_t nextc, string s,
		int64_t snum) {
	type = GET_COMTREE_SET; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(count); put(nextc); put(s);
	paylen = next - payload;
}

/** Extract a GET_COMTREE_SET control packet reply.
 *  @param count is the number of entries actually retirieved
 *  @param next is the number of the next comtree following the last
 *  one retrieved (or 0 if no more)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetComtreeSetReply(int& count, comt_t& nextc, string& s) {
	return	type == GET_COMTREE_SET && mode == REQUEST
		&& get(count) && get(nextc) && get(s)
		&& paylen >= (next - payload);
}

/** Format a ADD_COMTREE_LINK control packet (request).
 *  @param comt is the number of the comtree
 *  @param lnk is the number of the link to be added to the comtree
 *  (may be zero)
 *  @param coreFlag is true if the peer node is a core node in this comtree
 *  @param peerIp is the IP address used by the peer node (may be zero)
 *  @param peerPort is the port number used by the peer node (may be zero)
 *  @param peerAdr is the Forest address of the peer node
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtAddComtreeLink(comt_t comt, int lnk, int coreFlag,
		ipa_t peerIp, ipp_t peerPort, fAdr_t peerAdr, int64_t snum) {
	type = ADD_COMTREE_LINK; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(lnk); put(coreFlag); put(peerIp); put(peerPort);
	put(peerAdr); 
	paylen = next - payload;
}

/** Extract a ADD_COMTREE_LINK control packet (request).
 *  @param comt is the number of the comtree
 *  @param lnk is the number of the link to be added to the comtree
 *  (may be zero)
 *  @param coreFlag is true if the peer node is a core node in this comtree
 *  @param peerIp is the IP address used by the peer node (may be zero)
 *  @param peerPort is the port number used by the peer node (may be zero)
 *  @param peerAdr is the Forest address of the peer node
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddComtreeLink(comt_t& comt, int& lnk, int& coreFlag,
		ipa_t& peerIp, ipp_t& peerPort, fAdr_t& peerAdr) {
	return	type == ADD_COMTREE_LINK && mode == REQUEST
		&& get(comt) && get(lnk) && get(coreFlag)
		&& get(peerIp) && get(peerPort) && get(peerAdr) 
		&& paylen >= (next - payload);
}

/** Format a ADD_COMTREE_LINK control packet reply.
 *  @param lnk is the number of the link added
 *  @param availRates is the available rate on the link,
 *  after the comtree link has been added
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtAddComtreeLinkReply(int lnk, RateSpec availRates, int64_t snum){
	type = ADD_COMTREE_LINK; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(lnk); put(availRates); 
	paylen = next - payload;
}

/** Extract a ADD_COMTREE_LINK control packet reply.
 *  @param lnk is the number of the link added
 *  @param availRates is the available rate on the link,
 *  after the comtree link has been added
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddComtreeLinkReply(int& lnk, RateSpec& availRates) {
	return	type == ADD_COMTREE_LINK && mode == REQUEST
		&& get(lnk) && get(availRates) 
		&& paylen >= (next - payload);
}

/** Format a DROP_COMTREE_LINK control packet (request).
 *  @param comt is the number of the comtree
 *  @param lnk is the number of the link to be dropped (may be zero)
 *  @param peerIp is the IP address used by the peer node (may be zero)
 *  @param peerPort is the port number used by the peer node (may be zero)
 *  @param peerAdr is the Forest address of the peer node
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtDropComtreeLink(comt_t comt, int lnk, ipa_t peerIp,
		ipp_t peerPort, fAdr_t peerAdr, int64_t snum) {
	type = DROP_COMTREE_LINK; mode = REQUEST; seqNum = snum;
	fmtBase();
}

/** Extract a DROP_COMTREE_LINK control packet (request).
 *  @param comt is the number of the comtree
 *  @param lnk is the number of the link to be dropped (may be zero)
 *  @param peerIp is the IP address used by the peer node (may be zero)
 *  @param peerPort is the port number used by the peer node (may be zero)
 *  @param peerAdr is the Forest address of the peer node
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropComtreeLink(comt_t& comt, int& lnk, ipa_t& peerIp,
		ipp_t& peerPort, fAdr_t& peerAdr) {
	return	type == DROP_COMTREE_LINK && mode == REQUEST
		&& get(comt) && get(lnk) && get(peerIp)
		&& get(peerPort) && get(peerAdr) 
		&& paylen >= (next - payload);
}

/** Format a DROP_COMTREE_LINK control packet reply.
 *  @param availRates specifies the rates now available on the link
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtDropComtreeLinkReply(RateSpec availRates, int64_t snum) {
	type = DROP_COMTREE_LINK; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(availRates); 
	paylen = next - payload;
}

/** Extract a DROP_COMTREE_LINK control packet reply.
 *  @param availRates specifies the rates now available on the link
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropComtreeLinkReply(RateSpec& availRates) {
	return	type == DROP_COMTREE_LINK && mode == REQUEST
		&& get(availRates) 
		&& paylen >= (next - payload);
}

/** Format a MOD_COMTREE_LINK control packet (request).
 *  @param comt is the number of the comtree
 *  @param lnk is the number of the link to be modified
 *  @param rates is the new rates for the comtree link
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtModComtreeLink(comt_t comt, int lnk, RateSpec rates,int64_t snum) {
	type = MOD_COMTREE_LINK; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(lnk); put(rates); 
	paylen = next - payload;
}

/** Extract a MOD_COMTREE_LINK control packet (request).
 *  @param comt is the number of the comtree
 *  @param lnk is the number of the link to be modified
 *  @param rates is the new rates for the comtree link
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModComtreeLink(comt_t& comt, int& lnk, RateSpec& rates) {
	return	type == MOD_COMTREE_LINK && mode == REQUEST
		&& get(comt) && get(lnk) && get(rates) 
		&& paylen >= (next - payload);
}

/** Format a MOD_COMTREE_LINK control packet reply.
 *  @param availRates specifies the rates now a vailable on the link
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtModComtreeLinkReply(RateSpec& availRates, int64_t snum) {
	type = MOD_COMTREE_LINK; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(availRates); 
	paylen = next - payload;
}

/** Extract a MOD_COMTREE_LINK control packet reply.
 *  @param availRates specifies the rates now available on the link
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModComtreeLinkReply(RateSpec& availRates) {
	return	type == MOD_COMTREE_LINK && mode == REQUEST
		&& get(availRates) 
		&& paylen >= (next - payload);
}

/** Format a GET_COMTREE_LINK control packet (request).
 *  @param comt is the number of the comtree whose link is to be retrieved
 *  @param lnk is the number of the comtree link to be retrieved
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetComtreeLink(comt_t comt, int lnk, int64_t snum) {
	type = GET_COMTREE_LINK; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(lnk); 
	paylen = next - payload;
}

/** Extract a GET_COMTREE_LINK control packet (request).
 *  @param comt is the number of the comtree whose link is to be retrieved
 *  @param lnk is the number of the comtree link to be retrieved
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetComtreeLink(comt_t& comt, int& lnk) {
	return	type == GET_COMTREE_LINK && mode == REQUEST
		&& get(comt) && get(lnk) 
		&& paylen >= (next - payload);
}

/** Format a GET_COMTREE_LINK control packet reply.
 *  @param comt is the comtree number for the retrieved entry
 *  @param lnk is the link number for the retrieved comtree link
 *  @param rates is the rates for the comtree link
 *  @param qid is the queue number used by the comtree on the specified link
 *  @param dest is the allowed Forest address for unicast packets
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetComtreeLinkReply(comt_t comt, int lnk, RateSpec rates,
		int qid, fAdr_t dest, int64_t snum) {
	type = GET_COMTREE_LINK; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(comt); put(lnk); put(rates); put(qid); put(dest); 
	paylen = next - payload;
}

/** Extract a GET_COMTREE_LINK control packet reply.
 *  @param comt is the comtree number for the retrieved entry
 *  @param lnk is the link number for the retrieved comtree link
 *  @param rates is the rates for the comtree link
 *  @param qid is the queue number used by the comtree on the specified link
 *  @param dest is the allowed Forest address for unicast packets
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetComtreeLinkReply(comt_t& comt, int& lnk, RateSpec& rates,
		int& qid, fAdr_t& dest) {
	return	type == GET_COMTREE_LINK && mode == REQUEST
		&& get(comt) && get(lnk) && get(rates)
		&& get(qid) && get(dest) 
		&& paylen >= (next - payload);
}

/** Format a ADD_ROUTE control packet (request).
 *  @param comt is the number of the comtree to which a route is to be added
 *  @param destAdr is the destination address for the route
 *  @param lnk is the number of an output link for the route (may be zero)
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtAddRoute(comt_t comt, fAdr_t destAdr, int lnk, int64_t snum) {
	type = ADD_ROUTE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(destAdr); put(lnk); 
	paylen = next - payload;
}

/** Extract a ADD_ROUTE control packet (request).
 *  @param comt is the number of the comtree to which a route is to be added
 *  @param destAdr is the destination address for the route
 *  @param lnk is the number of an output link for the route (may be zero)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddRoute(comt_t& comt, fAdr_t& destAdr, int& lnk) {
	return	type == ADD_ROUTE && mode == REQUEST
		&& get(comt) && get(destAdr) && get(lnk) 
		&& paylen >= (next - payload);
}

/** Format a ADD_ROUTE control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtAddRouteReply(int64_t snum) {
	type = ADD_ROUTE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a ADD_ROUTE control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddRouteReply() {
	return	type == ADD_ROUTE && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a DROP_ROUTE control packet (request).
 *  @param comt is the number of the comtree from which the route
 *  is to be dropped
 *  @param destAdr is the destination address for the route
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtDropRoute(comt_t comt, fAdr_t destAdr, int64_t snum) {
	type = DROP_ROUTE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(destAdr); 
	paylen = next - payload;
}

/** Extract a DROP_ROUTE control packet (request).
 *  @param comt is the number of the comtree from which the route
 *  is to be dropped
 *  @param destAdr is the destination address for the route
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropRoute(comt_t& comt, fAdr_t& destAdr) {
	return	type == DROP_ROUTE && mode == REQUEST
		&& get(comt) && get(destAdr) 
		&& paylen >= (next - payload);
}

/** Format a DROP_ROUTE control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtDropRouteReply(int64_t snum) {
	type = DROP_ROUTE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a DROP_ROUTE control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropRouteReply() {
	return	type == DROP_ROUTE && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a MOD_ROUTE control packet (request).
 *  @param comt is the number of the comtree for which a route
 *  is to be modified
 *  @param destAdr is the destination address for the route
 *  @param lnk is the number of an output link for the route
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtModRoute(comt_t comt, fAdr_t destAdr, int lnk, int64_t snum) {
	type = MOD_ROUTE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(destAdr); put(lnk); 
	paylen = next - payload;
}

/** Extract a MOD_ROUTE control packet (request).
 *  @param comt is the number of the comtree for which a route is to be modified
 *  @param destAdr is the destination address for the route
 *  @param lnk is the number of an output link for the route
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModRoute(comt_t& comt, fAdr_t& destAdr, int& lnk) {
	return	type == MOD_ROUTE && mode == REQUEST
		&& get(comt) && get(destAdr) && get(lnk) 
		&& paylen >= (next - payload);
}

/** Format a MOD_ROUTE control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtModRouteReply(int64_t snum) {
	type = MOD_ROUTE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a MOD_ROUTE control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModRouteReply() {
	return	type == MOD_ROUTE && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a GET_ROUTE control packet (request).
 *  @param comt is the number of the route to be retrieved
 *  @param destAdr is the destination addre
ss for the route
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetRoute(comt_t comt, fAdr_t destAdr, int64_t snum) {
	type = GET_ROUTE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(destAdr); 
	paylen = next - payload;
}

/** Extract a GET_ROUTE control packet (request).
 *  @param comt is the number of the route to be retrieved
 *  @param destAdr is the destination addre
ss for the route
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetRoute(comt_t& comt, fAdr_t& destAdr) {
	return	type == GET_ROUTE && mode == REQUEST
		&& get(comt) && get(destAdr) 
		&& paylen >= (next - payload);
}

/** Format a GET_ROUTE control packet reply.
 *  @param comt is the number of the comtree
 *  @param destAdr is the destination address for the route
 *  @param lnk is the number of an output link for the route
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetRouteReply(comt_t comt, fAdr_t destAdr, int lnk, int64_t snum) {
	type = GET_ROUTE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(comt); put(destAdr); put(lnk); 
	paylen = next - payload;
}

/** Extract a GET_ROUTE control packet reply.
 *  @param comt is the number of the comtree
 *  @param destAdr is the destination address for the route
 *  @param lnk is the number of an output link for the route
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetRouteReply(comt_t& comt, fAdr_t& destAdr, int& lnk) {
	return	type == GET_ROUTE && mode == REQUEST
		&& get(comt) && get(destAdr) && get(lnk) 
		&& paylen >= (next - payload);
}

/** Format a GET_ROUTE_SET control packet (request).
 *  @param rtx is the index of the first route to be retrieved
 *  (if zero, start with first defined route)
 *  @param count is the number of entries to retrieve (at most 10)
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetRouteSet(int rtx, int count, int64_t snum) {
	type = GET_ROUTE_SET; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(rtx); put(count); 
	paylen = next - payload;
}

/** Extract a GET_ROUTE_SET control packet (request).
 *  @param rtx is the index of the first route to be retrieved
 *  (if zero, start with first defined route)
 *  @param count is the number of entries to retrieve (at most 10)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetRouteSet(int& rtx, int& count) {
	return	type == GET_ROUTE_SET && mode == REQUEST
		&& get(rtx) && get(count) 
		&& paylen >= (next - payload);
}

/** Format a GET_ROUTE_SET control packet reply.
 *  @param count is the number of entries actually retirieved
 *  @param nextRtx is the index of the next route following the last
 *  one retrieved (or 0 if no more)
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetRouteSetReply(int count, int nextRtx, string s,
		int64_t snum) {
	type = GET_ROUTE_SET; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(count); put(nextRtx); put(s); 
	paylen = next - payload;
}

/** Extract a GET_ROUTE_SET control packet reply.
 *  @param count is the number of entries actually retirieved
 *  @param nextRtx is the number of the next route following the last
 *  one retrieved (or 0 if no more)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetRouteSetReply(int& count, int& nextRtx, string& s) {
	return	type == GET_ROUTE_SET && mode == REQUEST
		&& get(count) && get(nextRtx) &&  get(s)
		&& paylen >= (next - payload);
}

/** Format a ADD_ROUTE_LINK control packet (request).
 *  @param comt is the number of the comtree for the route
 *  @param destAdr is the destination address for the route
 *  @param lnk is the number of a link to be added to the route
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtAddRouteLink(comt_t comt, fAdr_t destAdr, int lnk, int64_t snum) {
	type = ADD_ROUTE_LINK; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(destAdr); put(lnk); 
	paylen = next - payload;
}

/** Extract a ADD_ROUTE_LINK control packet (request).
 *  @param comt is the number of the comtree for the route
 *  @param destAdr is the destination address for the route
 *  @param lnk is the number of a link to be added to the route
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddRouteLink(comt_t& comt, fAdr_t& destAdr, int& lnk) {
	return	type == ADD_ROUTE_LINK && mode == REQUEST
		&& get(comt) && get(destAdr) && get(lnk) 
		&& paylen >= (next - payload);
}

/** Format a ADD_ROUTE_LINK control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtAddRouteLinkReply(int64_t snum) {
	type = ADD_ROUTE_LINK; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a ADD_ROUTE_LINK control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddRouteLinkReply() {
	return	type == ADD_ROUTE_LINK && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a DROP_ROUTE_LINK control packet (request).
 *  @param comt is the number of the comtree for the route
 *  @param destAdr is the destination address for the route
 *  @param lnk is the number of a link to be dropped from the route
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtDropRouteLink(comt_t comt, fAdr_t destAdr, int lnk, int64_t snum) {
	type = DROP_ROUTE_LINK; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(comt); put(destAdr); put(lnk); 
	paylen = next - payload;
}

/** Extract a DROP_ROUTE_LINK control packet (request).
 *  @param comt is the number of the comtree for the route
 *  @param destAdr is the destination address for the route
 *  @param lnk is the number of a link to be dropped from the route
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropRouteLink(comt_t& comt, fAdr_t& destAdr, int& lnk) {
	return	type == DROP_ROUTE_LINK && mode == REQUEST
		&& get(comt) && get(destAdr) && get(lnk) 
		&& paylen >= (next - payload);
}

/** Format a DROP_ROUTE_LINK control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtDropRouteLinkReply(int64_t snum) {
	type = DROP_ROUTE_LINK; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a DROP_ROUTE_LINK control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropRouteLinkReply() {
	return	type == DROP_ROUTE_LINK && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a ADD_FILTER control packet (request).
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtAddFilter(int64_t snum) {
	type = ADD_FILTER; mode = REQUEST; seqNum = snum;
	fmtBase();
}

/** Extract a ADD_FILTER control packet (request).
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddFilter() {
	return	type == ADD_FILTER && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a ADD_FILTER control packet reply.
 *  @param filterNum is the index of the added filter
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtAddFilterReply(int filterNum, int64_t snum) {
	type = ADD_FILTER; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(filterNum); 
	paylen = next - payload;
}

/** Extract a ADD_FILTER control packet reply.
 *  @param filterNum is the index of the added filter
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrAddFilterReply(int& filterNum) {
	return	type == ADD_FILTER && mode == REQUEST
		&& get(filterNum) 
		&& paylen >= (next - payload);
}

/** Format a DROP_FILTER control packet (request).
 *  @param filterNum is the index of the filter to be dropped
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtDropFilter(int filterNum, int64_t snum) {
	type = DROP_FILTER; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(filterNum); 
	paylen = next - payload;
}

/** Extract a DROP_FILTER control packet (request).
 *  @param filterNum is the index of the filter to be dropped
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropFilter(int& filterNum) {
	return	type == DROP_FILTER && mode == REQUEST
		&& get(filterNum) 
		&& paylen >= (next - payload);
}

/** Format a DROP_FILTER control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtDropFilterReply(int64_t snum) {
	type = DROP_FILTER; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a DROP_FILTER control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrDropFilterReply() {
	return	type == DROP_FILTER && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a MOD_FILTER control packet (request).
 *  @param filterNum is the index of the filter to be modified
 *  @param filterString is the string describing the filter
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtModFilter(int filterNum, string filterString, int64_t snum) {
	type = MOD_FILTER; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(filterNum); put(filterString); 
	paylen = next - payload;
}

/** Extract a MOD_FILTER control packet (request).
 *  @param filterNum is the index of the filter to be modified
 *  @param filterString is the string describing the filter
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModFilter(int& filterNum, string& filterString) {
	return	type == MOD_FILTER && mode == REQUEST
		&& get(filterNum) && get(filterString) 
		&& paylen >= (next - payload);
}

/** Format a MOD_FILTER control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtModFilterReply(int64_t snum) {
	type = MOD_FILTER; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a MOD_FILTER control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrModFilterReply() {
	return	type == MOD_FILTER && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a GET_FILTER control packet (request).
 *  @param filterNum is the index of the filter to be modified
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetFilter(int filterNum, int64_t snum) {
	type = GET_FILTER; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(filterNum); 
	paylen = next - payload;
}

/** Extract a GET_FILTER control packet (request).
 *  @param filterNum is the index of the filter to be modified
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetFilter(int& filterNum) {
	return	type == GET_FILTER && mode == REQUEST
		&& get(filterNum) 
		&& paylen >= (next - payload);
}

/** Format a GET_FILTER control packet reply.
 *  @param filterString is the string describing the filter
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetFilterReply(string filterString, int64_t snum) {
	type = GET_FILTER; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(filterString); 
	paylen = next - payload;
}

/** Extract a GET_FILTER control packet reply.
 *  @param filterString is the string describing the filter
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetFilterReply(string& filterString) {
	return	type == GET_FILTER && mode == REQUEST
		&& get(filterString) 
		&& paylen >= (next - payload);
}

/** Format a GET_FILTER_SET control packet (request).
 *  @param filterNum is the number of the first filter to be retrieved (if zero, start with first defined filter)
 *  @param count is the number of entries to retrieve (at most 10)
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetFilterSet(int filterNum, int count, int64_t snum) {
	type = GET_FILTER_SET; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(filterNum); put(count); 
	paylen = next - payload;
}

/** Extract a GET_FILTER_SET control packet (request).
 *  @param filterNum is the number of the first filter to be retrieved (if zero, start with first defined filter)
 *  @param count is the number of entries to retrieve (at most 10)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetFilterSet(int& filterNum, int& count) {
	return	type == GET_FILTER_SET && mode == REQUEST
		&& get(filterNum) && get(count) 
		&& paylen >= (next - payload);
}

/** Format a GET_FILTER_SET control packet reply.
 *  @param count is the number of entries actually retirieved
 *  @param next is the number of the next filter following the last one
 *  retrieved (or 0 if no more)
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetFilterSetReply(int count, int nexti, string s,int64_t snum) {
	type = GET_FILTER_SET; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(count); put(nexti); put(s);
	paylen = next - payload;
}

/** Extract a GET_FILTER_SET control packet reply.
 *  @param count is the number of entries actually retirieved
 *  @param next is the number of the next filter following the last one retrieved (or 0 if no more)
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetFilterSetReply(int& count, int& nexti, string& s) {
	return	type == GET_FILTER_SET && mode == REQUEST
		&& get(count) && get(nexti) && get(s)
		&& paylen >= (next - payload);
}

/** Format a GET_LOGGED_PACKETS control packet (request).
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtGetLoggedPackets(int64_t snum) {
	type = GET_LOGGED_PACKETS; mode = REQUEST; seqNum = snum;
	fmtBase();
}

/** Extract a GET_LOGGED_PACKETS control packet (request).
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetLoggedPackets() {
	return	type == GET_LOGGED_PACKETS && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a GET_LOGGED_PACKETS control packet reply.
 *  @param count is the number of packets in the log string
 *  @param logString is the string describing the logged packets
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtGetLoggedPacketsReply(int count, string logString,
				      int64_t snum) {
	type = GET_LOGGED_PACKETS; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(count); put(logString); 
	paylen = next - payload;
}

/** Extract a GET_LOGGED_PACKETS control packet reply.
 *  @param count is the number of packets in the log string
 *  @param logString is the string describing the logged packets
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrGetLoggedPacketsReply(int& count, string& logString) {
	return	type == GET_LOGGED_PACKETS && mode == REQUEST
		&& get(count) && get(logString) 
		&& paylen >= (next - payload);
}

/** Format a ENABLE_PACKET_LOG control packet (request).
 *  @param enable is 1 if logging is to be turned on, else 0
 *  @param local is 1 if local logging is to be turned on, else 0
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtEnablePacketLog(int enable, int local, int64_t snum) {
	type = ENABLE_PACKET_LOG; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(enable); put(local); 
	paylen = next - payload;
}

/** Extract a ENABLE_PACKET_LOG control packet (request).
 *  @param enable is 1 if logging is to be turned on, else 0
 *  @param local is 1 if local logging is to be turned on, else 0
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrEnablePacketLog(int& enable, int& local) {
	return	type == ENABLE_PACKET_LOG && mode == REQUEST
		&& get(enable) && get(local) 
		&& paylen >= (next - payload);
}

/** Format a ENABLE_PACKET_LOG control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtEnablePacketLogReply(int64_t snum) {
	type = ENABLE_PACKET_LOG; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a ENABLE_PACKET_LOG control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrEnablePacketLogReply() {
	return	type == ENABLE_PACKET_LOG && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a NEW_SESSION control packet (request).
 *  @param clientIp is the IP address of the client starting a new session
 *  @param rates is the rates requested for the client's access link
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtNewSession(ipa_t clientIp, RateSpec rates, int64_t snum) {
	type = NEW_SESSION; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(clientIp); put(rates); 
	paylen = next - payload;
}

/** Extract a NEW_SESSION control packet (request).
 *  @param clientIp is the IP address of the client starting a new session
 *  @param rates is the rates requested for the client's access link
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrNewSession(ipa_t& clientIp, RateSpec& rates) {
	return	type == NEW_SESSION && mode == REQUEST
		&& get(clientIp) && get(rates) 
		&& paylen >= (next - payload);
}

/** Format a NEW_SESSION control packet reply.
 *  @param clientAdr is the Forest address assigned to the client
 *  @param rtrAdr is the Forest address of the client's assigned access router
 *  @param rtrIp is the IP address for the interface of the client's access router
 *  @param rtrPort is the port number for the interface of the client's access router
 *  @param nonce is the nonce to be used by the client when connecting to the access router
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtNewSessionReply(fAdr_t clientAdr, fAdr_t rtrAdr, ipa_t rtrIp, ipp_t rtrPort, uint64_t nonce, int64_t snum) {
	type = NEW_SESSION; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
	put(clientAdr); put(rtrAdr); put(rtrIp); put(rtrPort); put(nonce); 
	paylen = next - payload;
}

/** Extract a NEW_SESSION control packet reply.
 *  @param clientAdr is the Forest address assigned to the client
 *  @param rtrAdr is the Forest address of the client's assigned access router
 *  @param rtrIp is the IP address for the interface of the client's
 *  access router
 *  @param rtrPort is the port number for the interface of the client's
 *  access router
 *  @param nonce is the nonce to be used by the client when connecting to
 *  the access router
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrNewSessionReply(fAdr_t& clientAdr, fAdr_t& rtrAdr, ipa_t&
		rtrIp, ipp_t& rtrPort, uint64_t& nonce) {
	return	type == NEW_SESSION && mode == REQUEST
		&& get(clientAdr) && get(rtrAdr)
		&& get(rtrIp) && get(rtrPort) && get(nonce) 
		&& paylen >= (next - payload);
}

/** Format a CANCEL_SESSION control packet (request).
 *  @param clientAdr is the Forest address assigned to the client
 *  @param rtrAdr is the Forest address of the client's access router
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtCancelSession(fAdr_t clientAdr, fAdr_t rtrAdr, int64_t snum) {
	type = CANCEL_SESSION; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(clientAdr); put(rtrAdr); 
	paylen = next - payload;
}

/** Extract a CANCEL_SESSION control packet (request).
 *  @param clientAdr is the Forest address assigned to the client
 *  @param rtrAdr is the Forest address of the client's access router
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrCancelSession(fAdr_t& clientAdr, fAdr_t& rtrAdr) {
	return	type == CANCEL_SESSION && mode == REQUEST
		&& get(clientAdr) && get(rtrAdr) 
		&& paylen >= (next - payload);
}

/** Format a CANCEL_SESSION control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtCancelSessionReply(int64_t snum) {
	type = CANCEL_SESSION; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a CANCEL_SESSION control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrCancelSessionReply() {
	return	type == CANCEL_SESSION && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a CLIENT_CONNECT control packet (request).
 *  @param clientAdr is the Forest address of the connecting client
 *  @param rtrAdr is the Forest address of the client's access router
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtClientConnect(fAdr_t clientAdr, fAdr_t rtrAdr, int64_t snum) {
	type = CLIENT_CONNECT; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(clientAdr); put(rtrAdr); 
	paylen = next - payload;
}

/** Extract a CLIENT_CONNECT control packet (request).
 *  @param clientAdr is the Forest address of the connecting client
 *  @param rtrAdr is the Forest address of the client's access router
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrClientConnect(fAdr_t& clientAdr, fAdr_t& rtrAdr) {
	return	type == CLIENT_CONNECT && mode == REQUEST
		&& get(clientAdr) && get(rtrAdr) 
		&& paylen >= (next - payload);
}

/** Format a CLIENT_CONNECT control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtClientConnectReply(int64_t snum) {
	type = CLIENT_CONNECT; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a CLIENT_CONNECT control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrClientConnectReply() {
	return	type == CLIENT_CONNECT && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a CLIENT_DISCONNECT control packet (request).
 *  @param clientAdr is the Forest address of the disconnecting client
 *  @param rtrAdr is the Forest address of the client's access router
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtClientDisconnect(fAdr_t clientAdr, fAdr_t rtrAdr, int64_t snum) {
	type = CLIENT_DISCONNECT; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(clientAdr); put(rtrAdr); 
	paylen = next - payload;
}

/** Extract a CLIENT_DISCONNECT control packet (request).
 *  @param clientAdr is the Forest address of the disconnecting client
 *  @param rtrAdr is the Forest address of the client's access router
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrClientDisconnect(fAdr_t& clientAdr, fAdr_t& rtrAdr) {
	return	type == CLIENT_DISCONNECT && mode == REQUEST
		&& get(clientAdr) && get(rtrAdr) 
		&& paylen >= (next - payload);
}

/** Format a CLIENT_DISCONNECT control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtClientDisconnectReply(int64_t snum) {
	type = CLIENT_DISCONNECT; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a CLIENT_DISCONNECT control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrClientDisconnectReply() {
	return	type == CLIENT_DISCONNECT && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a CONFIG_LEAF control packet (request).
 *  @param leafAdr is the Forest address being assigned to the leaf node
 *  @param rtrAdr is the address of the leaf node's access router
 *  @param rtrIp is the IP address of the leaf's access router
 *  @param port is the port number of the leaf node's access router
 *  @param nonce is the nonce that the leaf should use to connect
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtConfigLeaf(fAdr_t leafAdr, fAdr_t rtrAdr, ipa_t rtrIp,
		ipp_t port, uint64_t nonce, int64_t snum) {
	type = CONFIG_LEAF; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(leafAdr); put(rtrAdr); put(rtrIp); put(port); put(nonce); 
	paylen = next - payload;
}

/** Extract a CONFIG_LEAF control packet (request).
 *  @param leafAdr is the Forest address being assigned to the leaf node
 *  @param rtrAdr is the address of the leaf node's access router
 *  @param rtrIp is the IP address of the leaf's access router
 *  @param port is the port number of the leaf node's access router
 *  @param nonce is the nonce that the leaf should use to connect
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrConfigLeaf(fAdr_t& leafAdr, fAdr_t& rtrAdr, ipa_t& rtrIp,
		ipp_t& port, uint64_t& nonce) {
	return	type == CONFIG_LEAF && mode == REQUEST
		&& get(leafAdr) && get(rtrAdr) && get(rtrIp)
		&& get(port) && get(nonce) 
		&& paylen >= (next - payload);
}

/** Format a CONFIG_LEAF control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtConfigLeafReply(int64_t snum) {
	type = CONFIG_LEAF; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a CONFIG_LEAF control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrConfigLeafReply() {
	return	type == CONFIG_LEAF && mode == REQUEST
		&& paylen >= (next - payload);
}


/** Format a SET_LEAF_RANGE control packet (request).
 *  @param firstAdr is the first Forest address in the leaf range
 *  @param lastAdr is the last Forest address in the leaf range
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtSetLeafRange(fAdr_t firstAdr, fAdr_t lastAdr, int64_t snum) {
	type = SET_LEAF_RANGE; mode = REQUEST; seqNum = snum;
	fmtBase();
	put(firstAdr); put(lastAdr); 
	paylen = next - payload;
}

/** Extract a SET_LEAF_RANGE control packet (request).
 *  @param firstAdr is the first Forest address in the leaf range
 *  @param lastAdr is the last Forest address in the leaf range
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrSetLeafRange(fAdr_t& firstAdr, fAdr_t& lastAdr) {
	return	type == SET_LEAF_RANGE && mode == REQUEST
		&& get(firstAdr) && get(lastAdr) 
		&& paylen >= (next - payload);
}

/** Format a SET_LEAF_RANGE control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtSetLeafRangeReply(int64_t snum) {
	type = SET_LEAF_RANGE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a SET_LEAF_RANGE control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrSetLeafRangeReply() {
	return	type == SET_LEAF_RANGE && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a BOOT_ROUTER control packet (request).
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtBootRouter(int64_t snum) {
	type = BOOT_ROUTER; mode = REQUEST; seqNum = snum;
	fmtBase();
}

/** Extract a BOOT_ROUTER control packet (request).
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrBootRouter() {
	return	type == BOOT_ROUTER && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a BOOT_ROUTER control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtBootRouterReply(int64_t snum) {
	type = BOOT_ROUTER; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a BOOT_ROUTER control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrBootRouterReply() {
	return	type == BOOT_ROUTER && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a BOOT_LEAF control packet (request).
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtBootLeaf(int64_t snum) {
	type = BOOT_LEAF; mode = REQUEST; seqNum = snum;
	fmtBase();
}

/** Extract a BOOT_LEAF control packet (request).
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrBootLeaf() {
	return	type == BOOT_LEAF && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a BOOT_LEAF control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtBootLeafReply(int64_t snum) {
	type = BOOT_LEAF; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a BOOT_LEAF control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrBootLeafReply() {
	return	type == BOOT_LEAF && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a BOOT_COMPLETE control packet (request).
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtBootComplete(int64_t snum) {
	type = BOOT_COMPLETE; mode = REQUEST; seqNum = snum;
	fmtBase();
}

/** Extract a BOOT_COMPLETE control packet (request).
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrBootComplete() {
	return	type == BOOT_COMPLETE && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a BOOT_COMPLETE control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtBootCompleteReply(int64_t snum) {
	type = BOOT_COMPLETE; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a BOOT_COMPLETE control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrBootCompleteReply() {
	return	type == BOOT_COMPLETE && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a BOOT_ABORT control packet (request).
 *  @param snum is the sequence number for the control packet
 */
void CtlPkt::fmtBootAbort(int64_t snum) {
	type = BOOT_ABORT; mode = REQUEST; seqNum = snum;
	fmtBase();
}

/** Extract a BOOT_ABORT control packet (request).
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrBootAbort() {
	return	type == BOOT_ABORT && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Format a BOOT_ABORT control packet reply.
 *  @param snum is the sequence number for the reply (optional)
 */
void CtlPkt::fmtBootAbortReply(int64_t snum) {
	type = BOOT_ABORT; mode = POS_REPLY; 
	if (snum != 0) seqNum = snum;
	fmtBase();
}

/** Extract a BOOT_ABORT control packet reply.
 *  @return true if the extracted packet passes basic checks
 */
bool CtlPkt::xtrBootAbortReply() {
	return	type == BOOT_ABORT && mode == REQUEST
		&& paylen >= (next - payload);
}

/** Create a string representing an (attribute,value) pair.
 *  @param attr is an attribute code
 *  @return a string representing attr
string CtlPkt::avPair2string(CpAttr attr) {
	stringstream ss; int len;
	switch (attr) {
	case ADR1:
		if (adr1 != 0) ss << "adr1=" << Forest::fAdr2string(adr1);
		break;
	case ADR2:
		if (adr2 != 0) ss << "adr2=" << Forest::fAdr2string(adr2);
		break;
	case ADR3:
		if (adr3 != 0) ss << "adr3=" << Forest::fAdr2string(adr3);
		break;
	case IP1:
		if (ip1 != 0) ss << "ip1=" << Np4d::ip2string(ip1);
		break;
	case IP2:
		if (ip2 != 0) ss << "ip2=" << Np4d::ip2string(ip2);
		break;
	case PORT1:
		if (port1 != 0) ss << "port1=" << port1;
		break;
	case PORT2:
		if (port2 != 0) ss << "port2=" << port2;
		break;
	case RSPEC1:
		if (rspec1.isSet()) ss << "rspec1=" << rspec1.toString();
		break;
	case RSPEC2:
		if (rspec2.isSet()) ss << "rspec2=" << rspec2.toString();
		break;
	case CORE_FLAG:
		if (coreFlag >= 0)
			ss << "coreFlag=" << (coreFlag ? "true" : "false");
		break;
	case IFACE:
		if (iface != 0) ss << "iface=" << iface;
		break;
	case LINK:
		if (link != 0) ss << "link=" << link;
		break;
	case NODE_TYPE:
		if (nodeType != Forest::UNDEF_NODE)
			ss << "nodeType="
			   << Forest::nodeType2string(nodeType);
		break;
	case COMTREE:
		if (comtree != 0) ss << "comtree=" << comtree;
		break;
	case COMTREE_OWNER:
		ss << "comtreeOwner=" << Forest::fAdr2string(comtreeOwner);
		break;
	case INDEX1:
		if (index1 >= 0) ss << "index1=" << index1;
		break;
	case INDEX2:
		if (index2 >= 0) ss << "index2=" << index2;
		break;
	case COUNT:
		if (count >= 0) ss << "count=" << count;
		break;
	case QUEUE:
		if (queue != 0) ss << "queue=" << queue;
		break;
	case NONCE:
		if (nonce != 0) ss << "nonce=" << nonce;
		break;
	case ZIPCODE:
		if (zipCode != 0) ss << "zipCode=" << zipCode;
		break;
	case ERRMSG:
		if (errMsg.length() != 0) ss << "errMsg=" << errMsg;
		break;
	case STRING:
		if (stringData.length() != 0) ss << "stringData=" << stringData;
		break;
	case INTVEC:
		len = ivec.size();
		if (len != 0) {
			ss << "ivec=";
			for (int i = 0; i < len; i++) ss << ivec[i] << " ";
		}
		break;
	default: break;
	}
	return ss.str();
}
 */


string CtlPkt::cpType2string(CpType type) {
	string s;
	switch (type) {
	case ADD_IFACE: s = "add_iface"; break;
	case DROP_IFACE: s = "drop_iface"; break;
	case GET_IFACE: s = "get_iface"; break;
	case MOD_IFACE: s = "mod_iface"; break;
	case ADD_LINK: s = "add_link"; break;
	case DROP_LINK: s = "drop_link"; break;
	case GET_LINK: s = "get_link"; break;
	case GET_LINK_SET: s = "get_link_set"; break;
	case GET_COMTREE_SET: s = "get_comtree_set"; break;
	case GET_IFACE_SET: s = "get_iface_set"; break;
	case GET_ROUTE_SET: s = "get_route_set"; break;
	case MOD_LINK: s = "mod_link"; break;
	case ADD_COMTREE: s = "add_comtree"; break;
	case DROP_COMTREE: s = "drop_comtree"; break;
	case GET_COMTREE: s = "get_comtree"; break;
	case MOD_COMTREE: s = "mod_comtree"; break;
	case ADD_COMTREE_LINK: s = "add_comtree_link"; break;
	case DROP_COMTREE_LINK: s = "drop_comtree_link"; break;
	case MOD_COMTREE_LINK: s = "mod_comtree_link"; break;
	case GET_COMTREE_LINK: s = "get_comtree_link"; break;
	case ADD_ROUTE: s = "add_route"; break;
	case DROP_ROUTE: s = "drop_route"; break;
	case GET_ROUTE: s = "get_route"; break;
	case MOD_ROUTE: s = "mod_route"; break;
	case ADD_ROUTE_LINK: s = "add_route_link"; break;
	case DROP_ROUTE_LINK: s = "drop_route_link"; break;

	case ADD_FILTER: s = "add_filter"; break;
	case DROP_FILTER: s = "drop_filter"; break;
	case GET_FILTER: s = "get_filter"; break;
	case MOD_FILTER: s = "mod_filter"; break;
	case GET_FILTER_SET: s = "get_filter_set"; break;
	case GET_LOGGED_PACKETS: s = "get_logged_packets"; break;
	case ENABLE_PACKET_LOG: s = "enable_packet_log"; break;

	case NEW_SESSION: s = "new_session"; break;
	case CANCEL_SESSION: s = "cancel_session"; break;
	case CLIENT_CONNECT: s = "client_connect"; break;
	case CLIENT_DISCONNECT: s = "client_disconnect"; break;
	case CONFIG_LEAF: s = "config_leaf"; break;
	case SET_LEAF_RANGE: s = "set_leaf_range"; break;
	case BOOT_ROUTER: s = "boot_router"; break;
	case BOOT_LEAF: s = "boot_leaf"; break;
	case BOOT_COMPLETE: s = "boot_complete"; break;
	case BOOT_ABORT: s = "boot_abort"; break;
	case COMTREE_PATH: s = "comtree_path"; break;
	case ADD_NODE: s = "comtree_new_leaf"; break;
	case ADD_BRANCH: s = "comtree_add_branch"; break;
	case PRUNE: s = "comtree_prune"; break;
	default: s = "undefined"; break;
	}
	return s;
}

bool CtlPkt::string2cpType(string& s, CpType& type) {
	if (s == "undef") type = UNDEF_CPTYPE;

	else if (s == "add_iface") type = ADD_IFACE;
	else if (s == "drop_iface") type = DROP_IFACE;
	else if (s == "get_iface") type = GET_IFACE;
	else if (s == "mod_iface") type = MOD_IFACE;

	else if (s == "add_link") type = ADD_LINK;
	else if (s == "drop_link") type = DROP_LINK;
	else if (s == "get_link") type = GET_LINK;
	else if (s == "mod_link") type = MOD_LINK;
	else if (s == "get_link_set") type = GET_LINK_SET;

	else if (s == "add_comtree") type = ADD_COMTREE;
	else if (s == "drop_comtree") type = DROP_COMTREE;
	else if (s == "get_comtree") type = GET_COMTREE;
	else if (s == "mod_comtree") type = MOD_COMTREE;

	else if (s == "add_comtree_link") type = ADD_COMTREE_LINK;
	else if (s == "drop_comtree_link") type = DROP_COMTREE_LINK;
	else if (s == "get_comtree_link") type = GET_COMTREE_LINK;
	else if (s == "mod_comtree_link") type = MOD_COMTREE_LINK;

	else if (s == "add_route") type = ADD_ROUTE;
	else if (s == "drop_route") type = DROP_ROUTE;
	else if (s == "get_route") type = GET_ROUTE;
	else if (s == "mod_route") type = MOD_ROUTE;
	else if (s == "add_route_link") type = ADD_ROUTE;
	else if (s == "drop_route_link") type = DROP_ROUTE;

	else if (s == "add_filter") type = ADD_FILTER;
	else if (s == "drop_filter") type = DROP_FILTER;
	else if (s == "mod_filter") type = MOD_FILTER;
	else if (s == "get_filter") type = GET_FILTER;
	else if (s == "get_filter_set") type = GET_FILTER_SET;
	else if (s == "get_logged_packets") type = GET_LOGGED_PACKETS;
	else if (s == "enable_packet_log") type = ENABLE_PACKET_LOG;

	else if (s == "new_session") type = NEW_SESSION;
	else if (s == "cancel_session") type = CANCEL_SESSION;
	else if (s == "client_connect") type = CLIENT_CONNECT;
	else if (s == "client_disconnect") type = CLIENT_DISCONNECT;

	else if (s == "set_leaf_range") type = SET_LEAF_RANGE;
	else if (s == "config_leaf") type = CONFIG_LEAF;

	else if (s == "boot_router") type = BOOT_ROUTER;
	else if (s == "boot_complete") type = BOOT_COMPLETE;
	else if (s == "boot_abort") type = BOOT_ABORT;
	else if (s == "boot_leaf") type = BOOT_LEAF;

	else if (s == "comtree_path") type = COMTREE_PATH;
	else if (s == "comtree_new_leaf") type = ADD_NODE;
	else if (s == "comtree_add_branch") type = ADD_BRANCH;
	else if (s == "comtree_prune") type = PRUNE;

	else return false;
	return true;
}

string CtlPkt::cpMode2string(CpMode mode) {
	string s;
	switch (mode) {
	case REQUEST: s = "request"; break;
	case POS_REPLY: s = "pos reply"; break;
	case NEG_REPLY: s = "neg reply"; break;
	default: break;
	}
	return s;
}

bool CtlPkt::string2cpMode(string& s, CpMode& mode) {
	if (s == "undef") mode = UNDEF_MODE;
	else if (s == "request") mode = REQUEST;
	else if (s == "pos reply") mode = POS_REPLY;
	else if (s == "neg reply") mode = NEG_REPLY;
	else return false;
	return true;
}

/** Create string version of control packet.
 *  @return the string
 */
string CtlPkt::toString() {
	xtrBase();
	stringstream ss;
	ss << cpType2string(type);
	ss << " (" << cpMode2string(mode) << "," << seqNum << "): ";
	if (mode == NEG_REPLY) {
		string s; xtrError(s);
		ss << s << endl;
		return ss.str();
	}

	int iface, lnk, coreFlag, qid, fltr, count, enable, local;
	comt_t comt; RateSpec rs1, rs2;
	ipa_t ip1; ipp_t port1;
	fAdr_t adr1, adr2;
	uint64_t nonce;
	Forest::ntyp_t ntyp;
	string s;

	switch (type) {
	case ADD_IFACE:
		if (mode == REQUEST) {
			xtrAddIface(iface,ip1,rs1);
			ss << " " << iface;
			ss << " " << Np4d::ip2string(ip1);
			ss << " " << rs1.toString();
		} else {
			xtrAddIfaceReply(ip1,port1);
			ss << " " << Np4d::ip2string(ip1);
			ss << " " << port1;
		}
		break;
	case DROP_IFACE:
		if (mode == REQUEST) {
			xtrDropIface(iface);
			ss << " " << iface;
		}
		break;
	case GET_IFACE:
		if (mode == REQUEST) {
			xtrGetIface(iface);
			ss << " " << iface;
		} else {
			xtrGetIfaceReply(iface,ip1,port1,rs1,rs2);
			ss << " " << iface;
			ss << " " << Np4d::ip2string(ip1) << " " << port1;
			ss << " " << rs1.toString();
			ss << " " << rs2.toString();
		}
		break;
	case MOD_IFACE:
		if (mode == REQUEST) {
			xtrModIface(iface,rs1);
			ss << " " << iface;
			ss << " " << rs1.toString();
		}
		break;
	case ADD_LINK:
		if (mode == REQUEST) {
			xtrAddLink(ntyp,iface,lnk,ip1,port1,adr1,nonce);
			ss << " " << iface;
			ss << " " << lnk;
			ss << " " << Forest::nodeType2string(ntyp);
			ss << " " << Np4d::ip2string(ip1);
			ss << " " << port1;
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << nonce;
		} else {
			xtrAddLinkReply(lnk,adr1);
			ss << " " << lnk;
			ss << " " << Forest::fAdr2string(adr1);
		}
		break;
	case DROP_LINK:
		if (mode == REQUEST) {
			xtrDropLink(lnk,adr1);
			if (link != 0) ss << " " << lnk;
			if (adr1 != 0) ss << " " << Forest::fAdr2string(adr1);
		}
		break;
	case GET_LINK:
		if (mode == REQUEST) {
			xtrGetLink(lnk);
			ss << " " << lnk;
		} else {
			xtrGetLinkReply(lnk,iface,ntyp,ip1,port1,adr1,rs1,rs2);
			ss << " " << lnk;
			ss << " " << iface;
			ss << " " << Forest::nodeType2string(ntyp);
			ss << " " << Np4d::ip2string(ip1);
			ss << " " << port1;
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << rs1.toString();
			ss << " " << rs2.toString();
		}
		break;
	case GET_LINK_SET:
		if (mode == REQUEST) {
			xtrGetLinkSet(lnk,count);
			ss << " " << lnk << " " << count;
		} else {
			xtrGetLinkSetReply(count,lnk,s);
			ss << " " << count << " " << lnk << " " << s;
		}
		break;
	case GET_COMTREE_SET:
		if (mode == REQUEST) {
			xtrGetComtreeSet(comt,count);
			ss << " " << comt << " " << count;
		} else {
			xtrGetComtreeSetReply(count,comt,s);
			ss << " " << count << " " << comt << " " << s;
		}
		break;
	case GET_IFACE_SET:
		if (mode == REQUEST) {
			xtrGetIfaceSet(lnk,count);
			ss << " " << lnk << " " << count;
		} else {
			xtrGetIfaceSetReply(count,lnk,s);
			ss << " " << count << " " << lnk << " " << s;
		}
		break;
	case GET_ROUTE_SET:
		if (mode == REQUEST) {
			xtrGetRouteSet(lnk,count);
			ss << " " << lnk << " " << count;
		} else {
			xtrGetRouteSetReply(count,lnk,s);
			ss << " " << count << " " << lnk << " " << s;
		}
		break;
	case MOD_LINK:
		if (mode == REQUEST) {
			xtrModLink(lnk,rs1);
			ss << " " << lnk;
			ss << " " << rs1.toString();
		}
		break;
	case ADD_COMTREE:
		if (mode == REQUEST) {
			xtrAddComtree(comt);
			ss << " " << comt;
		}
		break;
	case DROP_COMTREE:
		if (mode == REQUEST) {
			xtrDropComtree(comt);
			ss << " " << comt;
		} else
			xtrDropComtreeReply(rs1);
			rs1.toString();
		break;
	case GET_COMTREE:
		if (mode == REQUEST) {
			xtrGetComtree(comt);
			ss << " " << comt;
		} else {
			xtrGetComtreeReply(comt,coreFlag,lnk,count);
			ss << " " << comt;
			ss << " " << (coreFlag ? "inCore" : "notInCore");
			ss << " " << lnk;
			ss << " " << count;
		}
		break;
	case MOD_COMTREE:
		if (mode == REQUEST) {
			xtrModComtree(comt,coreFlag,lnk);
			ss << " " << comt;
			ss << " " << (coreFlag ? "inCore" : "notInCore");
			ss << " " << lnk;
		}
		break;
	case ADD_COMTREE_LINK:
		if (mode == REQUEST) {
			xtrAddComtreeLink(comt,lnk,coreFlag,ip1,port1,adr1);
			ss << " " << comt;
			ss << " " << lnk;
			ss << " " << (coreFlag ? "inCore" : "notInCore");
			ss << " " << Np4d::ip2string(ip1);
			ss << " " << port1;
			ss << " " << Forest::fAdr2string(adr1);
		} else {
			xtrAddComtreeLinkReply(lnk,rs1);
			ss << " " << lnk;
			ss << " " << rs1.toString();
		}
		break;
	case DROP_COMTREE_LINK:
		if (mode == REQUEST) {
			xtrDropComtreeLink(comt,lnk,ip1,port1,adr1);
			ss << " " << comt;
			ss << " " << lnk;
			ss << " " << Np4d::ip2string(ip1);
			ss << " " << port1;
			ss << " " << Forest::fAdr2string(adr1);
		} else
			xtrDropComtreeLinkReply(rs1);
			ss << " " << rs1.toString();
		break;
	case MOD_COMTREE_LINK:
		if (mode == REQUEST) {
			xtrModComtreeLink(comt,lnk,rs1);
			ss << " " << comt;
			ss << " " << lnk;
			ss << " " << rs1.toString();
		} else
			xtrModComtreeLinkReply(rs1);
			ss << " " << rs1.toString();
		break;
	case GET_COMTREE_LINK:
		if (mode == REQUEST) {
			xtrGetComtreeLink(comt,lnk);
			ss << " " << comt;
			ss << " " << lnk;
		} else {
			xtrGetComtreeLinkReply(comt,lnk,rs1,qid,adr1);
			ss << " " << comt;
			ss << " " << lnk;
			ss << " " << rs1.toString();
			ss << " " << qid;
			ss << " " << Forest::fAdr2string(adr1);
		}
		break;
	case ADD_ROUTE:
		if (mode == REQUEST) {
			xtrAddRoute(comt,adr1,lnk);
			ss << " " << comt;
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << lnk;
		}
		break;
	case DROP_ROUTE:
		if (mode == REQUEST) {
			xtrDropRoute(comt,adr1);
			ss << " " << comt;
			ss << " " << Forest::fAdr2string(adr1);
		}
		break;
	case GET_ROUTE:
		if (mode == REQUEST) {
			xtrGetRoute(comt,adr1);
			ss << " " << comt;
			ss << " " << Forest::fAdr2string(adr1);
		} else {
			xtrGetRouteReply(comt,adr1,lnk);
			ss << " " << comt;
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << lnk;
		}
		break;
	case MOD_ROUTE:
		if (mode == REQUEST) {
			xtrModRoute(comt,adr1,lnk);
			ss << " " << comt;
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << lnk;
		}
		break;
	case ADD_ROUTE_LINK:
		if (mode == REQUEST) {
			xtrAddRouteLink(comt,adr1,lnk);
			ss << " " << comt;
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << lnk;
		}
		break;
	case DROP_ROUTE_LINK:
		if (mode == REQUEST) {
			xtrDropRouteLink(comt,adr1,lnk);
			ss << " " << comt;
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << lnk;
		}
		break;

	case ADD_FILTER:
		if (mode == POS_REPLY) {
			xtrAddFilterReply(fltr);
			ss << " " << fltr;
		}
		break;
	case DROP_FILTER:
		if (mode == REQUEST) {
			xtrDropFilter(fltr);
			ss << " " << fltr;
		}
		break;
	case MOD_FILTER:
		if (mode == REQUEST) {
			xtrModFilter(fltr,s);
			ss << " " << fltr;
			ss << " " << s;
		}
		break;
	case GET_FILTER:
		if (mode == REQUEST) {
			xtrGetFilter(fltr);
			ss << " " << fltr;
		} else {
			xtrGetFilterReply(s);
			ss << " " << s;
		}
		break;
	case GET_FILTER_SET:
		if (mode == REQUEST) {
			xtrGetFilterSet(fltr,count);
			ss << " " << fltr;
			ss << " " << count;
		} else {
			xtrGetFilterSetReply(fltr,count,s);
			ss << " " << fltr;
			ss << " " << count;
			ss << " " << s;
		}
		break;
	case GET_LOGGED_PACKETS:
		if (mode == POS_REPLY) {
			xtrGetLoggedPacketsReply(count,s);
			ss << " " << count << " " << s;
		}
		break;
	case ENABLE_PACKET_LOG:
		if (mode == REQUEST) {
			xtrEnablePacketLog(enable,local);
			ss << " " << (enable ? "on" : "off");
			ss << " " << (local ? "local" : "remote");
		}
		break;

	case NEW_SESSION:
		if (mode == REQUEST) {
			xtrNewSession(ip1,rs1);
			ss << " " << Np4d::ip2string(ip1);
			ss << " " << rs1.toString();
		} else {
			xtrNewSessionReply(adr1,adr2,ip1,port1,nonce);
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << Forest::fAdr2string(adr2);
			ss << " " << Np4d::ip2string(ip1);
			ss << " " << port1;
			ss << " " << nonce;
		}
		break;
	case CANCEL_SESSION:
		if (mode == REQUEST) {
			xtrCancelSession(adr1,adr2);
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << Forest::fAdr2string(adr2);
		} 
		break;
	case CLIENT_CONNECT:
		if (mode == REQUEST) {
			xtrClientConnect(adr1,adr2);
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << Forest::fAdr2string(adr2);
		}
		break;
	case CLIENT_DISCONNECT:
		if (mode == REQUEST) {
			xtrClientDisconnect(adr1,adr2);
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << Forest::fAdr2string(adr2);
		}
		break;
	case CONFIG_LEAF:
		if (mode == REQUEST) {
			xtrConfigLeaf(adr1,adr2,ip1,port1,nonce);
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << Forest::fAdr2string(adr2);
			ss << " " << Np4d::ip2string(ip1);
			ss << " " << port1;
			ss << " " << nonce;
		}
		break;
	case SET_LEAF_RANGE:
		if (mode == REQUEST) {
			xtrSetLeafRange(adr1,adr2);
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << Forest::fAdr2string(adr2);
		}
		break;
	case BOOT_ROUTER:
	case BOOT_LEAF:
	case BOOT_COMPLETE:
	case BOOT_ABORT:
		break;

/*
	case COMTREE_PATH:
		if (mode == REQUEST) {
			ss << " " << comt;
			ss << " " << Forest::fAdr2string(adr1);
		} else {
			ss << " " << rs1.toString();
			ss << " " << rs2.toString();
			ss << " " << avPair2string(INTVEC);
		}
		break;

	case ADD_NODE:
		if (mode == REQUEST) {
			ss << " " << comt;
			ss << " " << Forest::fAdr2string(adr1);
			ss << " " << lnk;
			ss << " " << Forest::fAdr2string(adr2);
			ss << " " << rs1.toString();
			ss << " " << rs2.toString();
			ss << " " << avPair2string(INTVEC);
		}
		break;

	case ADD_BRANCH:
		if (mode == REQUEST) {
			ss << " " << avPair2string(INTVEC);
			ss << " " << avPair2string(INDEX1);
			ss << " " << rs1.toString();
			ss << " " << rs2.toString();
		} else if (mode == POS_REPLY) {
			ss << " " << Forest::fAdr2string(adr1);
		}
		break;

	case PRUNE:
		if (mode == REQUEST) {
			ss << " " << comt;
			ss << " " << Forest::fAdr2string(adr1);
		}
		break;
*/

	default: break;
	}
	ss << endl;
	return ss.str();
}

} // ends namespace

