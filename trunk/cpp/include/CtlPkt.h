/** @file CtlPkt.h
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef CTLPKT_H
#define CTLPKT_H

#include "Forest.h"
#include "Packet.h"
#include "RateSpec.h"
#include <vector>

using namespace std;

namespace forest {

/** This class provides a mechanism for handling forest signalling packets.
 *  Signalling packets have a packet type of CLIENT_SIG or NET_SIG in the
 *  first word of the forest head. The payload identifies the specific
 *  type of packet. Every control packet includes the following three fields.
 *
 *  control packet type		identifies the specific type of packet (16 bits)
 *  mode 			specifies that this is a request packet,
 *				a positive reply, or a negative reply (16 bits)
 *  sequence number		this field is set by the originator of any
 *				request packet, and the same value is returned
 *				in the reply packet; this allows the requestor
 *				to match repies with earlier requests (64 bits)
 *
 *  The remainder of a control packet payload consists of
 *  (attribute,length,value) tuples. Each attribute is identified by a 16
 *  bit code and the 16 bit length specifies the number of bytes in the value.
 *  Each control packet type has a fixed format, that is, the number of
 *  attributes is fixed, and the types are fixed (with a fixed order).
 *
 *  Methods are provided to conveniently create control packets to
 *  be sent, or to "unpack" received control packets.
 */
class CtlPkt {
public:
	/** Control packet types */
	enum CpType {
		UNDEF_CPTYPE = 0,

		JOIN_COMTREE = 10, LEAVE_COMTREE = 11,

		CLIENT_NET_SIG_SEP = 29,

		ADD_IFACE = 30, DROP_IFACE = 31,
		GET_IFACE = 32, MOD_IFACE = 33,
		GET_IFACE_SET = 34,

		ADD_LINK = 40, DROP_LINK = 41,
		GET_LINK = 42, MOD_LINK = 43,
		GET_LINK_SET = 44,

		ADD_COMTREE = 50, DROP_COMTREE = 51,
		GET_COMTREE = 52, MOD_COMTREE = 53,

		ADD_COMTREE_LINK = 54, DROP_COMTREE_LINK = 55,
		MOD_COMTREE_LINK = 56, GET_COMTREE_LINK = 57,
		GET_COMTREE_SET = 58,

		ADD_ROUTE = 70, DROP_ROUTE = 71,
		GET_ROUTE = 72, MOD_ROUTE = 73,
		ADD_ROUTE_LINK = 74, DROP_ROUTE_LINK = 75,
		GET_ROUTE_SET = 76,

		ADD_FILTER = 80, DROP_FILTER = 81,
		GET_FILTER = 82, MOD_FILTER = 83,
		GET_FILTER_SET = 84, GET_LOGGED_PACKETS = 85,
		ENABLE_PACKET_LOG = 86,

		NEW_SESSION = 100, CANCEL_SESSION = 103,
		CLIENT_CONNECT = 101, CLIENT_DISCONNECT = 102,

		SET_LEAF_RANGE = 110, CONFIG_LEAF = 111,

		BOOT_ROUTER = 120, BOOT_COMPLETE = 121, BOOT_ABORT = 122,
		BOOT_LEAF = 123,

		COMTREE_PATH = 130,
		ADD_BRANCH = 131, CONFIRM = 132, ABORT = 133,
		PRUNE = 134, ADD_NODE = 135, DROP_NODE = 136
	};

	// Control packet attribute types
	enum CpAttr {
		UNDEF_ATTR = 0,
		s16 = 1, u16=2,
		s32 = 3, u32=4,
		s64 = 5, u64=6,
		rateSpec = 7,
		attrString = 8,
		intVec = 9
	};

	/** Control packet modes */
	enum CpMode {
		UNDEF_MODE = 0, REQUEST = 1, POS_REPLY = 2, NEG_REPLY = 3,
		NO_REPLY = 4
	};

	// constructors/destructor
		CtlPkt();
		CtlPkt(const Packet&);
		~CtlPkt();

	// resetting control packet
	void	reset();
	void	reset(const Packet&);

	void	put(int16_t);
	void	put(uint16_t);
	void	put(int32_t);
	void	put(uint32_t);
	void	put(int64_t);
	void	put(uint64_t);
	void	put(Forest::ntyp_t);
	void	put(const RateSpec&);
	void	put(const string&);
	void	put(const vector<int32_t>&);

