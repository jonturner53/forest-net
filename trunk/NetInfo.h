/** @file NetInfo.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef NETINFO_H
#define NETINFO_H

#include <list>
#include <map>
#include "CommonDefs.h"
#include "UiHashTbl.h"
#include "UiDlist.h"
#include "Graph.h"

/** Maintains information about a Forest network.
 */
class NetInfo {
public:
		NetInfo(int,int,int,int);
		~NetInfo();

	// access methods
	string& getNodeName(int);
	int	getNodeNum(string&);
	ntyp_t	getNodeType(int);
	fAdr_t	getNodeAdr(int);
	double	getNodeLat(int);
	double	getNodeLong(int);
	// leaf only
	ipa_t	getLeafIpAdr(int);
	int	getLeafBitRate(int);
	int	getLeafPktRate(int);
	// router only
	int	getNumIf(int);
	fAdr_t	getFirstCliAdr(int);
	fAdr_t	getLastCliAdr(int);
	// router interfaces
	ipa_t	getIfIpAdr(int,int);
	int	getIfBitRate(int,int);
	int	getIfPktRate(int,int);
	int	getIfFirstLink(int,int);
	int	getIfLastLink(int,int);

	int	getLnkL(int);
	int	getLnkR(int);
	int	getLocLnkL(int);
	int	getLocLnkR(int);
	int	getLinkBitRate(int);
	int	getLinkPktRate(int);
	int	getLinkLength(int);
	int	getLinkNum(int,int);

	// predicates
	bool	validNode(int);
	bool	isLeaf(int);
	bool	isRouter(int);
	bool	validLink(int);
	bool	validIf(int, int);

	// modifiers
	void	setNodeName(int, string&);
	void	setNodeType(int, ntyp_t);
	void	setNodeAdr(int, fAdr_t);
	void	setNodeLat(int, double);
	void	setNodeLong(int, double);
	// leaf only
	void	setLeafIpAdr(int, ipa_t);
	void	setLeafBitRate(int, int);
	void	setLeafPktRate(int, int);
	// router only
	void	setNumIf(int, int);
	void	setFirstCliAdr(int, fAdr_t);
	void	setLastCliAdr(int, fAdr_t);
	// router interfaces
	void	setIfIpAdr(int,int,ipa_t);
	void	setIfIpAdr(int,ipa_t);
	void	setIfBitRate(int,int,int);
	void	setIfPktRate(int,int,int);
	void	setIfFirstLink(int,int,int);
	void	setIfLastLink(int,int,int);

	void	setLocLnkL(int,int);
	void	setLocLnkR(int,int);
	void	setLinkBitRate(int,int);
	void	setLinkPktRate(int,int);
	void	setLinkLength(int,int);

	// io routines
	bool read(istream&);
	void write(ostream&);

private:
	int maxRtr;		///< max node number for a router;
				///< leaf nodes all have larger node numbers
	int maxNode;		///< max node number in netTopo graph
	int maxLink;		///< max link number in netTopo graph
	int maxLeaf;		///< max number of leafs
	int maxCtl;		///< maximum number of controllers

	/** NetTopo is a weighted graph defining the network topology.
	 *  Weights represent link costs */
	Graph	*netTopo;

	struct IfInfo {
	ipa_t	ipAdr;		///< ip address of forest interface
	int	bitRate;	///< max bit rate for interface (Kb/s)
	int	pktRate;	///< max pkt rate (packets/s)
	int	firstLink;	///< number of first link assigned to interface
	int	lastLink;	///< number of last link assigned to interface
	};

	/** structure holding information used by leaf nodes */
	struct LeafNodeInfo {
	string	name;		///< leaf name
	ntyp_t	nType;		///< leaf type
	ipa_t	ipAdr;		///< IP address used to initialize node
	fAdr_t	fAdr;		///< leaf's forest address
	int	latitude;	///< latitude of leaf (in micro-degrees, + or -)
	int	longitude;	///< latitude of leaf (in micro-degrees, + or -)
	int	bitRate;	///< max bit rate for interface (Kb/s)
	int	pktRate;	///< max pkt rate (packets/s)
	};
	LeafNodeInfo *leaf;

	static int const UNDEF_LAT = 91;	// invalid latitude
	static int const UNDEF_LONG = 361;	// invalid longitude

