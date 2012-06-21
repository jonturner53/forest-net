
package forest.common;

public enum CpAttr {
	//index			code	name
	CPA_START(		0,	"cp attr start"),
	
	AVAIL_BIT_RATE(		37,	"availBitRate"),
	AVAIL_PKT_RATE(		38,	"availPktRate"),
	AVAIL_BIT_RATE_IN(	39,	"availBitRateIn"),
	AVAIL_PKT_RATE_IN(	40,	"availPktRateIn"),
	AVAIL_BIT_RATE_OUT(	41,	"availBitRateOut"),
	AVAIL_PKT_RATE_OUT(	42,	"availPktRateOut"),
	BIT_RATE(		1,	"bitRate"),
	BIT_RATE_DOWN(		2,	"bitRateDown"),
	BIT_RATE_UP(		3,	"bitRateUp"),
	BIT_RATE_IN(		45,	"bitRateIn"),
	BIT_RATE_OUT(		46,	"bitRateOut"),
	CLIENT_ADR(		33,	"clientAdr"),
	CLIENT_IP(		34,	"clientIp"),
	CLIENT_PORT(		43,	"clientPort"),
	COMTREE_NUM(		4,	"comtree"),
	COMTREE_OWNER(		5,	"comtreeOwner"),
	CORE_FLAG(		6,	"coreFlag"),
	DEST_ADR(		7,	"destAdr"),
	EXT_BIT_RATE_DOWN(	8,	"extBitRateDown"),
	EXT_BIT_RATE_UP(	9,	"extBitRateUp"),
	EXT_PKT_RATE_DOWN(	10,	"extPktRateDown"),
	EXT_PKT_RATE_UP(	11,	"extPktRateUp"),
	FIRST_LEAF_ADR(		50,	"firstLeafAdr"),
	LAST_LEAF_ADR(		51,	"lastLeafAdr"),
	IFACE_NUM(		12,	"iface"),
	INT_BIT_RATE_DOWN(	13,	"intBitRateDown"),
	INT_BIT_RATE_UP(	14,	"intBitRateUp"),
	INT_PKT_RATE_DOWN(	15,	"intPktRateDown"),
	INT_PKT_RATE_UP(	16,	"intPktRateUp"),
	LEAF_ADR(		17,	"leafAdr"),
	LEAF_COUNT(		18,	"leafCount"),
	LINK_COUNT(		44,	"linkCount"),
	LINK_NUM(		19,	"link"),
	LOCAL_IP(		20,	"localIp"),
	MAX_BIT_RATE(		21,	"maxBitRate"),
	MAX_PKT_RATE(		22,	"maxPktrate"),
	PARENT_LINK(		23,	"parentLink"),
	PEER_ADR(		24,	"peerAdr"),
	PEER_CORE_FLAG(		49,	"peerCoreFlag"),
	PEER_DEST(		25,	"peerDest"),
	PEER_IP(		26,	"peerIp"),
	PEER_PORT(		27,	"peerPort"),
	PEER_TYPE(		28,	"peerType"),
	PKT_RATE(		29,	"pktRate"),
	PKT_RATE_DOWN(		30,	"pktRateDown"),
	PKT_RATE_UP(		31,	"pktRateUp"),
	PKT_RATE_IN(		47,	"pktRateIn"),
	PKT_RATE_OUT(		48,	"pktRateOut"),
	QUEUE_NUM(		32,	"queue"),
	ROOT_ZIP(		52,	"rootZip"),
	RTR_ADR(		35,	"rtrAdr"),
	RTR_IP(			36,	"rtrIp"),

	CPA_END(		64,	"cp attr end");

	private final int code;
	private final String name;
	//getters
	public int getCode() { return this.code; }
	public String getName() { return this.name; }
	
	CpAttr(int code, String name) {
		this.code = code;
		this.name = name;
	}
	
	public static CpAttr getIndexByCode(int code) {
		if(code == 0) return null;
		for(CpAttr cpa : CpAttr.values()) {
			if(cpa.getCode() == code) return cpa;
		}
		return null;
	}
	
	public static boolean isValidIndex(int i) {
		return (i > 0) && (i <= CPA_END.ordinal());
	}
}