	int	availSpace();

	bool	get(int16_t&);
	bool	get(uint16_t&);
	bool	get(int32_t&);
	bool	get(uint32_t&);
	bool	get(int64_t&);
	bool	get(uint64_t&);
	bool	get(Forest::ntyp_t&);
	bool	get(RateSpec&);
	bool	get(string&);
	bool	get(vector<int32_t>&);

	void	fmtBase();
	bool	xtrBase();
	void	updateSeqNum();

	void	fmtReply();
	void	fmtError(const string&, int64_t=0);
	bool	xtrError(string&);
			
	void	fmtAddIface(int, ipa_t, RateSpec, int64_t=0);
	bool	xtrAddIface(int&, ipa_t&, RateSpec&);
	void	fmtAddIfaceReply(ipa_t, ipp_t, int64_t=0);
	bool	xtrAddIfaceReply(ipa_t&, ipp_t&);

	void	fmtDropIface(int, int64_t=0);
	bool	xtrDropIface(int&);
	void	fmtDropIfaceReply(int64_t=0);
	bool	xtrDropIfaceReply();

	void	fmtModIface(int, RateSpec, int64_t=0);
	bool	xtrModIface(int&, RateSpec&);
	void	fmtModIfaceReply(int64_t=0);
	bool	xtrModIfaceReply();

	void	fmtGetIface(int, int64_t=0);
	bool	xtrGetIface(int&);
	void	fmtGetIfaceReply(int, ipa_t, ipp_t, RateSpec,
				RateSpec,	int64_t=0);
	bool	xtrGetIfaceReply(int&, ipa_t&, ipp_t&, RateSpec&,
				RateSpec&);

	void	fmtGetIfaceSet(int, int, int64_t=0);
	bool	xtrGetIfaceSet(int&, int&);
	void	fmtGetIfaceSetReply(int, int, string, int64_t=0);
	bool	xtrGetIfaceSetReply(int&, int&, string&);

	void	fmtAddLink(Forest::ntyp_t, int, int, ipa_t, ipp_t,
				fAdr_t,	uint64_t, int64_t=0);
	bool	xtrAddLink(Forest::ntyp_t&, int&, int&, ipa_t&, ipp_t&,
				fAdr_t&,	uint64_t&);
	void	fmtAddLinkReply(int, fAdr_t, int64_t=0);
	bool	xtrAddLinkReply(int&, fAdr_t&);

	void	fmtDropLink(int, fAdr_t, int64_t=0);
	bool	xtrDropLink(int&, fAdr_t&);
	void	fmtDropLinkReply(int64_t=0);
	bool	xtrDropLinkReply();

	void	fmtModLink(int, RateSpec, int64_t=0);
	bool	xtrModLink(int&, RateSpec&);
	void	fmtModLinkReply(int64_t=0);
	bool	xtrModLinkReply();

	void	fmtGetLink(int, int64_t=0);
	bool	xtrGetLink(int&);
	void	fmtGetLinkReply(int, int, Forest::ntyp_t, ipa_t, ipp_t,
			fAdr_t, RateSpec&, RateSpec&, int64_t=0);
	bool	xtrGetLinkReply(int&, int&, Forest::ntyp_t&, ipa_t&, ipp_t&,
			fAdr_t&, RateSpec&, RateSpec&);

	void	fmtGetLinkSet(int, int, int64_t=0);
	bool	xtrGetLinkSet(int&, int&);
	void	fmtGetLinkSetReply(int, int, string, int64_t=0);
	bool	xtrGetLinkSetReply(int&, int&, string&);

	void	fmtAddComtree(comt_t, int64_t=0);
	bool	xtrAddComtree(comt_t&);
	void	fmtAddComtreeReply(int64_t=0);
	bool	xtrAddComtreeReply();

	void	fmtDropComtree(comt_t, int64_t=0);
	bool	xtrDropComtree(comt_t&);
	void	fmtDropComtreeReply(RateSpec, int64_t=0);
	bool	xtrDropComtreeReply(RateSpec&);

	void	fmtModComtree(comt_t, int, int, int64_t=0);
	bool	xtrModComtree(comt_t&, int&, int&);
	void	fmtModComtreeReply(int64_t=0);
	bool	xtrModComtreeReply();

