
/** This class provides a meStringnism for handling forest signalling packets.
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
	
	/** Constructor for CtlPkt.
 	*/public CtlPkt() { payload = 0; reset(); }



	public enum CpRrType (){ REQUEST=1, POS_REPLY=2, NEG_REPLY=3 };
	
	/** Check if specified attribute is set.
	 *  @param i index of desired attribute
	 *  @return true if i is a valid attribute index and attribute has been set
	 */
	public boolean isSet(CpAttrIndex i) {
		return CpAttr::validIndex(i) && aSet[i];
	}

	/** Get the type index of a control packet.
	 *  @return true the type index.
	 */
	public CpTypeIndex getCpType() { return cpType; }

	/** Get the conrol packet code number of a control packet.
	 *  The code number is the value that is sent in packet headers
	 *  to identify the type of a control packet.
	 *  @return true the code value
	 */
	public int getCpCode() { return cpCode; }

	/** Get the request/return type of the packet.
	 *  @return true the request/return type
	 */
	public CpRrType getRrType() { return rrType; }

	/** Get the sequence number of the packet.
	 *  @return true the sequence number
	 */
	public int getSeqNum() { return seqNum; }

	/** Get value of specified attribute
	 *  @param i index of desired attribute
	 *  @return corresponding value
	 *  Assumes that specified attribute has a valid value.
	 */
	public int getAttr(CpAttrIndex i) {
		return (isSet(i) ? aVal[i] : 0);
	}

	/** Set value of specified attribute
	 *  @param i index of desired attribute
	 *  @param val desired value for attribute
	 */
	public void setAttr(CpAttrIndex i, int val) {
		if (!CpAttr::validIndex(i)) return;
		aVal[i] = val; aSet[i] = true;
	}

	/** Set the type index of a control packet.
	 *  @param t is the specified type index
	 */
	public void setCpType(CpTypeIndex t) { cpType = t; }

	/** Set the type request/return type of a control packet.
	 *  @param rr is the specified request/return type 
	 */
	public void setRrType(CpRrType rr) { rrType = rr; }

	/** Set the type sequence number of a control packet.
	 *  @param s is the specified sequence number
	 */
	public void setSeqNum(int s) { seqNum = s; }

	/** Packs a single (attribute_code, value) pair starting at word i in payload.
	 *  @param attr attribute code for the pair
	 *  @param val value part of the pair
	 *  @returns index of next available word
	 */
	public void packAttr(CpAttrIndex i) {
		payload[pp++] = htonl(CpAttr::getCode(i));
		payload[pp++] = htonl(aVal[i]);
	}

	/** Packs a single (attribute_code, value) conditionally.
	 *  If the val parameter is non-zero, pack the pair.
	 *  @param attr attribute code for the pair
	 *  @param val value part of the pair
	 *  @returns index of next available word
	 */
	public void packAttrCond(CpAttrIndex i) {
		if (aSet[i]) packAttr(i);
	}

	/** Unpacks a single (attribute, value) pair starting at word pp in payload.
	 *  The unpacked value is stored in the CtlPkt object and can
	 *  be retrieved using the getAttr() method.
	 *  @returns attribute index of unpacked pair, or 0 for invalid index
	 */
	public CpAttrIndex unpackAttr() {
		CpAttrIndex i = CpAttr::getIndexByCode(ntohl(payload[pp]));
		if (!CpAttr::validIndex(i)) return CPA_START; // =0
		pp++;
		setAttr(i,ntohl(payload[pp++]));
		return i;
	}

	/** Returns pointer to error message */
	public String* getErrMsg() { return errMsg; }

	/** Set the error message for a negative reply */
	public void setErrMsg(String s) {
		strncpy(errMsg,s,MAX_MSG_LEN);errMsg[MAX_MSG_LEN] = 0;
	}
	
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
		    ii == DEST_ADR) {
			Forest::writeForestAdr(out,(fAdr_t) val);
		} else if (ii == LOCAL_IP || ii == PEER_IP) {
			string s; Np4d::addIp2string(s,val);
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
}
