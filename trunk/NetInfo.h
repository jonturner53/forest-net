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
#include <set>
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

	// predicates
	bool	validNode(int) const;
	bool	isLeaf(int) const;
	bool	isRouter(int) const;
	bool	validLink(int) const;
	bool	validIf(int, int) const;
	bool	validComtree(int) const;
	bool	validComtIndex(int) const;
	bool	isComtCoreNode(int,int) const;
	bool	isComtLink(int,int) const;

	// access methods
	string& getNodeName(int);
	int	getNodeNum(string&) const;
	ntyp_t	getNodeType(int) const;
	fAdr_t	getNodeAdr(int) const;
	double	getNodeLat(int) const;
	double	getNodeLong(int) const;
	// leaf only
	ipa_t	getLeafIpAdr(int) const;
	// router only
	int	getNumIf(int) const;
	fAdr_t	getFirstCliAdr(int) const;
	fAdr_t	getLastCliAdr(int) const;
	// router interfaces
	ipa_t	getIfIpAdr(int,int) const;
	int	getIfBitRate(int,int) const;
	int	getIfPktRate(int,int) const;
	int	getIfFirstLink(int,int) const;
	int	getIfLastLink(int,int) const;
	// link attributes
	int	getLnkL(int) const;
	int	getLnkR(int) const;
	int	getLocLnk(int,int) const;
	int	getLocLnkL(int) const;
	int	getLocLnkR(int) const;
	int	getLinkBitRate(int) const;
	int	getLinkPktRate(int) const;
	int	getLinkLength(int) const;
	int	getLinkNum(int) const;
	int	getLinkNum(int,int) const;
	// comtree attributes
	int	lookupComtree(int) const;
	int	getComtree(int) const;
	int	getComtRoot(int) const;
	int	getComtBrDown(int) const;
	int	getComtBrUp(int) const;
	int	getComtPrDown(int) const;
	int	getComtPrUp(int) const;
	int	getComtLeafBrDown(int) const;
	int	getComtLeafBrUp(int) const;
	int	getComtLeafPrDown(int) const;
	int	getComtLeafPrUp(int) const;
	set<int>::iterator getFirstCore(int) const;
	set<int>::iterator getLastCore(int) const;
	set<int>::iterator getFirstComtLink(int) const;
	set<int>::iterator getLastComtLink(int) const;

	// modifiers
	void	setNodeName(int, string&);
	void	setNodeType(int, ntyp_t);
	void	setNodeAdr(int, fAdr_t);
	void	setNodeLat(int, double);
	void	setNodeLong(int, double);
	// leaf only
	void	setLeafIpAdr(int, ipa_t);
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
	// links
	void	setLocLnkL(int,int);
	void	setLocLnkR(int,int);
	void	setLinkBitRate(int,int);
	void	setLinkPktRate(int,int);
	void	setLinkLength(int,int);
	// comtrees
	bool	addComtree(int);
	bool	removeComtree(int);
	bool	setComtRoot(int,int);
	bool	setComtBrDown(int,int);
	bool	setComtBrUp(int,int);
	bool	setComtPrDown(int,int);
	bool	setComtPrUp(int,int);
	bool	setComtLeafBrDown(int,int);
	bool	setComtLeafBrUp(int,int);
	bool	setComtLeafPrDown(int,int);
	bool	setComtLeafPrUp(int,int);
	bool	addComtCoreNode(int,int);
	bool	removeComtCoreNode(int,int);
	bool	addComtLink(int,int);
	bool	removeComtLink(int,int);

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
	int maxComtree;		///< maximum number of comtrees

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
	};
	LeafNodeInfo *leaf;
	UiList	*freeLeaves;	///< unused leaf numbers

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
	UiList	*freeRouters;	///< unused router numbers

	string	tmpBuffer;	///< used to return strings to callers

	/** maps a node name back to corresponding node number */
	map<string, int> *nodeNumMap;

	UiHashTbl *locLnk2lnk;	///< maps router/local link# to global link#
	uint64_t ll2l_key(int,int) const; ///< returns key used with locLnk2lnk

	struct LinkInfo {
	int	leftLnum;	///< local link number used by "left endpoint"
	int	rightLnum;	///< local link number used by "right endpoint"
	int	bitRate;	///< max bit rate for link
	int	pktRate;	///< max packet rate for link
	};
	LinkInfo *link;

	list<int> *routers;	///< list of routers (by node #) in network
	list<int> *controllers;	///< list of controllers (by node #)

	struct ComtreeInfo {
	int	comtreeNum;	///< number of comtree
	int	root;		///< root node of comtree
	int	bitRateDown;	///< downstream bit rate for backbone links
	int	bitRateUp;	///< upstream bit rate for backbone links
	int	pktRateDown;	///< downstream packet rate for backbone links
	int	pktRateUp;	///< upstream packet rate for backbone links
	int	leafBitRateDown;///< downstream bit rate for leaf links
	int	leafBitRateUp;	///< upstream bit rate for leaf links
	int	leafPktRateDown;///< downstream packet rate for leaf links
	int	leafPktRateUp;	///< upstream packet rate for leaf links
	set<int> *coreSet;	///< set of core nodes in comtree
	set<int> *linkSet;	///< set of links in comtree
	};
	ComtreeInfo *comtree;	///< array of comtrees
	UiList	*freeComtIndices;///< unused indices in table of comtrees
	UiHashTbl *comtreeMap;	///< maps comtree numbers to indices in array
	uint64_t comtreeMapKey(int) const; ///< returns key used with comtreeMap
};