	void	fmtGetComtree(comt_t, int64_t=0);
	bool	xtrGetComtree(comt_t&);
	void	fmtGetComtreeReply(comt_t, int, int, int, int64_t=0);
	bool	xtrGetComtreeReply(comt_t&, int&, int&, int&);

	void	fmtGetComtreeSet(comt_t, int, int64_t=0);
	bool	xtrGetComtreeSet(comt_t&, int&);
	void	fmtGetComtreeSetReply(int, comt_t, string, int64_t=0);
	bool	xtrGetComtreeSetReply(int&, comt_t&, string&);

	void	fmtAddComtreeLink(comt_t, int, int, ipa_t, ipp_t,
				fAdr_t,	int64_t=0);
	bool	xtrAddComtreeLink(comt_t&, int&, int&, ipa_t&, ipp_t&,
				fAdr_t&);
	void	fmtAddComtreeLinkReply(int, RateSpec, int64_t=0);
	bool	xtrAddComtreeLinkReply(int&, RateSpec&);

	void	fmtDropComtreeLink(comt_t, int, ipa_t, ipp_t, fAdr_t, int64_t=0);
	bool	xtrDropComtreeLink(comt_t&, int&, ipa_t&, ipp_t&, fAdr_t&);
	void	fmtDropComtreeLinkReply(RateSpec, int64_t=0);
	bool	xtrDropComtreeLinkReply(RateSpec&);

	void	fmtModComtreeLink(comt_t, int, RateSpec, fAdr_t, int64_t=0);
	bool	xtrModComtreeLink(comt_t&, int&, RateSpec&, fAdr_t&);
	void	fmtModComtreeLinkReply(RateSpec&, int64_t=0);
	bool	xtrModComtreeLinkReply(RateSpec&);

	void	fmtGetComtreeLink(comt_t, int, int64_t=0);
	bool	xtrGetComtreeLink(comt_t&, int&);
	void	fmtGetComtreeLinkReply(comt_t, int, RateSpec, int,
				fAdr_t,	int64_t=0);
	bool	xtrGetComtreeLinkReply(comt_t&, int&, RateSpec&, int&,
				fAdr_t&);

	void	fmtAddRoute(comt_t, fAdr_t, int, int64_t=0);
	bool	xtrAddRoute(comt_t&, fAdr_t&, int&);
	void	fmtAddRouteReply(int64_t=0);
	bool	xtrAddRouteReply();

	void	fmtDropRoute(comt_t, fAdr_t, int64_t=0);
	bool	xtrDropRoute(comt_t&, fAdr_t&);
	void	fmtDropRouteReply(int64_t=0);
	bool	xtrDropRouteReply();

	void	fmtModRoute(comt_t, fAdr_t, int, int64_t=0);
	bool	xtrModRoute(comt_t&, fAdr_t&, int&);
	void	fmtModRouteReply(int64_t=0);
	bool	xtrModRouteReply();

	void	fmtGetRoute(comt_t, fAdr_t, int64_t=0);
	bool	xtrGetRoute(comt_t&, fAdr_t&);
	void	fmtGetRouteReply(comt_t, fAdr_t, int, int64_t=0);
	bool	xtrGetRouteReply(comt_t&, fAdr_t&, int&);

	void	fmtGetRouteSet(int, int, int64_t=0);
	bool	xtrGetRouteSet(int&, int&);
	void	fmtGetRouteSetReply(int, int, string, int64_t=0);
	bool	xtrGetRouteSetReply(int&, int&, string&);

	void	fmtAddRouteLink(comt_t, fAdr_t, int, int64_t=0);
	bool	xtrAddRouteLink(comt_t&, fAdr_t&, int&);
	void	fmtAddRouteLinkReply(int64_t=0);
	bool	xtrAddRouteLinkReply();

	void	fmtDropRouteLink(comt_t, fAdr_t, int, int64_t=0);
	bool	xtrDropRouteLink(comt_t&, fAdr_t&, int&);
	void	fmtDropRouteLinkReply(int64_t=0);
	bool	xtrDropRouteLinkReply();

	void	fmtAddFilter(int64_t=0);
	bool	xtrAddFilter();
	void	fmtAddFilterReply(int, int64_t=0);
	bool	xtrAddFilterReply(int&);

