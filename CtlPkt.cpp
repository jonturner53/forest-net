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
 *  constructs a CtlPkt object with a pointer to the payload part
 *  of the buffer as the constructor argument. The user then sets
 *  selected fields to non-zero values and calls pack,
 *  which packs the appropriate fields into the packet's payload
 *  and returns the number of 32 bit words that were packed.
 *  
 *  To unpack a buffer, the user constructs a CtlPkt object
 *  and then calls unpack, specifying the number of 32 bit words
 *  in the payload that are to be unpacked. The control packet fields
 *  can then be retrieved from the corresponding fields of the CtlPkt
 *  object.
 */

#include "CtlPkt.h"

/** Constructor for CtlPkt.
 */
CtlPkt::CtlPkt() { payload = 0; reset(); }

/** Destructor for CtlPkt. */
CtlPkt::~CtlPkt() { } 

/** Clears CtlPkt fields for re-use with a new buffer.
 */
void CtlPkt::reset() {
	for (int i = CPA_START; i < CPA_END; i++) aSet[i] = false;
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
	payload[pp++] = htonl(rrType);
	payload[pp++] = htonl(CpType::getCode(cpType));
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
		int j = Misc::strnlen(errMsg,MAX_MSG_LEN);
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
	rrType = (CpRrType) ntohl(payload[pp++]);
	cpType = CpType::getIndexByCode(ntohl(payload[pp++]));
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

void CtlPkt::writeAvPair(ostream& out, CpAttrIndex ii) {
	out << CpAttr::getName(ii) << "=";
	if (!isSet(ii)) {
		cout << "(missing)";
		return;
	}
	int32_t val = getAttr(ii);
	if (ii == COMTREE_OWNER || ii == LEAF_ADR ||
	    ii == PEER_ADR || ii == PEER_DEST ||
	    ii == RTR_ADR || ii == CLIENT_ADR ||
	    ii == DEST_ADR) {
		Forest::writeForestAdr(out,(fAdr_t) val);
	} else if (ii == LOCAL_IP || ii == PEER_IP ||
		   ii == CLIENT_IP || ii == RTR_IP) {
		string s; Np4d::addIp2string(s,val);
		out << s;
	} else if (ii == PEER_TYPE) {
		string s; Forest::nodeType2string((ntyp_t) val,s);
		out << s;
	} else {
		out << val;
	}
}

void CtlPkt::write(ostream& out) {
// Prints CtlPkt content.
	bool reqPkt = (rrType == REQUEST);
	bool replyPkt = !reqPkt;
	bool success = (rrType == POS_REPLY);

	char xstr[50];
	if (reqPkt) strncpy(xstr," (request,",15);
	else if (rrType == POS_REPLY) strncpy(xstr," (pos reply,",15);
	else strncpy(xstr," (neg reply,",15);
	char seqStr[20];
	sprintf(seqStr,"%lld",seqNum);
	strncat(xstr,seqStr,20); strncat(xstr,"):",5);
	out << CpType::getName(cpType) << xstr;

	if (rrType == REQUEST) {
		for (int i = CPA_START+1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (!CpType::isReqAttr(cpType,ii)) continue;
			if (!CpType::isReqReqAttr(cpType,ii) && !isSet(ii))
				continue;
			out << " "; writeAvPair(out, ii);
		}
	} else if (rrType == POS_REPLY) {
		for (int i = CPA_START+1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (!CpType::isRepAttr(cpType,ii)) continue;
			out << " "; writeAvPair(out, ii);
		}
	} else {
		out << " errMsg=" << errMsg;
	}
	out << endl;
}
