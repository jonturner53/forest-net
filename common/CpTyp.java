
package forest.common;

public enum CpTyp {
	//index,code,name	   ,abbrev
	CPT_START(0,"ctl pkt start","cps",
		0,0,0),
	CLIENT_ADD_COMTREE(10,"client add comtree","cac",
		(1L << CpAttr.ROOT_ZIP.ordinal()),
		(1L << CpAttr.ROOT_ZIP.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())),
	CLIENT_DROP_COMTREE(11,"client drop comtree","cdc",
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		0),
	CLIENT_GET_COMTREE(12,"client get comtree","cgc",
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.COMTREE_OWNER.ordinal())|
		  (1L << CpAttr.LEAF_COUNT.ordinal())|(1L << CpAttr.EXT_BIT_RATE_DOWN.ordinal())|
		  (1L << CpAttr.EXT_BIT_RATE_UP.ordinal())|(1L << CpAttr.EXT_PKT_RATE_DOWN.ordinal())|
		  (1L << CpAttr.EXT_PKT_RATE_UP.ordinal())|(1L << CpAttr.INT_BIT_RATE_DOWN.ordinal())|
		  (1L << CpAttr.INT_BIT_RATE_UP.ordinal())|(1L << CpAttr.INT_PKT_RATE_DOWN.ordinal())|
		  (1L << CpAttr.INT_PKT_RATE_UP.ordinal())),
	CLIENT_MOD_COMTREE(13,"client modify comtree","cmc",
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.COMTREE_OWNER.ordinal())|
		  (1L << CpAttr.LEAF_COUNT.ordinal())|(1L << CpAttr.EXT_BIT_RATE_DOWN.ordinal())|
		  (1L << CpAttr.EXT_BIT_RATE_UP.ordinal())|(1L << CpAttr.EXT_PKT_RATE_DOWN.ordinal())|
		  (1L << CpAttr.EXT_PKT_RATE_UP.ordinal())|(1L << CpAttr.INT_BIT_RATE_DOWN.ordinal())|
		  (1L << CpAttr.INT_BIT_RATE_UP.ordinal())|(1L << CpAttr.INT_PKT_RATE_DOWN.ordinal())|
		  (1L << CpAttr.INT_PKT_RATE_UP.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		0),
	CLIENT_JOIN_COMTREE(14,"client join comtree","cjc",
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.CLIENT_IP.ordinal())|
		  (1L << CpAttr.CLIENT_PORT.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.CLIENT_IP.ordinal())|
		  (1L << CpAttr.CLIENT_PORT.ordinal()),
		0),
	CLIENT_LEAVE_COMTREE(15,"client leave comtree","clc",
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.CLIENT_IP.ordinal())|
		  (1L << CpAttr.CLIENT_PORT.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.CLIENT_IP.ordinal())|
		  (1L << CpAttr.CLIENT_PORT.ordinal()),
		0),
	CLIENT_RESIZE_COMTREE(16,"client resize comtree","crc",
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		0),
	CLIENT_GET_LEAF_RATE(17,"client get leaf rate","cglr",
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LEAF_ADR.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LEAF_ADR.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LEAF_ADR.ordinal())|
		  (1L << CpAttr.BIT_RATE_DOWN.ordinal())|(1L << CpAttr.BIT_RATE_UP.ordinal())|
		  (1L << CpAttr.PKT_RATE_DOWN.ordinal())|(1L << CpAttr.PKT_RATE_UP.ordinal())),
	CLIENT_MOD_LEAF_RATE(18,"client mod leaf rate","cmlr",
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LEAF_ADR.ordinal())|
		  (1L << CpAttr.BIT_RATE_DOWN.ordinal())|(1L << CpAttr.BIT_RATE_UP.ordinal())|
		  (1L << CpAttr.PKT_RATE_DOWN.ordinal())|(1L << CpAttr.PKT_RATE_UP.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LEAF_ADR.ordinal()),
		0),
	CLIENT_NET_SIG_SEP(0,"client net/sig seperator","cnss",
		0,0,0),
	ADD_IFACE(30,"add interface","ai",
		(1L << CpAttr.IFACE_NUM.ordinal())|(1L << CpAttr.LOCAL_IP.ordinal())|
		  (1L << CpAttr.MAX_BIT_RATE.ordinal())|(1L << CpAttr.MAX_PKT_RATE.ordinal()),
		(1L << CpAttr.IFACE_NUM.ordinal())|(1L << CpAttr.LOCAL_IP.ordinal())|
		  (1L << CpAttr.MAX_BIT_RATE.ordinal())|(1L << CpAttr.MAX_PKT_RATE.ordinal()),
		0),
	DROP_IFACE(31,"drop interface","di",
		(1L << CpAttr.IFACE_NUM.ordinal()),
		(1L << CpAttr.IFACE_NUM.ordinal()),
		0),
	GET_IFACE(32,"get interface","gi",
		(1L << CpAttr.IFACE_NUM.ordinal()),
		(1L << CpAttr.IFACE_NUM.ordinal()),
		(1L << CpAttr.IFACE_NUM.ordinal())|(1L << CpAttr.LOCAL_IP.ordinal())|
		  (1L << CpAttr.AVAIL_BIT_RATE.ordinal())|(1L << CpAttr.AVAIL_PKT_RATE.ordinal())|
		  (1L << CpAttr.MAX_BIT_RATE.ordinal())|(1L << CpAttr.MAX_PKT_RATE.ordinal())),
	MOD_IFACE(33,"modify interface","mi",
		(1L << CpAttr.IFACE_NUM.ordinal())|(1L << CpAttr.MAX_BIT_RATE.ordinal())|
		  (1L << CpAttr.MAX_PKT_RATE.ordinal()),
		(1L << CpAttr.IFACE_NUM.ordinal()),
		0),
	ADD_LINK(40,"add link","al",
		(1L << CpAttr.IFACE_NUM.ordinal())|(1L << CpAttr.LINK_NUM.ordinal())|
		  (1L << CpAttr.PEER_TYPE.ordinal())|(1L << CpAttr.PEER_IP.ordinal())|
		  (1L << CpAttr.PEER_ADR.ordinal())|(1L << CpAttr.PEER_PORT.ordinal()),
		(1L << CpAttr.PEER_TYPE.ordinal())|(1L << CpAttr.PEER_IP.ordinal()),
		(1L << CpAttr.LINK_NUM.ordinal())|(1L << CpAttr.PEER_ADR.ordinal())|
		  (1L << CpAttr.RTR_IP.ordinal())),
	DROP_LINK(41,"drop link","dl",
		(1L << CpAttr.LINK_NUM.ordinal()),
		(1L << CpAttr.LINK_NUM.ordinal()),
		0),
	GET_LINK(42,"get link","gl",
		(1L << CpAttr.LINK_NUM.ordinal()),
		(1L << CpAttr.LINK_NUM.ordinal()),
		(1L << CpAttr.LINK_NUM.ordinal())|(1L << CpAttr.IFACE_NUM.ordinal())|
		  (1L << CpAttr.PEER_TYPE.ordinal())|(1L << CpAttr.PEER_IP.ordinal())|
		  (1L << CpAttr.PEER_ADR.ordinal())|(1L << CpAttr.PEER_PORT.ordinal())|
		  (1L << CpAttr.AVAIL_BIT_RATE_IN.ordinal())|(1L << CpAttr.AVAIL_PKT_RATE_IN.ordinal())|
		  (1L << CpAttr.AVAIL_BIT_RATE_OUT.ordinal())|(1L << CpAttr.AVAIL_PKT_RATE_OUT.ordinal())|
		  (1L << CpAttr.BIT_RATE.ordinal())|(1L << CpAttr.PKT_RATE.ordinal())),
	MOD_LINK(43,"modify link","ml",
		(1L << CpAttr.LINK_NUM.ordinal())|(1L << CpAttr.BIT_RATE.ordinal())|
		  (1L << CpAttr.PKT_RATE.ordinal()),
		(1L << CpAttr.LINK_NUM.ordinal()),
		0),
	ADD_COMTREE(50,"add comtree","ac",
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		0),
	DROP_COMTREE(51,"drop comtree","dc",
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		0),
	GET_COMTREE(52,"get comtree","gc",
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.CORE_FLAG.ordinal())|
		  (1L << CpAttr.PARENT_LINK.ordinal())|(1L << CpAttr.LINK_COUNT.ordinal())),
	MOD_COMTREE(53,"mod comtree","mc",
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.CORE_FLAG.ordinal())|
		  (1L << CpAttr.PARENT_LINK.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		0),
	ADD_COMTREE_LINK(54,"add comtree link","acl",
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LINK_NUM.ordinal())|
		  (1L << CpAttr.PEER_CORE_FLAG.ordinal())|(1L << CpAttr.PEER_IP.ordinal())|
		  (1L << CpAttr.PEER_PORT.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.LINK_NUM.ordinal())),
	DROP_COMTREE_LINK(55,"drop comtree link","dcl",
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LINK_NUM.ordinal())|
		  (1L << CpAttr.PEER_IP.ordinal())|(1L << CpAttr.PEER_PORT.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal()),
		0),
	MOD_COMTREE_LINK(56,"mod comtree link","mcl",
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LINK_NUM.ordinal())|
		  (1L << CpAttr.BIT_RATE_IN.ordinal())|(1L << CpAttr.BIT_RATE_OUT.ordinal())|
		  (1L << CpAttr.PKT_RATE_IN.ordinal())|(1L << CpAttr.PKT_RATE_OUT.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LINK_NUM.ordinal()),
		0),
	GET_COMTREE_LINK(57,"get comtree link","gcl",
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LINK_NUM.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LINK_NUM.ordinal()),
		(1L << CpAttr.COMTREE_NUM.ordinal())|(1L << CpAttr.LINK_NUM.ordinal())|
		  (1L << CpAttr.BIT_RATE_IN.ordinal())|(1L << CpAttr.BIT_RATE_OUT.ordinal())|
		  (1L << CpAttr.PKT_RATE_IN.ordinal())|(1L << CpAttr.PKT_RATE_OUT.ordinal())|
		  (1L << CpAttr.QUEUE_NUM.ordinal())|(1L << CpAttr.PEER_DEST.ordinal())),
	ADD_ROUTE(70,"add route","ar",
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal())|
		  (1L << CpAttr.LINK_NUM.ordinal())|(1L << CpAttr.QUEUE_NUM.ordinal()),
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal()),
		0),
	DROP_ROUTE(71,"drop route","dr",
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal()),
		0),
	GET_ROUTE(72,"get route","gr",
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal()),
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal())|
		  (1L << CpAttr.LINK_NUM.ordinal())),
	MOD_ROUTE(73,"modify route","mr",
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal())|
		  (1L << CpAttr.LINK_NUM.ordinal())|(1L << CpAttr.QUEUE_NUM.ordinal()),
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal()),
		0),
	ADD_ROUTE_LINK(74,"add route link","arl",
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal())|
		  (1L << CpAttr.LINK_NUM.ordinal()),
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal())|
		  (1L << CpAttr.LINK_NUM.ordinal()),
		0),
	DROP_ROUTE_LINK(75,"drop route link","drl",
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal())|
		  (1L << CpAttr.LINK_NUM.ordinal()),
		(1L << CpAttr.DEST_ADR.ordinal())|(1L << CpAttr.COMTREE_NUM.ordinal())|
		  (1L << CpAttr.LINK_NUM.ordinal()),
		0),
	NEW_CLIENT(100,"new client","nc",
		(1L << CpAttr.CLIENT_IP.ordinal())|(1L << CpAttr.CLIENT_PORT.ordinal()),
		(1L << CpAttr.CLIENT_IP.ordinal())|(1L << CpAttr.CLIENT_PORT.ordinal()),
		(1L << CpAttr.CLIENT_ADR.ordinal())|(1L << CpAttr.RTR_IP.ordinal())|
		  (1L << CpAttr.RTR_ADR.ordinal())),
	CLIENT_CONNECT(101,"client connect","cc",
		(1L << CpAttr.CLIENT_ADR.ordinal())|(1L << CpAttr.RTR_ADR.ordinal()),
		(1L << CpAttr.CLIENT_ADR.ordinal())|(1L << CpAttr.RTR_ADR.ordinal()),
		0),
	CLIENT_DISCONNECT(102,"client disconnect","cd",
		(1L << CpAttr.CLIENT_ADR.ordinal())|(1L << CpAttr.RTR_ADR.ordinal()),
		(1L << CpAttr.CLIENT_ADR.ordinal())|(1L << CpAttr.RTR_ADR.ordinal()),
		0),
	BOOT_REQUEST(120,"boot request","br",
		0,0,
		(1L << CpAttr.FIRST_LEAF_ADR.ordinal())|(1L << CpAttr.LAST_LEAF_ADR.ordinal())),
	BOOT_COMPLETE(121,"boot complete","bc",
		0,0,0),
	BOOT_ABORT(122,"boot abort","ba",
		0,0,0),
	CPT_END(255,"ctl pkt end","cpe",
		0,0,0);
	

	private final int code;
	private final String cmdName;
	private final String abbrev;
	private final long reqAttrs;
	private final long requiredReqAttrs;
	private final long replyAttrs;
	
	CpTyp(int code, String cmdName, String abbrev,
		long reqAttrs, long requiredReqAttrs, long replyAttrs) {
		this.code = code;
		this.cmdName = cmdName;
		this.abbrev = abbrev;
		this.reqAttrs = reqAttrs;
		this.requiredReqAttrs = requiredReqAttrs;
		this.replyAttrs = replyAttrs;
	}
	
	public int getCode() { return code; }
	public String getCmdName() { return cmdName; }
	public String getAbbrev() { return abbrev; }
	public long getReqAttrs() { return reqAttrs; }
	public long getReqReqAttrs() { return requiredReqAttrs; }
	public long getRepAttrs() { return replyAttrs; }

	public static CpTyp getIndexByCode(int code) {
		for(CpTyp cpt : CpTyp.values()) {
			if(cpt.getCode() == code) return cpt;
		}
		return null;
	}
	public static boolean isReqAttr(CpTyp c,CpAttr j) {
		return ((c.getReqAttrs() & (1L << j.ordinal())) != 0);
	}
	public static boolean isReqReqAttr(CpTyp c,CpAttr j) {
		return ((c.getReqReqAttrs() & (1L << j.ordinal())) != 0);
	}
	public static boolean isRepAttr(CpTyp c,CpAttr j) {
		return ((c.getRepAttrs() & (1L << j.ordinal())) != 0);
	}
}