	void	fmtDropFilter(int, int64_t=0);
	bool	xtrDropFilter(int&);
	void	fmtDropFilterReply(int64_t=0);
	bool	xtrDropFilterReply();

	void	fmtModFilter(int, string, int64_t=0);
	bool	xtrModFilter(int&, string&);
	void	fmtModFilterReply(int64_t=0);
	bool	xtrModFilterReply();

	void	fmtGetFilter(int, int64_t=0);
	bool	xtrGetFilter(int&);
	void	fmtGetFilterReply(string, int64_t=0);
	bool	xtrGetFilterReply(string&);

	void	fmtGetFilterSet(int, int, int64_t=0);
	bool	xtrGetFilterSet(int&, int&);
	void	fmtGetFilterSetReply(int, int, string, int64_t=0);
	bool	xtrGetFilterSetReply(int&, int&, string&);

	void	fmtGetLoggedPackets(int64_t=0);
	bool	xtrGetLoggedPackets();
	void	fmtGetLoggedPacketsReply(int, string, int64_t=0);
	bool	xtrGetLoggedPacketsReply(int&, string&);

	void	fmtEnablePacketLog(int, int, int64_t=0);
	bool	xtrEnablePacketLog(int&, int&);
	void	fmtEnablePacketLogReply(int64_t=0);
	bool	xtrEnablePacketLogReply();

	void	fmtNewSession(ipa_t, RateSpec, int64_t=0);
	bool	xtrNewSession(ipa_t&, RateSpec&);
	void	fmtNewSessionReply(fAdr_t, fAdr_t, ipa_t, ipp_t,
					uint64_t, int64_t=0);
	bool	xtrNewSessionReply(fAdr_t&, fAdr_t&, ipa_t&, ipp_t&,
				uint64_t&);

	void	fmtCancelSession(fAdr_t, fAdr_t, int64_t=0);
	bool	xtrCancelSession(fAdr_t&, fAdr_t&);
	void	fmtCancelSessionReply(int64_t=0);
	bool	xtrCancelSessionReply();

	void	fmtClientConnect(fAdr_t, fAdr_t, int64_t=0);
	bool	xtrClientConnect(fAdr_t&, fAdr_t&);
	void	fmtClientConnectReply(int64_t=0);
	bool	xtrClientConnectReply();

	void	fmtClientDisconnect(fAdr_t, fAdr_t, int64_t=0);
	bool	xtrClientDisconnect(fAdr_t&, fAdr_t&);
	void	fmtClientDisconnectReply(int64_t=0);
	bool	xtrClientDisconnectReply();

	void	fmtConfigLeaf(fAdr_t, fAdr_t, ipa_t, ipp_t,
			uint64_t, int64_t=0);
	bool	xtrConfigLeaf(fAdr_t&, fAdr_t&, ipa_t&, ipp_t&, uint64_t&);
	void	fmtConfigLeafReply(int64_t=0);
	bool	xtrConfigLeafReply();

	void	fmtSetLeafRange(fAdr_t, fAdr_t, int64_t=0);
	bool	xtrSetLeafRange(fAdr_t&, fAdr_t&);
	void	fmtSetLeafRangeReply(int64_t=0);
	bool	xtrSetLeafRangeReply();

	void	fmtBootRouter(int64_t=0);
	bool	xtrBootRouter();
	void	fmtBootRouterReply(int64_t=0);
	bool	xtrBootRouterReply();

	void	fmtBootLeaf(int64_t=0);
	bool	xtrBootLeaf();
	void	fmtBootLeafReply(int64_t=0);
	bool	xtrBootLeafReply();

	void	fmtBootComplete(int64_t=0);
	bool	xtrBootComplete();
	void	fmtBootCompleteReply(int64_t=0);
	bool	xtrBootCompleteReply();

	void	fmtBootAbort(int64_t=0);
	bool	xtrBootAbort();
	void	fmtBootAbortReply(int64_t=0);
	bool	xtrBootAbortReply();

	static string cpType2string(CpType);
	static string cpMode2string(CpMode);
	static bool string2cpType(string&, CpType&);
	static bool string2cpMode(string&, CpMode&);

	string	avPair2string(CpAttr); 
	string	toString(); 

