
package forest.common;

public class CtlPkt {
	/*
	Control packet request/reply types
	*/
	public enum CpRrTyp {
		REQUEST(1),
		POS_REPLY(2),
		NEG_REPLY(3);
		
		private int code;
		CpRrTyp(int code) { this.code = code; }
		
		public int getCode() { return code; }
		
		public static CpRrTyp getIndexByCode(int c) {
			for(CpRrTyp crt : CpRrTyp.values()) {
				if(c == crt.code) return crt;
			}
			return null;
		}
	}

	public CtlPkt() {
		aSet = new boolean[CpAttr.CPA_END.ordinal()+1];
		aVal = new int[CpAttr.CPA_END.ordinal()+1];
	}
	public CtlPkt(CpTyp cpt, CpRrTyp crt, long seq) {
		this.cpType = cpt;
		this.rrType = crt;
		this.seqNum = seq;
		aSet = new boolean[CpAttr.CPA_END.ordinal()+1];
		aVal = new int[CpAttr.CPA_END.ordinal()+1];
	}
	private CpTyp cpType;
	private CpRrTyp rrType;
	private long seqNum;
	private int pp;
	private boolean[] aSet;
	private int[] aVal;
	private Forest.PktBuffer buff;
	private String errMsg;
	private char[] err;
	private final int MAX_MSG_LEN = 500;
	//setters
	public void setCpType(CpTyp cpType) { this.cpType = cpType; }
	public void setRrType(CpRrTyp rrType) { this.rrType = rrType; }
	public void setSeqNum(long seqNum) { this.seqNum = seqNum; }
	
	//getters
	public CpTyp getCpType() { return cpType; }
	public CpRrTyp getRrType() { return rrType; }
	public long getSeqNum() { return seqNum; }

	public int pack(Forest.PktBuffer p) {
		buff = p;
		if(rrType != CpRrTyp.REQUEST && rrType != CpRrTyp.POS_REPLY &&
			rrType != CpRrTyp.NEG_REPLY) return 0;
		pp = Forest.HDR_LENG / 4;

		buff.set(pp++,cpType.getCode());
		buff.set(pp++,rrType.getCode());
		buff.set(pp++,(int)(seqNum >> 32));
		buff.set(pp++,(int)(seqNum & 0xffffffff));
		
		if(rrType == CpRrTyp.REQUEST) {
			for(CpAttr cpa : CpAttr.values()) {
				if(CpTyp.isReqAttr(cpType,cpa)) {
					if(isSet(cpa)) packAttr(cpa);
					else if(CpTyp.isReqReqAttr(cpType,cpa))
						return 0;
				}
			}
		} else if(rrType == CpRrTyp.POS_REPLY) {
			for(CpAttr cpa : CpAttr.values()) {
				if(CpTyp.isRepAttr(cpType,cpa)) {
					if(isSet(cpa)) packAttr(cpa);
					else return 0;
				}
			}
		} else {
			err = new char[MAX_MSG_LEN+1];
			int i;
			for(i = 0;i <= MAX_MSG_LEN; i++) {
				int val = buff.get(i);
				err[pp+i*2] = (char) (val >> 16);
				if(err[pp+i*2] == '\0') break;
				err[pp+i*2+1] = (char) (val & 0xffff);
				if(err[pp+i*2+1] == '\0') break;
			}
			errMsg = new String(err);
			return 4*pp + i*2;
		}
		return 4*pp;
	}

