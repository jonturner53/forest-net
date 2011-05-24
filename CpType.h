/** \file CpType.h
 *  Header file for CpType class.
 */

#ifndef CPTYPE_H
#define CPTYPE_H

#include "CommonDefs.h"
#include "CpAttr.h"

/** Control packet types indices */
enum CpTypeIndex {
	CPT_START,			///< marker preceding first index

	// messages between clients and comtree controller
	CLIENT_ADD_COMTREE, CLIENT_DROP_COMTREE,
	CLIENT_GET_COMTREE, CLIENT_MOD_COMTREE,
	CLIENT_JOIN_COMTREE, CLIENT_LEAVE_COMTREE,
	CLIENT_RESIZE_COMTREE,
	CLIENT_GET_LEAF_RATE, CLIENT_MOD_LEAF_RATE,

	// internal control packet types
	ADD_IFACE, DROP_IFACE, GET_IFACE, MOD_IFACE,

	ADD_LINK, DROP_LINK, GET_LINK, MOD_LINK,

	ADD_COMTREE, DROP_COMTREE, GET_COMTREE, MOD_COMTREE,

	ADD_COMTREE_LINK, DROP_COMTREE_LINK,
	RESIZE_COMTREE_LINK,

	ADD_ROUTE, DROP_ROUTE, GET_ROUTE, MOD_ROUTE,

	ADD_ROUTE_LINK, DROP_ROUTE_LINK,
	
	CPT_END			///< marker following last CtlPktType
};

class CpType {
private:
	struct CpTypeInfo {
		int index;
		int code;
		const char* name;
		const char* abbrev;
		uint64_t reqAttr;	///< possible attributes for requests
		uint64_t reqReqAttr;	///< required attributes for requests
		uint64_t replyAttr;	///< possible attributes for replies
	};
	
	static CpTypeInfo typeInfo[];

	static bool firstCall;		///< used to check index consistency

	static void check(); 		///< check consistency of index values

public:
	static bool validIndex(CpTypeIndex);	///< check validity of index value
	static CpTypeIndex getIndexByCode(int);

	static int getCode(CpTypeIndex);		///< get code for a given index
	static const char*  getName(CpTypeIndex);	///< get name for a given index
	static const char*  getAbbrev(CpTypeIndex);	///< get abbreviation for a given index
	static bool isReqAttr(CpTypeIndex, CpAttrIndex);	///< true for valid request attributes
	static bool isReqReqAttr(CpTypeIndex, CpAttrIndex);	///< true for required request attributes
	static bool isRepAttr(CpTypeIndex, CpAttrIndex);	///< true for valid reply attributes

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

