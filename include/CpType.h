/** @file CpType.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef CPTYPE_H
#define CPTYPE_H

#include "CommonDefs.h"
#include "CpAttr.h"

/** Control packet types indices.
 *  Each control packet has a numerical index which is used within
 *  programs to access various pieces of information about control
 *  packet types.
 */
enum CpTypeIndex {
	CPT_START,			///< marker preceding first index

	// messages used by clients to manage comtrees
	CLIENT_ADD_COMTREE, CLIENT_DROP_COMTREE,
	CLIENT_GET_COMTREE, CLIENT_MOD_COMTREE,
	CLIENT_JOIN_COMTREE, CLIENT_LEAVE_COMTREE,
	CLIENT_RESIZE_COMTREE,
	CLIENT_GET_LEAF_RATE, CLIENT_MOD_LEAF_RATE,

	CLIENT_NET_SIG_SEP,

	// packet types to manage router interfaces
	ADD_IFACE, DROP_IFACE, GET_IFACE, MOD_IFACE,

	// packet types to manage router links
	ADD_LINK, DROP_LINK, GET_LINK, MOD_LINK,

	// packet types to manage comtrees at routers
	ADD_COMTREE, DROP_COMTREE, GET_COMTREE, MOD_COMTREE,
	ADD_COMTREE_LINK, DROP_COMTREE_LINK,
	MOD_COMTREE_LINK, GET_COMTREE_LINK,

	// packet types to manage routes at routers
	ADD_ROUTE, DROP_ROUTE, GET_ROUTE, MOD_ROUTE,
	ADD_ROUTE_LINK, DROP_ROUTE_LINK,

	// packet types used by client manager
	NEW_CLIENT, CLIENT_CONNECT, CLIENT_DISCONNECT,
	// packet types to boot router from netMgr
	BOOT_REQUEST, BOOT_COMPLETE, BOOT_ABORT,
	
	CPT_END			///< marker following last CtlPktType
};

/** This class provides information about control packet types.
 *  It provides methods to access information about types using
 *  their index and identifies which attributes are relevant
 *  for which types.
 */
class CpType {
private:
	struct CpTypeInfo {
		int index;		///< index for control packet type
		int code;		///< control packet type code
		const char* name;	///< name for control packet type
		const char* abbrev;	///< short name for control packet type
		uint64_t reqAttr;	///< possible attributes for requests
		uint64_t reqReqAttr;	///< required attributes for requests
		uint64_t replyAttr;	///< possible attributes for replies
	};
	
	static CpTypeInfo typeInfo[];

	static bool firstCall;		///< used to check index consistency

	static void check(); 		///< check consistency of index values

public:
	static bool validIndex(CpTypeIndex);	
	static CpTypeIndex getIndexByCode(int);

	static int getCode(CpTypeIndex);
	static const char* getName(CpTypeIndex);
	static const char* getAbbrev(CpTypeIndex);	
	static bool isReqAttr(CpTypeIndex, CpAttrIndex);
	static bool isReqReqAttr(CpTypeIndex, CpAttrIndex);
	static bool isRepAttr(CpTypeIndex, CpAttrIndex);

	static CpTypeIndex findMatch(const char*);
};

inline bool CpType::validIndex(CpTypeIndex i) {
	if (firstCall) check();
	return i > CPT_START && i < CPT_END;
}

/** Get the code for a given control packet type index
 */
inline int CpType::getCode(CpTypeIndex i) {
	if (firstCall) check();
	return (validIndex(i) ? typeInfo[i].code : 0);
}

/** Get the name for a given control packet type index
 */
inline const char* CpType::getName(CpTypeIndex i) {
	if (firstCall) check();
	return (validIndex(i) ? typeInfo[i].name : "undefined");
}

/** Get the abbreviation for a given control packet type index
 */
inline const char* CpType::getAbbrev(CpTypeIndex i) {
	if (firstCall) check();
	return (validIndex(i) ?  typeInfo[i].abbrev : "undefined");
}

/** True if specified attribute is valid for requests of given type */
inline bool CpType::isReqAttr(CpTypeIndex i, CpAttrIndex j) {
	return validIndex(i) && CpAttr::validIndex(j) &&
		(typeInfo[i].reqAttr & (1ull << j));
}

/** True if specified attribute is required for requests of given type */
inline bool CpType::isReqReqAttr(CpTypeIndex i, CpAttrIndex j) {
	return CpType::validIndex(i) && CpAttr::validIndex(j) &&
		(typeInfo[i].reqReqAttr & (1ull << j));
}

/** True if specified attribute is valid for replies of given type */
inline bool CpType::isRepAttr(CpTypeIndex i, CpAttrIndex j) {
	return CpType::validIndex(i) && CpAttr::validIndex(j) &&
		(typeInfo[i].replyAttr & (1ull << j));
}

#endif
