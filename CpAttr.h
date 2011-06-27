/** @file CpAttr.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef CPATTR_H
#define CPATTR_H

#include "CommonDefs.h"

/** Control packet attribute indices.
 *  Each attribute has an integer index used within programs to access
 *  information about that attribute.
 */
enum CpAttrIndex {
	CPA_START,			///< marker preceding first index

	BIT_RATE,		///< bit rate in Kb/s
	BIT_RATE_DOWN,		///< downstream bit rate in Kb/s
	BIT_RATE_UP,		///< upstream bit rate in Kb/s
	CLIENT_ADR,		///< forest address assigned to a client
	CLIENT_IP,		///< IP address of a client
	COMTREE_NUM,		///< a comtree number
	COMTREE_OWNER,		///< the comtree owner
	CORE_FLAG,		///< core flag for a comtree
	DEST_ADR,		///< destination address
	EXT_BIT_RATE_DOWN,	///< bit rate of an access link in Kb/s
	EXT_BIT_RATE_UP,	///< bit rate of an access link in Kb/s
	EXT_PKT_RATE_DOWN,	///< packet rate of an access link in p/s
	EXT_PKT_RATE_UP,	///< packet rate of an access link in p/s
	IFACE_NUM,		///< interface number
	INT_BIT_RATE_DOWN,	///< bit rate of an internal link in Kb/s
	INT_BIT_RATE_UP,	///< bit rate of an internal link in Kb/s
	INT_PKT_RATE_DOWN,	///< packet rate of an internal link in p/s
	INT_PKT_RATE_UP,	///< packet rate of an internal link in p/s
	LEAF_ADR,		///< address of a leaf node in a comtree
	LEAF_COUNT,		///< number of leaves in a comtree
	LINK_NUM,		///< link number
	LOCAL_IP, 		///< ip address of a local interface 
	MAX_BIT_RATE,		///< max bit rate of an interface in Kb/s
	MAX_PKT_RATE,		///< max packet rate of an interface in p/s
	PARENT_LINK,		///< link to parent in a comtree
	PEER_ADR,		///< address of a link's peer node
	PEER_DEST,		///< restricted destination address for peer
	PEER_IP,		///< IP address of a link's peer node
	PEER_PORT,		///< UDP port number of a link's peer node
	PEER_TYPE,		///< node type address of a link's peer node
	PKT_RATE,		///< packet rate in p/s
	PKT_RATE_DOWN,		///< downstream packet rate in p/s
	PKT_RATE_UP,		///< upstream packet rate in p/s
	QUEUE_NUM,		///< queue number
	RTR_ADR,		///< forest address of a router
	RTR_IP,			///< IP address of a router

	CPA_END			///< marker following last attribute
};


/** This class provides support for control packet attributes in Forest.
 */
class CpAttr {
private:
	/** structure to hold attribute information */
	struct CpAttrInfo {
		int index; int code; const char* name;
	};

	/** vector of information for attributes */
	static const CpAttrInfo attrInfo[CPA_END+1]; 

	static bool firstCall;		///< used to check index consistency

	static void check(); 		
public:
	static bool validIndex(CpAttrIndex);
	static CpAttrIndex getIndexByCode(int);

	static int getCode(CpAttrIndex);
	static const char* getName(CpAttrIndex);

	static CpAttrIndex findMatch(char *);
};

/** Determine if a given attribute index is valid.
 *  @param i is a puported attribute index
 *  @return true if i is a valid attribute index
 */
inline bool CpAttr::validIndex(CpAttrIndex i) {
	if (firstCall) check();
	return i > CPA_START && i < CPA_END;
}

/** Get the code for a given attribute index.
 *  @param i is an attribute index
 *  @return the code for the attribute with index i
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
