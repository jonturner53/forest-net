/** @file CpAttr.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "CpAttr.h"

bool CpAttr::firstCall = true;

const CpAttr::CpAttrInfo CpAttr::attrInfo[CPA_END+1] = {
	// index		code	 name			  
	{ CPA_START,	 	0,	"cp attr start" }, 

	{ BIT_RATE,		1,	"bitRate" },
	{ BIT_RATE_DOWN,	2,	"bitRateUp" },
	{ BIT_RATE_UP,		3,	"bitRateDown" },
	{ COMTREE_NUM,		4,	"comtree" },
	{ COMTREE_OWNER,	5,	"comtreeOwner" },
	{ CORE_FLAG,		6,	"coreFlag" },
	{ DEST_ADR,		7,	"destAdr" },
	{ EXT_BIT_RATE_DOWN,	8,	"extBitRateDown" },
	{ EXT_BIT_RATE_UP,	9,	"extBitRateUp" },
	{ EXT_PKT_RATE_DOWN,	10,	"extPktRateDown" },
	{ EXT_PKT_RATE_UP,	11,	"extPktRateUp" },
	{ IFACE_NUM,		12,	"iface" },
	{ INT_BIT_RATE_DOWN,	13,	"intBitRateDown" },
	{ INT_BIT_RATE_UP,	14,	"intBitRateUp" },
	{ INT_PKT_RATE_DOWN,	15,	"intPktRateDown" },
	{ INT_PKT_RATE_UP,	16,	"intPktRateUp" },
	{ LEAF_ADR,		17,	"leafAdr" },
	{ LEAF_COUNT,		18,	"leafCount" },
	{ LINK_NUM,		19,	"link" },
	{ LOCAL_IP,		20,	"localIp" }, 		
	{ MAX_BIT_RATE,		21,	"maxBitRate" },
	{ MAX_PKT_RATE,		22,	"maxPktRate" },
	{ PARENT_LINK,		23,	"parentLink" },
	{ PEER_ADR,		24,	"peerAdr" },
	{ PEER_DEST,		25,	"peerDest" },
	{ PEER_IP,		26,	"peerIp" },
	{ PEER_PORT,		27,	"peerPort" },
	{ PEER_TYPE,		28,	"peerType" },
	{ PKT_RATE,		29,	"pktRate" },
	{ PKT_RATE_DOWN,	30,	"pktRateDown" },
	{ PKT_RATE_UP,		31,	"pktRateUp" },
	{ QUEUE_NUM,		32,	"queue" },

	{ CPA_END,	 	255,	"cp attr end" }
};

void CpAttr::check() {
	if (!firstCall) return;
	firstCall = false;
	for (int i = CPA_START+1; i < CPA_END; i++) {
		if (attrInfo[i].index != CpAttrIndex(i)) {
cerr << "---- " << attrInfo[i].index << " " << i << " " << CpAttrIndex(i) << " -----\n";
			fatal("CpAttr::check: mismatched index values");
		}
	}
}

CpAttrIndex CpAttr::getIndexByCode(int c) {
	for (int i = CPA_START+1; i < CPA_END; i++) {
		if (c == attrInfo[i].code) return (CpAttrIndex) i;
	}
	return CPA_START; // == 0
}

CpAttrIndex CpAttr::findMatch(char* s) {
	for (int i = CPA_START+1; i < CPA_END; i++) {
		if (strcmp(s,attrInfo[i].name))
			return (CpAttrIndex) i;
	}
	return CPA_START; // == 0
}