	CpType	type;			///< control packet type 
	CpMode	mode;			///< request/return type
	int64_t seqNum;			///< sequence number

	char*	payload;		///< pointer to start of packet payload
	int	paylen;			///< payload length in bytes
private:
	char*	next;			///< pointer to next byte in payload
};

/** Format the base fields of the control packet.
 *  Packs the type, mode and sequence number into the payload region
 *  of the associated packet object.
 */
inline void CtlPkt::fmtBase() {
	next = payload;
	*((int16_t*) next)=htons((int16_t) type);	next+=sizeof(int16_t);
	*((int16_t*) next)=htons((int16_t) mode);	next+=sizeof(int16_t);
	*((int32_t*) next)=htonl((int32_t) ((seqNum >> 32) & 0xffffffff));
							next+=sizeof(int32_t);
	*((int32_t*) next)=htonl((int32_t) (seqNum & 0xffffffff));
							next+=sizeof(int32_t);
	paylen = next - payload;
}

/** Extract the base fields of the control packet.
 *  Extracts the type, mode and sequence number into the payload region
 *  of the associated packet object.
 */
inline bool CtlPkt::xtrBase() {
	next = payload;
	type = (CpType) ntohs(*((int16_t*) next)); next += sizeof(int16_t);
        mode = (CpMode) ntohs(*((int16_t*) next)); next += sizeof(int16_t);
        seqNum = ntohl(*((uint32_t*) next)); next += sizeof(uint32_t);
        seqNum <<= 32;
        seqNum  |= ntohl(*((uint32_t*) next)); next += sizeof(uint32_t);
	return (paylen >= next - payload);
}

/** Update the sequence number field of a control packet.
 */
inline void CtlPkt::updateSeqNum() {
	char* p = payload + 2*sizeof(int16_t);
	*((int32_t*) p) = htonl((int32_t) ((seqNum >> 32) & 0xffffffff));
	p += sizeof(int32_t);
	*((int32_t*) p) = htonl((int32_t) (seqNum & 0xffffffff));
}

/** Add an (attribute, length, value) tuple to the packet.
 *  @param x is a value to be added to the packet.
 */
inline void CtlPkt::put(int16_t x) {
	*((int16_t*) next)=htons(s16); 			next+=sizeof(int16_t);
	*((int16_t*) next)=htons((int16_t) sizeof(x));	next+=sizeof(int16_t);
	*((int16_t*) next)=htons(x); 			next+=sizeof(x);
}

/** Add an (attribute, length, value) tuple to the packet.
 *  @param x is a value to be added to the packet.
 */
inline void CtlPkt::put(uint16_t x) {
	*((int16_t*) next)=htons(u16); 			next+=sizeof(int16_t);
	*((int16_t*) next)=htons((int16_t) sizeof(x));	next+=sizeof(int16_t);
	*((uint16_t*) next)=htons(x); 			next+=sizeof(x);
}

/** Add an (attribute, length, value) tuple to the packet.
 *  @param x is a value to be added to the packet.
 */
inline void CtlPkt::put(int32_t x) {
	*((int16_t*) next)=htons(s32); 			next+=sizeof(int16_t);
	*((int16_t*) next)=htons((int16_t) sizeof(x));	next+=sizeof(int16_t);
	*((int32_t*) next)=htonl(x); 			next+=sizeof(x);
}

/** Add an (attribute, length, value) tuple to the packet.
 *  @param x is a value to be added to the packet.
 */
inline void CtlPkt::put(uint32_t x) {
	*((int16_t*) next)=htons(u32); 			next+=sizeof(int16_t);
	*((int16_t*) next)=htons((int16_t) sizeof(x));	next+=sizeof(int16_t);
	*((uint32_t*) next)=htonl(x); 			next+=sizeof(x);
}

/** Add an (attribute, length, value) tuple to the packet.
 *  @param x is a value to be added to the packet.
 */
inline void CtlPkt::put(int64_t x) {
	*((int16_t*) next)=htons(s64); 			next+=sizeof(int16_t);
	*((int16_t*) next)=htons((int16_t) sizeof(x));	next+=sizeof(int16_t);
	*((int32_t*) next)=htonl((int32_t) ((x >> 32) & 0xffffffff));
							next+=sizeof(int32_t);
	*((int32_t*) next)=htonl((int32_t) (x & 0xffffffff));
							next+=sizeof(int32_t);
}