	/** structure holding information used by all nodes */
	struct RtrNodeInfo {
	string	name;		///< node name
	ntyp_t	nType;		///< node type
	fAdr_t	fAdr;		///< node's forest address
	int	latitude;	///< latitude of node (in micro-degrees, + or -)
	int	longitude;	///< latitude of node (in micro-degrees, + or -)
	fAdr_t	firstCliAdr;	///< router's first assignable forest address
	fAdr_t  lastCliAdr;	///< router's last assignable forest address
	int	numIf;		///< number of interfaces
	IfInfo	*iface;		///< interface information
	};
	RtrNodeInfo *rtr;

	string	tmpBuffer;	///< used to return strings to callers

	/** maps a node name back to corresponding node number */
	map<string, int> *nodeNumMap;

	UiHashTbl *locLnk2lnk;	///< maps router/local link# to global link#
	uint64_t ll2l_key(int,int); ///< returns key used with locLnk2lnk

	struct LinkInfo {
	int	leftLnum;	///< local link number used by "left endpoint"
	int	rightLnum;	///< local link number used by "right endpoint"
	int	bitRate;	///< max bit rate for link
	int	pktRate;	///< max packet rate for link
	};
	LinkInfo *link;

	UiDlist	*freeNodes;	///< unused node numbers
	UiDlist	*freeLinks;	///< unused link numbers

	list<int> *routers;	///< list of routers (by node #) in network
	list<int> *controllers;	///< list of controllers (by node #)
};

inline bool NetInfo::validNode(int n) {
	return (isLeaf(n) || isRouter(n));
}

inline bool NetInfo::isRouter(int n) {
	return (1 <= n && n <= maxRtr && rtr[n].fAdr != 0);
}

inline bool NetInfo::isLeaf(int n) {
	return (maxRtr < n && n <= maxNode && leaf[n-maxRtr].fAdr != 0);
}

inline bool NetInfo::validLink(int lnk) {
	return (1 <= lnk && lnk <= netTopo->m() && netTopo->left(lnk) > 0);
}

inline bool NetInfo::validIf(int n, int iface) {
	return isRouter(n) && (1 <= iface && iface <= rtr[n].numIf &&
					  rtr[n].iface[iface].ipAdr != 0);
}

/** Get the name for a given node number.
 *  @param n is a node number
 *  @return a reference to an internal string object with string
 *  value being the name corresponding to the given node number,
 *  or the empty string if there is no such node;
 *  the value of this string may change the next time any method
 *  is called on this NetInfo object
 */
inline string& NetInfo::getNodeName(int n) {
	tmpBuffer = (isLeaf(n) ? leaf[n-maxRtr].name : 
		     (isRouter(n) ? rtr[n].name : ""));
	return tmpBuffer;
}

inline int NetInfo::getNodeNum(string& s) {
	map<string,int>::iterator p = nodeNumMap->find(s);
	return ((p != nodeNumMap->end()) ? p->second : 0);
}

inline ntyp_t NetInfo::getNodeType(int n) {
	return (isLeaf(n) ? leaf[n-maxRtr].nType : 
		(isRouter(n) ? rtr[n].nType : UNDEF_NODE));
}

inline ipa_t NetInfo::getLeafIpAdr(int n) {
	return (isLeaf(n) ? leaf[n-maxRtr].ipAdr : 0);
}

inline int NetInfo::getLeafBitRate(int n) {
	return (isLeaf(n) ? leaf[n-maxRtr].bitRate : 0);
}

inline int NetInfo::getLeafPktRate(int n) {
	return (isLeaf(n) ? leaf[n-maxRtr].pktRate : 0);
}

inline fAdr_t NetInfo::getNodeAdr(int n) {
	return (isLeaf(n) ? leaf[n-maxRtr].fAdr :
		(isRouter(n) ? rtr[n].fAdr : 0));
}

inline fAdr_t NetInfo::getFirstCliAdr(int n) {
	return (isRouter(n) ? rtr[n].firstCliAdr : 0);
}

inline fAdr_t NetInfo::getLastCliAdr(int n) {
	return (isRouter(n) ? rtr[n].lastCliAdr : 0);
}

inline int NetInfo::getNumIf(int n) {
	return (isRouter(n) ? rtr[n].numIf : 0);
}

