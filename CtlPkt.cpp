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

/** Constructor for CtlPkt. */
CtlPkt::CtlPkt(buffer_t b) {
	reset(b);
}

/** Clears CtlPkt fields for re-use with a new buffer. */
void CtlPkt::reset(buffer_t b) {
	for (int i = CPA_START; i < CPA_END; i++) aSet[i] = false;
	buf = b;
}

/** Destructor for CtlPkt. */
CtlPkt::~CtlPkt() { } 

/** Pack CtlPkt fields into buffer.
 *  @return the number of 32 bit payload words in the control packet.
 *  or 0 if an error was detected.
 */
int CtlPkt::pack() {
	if (!CpType::validIndex(cpType))
		return 0;
	if (rrType != REQUEST && rrType != POS_REPLY && rrType != NEG_REPLY)
		return 0;
	bp = Forest::HDR_LENG/4;
	buf[bp++] = htonl(
			(rrType << 30) |
			((CpType::getCode(cpType) & 0x3fff) << 16) |
			((int) ((seqNum >> 32) & 0xffff))
			);
	buf[bp++] = htonl((int) (seqNum & 0xffffffff));

	if (rrType == REQUEST) {
		// pack all request attributes and confirm that
		// required attributes are present
		for (int i = CPA_START + 1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (CpType::isReqAttr(cpType, ii)) {
				if (isSet(CpAttrIndex(i))) packAttr(CpAttrIndex(i));
				else if (CpType::isReqReqAttr(cpType,CpAttrIndex(i)))
					return 0;
			}
		}
	} else if (rrType == POS_REPLY) {
		for (int i = CPA_START + 1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (CpType::isRepAttr(cpType, ii)) {
				if (isSet(ii)) packAttr(ii);
				else return 0;
			}
		}
	} else {
		int j = Misc::strnlen(errMsg,MAX_MSG_LEN);
		if (j == MAX_MSG_LEN) errMsg[MAX_MSG_LEN] = 0;
		strncpy((char*) &buf[bp], errMsg, j+1);
		return bp + 1 + (j+1)/4;
	}

	return bp;
	
}

bool CtlPkt::unpack(int pleng) {
// Unpack CtlPkt fields from buffer. Pleng is the number
// of 32 bit words in the payload to be unpacked.
// Return true on success, 0 on failure.

	if (pleng < 3) return false; // too short for control packet

	bp = Forest::HDR_LENG/4;
	buf[bp++] = htonl(
			(rrType << 30) |
			((CpType::getCode(cpType) & 0x3fff) << 16) |
			((int) ((seqNum >> 32) & 0xffff))
			);
	buf[bp++] = htonl((int) (seqNum & 0xffffffff));
	uint32_t x = ntohl(buf[bp++]);
	rrType = (CpRrType) ((x >> 30) & 0x3);
	cpType = CpType::getIndexByCode((x >> 16) & 0x3fff);
	seqNum = x & 0xffff;
	seqNum = (seqNum << 32) | ntohl(buf[bp++]);
	
	if (!CpType::validIndex(cpType)) return false;
	if (rrType != REQUEST && rrType != POS_REPLY && rrType != NEG_REPLY)
		return false;
	
	if (rrType == NEG_REPLY) {
		strncpy(errMsg,(char*) &buf[bp], MAX_MSG_LEN);
		errMsg[MAX_MSG_LEN] = 0;
		return true;
	}

	// unpack all attribute/value pairs
	while (bp < pleng-1) unpackAttr();

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
			out << " " << CpAttr::getName(ii) << "=";
			if (isSet(ii)) {
				int32_t val = getAttr(ii);
				if (ii == COMTREE_OWNER || ii == LEAF_ADR ||
				    ii == PEER_ADR || ii == PEER_DEST ||
				    ii == DEST_ADR) {
					Forest::writeForestAdr(out,(fAdr_t) val);
				} else if (ii == LOCAL_IP || ii == PEER_IP) {
					string s; Np4d::addIp2string(s,val);
					out << s;
				} else {
					out << val;
				}
			} else {
			     out << "(missing)";
			}
		}
	} else if (rrType == POS_REPLY) {
		for (int i = CPA_START+1; i < CPA_END; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (!CpType::isRepAttr(cpType,ii)) continue;
			out << " " << CpAttr::getName(ii) << "=";
			if (isSet(ii)) {
				int32_t val = getAttr(ii);
				if (ii == COMTREE_OWNER || ii == LEAF_ADR ||
				    ii == PEER_ADR || ii == PEER_DEST ||
				    ii == DEST_ADR) {
					Forest::writeForestAdr(out,(fAdr_t) val);
				} else if (ii == LOCAL_IP || ii == PEER_IP) {
					string s; Np4d::addIp2string(s,val);
					out << s;
				} else {
					out << val;
				}
			} else {
			      out << "(missing)";
			}
		}
	} else {
		out << " errMsg=" << errMsg;
	}
	out << endl;
}