/** Add an (attribute, length, value) tuple to the packet.
 *  @param x is a value to be added to the packet.
 */
inline void CtlPkt::put(uint64_t x) {
	*((int16_t*) next)=htons(u64); 			next+=sizeof(int16_t);
	*((int16_t*) next)=htons((int16_t) sizeof(x));	next+=sizeof(int16_t);
	*((int32_t*) next)=htonl((uint32_t) ((x >> 32) & 0xffffffff));
							next+=sizeof(int32_t);
	*((int32_t*) next)=htonl((uint32_t) (x & 0xffffffff));
							next+=sizeof(int32_t);
}

/** Add an (attribute, length, value) tuple to the packet.
 *  @param x is a value to be added to the packet.
 */
inline void CtlPkt::put(Forest::ntyp_t x) {
	*((int16_t*) next)=htons(u64); 			next+=sizeof(int16_t);
	*((int16_t*) next)=htons((int16_t) sizeof(int16_t));
							next+=sizeof(int16_t);
	*((int16_t*) next)=htons((int16_t) x); 		next+=sizeof(int16_t);
}

/** Add an (attribute, length, value) tuple to the packet.
 *  @param rs is a value to be added to the packet.
 */
inline void CtlPkt::put(const RateSpec& rs) {
	*((int16_t*) next)=htons(rateSpec);		next+=sizeof(int16_t);
	*((int16_t*) next)=htons((int16_t) sizeof(rs));	next+=sizeof(int16_t);
	*((int32_t*) next)=htonl(rs.bitRateUp);		next+=sizeof(uint32_t);
	*((int32_t*) next)=htonl(rs.bitRateDown);	next+=sizeof(uint32_t);
	*((int32_t*) next)=htonl(rs.pktRateUp);		next+=sizeof(uint32_t);
	*((int32_t*) next)=htonl(rs.pktRateDown);	next+=sizeof(uint32_t);
}

/** Add an (attribute, length, value) tuple to the packet.
 *  @param s is a value to be added to the packet.
 */
inline void CtlPkt::put(const string& s) {
	*((int16_t*) next) = htons(attrString);	next+=sizeof(int16_t);
	*((int16_t*) next) = htons((int16_t) s.length()); next+=sizeof(int16_t);
	std::copy(s.c_str(),s.c_str()+s.length(),next);	next+=s.length();
}

/** Add an (attribute, length, value) tuple to the packet.
 *  @param v is a value to be added to the packet.
 */
inline void CtlPkt::put(const vector<int32_t>& v) {
	*((int16_t*) next) = htons(intVec);		next+=sizeof(int16_t);
	*((int16_t*) next) = htons((int16_t) v.size()*sizeof(int32_t));
							next+=sizeof(int16_t);
	std::copy(v.begin(),v.end(),(int32_t*) next);
						next+=v.size()*sizeof(int32_t);
}

/** Determine how much space is available for the next attribute.
 *  @return the number of bytes that can be used by the next attribute
 *  to be packed (after accounting for the associated overhead fields).
 */
inline int CtlPkt::availSpace() {
	return (payload + (Forest::MAX_PLENG-Forest::OVERHEAD))
		- (next + 2*sizeof(int16_t));
}


/** Get an (attribute, length, value) tuple from the packet.
 *  @param x used to return the retrieved value
 */
inline bool CtlPkt::get(int16_t& x) {
	if (ntohs(*((int16_t*) next)) != s16) return false;
	next += sizeof(int16_t);
	if (ntohs(*((int16_t*) next)) != sizeof(x)) return false;
	next += sizeof(int16_t);
	x = ntohs(*((int16_t*) next));
	next += sizeof(x);
	return true;
}

/** Get an (attribute, length, value) tuple from the packet.
 *  @param x used to return the retrieved value
 */
inline bool CtlPkt::get(uint16_t& x) {
	if (ntohs(*((int16_t*) next)) != u16) return false;
	next += sizeof(int16_t);
	if (ntohs(*((int16_t*) next)) != sizeof(x)) return false;
	next += sizeof(int16_t);
	x = ntohs(*((uint16_t*) next));
	next += sizeof(x);
	return true;
}