inline double NetInfo::getNodeLat(int n) {
	double x = (isLeaf(n) ? leaf[n-maxRtr].latitude :
		    (isRouter(n) ? rtr[n].latitude : UNDEF_LAT));
	return x/1000000;
}

inline double NetInfo::getNodeLong(int n) {
	double x = (isLeaf(n) ? leaf[n-maxRtr].longitude :
		    (isRouter(n) ? rtr[n].longitude : UNDEF_LONG));
	return x/1000000;
}

inline ipa_t NetInfo::getIfIpAdr(int n, int iface) {
	return (validIf(n,iface) ? rtr[n].iface[iface].ipAdr : 0);
}

inline int NetInfo::getIfFirstLink(int n, int iface) {
	return (validIf(n,iface) ? rtr[n].iface[iface].firstLink : 0);
}

inline int NetInfo::getIfLastLink(int n, int iface) {
	return (validIf(n,iface) ? rtr[n].iface[iface].lastLink : 0);
}

inline int NetInfo::getIfBitRate(int n, int iface) {
	return (validIf(n,iface) ? rtr[n].iface[iface].bitRate : 0);
}

inline int NetInfo::getIfPktRate(int n, int iface) {
	return (validIf(n,iface) ? rtr[n].iface[iface].pktRate : 0);
}

inline int NetInfo::getLnkL(int lnk) {
	return (validLink(lnk) ? netTopo->left(lnk) : 0);
}

inline int NetInfo::getLnkR(int lnk) {
	return (validLink(lnk) ? netTopo->right(lnk) : 0);
}

inline int NetInfo::getLocLnkL(int lnk) {
	return (validLink(lnk) ? link[lnk].leftLnum : 0);
}

inline int NetInfo::getLocLnkR(int lnk) {
	return (validLink(lnk) ? link[lnk].rightLnum : 0);
}

inline int NetInfo::getLinkBitRate(int lnk) {
	return (validLink(lnk) ? link[lnk].bitRate : 0);
}

inline int NetInfo::getLinkPktRate(int lnk) {
	return (validLink(lnk) ? link[lnk].pktRate : 0);
}

inline int NetInfo::getLinkLength(int lnk) {
	return (validLink(lnk) ? netTopo->length(lnk) : 0);
}

inline int NetInfo::getLinkNum(int n, int llnk) {
	uint64_t key = n; key <<= 32; key |= (llnk & 0xffffffff);

	return locLnk2lnk->lookup(key);
}

inline void NetInfo::setNodeName(int n, string& nam) {
	if (isLeaf(n)) leaf[n-maxRtr].name = nam;
	else if (isRouter(n)) rtr[n-maxRtr].name = nam;
	return;
}

inline void NetInfo::setNodeType(int n, ntyp_t typ) {
	if (isLeaf(n)) leaf[n-maxRtr].nType = typ;
	else if (isRouter(n)) rtr[n].nType = typ;
	return;
}

inline void NetInfo::setNodeAdr(int n, fAdr_t adr) {
	if (isLeaf(n)) leaf[n-maxRtr].fAdr = adr;
	else if (isRouter(n)) rtr[n].fAdr = adr;
	return;
}

inline void NetInfo::setNodeLat(int n, double lat) {
	if (isLeaf(n)) leaf[n-maxRtr].latitude = (int) (lat*1000000);
	else if (isRouter(n)) rtr[n].latitude  = (int) (lat*1000000);
	return;
}

inline void NetInfo::setNodeLong(int n, double longg) {
	if (isLeaf(n)) leaf[n-maxRtr].longitude = (int) (longg*1000000);
	else if (isRouter(n)) rtr[n].longitude  = (int) (longg*1000000);
	return;
}

inline void NetInfo::setIfIpAdr(int n, int iface, ipa_t ip) {
	if (isRouter(iface)) rtr[n].iface[iface].ipAdr = ip;
	return;
}

inline void NetInfo::setLeafIpAdr(int n, ipa_t ip) {
	if (isLeaf(n)) leaf[n-maxRtr].ipAdr = ip;
	return;
}

inline void NetInfo::setLeafBitRate(int n, int br) {
	if (isLeaf(n)) leaf[n-maxRtr].bitRate = br;
	return;
}

