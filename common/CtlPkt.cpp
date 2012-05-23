/** @file CtlPkt.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

/**
 *
 *  The CtlPkt class is used to pack and unpack forest control messages.
 *  The class has a public field for every field that can be used in
 *  in a control packet. To create a control packet, the user
 *  constructs a CtlPkt object. The user then specifies
 *  selected fields and calls pack, which packs the specified
 *  fields into the packet's payload the length of the payload.
 *  
 *  To unpack a buffer, the user constructs a CtlPkt object
 *  and then calls unpack, specifying the length of the payload.
 *  The control packet fields can then be retrieved from the
 *  corresponding fields of the CtlPkt object.
 */

#include "CtlPkt.h"

/** Constructor for CtlPkt.
 */
CtlPkt::CtlPkt() { payload = 0; reset(); }

CtlPkt::CtlPkt(CpTypeIndex cpt, CpRrType rrt, uint64_t seq) 
		: cpType(cpt), rrType(rrt), seqNum(seq) {
	payload = 0; reset();
}

/** Destructor for CtlPkt. */
CtlPkt::~CtlPkt() { } 

/** Clears CtlPkt fields for re-use with a new buffer.
 */
void CtlPkt::reset() {
	for (int i = CPA_START; i < CPA_END; i++) aSet[i] = false;
}
void CtlPkt::reset(CpTypeIndex cpt, CpRrType rrt, uint64_t seq) {
	cpType = cpt; rrType = rrt; seqNum = seq;
	payload = 0; reset();
}

/** Pack CtlPkt fields into buffer.
 *  @param pl is pointer to start of payload
 *  @return the length of the packed payload in bytes.
 *  or 0 if an error was detected.
 */
int CtlPkt::pack(uint32_t* pl) {
	payload = pl;
	if (!CpType::validIndex(cpType))
		return 0;
	if (rrType != REQUEST && rrType != POS_REPLY && rrType != NEG_REPLY)
		return 0;
	pp = 0;
	payload[pp++] = htonl(CpType::getCode(cpType));
	payload[pp++] = htonl(rrType);
	payload[pp++] = htonl((uint32_t) (seqNum >> 32));
	payload[pp++] = htonl((uint32_t) (seqNum & 0xffffffff));

	if (rrType == REQUEST) {
		// pack all request attributes and confirm that
		// required attributes are present
		for (int i = CPA_START + 1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (CpType::isReqAttr(cpType, ii)) {
				if (isSet(ii)) packAttr(ii);
				else if (CpType::isReqReqAttr(cpType,ii))
					return 0;
			}
		}
	} else if (rrType == POS_REPLY) {
		for (int i = CPA_START + 1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (CpType::isRepAttr(cpType, ii)) {
				if (isSet(ii)) packAttr(ii);
				else { 
					return 0;
				}
			}
		}
	} else {
		int j = Util::strnlen(errMsg,MAX_MSG_LEN);
		if (j == MAX_MSG_LEN) errMsg[MAX_MSG_LEN] = 0;
		strncpy((char*) &payload[pp], errMsg, j+1);
		return 4*pp + j+1;
	}

	return 4*pp;
}

/** Unpack CtlPkt fields from the packet payload.
 *  @param pl is pointer to start of payload
 *  @param pleng is the length of the payload in bytes
 *  @return true on success, 0 on failure
 */
bool CtlPkt::unpack(uint32_t* pl, int pleng) {
	payload = pl;
	pleng /= 4; // to get length in 32 bit words
	if (pleng < 4) return false; // too short for control packet

	pp = 0;
	cpType = CpType::getIndexByCode(ntohl(payload[pp++]));
	rrType = (CpRrType) ntohl(payload[pp++]);
	seqNum = ntohl(payload[pp++]); seqNum <<= 32;
	seqNum |= ntohl(payload[pp++]);

	if (!CpType::validIndex(cpType)) return false;
	if (rrType != REQUEST && rrType != POS_REPLY && rrType != NEG_REPLY)
		return false;
	
	if (rrType == NEG_REPLY) {
		strncpy(errMsg,(char*) &payload[pp], MAX_MSG_LEN);
		errMsg[MAX_MSG_LEN] = 0;
		return true;
	}

	// unpack all attribute/value pairs
	while (pp < pleng-1) { if (unpackAttr() == 0) return false; }

	if (rrType == REQUEST) {
		// confirm that required attributes are present
		for (int i = CPA_START + 1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (CpType::isReqReqAttr(cpType, ii) && !isSet(ii)) {
				return false;
			}
		}
	} else {
		// confirm that all reply attributes are present
		for (int i = CPA_START + 1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (CpType::isRepAttr(cpType, ii) && !isSet(ii)) {
				return false;
			}
		}
	}

	return true;
}

/** Create a string representing an (attribute,value) pair.
 *  @param ii is the index of an attribute
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& CtlPkt::avPair2string(CpAttrIndex ii, string& s) {
	stringstream ss;
	ss << CpAttr::getName(ii) << "=";
	if (!isSet(ii)) {
		ss << "(missing)"; s = ss.str(); return s;
	}
	int32_t val = getAttr(ii);
	if (ii == COMTREE_OWNER || ii == LEAF_ADR ||
	    ii == PEER_ADR || ii == PEER_DEST ||
	    ii == RTR_ADR || ii == CLIENT_ADR ||
	    ii == FIRST_LEAF_ADR || ii == LAST_LEAF_ADR ||
	    ii == DEST_ADR) {
		ss << Forest::fAdr2string((fAdr_t) val,s);
	} else if (ii == LOCAL_IP || ii == PEER_IP ||
		   ii == CLIENT_IP || ii == RTR_IP) {
		ss << Np4d::ip2string(val,s);
	} else if (ii == PEER_TYPE) {
		ss << Forest::nodeType2string((ntyp_t) val,s);
	} else {
		ss << val;
	}
	s = ss.str();
	return s;
}

/** Create a string representing the control packet header fields.
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& CtlPkt::toString(string& s) {
	stringstream ss;
	bool reqPkt = (rrType == REQUEST);
	bool replyPkt = !reqPkt;
	bool success = (rrType == POS_REPLY);

	ss << CpType::getName(cpType);
	if (reqPkt) ss << " (request,";
	else if (rrType == POS_REPLY) ss << " (pos reply,";
	else ss << " (neg reply,";
	ss << seqNum << "):";

	if (rrType == REQUEST) {
		for (int i = CPA_START+1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (!CpType::isReqAttr(cpType,ii)) continue;
			if (!CpType::isReqReqAttr(cpType,ii) && !isSet(ii))
				continue;
			ss << " " << avPair2string(ii,s);
		}
	} else if (rrType == POS_REPLY) {
		for (int i = CPA_START+1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (!CpType::isRepAttr(cpType,ii)) continue;
			ss << " " << avPair2string(ii,s);
		}
	} else {
		ss << " errMsg=" << errMsg;
	}
	ss << endl;
	s = ss.str();
	return s;
}