/** Get an (attribute, length, value) tuple from the packet.
 *  @param x used to return the retrieved value
 */
inline bool CtlPkt::get(int32_t& x) {
	if (ntohs(*((int16_t*) next)) != s32) return false;
	next += sizeof(int16_t);
	if (ntohs(*((int16_t*) next)) != sizeof(x)) return false;
	next += sizeof(int16_t);
	x = ntohl(*((int32_t*) next));
	next += sizeof(x);
	return true;
}

/** Get an (attribute, length, value) tuple from the packet.
 *  @param x used to return the retrieved value
 */
inline bool CtlPkt::get(uint32_t& x) {
	if (ntohs(*((int16_t*) next)) != u32) return false;
	next += sizeof(int16_t);
	if (ntohs(*((int16_t*) next)) != sizeof(x)) return false;
	next += sizeof(int16_t);
	x = ntohl(*((uint32_t*) next));
	next += sizeof(x);
	return true;
}

/** Get an (attribute, length, value) tuple from the packet.
 *  @param x used to return the retrieved value
 */
inline bool CtlPkt::get(int64_t& x) {
	if (ntohs(*((int16_t*) next)) != s64) return false;
	next += sizeof(int16_t);
	if (ntohs(*((int16_t*) next)) != sizeof(x)) return false;
	next += sizeof(int16_t);
	x = ntohl(*((int64_t*) next));
	next += sizeof(x);
	return true;
}

/** Get an (attribute, length, value) tuple from the packet.
 *  @param x used to return the retrieved value
 */
inline bool CtlPkt::get(uint64_t& x) {
	if (ntohs(*((int16_t*) next)) != u64) return false;
	next += sizeof(int16_t);
	if (ntohs(*((int16_t*) next)) != sizeof(x)) return false;
	next += sizeof(int16_t);
	x = ntohl(*((uint64_t*) next));
	next += sizeof(x);
	return true;
}

/** Get an (attribute, length, value) tuple from the packet.
 *  @param x used to return the retrieved value
 */
inline bool CtlPkt::get(Forest::ntyp_t& x) {
	if (ntohs(*((int16_t*) next)) != u32) return false;
	next += sizeof(int16_t);
	if (ntohs(*((int16_t*) next)) != sizeof(int16_t)) return false;
	next += sizeof(int16_t);
	x = (Forest::ntyp_t) ntohs(*((uint16_t*) next));
	next += sizeof(int16_t);
	return true;
}

/** Get an (attribute, length, value) tuple from the packet.
 *  @param rs used to return the retrieved value
 */
inline bool CtlPkt::get(RateSpec& rs) {
	if (ntohs(*((int16_t*) next)) != rateSpec) return false;
	next += sizeof(int16_t);
	if (ntohs(*((int16_t*) next)) != sizeof(rs)) return false;
	next += sizeof(int16_t);
	rs.bitRateUp   = ntohl(*((int64_t*) next)); next += sizeof(uint32_t);
	rs.bitRateDown = ntohl(*((int64_t*) next)); next += sizeof(uint32_t);
	rs.pktRateUp   = ntohl(*((int64_t*) next)); next += sizeof(uint32_t);
	rs.pktRateDown = ntohl(*((int64_t*) next)); next += sizeof(uint32_t);
	return true;
}

/** Get an (attribute, length, value) tuple from the packet.
 *  @param s used to return the retrieved value
 */
inline bool CtlPkt::get(string& s) {
	if (ntohs(*((int16_t*) next)) != attrString) return false;
	next += sizeof(int16_t);
	int len = ntohs(*((int16_t*) next));
	next += sizeof(int16_t);
	std::copy(next, next+len, s.begin());
	next += len;
	return true;
}

/** Get an (attribute, length, value) tuple from the packet.
 *  @param v is used to return the retrieved value
 */
inline bool CtlPkt::get(vector<int32_t>& v) {
	if (ntohs(*((int16_t*) next)) != attrString) return false;
	next += sizeof(int16_t);
	int len = ntohs(*((int16_t*) next))/sizeof(int32_t);
	next += sizeof(int16_t);
	int32_t* vv = (int32_t*) next;
	std::copy(vv, vv+len, v.begin());
	next += len * sizeof(int32_t);
	return true;
}

} // ends namespace

#endif
