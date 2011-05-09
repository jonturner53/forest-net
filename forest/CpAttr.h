
/** \file CpAttr.h
 *  Header file for CpAttr class.
 */

#ifndef CPATTR_H
#define CPATTR_H

#include "forest.h"

/** Control packet types indices */
enum CpAttrIndex {
	CPA_START,			///< marker preceding first index

	BIT_RATE,
	BIT_RATE_DOWN,
	BIT_RATE_UP,
	COMTREE_NUM,
	COMTREE_OWNER,
	CORE_FLAG,
	DEST_ADR,
	EXT_BIT_RATE_DOWN,
	EXT_BIT_RATE_UP,
	EXT_PKT_RATE_DOWN,
	EXT_PKT_RATE_UP,
	IFACE_NUM,
	INT_BIT_RATE_DOWN,
	INT_BIT_RATE_UP,
	INT_PKT_RATE_DOWN,
	INT_PKT_RATE_UP,
	LEAF_ADR,
	LEAF_COUNT,
	LINK_NUM,
	LOCAL_IP, 		///< LOCAL_IP: ip address of a local interface 
	MAX_BIT_RATE,
	MAX_PKT_RATE,
	PARENT_LINK,
	PEER_ADR,
	PEER_DEST,
	PEER_IP,
	PEER_PORT,
	PEER_TYPE,
	PKT_RATE,
	PKT_RATE_DOWN,
	PKT_RATE_UP,
	QUEUE_NUM,
//	REMOTE_IP,
//	REMOTE_PORT,

	CPA_END			///< marker following last attribute
};

struct CpAttrInfo {
	int index; int code; const char* name;
};

class CpAttr {
private:
	static const CpAttrInfo attrInfo[50];

	static bool firstCall;		///< used to check index consistency

	static void check(); 		///< check consistency of index values

public:
	static bool validIndex(CpAttrIndex);	///< check validity of index value
	static CpAttrIndex getIndexByCode(int);

	static int getCode(CpAttrIndex);	///< get code for a given index
	static const char* getName(CpAttrIndex);	///< get name for a given index

	static CpAttrIndex findMatch(char *);
};

inline bool CpAttr::validIndex(CpAttrIndex i) {
	if (firstCall) check();
	return i > CPA_START && i < CPA_END;
}

/** Get the code for a given control packet attr index
 */
inline int CpAttr::getCode(CpAttrIndex i) {
	if (firstCall) check();
	return (validIndex(i) ? attrInfo[i].code : 0);
}

/** Get the name for a given control packet attr index
 */
inline const char* CpAttr::getName(CpAttrIndex i) {
	if (firstCall) check();
	return (validIndex(i) ? attrInfo[i].name : "undefined");
}

#endif
