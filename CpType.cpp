/** @file CpType.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "CpType.h"

bool CpType::firstCall = true;

CpType::CpTypeInfo CpType::typeInfo[] = {
	// index		code	 name			   abbrev  request/reply attribute sets
	{ CPT_START,	 	0,	"ctl pkt start",	   "cps",  0, 0, 0 },

	{ CLIENT_ADD_COMTREE,	10,	"client add comtree",	   "cac",  0, 0, (1ull << COMTREE_NUM) },
	{ CLIENT_DROP_COMTREE,	11,	"client drop comtree",	   "cdc",  (1ull << COMTREE_NUM),
									   (1ull << COMTREE_NUM), 0},
	{ CLIENT_GET_COMTREE,	12,	"client get comtree",	   "cgc",  (1ull << COMTREE_NUM),
									   (1ull << COMTREE_NUM),
									   (1ull << COMTREE_NUM)|
									   (1ull << COMTREE_OWNER)|
									   (1ull << LEAF_COUNT)|
									   (1ull << EXT_BIT_RATE_DOWN)|
									   (1ull << EXT_BIT_RATE_UP)|
									   (1ull << EXT_PKT_RATE_DOWN)|
									   (1ull << EXT_PKT_RATE_UP)|
									   (1ull << INT_BIT_RATE_DOWN)|
									   (1ull << INT_BIT_RATE_UP)|
									   (1ull << INT_PKT_RATE_DOWN)|
									   (1ull << INT_PKT_RATE_UP)},
	{ CLIENT_MOD_COMTREE,	13,	"client modify comtree",   "cmc",  (1ull << COMTREE_NUM)|
									   (1ull << EXT_BIT_RATE_DOWN)|
									   (1ull << EXT_BIT_RATE_UP)|
									   (1ull << EXT_PKT_RATE_DOWN)|
									   (1ull << EXT_PKT_RATE_UP)|
									   (1ull << INT_BIT_RATE_DOWN)|
									   (1ull << INT_BIT_RATE_UP)|
									   (1ull << INT_PKT_RATE_DOWN)|
									   (1ull << INT_PKT_RATE_UP),
									   (1ull << COMTREE_NUM), 0},
	{ CLIENT_JOIN_COMTREE,	14,	"client join comtree",	   "cjc",  (1ull << COMTREE_NUM),
									   (1ull << COMTREE_NUM), 0},
	{ CLIENT_LEAVE_COMTREE,	15,	"client leave comtree",	   "clc",  (1ull << COMTREE_NUM),
									   (1ull << COMTREE_NUM), 0},
	{ CLIENT_RESIZE_COMTREE,16,	"client	resize comtree",   "crc",  (1ull << COMTREE_NUM),
									   (1ull << COMTREE_NUM), 0},
	{ CLIENT_GET_LEAF_RATE,	17,	"client	get leaf rate",	   "cglr", (1ull << COMTREE_NUM)|
									   (1ull << LEAF_ADR),
									   (1ull << COMTREE_NUM)|
									   (1ull << LEAF_ADR),
									   (1ull << COMTREE_NUM)|
									   (1ull << LEAF_ADR)|
									   (1ull << BIT_RATE_DOWN)|
									   (1ull << BIT_RATE_UP)|
									   (1ull << PKT_RATE_DOWN)|
									   (1ull << PKT_RATE_UP)},
	{ CLIENT_MOD_LEAF_RATE,	18,	"client	modify leaf rate", "cmlr", (1ull << COMTREE_NUM)|
									   (1ull << LEAF_ADR)|
									   (1ull << BIT_RATE_DOWN)|
									   (1ull << BIT_RATE_UP)|
									   (1ull << PKT_RATE_DOWN)|
									   (1ull << PKT_RATE_UP),
									   (1ull << COMTREE_NUM)|
									   (1ull << LEAF_ADR), 0},

	{ ADD_IFACE,		30,	"add interface",	   "ai",   (1ull << IFACE_NUM)|
									   (1ull << LOCAL_IP)|
									   (1ull << MAX_BIT_RATE)|
									   (1ull << MAX_PKT_RATE),
									   (1ull << IFACE_NUM)|
									   (1ull << LOCAL_IP)|
									   (1ull << MAX_BIT_RATE)|
									   (1ull << MAX_PKT_RATE), 0},
	{ DROP_IFACE,		31,	"drop interface",	   "di",   (1ull << IFACE_NUM),
									   (1ull << IFACE_NUM), 0},
	{ GET_IFACE,		32,	"get interface",	   "gi",   (1ull << IFACE_NUM),
									   (1ull << IFACE_NUM),
									   (1ull << IFACE_NUM)|
									   (1ull << LOCAL_IP)|
									   (1ull << MAX_BIT_RATE)|
									   (1ull << MAX_PKT_RATE)},
	{ MOD_IFACE,		33,	"modify interface",	   "mi",   (1ull << IFACE_NUM)|
									   (1ull << MAX_BIT_RATE)|
									   (1ull << MAX_PKT_RATE),
									   (1ull << IFACE_NUM), 0},

	{ ADD_LINK,		40,	"add link",		   "al",   (1ull << IFACE_NUM)|
									   (1ull << LINK_NUM)|
									   (1ull << PEER_TYPE)|
									   (1ull << PEER_IP)|
									   (1ull << PEER_ADR),
									   (1ull << PEER_TYPE)|
									   (1ull << PEER_IP),
									   (1ull << LINK_NUM)|
									   (1ull << PEER_ADR)|
									   (1ull << RTR_IP) },
	{ DROP_LINK,		41,	"drop link",		   "dl",   (1ull << LINK_NUM),
									   (1ull << LINK_NUM), 0},
	{ GET_LINK,		42,	"get link",		   "gl",   (1ull << LINK_NUM),
									   (1ull << LINK_NUM),
									   (1ull << LINK_NUM)|
									   (1ull << IFACE_NUM)|
									   (1ull << PEER_TYPE)|
									   (1ull << PEER_IP)|
									   (1ull << PEER_ADR)|
									   (1ull << PEER_PORT)|
									   (1ull << PEER_DEST)|
									   (1ull << BIT_RATE)|
									   (1ull << PKT_RATE)},
	{ MOD_LINK,		43,	"modify link",		   "ml",   (1ull << LINK_NUM)|
									   (1ull << PEER_TYPE)|
									   (1ull << PEER_PORT)|
									   (1ull << PEER_DEST)|
									   (1ull << BIT_RATE)|
									   (1ull << PKT_RATE),
									   (1ull << LINK_NUM), 0},

	{ ADD_COMTREE,		50,	"add comtree",		   "ac",   (1ull << COMTREE_NUM),
									   (1ull << COMTREE_NUM), 0},
	{ DROP_COMTREE,		51,	"drop comtree",		   "dc",   (1ull << COMTREE_NUM),
									   (1ull << COMTREE_NUM), 0},
	{ GET_COMTREE,		52,	"get comtree",		   "gc",   (1ull << COMTREE_NUM),
									   (1ull << COMTREE_NUM),
									   (1ull << COMTREE_NUM)|
									   (1ull << CORE_FLAG)|
									   (1ull << PARENT_LINK)|
									   (1ull << LINK_COUNT)|
									   (1ull << QUEUE_NUM)},
	{ MOD_COMTREE,		53,	"modify comtree",	   "mc",   (1ull << COMTREE_NUM)|
									   (1ull << CORE_FLAG)|
									   (1ull << PARENT_LINK)|
									   (1ull << QUEUE_NUM),
									   (1ull << COMTREE_NUM), 0},
	{ ADD_COMTREE_LINK,	54,	"add comtree link",	   "acl",  (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM)|
									   (1ull << CORE_FLAG)|
									   (1ull << PEER_ADR),
									   (1ull << COMTREE_NUM), 0},
	{ DROP_COMTREE_LINK,	55,	"drop comtree link",	   "dcl",  (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM)|
									   (1ull << PEER_ADR),
									   (1ull << COMTREE_NUM), 0},
	{ RESIZE_COMTREE_LINK,	56,	"resize comtree link",	   "rcl",  (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM)|
									   (1ull << BIT_RATE_DOWN)|
									   (1ull << BIT_RATE_UP)|
									   (1ull << PKT_RATE_DOWN)|
									   (1ull << PKT_RATE_UP),
									   (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM), 0},

	{ ADD_ROUTE,		70,	"add route",		   "ar",   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM)|
									   (1ull << QUEUE_NUM),
									   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM), 0},
	{ DROP_ROUTE,		71,	"drop route",		   "dr",   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM),
									   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM), 0},
	{ GET_ROUTE,		72,	"get route",		   "gr",   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM),
									   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM),
									   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM)|
									   (1ull << QUEUE_NUM) },
	{ MOD_ROUTE,		73,	"modify route",		   "mr",   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM)|
									   (1ull << QUEUE_NUM),
									   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM), 0},
	{ ADD_ROUTE_LINK,	74,	"add route link",	   "arl",  (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM),
									   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM), 0},
	{ DROP_ROUTE_LINK,	75,	"drop route link",	   "drl",  (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM),
									   (1ull << DEST_ADR)|
									   (1ull << COMTREE_NUM)|
									   (1ull << LINK_NUM), 0},

	{ NEW_CLIENT,		100,	"new client",		   "ncl",  (1ull << CLIENT_IP),
									   (1ull << CLIENT_IP),
								           (1ull << CLIENT_ADR)|
								           (1ull << RTR_IP)|
								           (1ull << RTR_ADR)},
	{ CLIENT_CONNECT,	101,	"client connected",	   "cc",   (1ull << CLIENT_ADR)|
									   (1ull << RTR_ADR),
									   (1ull << CLIENT_ADR)|
									   (1ull << RTR_ADR),
									   (1ull << RTR_ADR)},
	{ CLIENT_DISCONNECT,	102,	"client disconnected",	   "cd",   (1ull << CLIENT_ADR)|
									   (1ull << RTR_ADR),
									   (1ull << CLIENT_ADR)|
									   (1ull << RTR_ADR),
									   (1ull << RTR_ADR)},
	{ CPT_END,	 	255,	"ctl pkt end",	   	   "cpe",  0,0,0 }
};

void CpType::check() {
	if (!firstCall) return;
	firstCall = false;
	for (int i = CPT_START+1; i < CPT_END; i++) {
		if (typeInfo[i].index != CpTypeIndex(i))
			fatal("CpType::check: mismatched index values");
	}
}

//bool CpType::validIndex(CpTypeIndex i) {
//	if (firstCall) check();
//	return i > CPT_START && i < CPT_END;
//}
//
///** Get the code for a given control packet type index
// */
//int CpType::getCode(CpTypeIndex i) {
//	if (firstCall) check();
//	return (validIndex(i) ? typeInfo[i].code : 0);
//}
//
///** Get the name for a given control packet type index
// */
//const char* CpType::getName(CpTypeIndex i) {
//	if (firstCall) check();
//	return (validIndex(i) ? typeInfo[i].name : "undefined");
//}
//
///** Get the abbreviation for a given control packet type index
// */
//const char* CpType::getAbbrev(CpTypeIndex i) {
//	if (firstCall) check();
//	return (validIndex(i) ?  typeInfo[i].abbrev : "undefined");
//}

CpTypeIndex CpType::getIndexByCode(int c) {
	for (int i = CPT_START+1; i < CPT_END; i++) {
		if (c == typeInfo[i].code) return (CpTypeIndex) i;
	}
	return CPT_START; // == 0
}

CpTypeIndex CpType::findMatch(const char *s) {
	for (int i = CPT_START+1; i < CPT_END; i++) {
		if (strcmp(s,typeInfo[i].name) || strcmp(s,typeInfo[i].abbrev))
			return (CpTypeIndex) i;
	}
	return CPT_START; // == 0
}