inline void NetInfo::setLeafPktRate(int n, int pr) {
	if (isLeaf(n)) leaf[n-maxRtr].pktRate = pr;
	return;
}

inline void NetInfo::setFirstCliAdr(int n, fAdr_t adr) {
	if (isRouter(n)) rtr[n].firstCliAdr = adr;
	return;
}

inline void NetInfo::setLastCliAdr(int n, fAdr_t adr) {
	if (isRouter(n)) rtr[n].lastCliAdr = adr;
	return;
}

inline void NetInfo::setNumIf(int n, int num) {
	if (isRouter(n)) rtr[n].numIf = num;
	return;
}

inline void NetInfo::setIfBitRate(int n, int iface, int br) {
	if (validIf(n,iface)) rtr[n].iface[iface].bitRate = br;
	return;
}

inline void NetInfo::setIfPktRate(int n, int iface, int pr) {
	if (validIf(n,iface)) rtr[n].iface[iface].pktRate = pr;
}

inline void NetInfo::setIfFirstLink(int n, int iface, int lnk) {
	if (validIf(n,iface)) rtr[n].iface[iface].firstLink = lnk;
	return;
}

inline void NetInfo::setIfLastLink(int n, int iface, int lnk) {
	if (validIf(n,iface)) rtr[n].iface[iface].lastLink = lnk;
	return;
}

inline void NetInfo::setLocLnkL(int lnk, int loc) {
	if (validLink(lnk)) link[lnk].leftLnum = loc;
	return;
}

inline void NetInfo::setLocLnkR(int lnk, int loc) {
	if (validLink(lnk)) link[lnk].rightLnum = loc;
	return;
}

inline void NetInfo::setLinkBitRate(int lnk, int br) {
	if (validLink(lnk)) link[lnk].bitRate = br;
	return;
}

inline void NetInfo::setLinkPktRate(int lnk, int pr) {
	if (validLink(lnk)) link[lnk].pktRate = pr;
	return;
}

inline void NetInfo::setLinkLength(int lnk, int len) {
	if (validLink(lnk)) netTopo->setLength(lnk,len);
	return;
}

inline uint64_t NetInfo::ll2l_key(int n, int llnk) {
	return (uint64_t(n) << 32) | (uint64_t(llnk) & 0xffffffff);
}

enum tokenType {
	RTR_SEC,
	CTLR_SEC,
	CLIENT_SEC,
	LINK_SEC,
	NAME,
	INTEGER,
	FLOAT,
	IP_ADR,
	F_ADR,
	LPAREN,
	RPAREN,
	PERIOD,
	COMMA,
	COLON,
	SEMI,
	SEMI2,
};
	

/* Notes

Use cases:

Initialization
- read network topology information and populate NetInfo data
- perform consistency checks
- contact each listed node and initialize it; nodes listen on
  bootstrap address; use blocking TCP connections
- start routers, then controllers, then clients (if any)

Adding/removing access links
- get request from client controller with ip address of a new client
- select a router and unused link
- send router an add link message - let router assign link#
  update netTopo and anything else that requires it
- send ack back to client controller
- when client sends connect, we should have router send informational
  message to NetMgr, then back to client manager
- when client sends disconnect to router, we should have router
  send informational update to NetMgr; NetMgr should also
  inform client manager when this happens, so client manager can
  maintain accounting information; could have router record
  data counters for access links and send this to NetMgr on close

Log updates to NetInfo?

Process control messages from CLI
- control messages can be directed to NetMgr; these are checked,
  then we issue required updates to routers; then we update internal data
  and send ack
  - messages for the NetMgr include things like adding new nodes,
    and adding or modifying links
  - in general, could also get information about nodes from NetMgr
  - should NetMgr send router names/forest addresses back to CLI?
    would allow users to refer to routers by name rather than address
  - might want to do same thing for controllers and even clients
- for messages to routers, forward and then update NetInfo when
  ack is received

Interactions with comtree controller(s)

- Initialize comtree controllers on startup; includes sending them
  topology information
- As links are added/removed/modified, let comtree controllers know,
  so that they can take this into account when selecting comtree routes

Statistics Collector
- Statistics collector consolidates data from routers (instead of routers
  writing these to files). Might have data combined in some fashion
  and maybe even analyzed. Could send processed data to remote GUI
  for display, but not clear if it's worth duplicating RLI functionality.
*/

#endif
