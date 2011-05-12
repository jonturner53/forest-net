/** \file CtlPkt.h
 *  Header file for CtlPkt class.
 *  This class provides a simple mechanism for handling forest control packets.
 *  The basic structure contains various named fields, all of type int32.
 *  To format a control packet, construct a new CtlPkt and set the
 *  desired fields to non-zero values. For the few fields for which
 *  0 is valid value, use -1. This includes flag fields, such as coreFlag,
 *  where 0 would normally denote false.
 *
 *  Format of control packets.
 *  The packet type in the first word of the forest header must be
 *  CLIENT_SIG or NET_SIG. The first word of the payload is the control
 *  packet type. The second word is a request/response type (1 for
 *  a request packet, 2 for a positive response, 3 for a negative response)
 *  and the third word is opaque data that is returned to the sender
 *  (typically used for a sequence number). After that comes the body of
 *  of the message in the form of a set of (attribute code, value) pairs,
 *  all of which are encoded as 32 bit integer values.
 */

#ifndef FCTLPKT_H
#define FCTLPKT_H

#include "forest.h"
#include "CpType.h"
#include "pktStore.h"

enum CpRrType { REQUEST=1, POS_REPLY=2, NEG_REPLY=3 };

class CtlPkt {
public:
		CtlPkt(buffer_t);
		~CtlPkt();
	void	reset(buffer_t);
	int	pack();			///< pack CtlPkt fields into packet
	bool	unpack(int);		///< unpack CtlPkt fields from packet
	void	print(ostream&) ; 	///< print CtlPkt

	int32_t	getAttr(CpAttrIndex);	///< return value of specified attribute
	void	setAttr(CpAttrIndex, int32_t); ///< set value of specified attribute
	bool	isSet(CpAttrIndex); 	///< return true if specified attribute has a value

	char*	getErrMsg();		///< return pointer to error message
	void	setErrMsg(const char*);	///< set error message in control packet

	// CtlPkt fields
	int32_t bitRate;
	int32_t bitRateUp;
	int32_t bitRateDown;
	int32_t	comtree;
	int32_t	comtreeOwner;
	int32_t	coreFlag;
	int32_t	destAdr;
	int32_t extBitRateUp;
	int32_t extBitRateDown;
	int32_t extPktRateUp;
	int32_t extPktRateDown;
	int32_t iface;
	int32_t intBitRateUp;
	int32_t intBitRateDown;
	int32_t intPktRateUp;
	int32_t intPktRateDown;
	int32_t	leafAdr;
	int32_t leafCount;
	int32_t link;
	int32_t	localIP;
	int32_t maxBitRate;
	int32_t maxPktRate;
	int32_t parentLink;
	int32_t	peerAdr;
	int32_t	peerDest;
	int32_t	peerIP;
	int32_t	peerPort;
	int32_t peerType;
	int32_t pktRate;
	int32_t pktRateUp;
	int32_t pktRateDown;
	int32_t queue;

	CpTypeIndex cpType;
	int cpCode;
	CpRrType rrType;
	int64_t seqNum;
private:
	uint32_t *buf;			///< reference to packet buffer
	int	bp;			///< index into buffer used by pack/unpack
	int32_t aVal[CPA_END+1];	///< array of attribute values
	bool 	aSet[CPA_END+1];	///< used to mark attributes that have been set
	static const int MAX_MSG_LEN=500;
	char	errMsg[MAX_MSG_LEN+1];

	// helper methods
	void	packAttr(CpAttrIndex);
	void	packAttrCond(CpAttrIndex);
	CpAttrIndex unpackAttr();
	void	condPrint(ostream&, int32_t, const char*); 
};

/** Check if specified attribute is set.
 *  @param i index of desired attribute
 *  @return true if i is a valid attribute index and attribute has been set
 */
inline bool CtlPkt::isSet(CpAttrIndex i) {
	return CpAttr::validIndex(i) && aSet[i];
}

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