inline bool NetInfo::validNode(int n) const {
	return (isLeaf(n) || isRouter(n));
}

inline bool NetInfo::isRouter(int n) const {
	return (1 <= n && n <= maxRtr && rtr[n].fAdr != 0);
}

inline bool NetInfo::isLeaf(int n) const {
	return (maxRtr < n && n <= maxNode && leaf[n-maxRtr].fAdr != 0);
}

inline bool NetInfo::validLink(int lnk) const {
	return (1 <= lnk && lnk <= netTopo->m() && netTopo->left(lnk) > 0);
}

inline bool NetInfo::validIf(int n, int iface) const {
	return isRouter(n) && (1 <= iface && iface <= rtr[n].numIf &&
					  rtr[n].iface[iface].ipAdr != 0);
}

inline bool NetInfo::validComtree(int comt) const {
	return comtreeMap->lookup(comtreeMapKey(comt)) != 0;
}

inline bool NetInfo::validComtIndex(int i) const {
	return 1 <= i && i <= maxComtree && comtree[i].comtreeNum != 0;
}

inline bool NetInfo::isComtCoreNode(int i, int x) const {
	return validComtIndex(i) &&
		comtree[i].coreSet->find(x) != comtree[i].coreSet->end();
}

inline bool NetInfo::isComtLink(int i, int lnk) const {
	return validComtIndex(i) &&
		comtree[i].linkSet->find(lnk) != comtree[i].linkSet->end();
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

inline int NetInfo::getNodeNum(string& s) const {
	map<string,int>::iterator p = nodeNumMap->find(s);
	return ((p != nodeNumMap->end()) ? p->second : 0);
}

inline ntyp_t NetInfo::getNodeType(int n) const {
	return (isLeaf(n) ? leaf[n-maxRtr].nType : 
		(isRouter(n) ? rtr[n].nType : UNDEF_NODE));
}

inline ipa_t NetInfo::getLeafIpAdr(int n) const {
	return (isLeaf(n) ? leaf[n-maxRtr].ipAdr : 0);
}

inline fAdr_t NetInfo::getNodeAdr(int n) const {
	return (isLeaf(n) ? leaf[n-maxRtr].fAdr :
		(isRouter(n) ? rtr[n].fAdr : 0));
}

inline fAdr_t NetInfo::getFirstCliAdr(int n) const {
	return (isRouter(n) ? rtr[n].firstCliAdr : 0);
}

inline fAdr_t NetInfo::getLastCliAdr(int n) const {
	return (isRouter(n) ? rtr[n].lastCliAdr : 0);
}

inline int NetInfo::getNumIf(int n) const {
	return (isRouter(n) ? rtr[n].numIf : 0);
}

inline double NetInfo::getNodeLat(int n) const {
	double x = (isLeaf(n) ? leaf[n-maxRtr].latitude :
		    (isRouter(n) ? rtr[n].latitude : UNDEF_LAT));
	return x/1000000;
}

inline double NetInfo::getNodeLong(int n) const {
	double x = (isLeaf(n) ? leaf[n-maxRtr].longitude :
		    (isRouter(n) ? rtr[n].longitude : UNDEF_LONG));
	return x/1000000;
}

inline ipa_t NetInfo::getIfIpAdr(int n, int iface) const {
	return (validIf(n,iface) ? rtr[n].iface[iface].ipAdr : 0);
}

inline int NetInfo::getIfFirstLink(int n, int iface) const {
	return (validIf(n,iface) ? rtr[n].iface[iface].firstLink : 0);
}

inline int NetInfo::getIfLastLink(int n, int iface) const {
	return (validIf(n,iface) ? rtr[n].iface[iface].lastLink : 0);
}

inline int NetInfo::getIfBitRate(int n, int iface) const {
	return (validIf(n,iface) ? rtr[n].iface[iface].bitRate : 0);
}

inline int NetInfo::getIfPktRate(int n, int iface) const {
	return (validIf(n,iface) ? rtr[n].iface[iface].pktRate : 0);
}

inline int NetInfo::getLnkL(int lnk) const {
	return (validLink(lnk) ? netTopo->left(lnk) : 0);
}

inline int NetInfo::getLnkR(int lnk) const {
	return (validLink(lnk) ? netTopo->right(lnk) : 0);
}

inline int NetInfo::getLocLnk(int lnk, int n) const {
	return (!validLink(lnk) ? 0 :
		(n == netTopo->left(lnk) ? getLocLnkL(lnk) :
		(n == netTopo->right(lnk) ? getLocLnkR(lnk) : 0)));
}

inline int NetInfo::getLocLnkL(int lnk) const {
	return (validLink(lnk) ? link[lnk].leftLnum : 0);
}

inline int NetInfo::getLocLnkR(int lnk) const {
	return (validLink(lnk) ? link[lnk].rightLnum : 0);
}

inline int NetInfo::getLinkBitRate(int lnk) const {
	return (validLink(lnk) ? link[lnk].bitRate : 0);
}

inline int NetInfo::getLinkPktRate(int lnk) const {
	return (validLink(lnk) ? link[lnk].pktRate : 0);
}

inline int NetInfo::getLinkLength(int lnk) const {
	return (validLink(lnk) ? netTopo->length(lnk) : 0);
}

inline int NetInfo::getLinkNum(int n) const {
	return (isLeaf(n) ? netTopo->first(n) : 0);
}

inline int NetInfo::getLinkNum(int n, int llnk) const {
	return (isRouter(n) ? locLnk2lnk->lookup(ll2l_key(n,llnk)) : 0);
}

inline int NetInfo::lookupComtree(int comt) const {
	return comtreeMap->lookup(comt);
}

inline int NetInfo::getComtree(int c) const {
	return (validComtIndex(c) ? comtree[c].comtreeNum : 0);
}

inline int NetInfo::getComtRoot(int c) const {
	return (validComtIndex(c) ? comtree[c].root : 0);
}

inline int NetInfo::getComtBrDown(int c) const {
	return (validComtIndex(c) ? comtree[c].bitRateDown : 0);
}

inline int NetInfo::getComtBrUp(int c) const {
	return (validComtIndex(c) ? comtree[c].bitRateUp : 0);
}

inline int NetInfo::getComtPrDown(int c) const {
	return (validComtIndex(c) ? comtree[c].pktRateDown : 0);
}

inline int NetInfo::getComtPrUp(int c) const {
	return (validComtIndex(c) ? comtree[c].pktRateUp : 0);
}

inline int NetInfo::getComtLeafBrDown(int c) const {
	return (validComtIndex(c) ? comtree[c].leafBitRateDown : 0);
}

inline int NetInfo::getComtLeafBrUp(int c) const {
	return (validComtIndex(c) ? comtree[c].leafBitRateUp : 0);
}

inline int NetInfo::getComtLeafPrDown(int c) const {
	return (validComtIndex(c) ? comtree[c].leafPktRateDown : 0);
}

inline int NetInfo::getComtLeafPrUp(int c) const {
	return (validComtIndex(c) ? comtree[c].leafPktRateUp : 0);
}

inline set<int>::iterator NetInfo::getFirstCore(int c) const {
	if (validComtIndex(c)) return comtree[c].coreSet->begin();
	else return 0;
	//return (validComtIndex(c) ? comtree[c].coreSet->begin() : 0);
}

inline set<int>::iterator NetInfo::getLastCore(int c) const {
	return (validComtIndex(c) ? comtree[c].coreSet->end() : 0);
}

inline set<int>::iterator NetInfo::getFirstComtLink(int c) const {
	return (validComtIndex(c) ? comtree[c].linkSet->begin() : 0);
}

inline set<int>::iterator NetInfo::getLastComtLink(int c) const {
	return (validComtIndex(c) ? comtree[c].linkSet->end() : 0);
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
inline bool NetInfo::addComtree(int comt) {
	int i = freeComtIndices->first();
	if (i == 0) return false;
	if (comtreeMap->insert(comtreeMapKey(comt),i)) {
		comtree[i].comtreeNum = comt;
		return true;
	}
	freeComtIndices->addFirst(i);
	return false;
}

inline bool NetInfo::removeComtree(int i) {
	if (!validComtIndex(i)) return false;
	comtreeMap->remove(comtreeMapKey(comtree[i].comtreeNum));
	comtree[i].comtreeNum = 0;
	freeComtIndices->addFirst(i);
	return true;
}

inline bool NetInfo::setComtRoot(int i, int r) {
	if (!validComtIndex(i)) return false;
	comtree[i].root = r;
	return true;
}

inline bool NetInfo::setComtBrDown(int i, int br) {
	if (!validComtIndex(i)) return false;
	comtree[i].bitRateDown = br;
	return true;
}

inline bool NetInfo::setComtBrUp(int i, int br) {
	if (!validComtIndex(i)) return false;
	comtree[i].bitRateUp = br;
	return true;
}

inline bool NetInfo::setComtPrDown(int i, int pr) {
	if (!validComtIndex(i)) return false;
	comtree[i].pktRateDown = pr;
	return true;
}

inline bool NetInfo::setComtPrUp(int i, int pr) {
	if (!validComtIndex(i)) return false;
	comtree[i].pktRateUp = pr;
	return true;
}

inline bool NetInfo::setComtLeafBrDown(int i, int br) {
	if (!validComtIndex(i)) return false;
	comtree[i].leafBitRateDown = br;
	return true;
}

inline bool NetInfo::setComtLeafBrUp(int i, int br) {
	if (!validComtIndex(i)) return false;
	comtree[i].leafBitRateUp = br;
	return true;
}

inline bool NetInfo::setComtLeafPrDown(int i, int pr) {
	if (!validComtIndex(i)) return false;
	comtree[i].leafPktRateDown = pr;
	return true;
}

inline bool NetInfo::setComtLeafPrUp(int i, int pr) {
	if (!validComtIndex(i)) return false;
	comtree[i].leafPktRateUp = pr;
	return true;
}

inline bool NetInfo::addComtCoreNode(int i, int n) {
	if (!validComtIndex(i) || !isRouter(n)) return false;
	comtree[i].coreSet->insert(n);
	return true;
}

inline bool NetInfo::removeComtCoreNode(int i, int n) {
	if (!validComtIndex(i) || !isRouter(n)) return false;
	comtree[i].coreSet->erase(n);
	return true;
}

inline bool NetInfo::addComtLink(int i, int lnk) {
	if (!validComtIndex(i) || !validLink(lnk)) return false;
	comtree[i].linkSet->insert(lnk);
	return true;
}

inline bool NetInfo::removeComtLink(int i, int lnk) {
	if (!validComtIndex(i) || !validLink(lnk)) return false;
	comtree[i].linkSet->erase(lnk);
	return true;
}

inline uint64_t NetInfo::ll2l_key(int n, int llnk) const {
	return (uint64_t(n) << 32) | (uint64_t(llnk) & 0xffffffff);
}

inline uint64_t NetInfo::comtreeMapKey(int comtree) const {
	return (uint64_t(comtree) << 32) | (uint64_t(comtree) & 0xffffffff);
}


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