	public boolean unpack(Forest.PktBuffer p) {
		buff = p;
		int pleng = (buff.get(0) >> 16 & 0xfff);
		pleng /= 4;
		if (pleng < 4) return false; //too short to be ctl pkt
		
		pp = Forest.HDR_LENG / 4;
		cpType = CpTyp.getIndexByCode(buff.get(pp++));
		rrType = CpRrTyp.getIndexByCode(buff.get(pp++));
		seqNum = buff.get(pp++); seqNum <<= 32;
		seqNum |= buff.get(pp++);

		if(cpType == null) return false;
		if(rrType != CpRrTyp.REQUEST && rrType != CpRrTyp.POS_REPLY &&
			rrType != CpRrTyp.NEG_REPLY)
			return false;
		if(rrType == CpRrTyp.NEG_REPLY) {
			err = new char[MAX_MSG_LEN+1];
			for(int i = 0;i <= MAX_MSG_LEN; i++) {
				int val = buff.get(i);
				err[pp+i*2] = (char) (val >> 16);
				if(err[pp+i*2] == '\0') break;
				err[pp+i*2+1] = (char) (val & 0xffff);
				if(err[pp+i*2+1] == '\0') break;
			}
			errMsg = new String(err);
			return true;
		}
		while(pp < pleng - 1) { if(unpackAttr() == null) return false; }

		if(rrType == CpRrTyp.REQUEST) {
			for(CpAttr cpa : CpAttr.values()) {
				if(CpTyp.isReqReqAttr(cpType,cpa) && !isSet(cpa))
					return false;
			}
		} else {
			for(CpAttr cpa : CpAttr.values()) {
				if(CpTyp.isRepAttr(cpType,cpa) && !isSet(cpa))
					return false;
			}
		}
		return true;
	}

	public String toString() {
		String s = "";
		boolean reqPkt = (rrType == CpRrTyp.REQUEST);
		boolean replyPkt = !reqPkt;
		boolean success = (rrType == CpRrTyp.POS_REPLY);
		
		s += cpType.getCmdName();
		if(reqPkt) s += " (request,";
		else if(rrType == CpRrTyp.POS_REPLY) s += " (pos reply,";
		else s += " (neg reply,";
		s+= seqNum + "):";

		if(rrType == CpRrTyp.REQUEST) {
			for(CpAttr cpa : CpAttr.values()) {
				if(!CpTyp.isReqAttr(cpType,cpa)) continue;
				if(!CpTyp.isReqReqAttr(cpType,cpa) && !isSet(cpa))
					continue;
				s += " " + avPair2string(cpa);
			}
		} else if(rrType == CpRrTyp.POS_REPLY) {
			for(CpAttr cpa : CpAttr.values()) {
				if(!CpTyp.isRepAttr(cpType,cpa)) continue;
				s += avPair2string(cpa);
			}
		} else {
			s += " errMsg=" + errMsg;
		}
		s += "\n";
		return s;
	}
	
	public String avPair2string(CpAttr cpa) {
		String s = "";
		s += cpa.getName() + "=";
		if(!isSet(cpa)) {
			s += "(missing)"; return s;
		}
		int val = getAttr(cpa);
		if(cpa == CpAttr.COMTREE_OWNER || cpa == CpAttr.LEAF_ADR ||
		  cpa == CpAttr.PEER_ADR || cpa == CpAttr.PEER_DEST ||
		  cpa == CpAttr.RTR_ADR || cpa == CpAttr.CLIENT_ADR ||
		  cpa == CpAttr.FIRST_LEAF_ADR || cpa == CpAttr.LAST_LEAF_ADR ||
		  cpa == CpAttr.DEST_ADR) {
			s += Forest.fAdr2string(val);
		} else if(cpa == CpAttr.LOCAL_IP || cpa == CpAttr.PEER_IP ||
			  cpa == CpAttr.CLIENT_IP || cpa == CpAttr.RTR_IP) {
			s += Forest.ip2string(val);
		} else if(cpa == CpAttr.PEER_TYPE) {
			s += Forest.nodeTyp2string(Forest.NodeTyp.getIndexByCode(val));
		} else {
			s += val;
		}
		return s;
	}
	
	public int getAttr(CpAttr cpa) {
		return isSet(cpa) ? aVal[cpa.ordinal()] : 0;
	}
	
	public boolean isSet(CpAttr cpa) {
		return aSet[cpa.ordinal()];
	}
	
	public void packAttr(CpAttr cpa) {
		buff.set(pp++, cpa.getCode());
		buff.set(pp++, aVal[cpa.ordinal()]);
	}
	public CpAttr unpackAttr() {
		CpAttr cpa = CpAttr.getIndexByCode(buff.get(pp));
		if(cpa == null) return null;
		pp++;
		setAttr(cpa,buff.get(pp++));
		return cpa;
	}
	public void setAttr(CpAttr cpa, int val) {
		if(cpa == null) return;
		int i = cpa.ordinal();
		aVal[i] = val; aSet[i] = true;
	}
}

