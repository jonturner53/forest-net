/** @file CtlPkt.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef CTLPKT_H
#define CTLPKT_H

#include "CommonDefs.h"
#include "CpType.h"
//#include "PacketStore.h"

enum CpRrType { REQUEST=1, POS_REPLY=2, NEG_REPLY=3 };

/** This class provides a mechanism for handling forest signalling packets.
 *  Signalling packets have a packet type of CLIENT_SIG or NET_SIG in the
 *  first word of the forest head. The payload identifies the specific
 *  type of packet. Every control packet includes the following three fields.
 *
 *  request/response type	specifies that this is a request packet,
 *				a positive reply, or a negative reply
 *  control packet type		identifies the specific type of packet
 *  sequence number		this field is set by the originator of any
 *				request packet, and the same value is returned
 *				in the reply packet; this allows the requestor
 *				to match repies with earlier requests; the
 *				target of the request does not use or interpret
 *				the value, so the requestor can use it however
 *				it finds most convenient
 *
 *  The remainder of a control packet payload consists of (attribute,value)
 *  pairs. Each attribute is identified by a 32 bit code and each value
 *  is a 32 bit integer.
 *
 *  To format a control packet, a requestor creates a new control packet
 *  object, sets the three fields listed above using the provided
 *  set methods, and sets the appropriate attriutes using the setAttr()
 *  method. For any specific control packets, some attributes are
 *  required, others are optional. Once the control packet object has
 *  been initialized, the information is transferred to a packet using
 *  the pack() method; the argument to the pack method is a pointer to
 *  the start of the packet payload.
 *
 *  To extract information from a control packet, the receiver first
 *  uses the unpack() method to transfer the information from the packet
 *  payload to a control packet object. The provided get methods can then
 *  be used to extract field values and attributes from the control packet.
 *
 *  There are two support classes used by CtlPkt: CpType and CpAttr.
 *  CpType defines all the control packet types. For each type, it defines
 *  four quantities, an index, a code, a command phrase and an abbreviation.
 *  Control packet indices are sequential integers and are generally used
 *  with programs to access type information. The control packet code is the
 *  value that is actually sent in a control packet to identify what type
 *  of packet it is. The command phrase is a descriptive phrase such as
 *  "get comtree" or "modify route" and the abbreviation is a corresponding
 *  short form (gc for "get comtree" for example).
 *
 *  The CpAttr class is similar. For each attribute it defines an index
 *  a code and a name.
 */
class CtlPkt {
public:
		CtlPkt();
		CtlPkt(CpTypeIndex,CpRrType,uint64_t);
		~CtlPkt();

	void	reset();
	void	reset(CpTypeIndex,CpRrType,uint64_t);
	int	pack(uint32_t*);	
	bool	unpack(uint32_t*, int);	
	/** predicates */
	bool	isSet(CpAttrIndex); 	

	/** getters */
	CpTypeIndex getCpType();
	CpRrType    getRrType();
	int64_t     getSeqNum();
	int32_t	    getAttr(CpAttrIndex);
	char*	    getErrMsg();	

	/** setters */
	void	setCpType(CpTypeIndex);
	void	setCpCode(int);
	void	setRrType(CpRrType);
	void	setSeqNum(int64_t);
	void	setAttr(CpAttrIndex, int32_t);
	void	setErrMsg(const char*);	

	/** input/output */
	string&	avPair2string(CpAttrIndex, string&); 
	string&	toString(string&); 
private:
	CpTypeIndex cpType;		///< control packet type index
	CpRrType rrType;		///< request/return type
	int64_t seqNum;			///< sequence number

	int32_t aVal[CPA_END+1];	///< array of attribute values
	bool 	aSet[CPA_END+1];	///< mark attributes that have been set

	uint32_t* payload;		///< pointer to start of packet payload
	int	pp;			///< index in payload used by pack/unpack

	static const int MAX_MSG_LEN=500; ///< bound on error message length
	char	errMsg[MAX_MSG_LEN+1];	///< buffer for error messages

	// helper methods
	void	packAttr(CpAttrIndex);
	void	packAttrCond(CpAttrIndex);
	CpAttrIndex unpackAttr();
};

/** Check if specified attribute is set.
 *  @param i index of desired attribute
 *  @return true if i is a valid attribute index and attribute has been set
 */
inline bool CtlPkt::isSet(CpAttrIndex i) {
	return CpAttr::validIndex(i) && aSet[i];
}

/** Get the type index of a control packet.
 *  @return true the type index.
 */
inline CpTypeIndex CtlPkt::getCpType() { return cpType; }

/** Get the request/return type of the packet.
 *  @return true the request/return type
 */
inline CpRrType CtlPkt::getRrType() { return rrType; }

/** Get the sequence number of the packet.
 *  @return true the sequence number
 */
inline int64_t CtlPkt::getSeqNum() { return seqNum; }

/** Get value of specified attribute
 *  @param i index of desired attribute
 *  @return corresponding value
 *  Assumes that specified attribute has a valid value.
 */
inline int32_t CtlPkt::getAttr(CpAttrIndex i) {
	return (isSet(i) ? aVal[i] : 0);
}

/** Set value of specified attribute
 *  @param i index of desired attribute
 *  @param val desired value for attribute
 */
inline void CtlPkt::setAttr(CpAttrIndex i, int32_t val) {
	if (!CpAttr::validIndex(i)) return;
	aVal[i] = val; aSet[i] = true;
}

/** Set the type index of a control packet.
 *  @param t is the specified type index
 */
inline void CtlPkt::setCpType(CpTypeIndex t) { cpType = t; }

/** Set the type request/return type of a control packet.
 *  @param rr is the specified request/return type 
 */
inline void CtlPkt::setRrType(CpRrType rr) { rrType = rr; }

/** Set the type sequence number of a control packet.
 *  @param s is the specified sequence number
 */
inline void CtlPkt::setSeqNum(int64_t s) { seqNum = s; }

/** Packs a single (attribute_code, value) pair starting at word i in payload.
 *  @param attr attribute code for the pair
 *  @param val value part of the pair
 *  @returns index of next available word
 */
inline void CtlPkt::packAttr(CpAttrIndex i) {
        payload[pp++] = htonl(CpAttr::getCode(i));
	payload[pp++] = htonl(aVal[i]);
}

/** Packs a single (attribute_code, value) conditionally.
 *  If the val parameter is non-zero, pack the pair.
 *  @param attr attribute code for the pair
 *  @param val value part of the pair
 *  @returns index of next available word
 */
inline void CtlPkt::packAttrCond(CpAttrIndex i) {
        if (aSet[i]) packAttr(i);
}

/** Unpacks a single (attribute, value) pair starting at word pp in payload.
 *  The unpacked value is stored in the CtlPkt object and can
 *  be retrieved using the getAttr() method.
 *  @returns attribute index of unpacked pair, or 0 for invalid index
 */
inline CpAttrIndex CtlPkt::unpackAttr() {
        CpAttrIndex i = CpAttr::getIndexByCode(ntohl(payload[pp]));
	if (!CpAttr::validIndex(i)) return CPA_START; // =0
	pp++;
	setAttr(i,ntohl(payload[pp++]));
	return i;
}

/** Returns pointer to error message */
inline char* CtlPkt::getErrMsg() { return errMsg; }

/** Set the error message for a negative reply */
inline void CtlPkt::setErrMsg(const char* s) {
	strncpy(errMsg,s,MAX_MSG_LEN);errMsg[MAX_MSG_LEN] = 0;
}

#endif
