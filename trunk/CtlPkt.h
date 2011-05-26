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
#include "PacketStore.h"

enum CpRrType { REQUEST=1, POS_REPLY=2, NEG_REPLY=3 };

/** This class provides a simple mechanism for handling forest control packets.
 *  The basic structure contains various named fields, all of type int32.
 *  To format a control packet, construct a new CtlPkt and set the
 *  desired fields to non-zero values. For the few fields for which
 *  0 is valid value, use -1. This includes flag fields, such as coreFlag,
 *  where 0 would normally denote false.
 *
 *  Format of control packets.
 *
 *  The packet type in the first word of the forest header must be
 *  CLIENT_SIG or NET_SIG.
 *
 *  The first word of the payload contains the packet's request/return
 *  type in its two high order bits. The next 14 bits are the type code
 *  for the packet. The remainder of the first payload word, plus the
 *  second payload word form a sequence number field. This is not used
 *  by the target of a control packet, but is returned as part of the
 *  reply, so that control packet senders can easily associate replies
 *  with control packets sent earlier.
 *  The remainder of the payload contains the body of the control
 *  packet in the form of a set of (attribute code, value) pairs;
 *  both attribute codes and values are encoded as 32 bit integer values.
 */
class CtlPkt {
public:
		CtlPkt(buffer_t);
		~CtlPkt();

	void	reset(buffer_t);
	int	pack();	
	bool	unpack(int);	
	/** predicates */
	bool	isSet(CpAttrIndex); 	

	/** getters */
	CpTypeIndex getCpType();
	int 	    getCpCode();
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
	void	write(ostream&) ; 
private:
	CpTypeIndex cpType;		///< control packet type index
	int cpCode;			///< control packet type code
	CpRrType rrType;		///< request/return type
	int64_t seqNum;			///< sequence number

	int32_t aVal[CPA_END+1];	///< array of attribute values
	bool 	aSet[CPA_END+1];	///< mark attributes that have been set

	uint32_t *buf;			///< reference to packet buffer
	int	bp;			///< index into buf used by pack/unpack

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

/** Get the conrol packet code number of a control packet.
 *  The code number is the value that is sent in packet headers
 *  to identify the type of a control packet.
 *  @return true the code value
 */
inline int CtlPkt::getCpCode() { return cpCode; }

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
	return aVal[i];
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

/** Set the type type code of a controle packet.
 *  @param c is the specified type code
 */
inline void CtlPkt::setCpCode(int c) { cpCode = c; }

/** Set the type request/return type of a control packet.
 *  @param rr is the specified request/return type 
 */
inline void CtlPkt::setRrType(CpRrType rr) { rrType = rr; }

/** Set the type sequence number of a control packet.
 *  @param s is the specified sequence number
 */
inline void CtlPkt::setSeqNum(int64_t s) { seqNum = s; }

/** Packs a single (attribute_code, value) pair starting at word i in buf.
 *  @param attr attribute code for the pair
 *  @param val value part of the pair
 *  @returns index of next available word
 */
inline void CtlPkt::packAttr(CpAttrIndex i) {
        buf[bp++] = htonl(CpAttr::getCode(i));
	buf[bp++] = htonl(aVal[i]);
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

/** Unpacks a single (attribute, value) pair starting at word bp in buf.
 *  The unpacked value is stored in within the CtlPkt object and can
 *  be retrieved using the getAttr() method.
 *  @returns attribute index of unpacked pair, or 0 for invalid index
 */
inline CpAttrIndex CtlPkt::unpackAttr() {
        CpAttrIndex i = CpAttr::getIndexByCode(ntohl(buf[bp]));
	if (!CpAttr::validIndex(i)) return CPA_START; // =0
	bp++;
	setAttr(i,ntohl(buf[bp++]));
	return i;
}

/** Returns pointer to error message */
inline char* CtlPkt::getErrMsg() { return errMsg; }

/** Set the error message for a negative reply */
inline void CtlPkt::setErrMsg(const char* s) {
	strncpy(errMsg,s,MAX_MSG_LEN);errMsg[MAX_MSG_LEN] = 0;
}

#endif
