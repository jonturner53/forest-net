/** @file CtlPkt.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef CTLPKT_H
#define CTLPKT_H

#include "Forest.h"
#include "Packet.h"
#include "RateSpec.h"

namespace forest {


/** This class provides a mechanism for handling forest signalling packets.
 *  Signalling packets have a packet type of CLIENT_SIG or NET_SIG in the
 *  first word of the forest head. The payload identifies the specific
 *  type of packet. Every control packet includes the following three fields.
 *
 *  control packet type		identifies the specific type of packet
 *  mode 			specifies that this is a request packet,
 *				a positive reply, or a negative reply
 *  sequence number		this field is set by the originator of any
 *				request packet, and the same value is returned
 *				in the reply packet; this allows the requestor
 *				to match repies with earlier requests
 *
 *  The remainder of a control packet payload consists of (attribute,value)
 *  pairs. Each attribute is identified by a 32 bit code and the size
 *  and type of the values is determined by the attribute code. Currently,
 *  most attributes are 32 bit integers. Some are RateSpecs.
 *  The control packet object contains fields for each of the possible
 *  attribute values and these fields can be directly read or written.
 *  Pack and unpack methods are provided to packet these fields into
 *  a payload of a packet buffer, or unpack attribute values from a
 *  packet buffer.
 */
class CtlPkt {
public:
	/** Control packet types */
	enum CpType {
		UNDEF_CPTYPE = 0,

		CLIENT_ADD_COMTREE = 10, CLIENT_DROP_COMTREE = 11,
		CLIENT_GET_COMTREE = 12, CLIENT_MOD_COMTREE = 13,
		CLIENT_JOIN_COMTREE = 14, CLIENT_LEAVE_COMTREE = 15,
		CLIENT_RESIZE_COMTREE = 16,
		CLIENT_GET_LEAF_RATE = 17, CLIENT_MOD_LEAF_RATE = 18,

		CLIENT_NET_SIG_SEP = 29,

		ADD_IFACE = 30, DROP_IFACE = 31,
		GET_IFACE = 32, MOD_IFACE = 33,

		ADD_LINK = 40, DROP_LINK = 41,
		GET_LINK = 42, MOD_LINK = 43,

		ADD_COMTREE = 50, DROP_COMTREE = 51,
		GET_COMTREE = 52, MOD_COMTREE = 53,

		ADD_COMTREE_LINK = 54, DROP_COMTREE_LINK = 55,
		MOD_COMTREE_LINK = 56, GET_COMTREE_LINK = 57,

		ADD_ROUTE = 70, DROP_ROUTE = 71,
		GET_ROUTE = 72, MOD_ROUTE = 73,
		ADD_ROUTE_LINK = 74, DROP_ROUTE_LINK = 75,

		NEW_SESSION = 100, CANCEL_SESSION = 103,
		CLIENT_CONNECT = 101, CLIENT_DISCONNECT = 102,

		SET_LEAF_RANGE = 110, CONFIG_LEAF = 111,

		BOOT_ROUTER = 120, BOOT_COMPLETE = 121, BOOT_ABORT = 122,
		BOOT_LEAF = 123
	};

	// Control packet attribute types
	enum CpAttr {
		UNDEF_ATTR = 0,
		ADR1 = 1, ADR2 = 2, ADR3 = 3,
		IP1 = 5, IP2 = 6,
		PORT1 = 9, PORT2 = 10,
		RSPEC1 = 13, RSPEC2 = 14,
		NONCE = 17,
		CORE_FLAG = 20,
		IFACE = 21,
		LINK = 22,
		NODE_TYPE = 23,
		COMTREE = 50,
		COMTREE_OWNER = 51,
		COUNT = 70,
		QUEUE = 71,
		ZIPCODE = 72,
		ERRMSG = 100
	};
	static const int MAX_STRING = 200;

	/** Control packet modes */
	enum CpMode {
		UNDEF_MODE = 0, REQUEST = 1, POS_REPLY = 2, NEG_REPLY = 3
	};

	// constructors/destructor
		CtlPkt();
		CtlPkt(const Packet&);
		CtlPkt(uint32_t*,int);
		CtlPkt(CpType,CpMode,uint64_t);
		CtlPkt(CpType,CpMode,uint64_t,uint32_t*);
		~CtlPkt();

	// resetting control packet
	void	reset();
	void	reset(const Packet&);
	void	reset(uint32_t*, int);
	void	reset(CpType,CpMode,uint64_t);
	void	reset(CpType,CpMode,uint64_t,uint32_t*);

	int	pack();	
	bool	unpack();	

	string& typeName(string&);
	string& modeName(string&);
	string&	avPair2string(CpAttr, string&); 
	string&	toString(string&); 

	CpType	type;			///< control packet type 
	CpMode	mode;			///< request/return type
	int64_t seqNum;			///< sequence number

	fAdr_t	adr1, adr2, adr3;	///< forest address fields
	ipa_t	ip1, ip2;		///< ip address fields
	ipp_t	port1, port2;		///< ip port number fields
	RateSpec rspec1, rspec2;	///< rate specs
	uint64_t nonce;			///< nonce used when connecting links
	int8_t	coreFlag;		///< comtree core flag
	int	iface;			///< interface number
	int	link;			///< link number
	Forest::ntyp_t nodeType;	///< type of a node
	comt_t	comtree;		///< comtree number
	fAdr_t	comtreeOwner;		///< comtree owner
	int	count;			///< count field
	int	queue;			///< queue number
	int	zipCode;		///< zip code

	uint32_t* payload;		///< pointer to start of packet payload
	int	paylen;			///< payload length in bytes

	string	errMsg;			///< buffer for error messages

private:
};

} // ends namespace


#endif
