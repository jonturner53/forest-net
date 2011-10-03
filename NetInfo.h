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
#include "IdMap.h"
#include "UiSetPair.h"
#include "Graph.h"

struct RateSpec {	///< used in linkMap of ComtreeInfo
	int	bitRateLeft;
	int	bitRateRight;
	int	pktRateLeft;
	int	pktRateRight;
};

/** Maintains information about an entire Forest network.
 *  The NetInfo data structure is intended for use by network
 *  control elements that require a global view of the network.
 *  Currently, this includes the NetMgr and the ComtreeController
 *  components.
 *
 *  Internally, a NetInfo object uses a Graph to represent the
 *  network topology. It also has tables of attributes for all
 *  nodes, links and comtrees. It provides a large number of
 *  methods for accessing the internal representation in a
 *  convenient way, It also has a read() method to read in a network
 *  description from a file and build the corresponding internal
 *  data structure. And it has a write() method that will write
 *  out a network description.
 */
class NetInfo {
public:
		NetInfo(int,int,int,int,int);
		~NetInfo();

	// methods for working with nodes
	// predicates
	bool	validNode(int) const;
	// iterating through nodes
	int	firstNode() const;
	int	nextNode(int) const;
	// access node attributes
	int	getMaxNode() const;
	string& getNodeName(int,string&);
	int	getNodeNum(string&) const;
	ntyp_t	getNodeType(int) const;
	fAdr_t	getNodeAdr(int) const;
	double	getNodeLat(int) const;
	double	getNodeLong(int) const;
	// modify node attributes
	bool	setNodeName(int, string&);
	bool	setNodeAdr(int, fAdr_t);
	bool	setNodeLat(int, double);
	bool	setNodeLong(int, double);

	// additional methods for leaf nodes
	// predicates
	bool	isLeaf(int) const;
	int	firstLeaf() const;
	int	nextLeaf(int) const;
	int	firstController() const;
	int	nextController(int) const;
	// access leaf attributes
	ipa_t	getLeafIpAdr(int) const;
	// add/modify leaves
	int	addLeaf(const string&, ntyp_t);
	bool	setLeafType(int, ntyp_t);
	bool	setLeafIpAdr(int, ipa_t);

	// additional methods for routers
	// predicates
	bool	isRouter(int) const;
	bool	validIf(int, int) const;
	// iterating through routers
	int	firstRouter() const;
	int	nextRouter(int) const;
	// access router attributes
	int	getMaxRouter() const;
	int	getNumRouter() const;
	int	getIface(int, int);
	int	getNumIf(int) const;
	fAdr_t	getFirstCliAdr(int) const;
	fAdr_t	getLastCliAdr(int) const;
	ipa_t	getIfIpAdr(int,int) const;
	int	getIfBitRate(int,int) const;
	int	getIfPktRate(int,int) const;
	int	getIfFirstLink(int,int) const;
	int	getIfLastLink(int,int) const;
	// add/modify routers
	int	addRouter(const string&);
	bool	addInterfaces(int, int);
	bool	setFirstCliAdr(int, fAdr_t);
	bool	setLastCliAdr(int, fAdr_t);
	bool	setIfIpAdr(int,int,ipa_t);
	bool	setIfIpAdr(int,ipa_t);
	bool	setIfBitRate(int,int,int);
	bool	setIfPktRate(int,int,int);
	bool	setIfFirstLink(int,int,int);
	bool	setIfLastLink(int,int,int);

	// methods for working with links
	// predicates
	bool	validLink(int) const;
	// iterating through links
	int	firstLink() const;
	int	nextLink(int) const;
	int	firstLinkAt(int) const;
	int	nextLinkAt(int,int) const;
	// access link attributes
	int	getMaxLink() const;
	int	getLinkL(int) const;
	int	getLinkR(int) const;
	int	getPeer(int,int) const;
	int	getLocLink(int,int) const;
	int	getLocLinkL(int) const;
	int	getLocLinkR(int) const;
	int	getLinkBitRate(int) const;
	int	getLinkPktRate(int) const;
	int	getLinkLength(int) const;
	int	getLinkNum(int) const;
	int	getLinkNum(int,int) const;
	// add/modify links
	int	addLink(int,int,int,int);
	bool	setLocLinkL(int,int);
	bool	setLocLinkR(int,int);
	bool	setLinkBitRate(int,int);
	bool	setLinkPktRate(int,int);
	bool	setLinkLength(int,int);

	// methods for working with comtrees
	// predicates
	bool	validComtree(int) const;
	bool	validComtIndex(int) const;
	bool	isComtCoreNode(int,int) const;
	bool	isComtLink(int,int) const;
	// methods for iterating through comtrees and comtree components
	int	firstComtIndex() const;
	int	nextComtIndex(int) const;
	int	firstCore(int) const;
	int	nextCore(int, int) const;
	int	firstComtLink(int) const;
	int	nextComtLink(int,int) const;
	// access comtree attributes
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
	// add/remove/modify comtrees
	bool	addComtree(int);
	bool	removeComtree(int);
	bool	addComtCoreNode(int,int);
	bool	removeComtCoreNode(int,int);
	bool	addComtLink(int,int);
	bool	removeComtLink(int,int);
	bool	setComtRoot(int,int);
	bool	setComtBrDown(int,int);
	bool	setComtBrUp(int,int);
	bool	setComtPrDown(int,int);
	bool	setComtPrUp(int,int);
	bool	setComtLeafBrDown(int,int);
	bool	setComtLeafBrUp(int,int);
	bool	setComtLeafPrDown(int,int);
	bool	setComtLeafPrUp(int,int);

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
	ipa_t	ipAdr;		///< IP address of leaf
	fAdr_t	fAdr;		///< leaf's forest address
	int	latitude;	///< latitude of leaf (in micro-degrees, + or -)
	int	longitude;	///< latitude of leaf (in micro-degrees, + or -)
	};
	LeafNodeInfo *leaf;
	UiSetPair *leaves;	///< in-use and free leaf numbers
	set<int> *controllers;	///< set of controllers (by node #)

	static int const UNDEF_LAT = 91;	// invalid latitude
	static int const UNDEF_LONG = 361;	// invalid longitude

	/** structure holding information used by router nodes */
	struct RtrNodeInfo {
	string	name;		///< node name
	ntyp_t	nType;		///< node type
	fAdr_t	fAdr;		///< router's forest address
	int	latitude;	///< latitude of node (in micro-degrees, + or -)
	int	longitude;	///< latitude of node (in micro-degrees, + or -)
	fAdr_t	firstCliAdr;	///< router's first assignable client address
	fAdr_t  lastCliAdr;	///< router's last assignable client address
	int	numIf;		///< number of interfaces
	IfInfo	*iface;		///< interface information
	};
	RtrNodeInfo *rtr;
	UiSetPair *routers;	///< tracks routers and unused router numbers

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
	map<int,RateSpec> *linkMap; ///< map for links in comtree
	};
	ComtreeInfo *comtree;	///< array of comtrees
	IdMap *comtreeMap;	///< maps comtree numbers to indices in array
};

// Methods for working with nodes ///////////////////////////////////

/** Check to see that a node number is valid.
 *  @param n is an integer node number
 *  @return true if n represents a valid node in the network,
 *  else false
 */
inline bool NetInfo::validNode(int n) const {
	return (isLeaf(n) || isRouter(n));
}

/** Get the number of the "first" node in the Forest network.
 *  This method is used when iterating through all the nodes.
 *  @return the node number of a nominal first node.
 */
inline int NetInfo::firstNode() const {
	return (firstRouter() != 0 ? firstRouter() : firstLeaf());
}

/** Get the node number of the "next" node in the Forest network.
 *  Used for iterating through all the nodes.
 *  @param n is the number of a node in the network
 *  @return the node number of the node following n, or 0 if there is
 *  no such node
 */
inline int NetInfo::nextNode(int n) const {
	return (isLeaf(n) ? nextLeaf(n) :
		(isRouter(n) ?
		 (nextRouter(n) != 0 ? nextRouter(n) : firstLeaf())
		: 0));
}

/** Get the node number of the first controller in the Forest network.
 *  Used for iterating through all the controllers
 *  @return the node number of the first controller, or 0 if
 *  no controllers have been defined.
 */
inline int NetInfo::firstController() const {
	set<int>::iterator p = controllers->begin();
	return (p != controllers->end() ? (*p)+maxRtr : 0);
}

/** Get the node number of the next controller in the Forest network.
 *  Used for iterating through all the controllers
 *  @param n is the node number for a controller
 *  @return the node number of the next controller, or 0 if
 *  there is no next controller.
 */
inline int NetInfo::nextController(int n) const {
	set<int>::iterator p = controllers->find(n-maxRtr);
	if (p == controllers->end()) return 0;
	p++;
	return (p != controllers->end() ? (*p)+maxRtr : 0);
}

/** Get the maximum node number for the network.
 *  @return the largest node number for nodes in this network
 */
inline int NetInfo::getMaxNode() const { return maxNode; }

/** Get the name for a specified node.
 *  @param n is a node number
 *  @param s is a provided string in which the result is to be returned
 *  @return a reference to s
 */
inline string& NetInfo::getNodeName(int n, string& s) {
	s = (isLeaf(n) ? leaf[n-maxRtr].name : 
	     (isRouter(n) ? rtr[n].name : ""));
	return s;
}

/** Get the node number corresponding to a give node name.
 *  @param s is the name of a node
 *  @return an integer node number or 0 if the given string does not
 *  the name of any node
 */
inline int NetInfo::getNodeNum(string& s) const {
	map<string,int>::iterator p = nodeNumMap->find(s);
	return ((p != nodeNumMap->end()) ? p->second : 0);
}

/** Get the type of a specified node .
 *  @param n is a node number
 *  @return the node type of n or UNDEF_NODE if n is not a valid node number
 */
inline ntyp_t NetInfo::getNodeType(int n) const {
	return (isLeaf(n) ? leaf[n-maxRtr].nType : 
		(isRouter(n) ? rtr[n].nType : UNDEF_NODE));
}

/** Set the name of a node.
 *  @param n is the node number for the node
 *  @param nam is its new name
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setNodeName(int n, string& nam) {
	if (!validNode(n)) return false;
	if (isRouter(n)) rtr[n].name = nam;
	else 		 leaf[n-maxRtr].name = nam;
	string s;
	nodeNumMap->erase(getNodeName(n,s));
	(*nodeNumMap)[nam] = n;
	return true;
}

/** Set the forest address of a node.
 *  @param n is the node number for the node
 *  @param adr is its new forest address
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setNodeAdr(int n, fAdr_t adr) {
	if (isLeaf(n)) leaf[n-maxRtr].fAdr = adr;
	else if (isRouter(n)) rtr[n].fAdr = adr;
	else return false;
	return true;
}

/** Set the latitude of a node.
 *  @param n is the node number for the node
 *  @param lat is its new latitude
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setNodeLat(int n, double lat) {
	if (isLeaf(n)) leaf[n-maxRtr].latitude = (int) (lat*1000000);
	else if (isRouter(n)) rtr[n].latitude  = (int) (lat*1000000);
	else return false;
	return true;
}

/** Set the longitude of a node.
 *  @param n is the node number for the node
 *  @param lat is its new longtitude
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setNodeLong(int n, double longg) {
	if (isLeaf(n)) leaf[n-maxRtr].longitude = (int) (longg*1000000);
	else if (isRouter(n)) rtr[n].longitude  = (int) (longg*1000000);
	else return false;
	return true;
}

// Additional methods for working with leaf nodes ///////////////////

/** Determine if a given node number identifies a leaf.
 *  @param n is an integer node number
 *  @return true if n corresponds to a leaf in the network,
 *  else false
 */
inline bool NetInfo::isLeaf(int n) const {
	return (n <= maxRtr ? 0 : leaves->isIn(n - maxRtr));
}

/** Get the node number of the first leaf node in the Forest network.
 *  Used for iterating through all the leaf nodes.
 *  @return the node number of the nominal first leaf, or 0 if
 *  no leaves have been defined.
 */
inline int NetInfo::firstLeaf() const {
	return (leaves->firstIn() != 0 ? maxRtr + leaves->firstIn() : 0);
}

/** Get the node number of the "next" leaf node in the Forest network.
 *  Used for iterating through all the leaves.
 *  @param n is the number of a leaf in the network
 *  @return the node number of the leaf following n, or 0 if there is
 *  no such leaf
 */
inline int NetInfo::nextLeaf(int n) const {
	int nxt = leaves->nextIn(n-maxRtr);
	return (nxt != 0 ? maxRtr + nxt : 0);
}

/** Set the node type of a leaf node.
 *  @param n is the node number of the leaf
 *  @param typ is its new node type
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setLeafType(int n, ntyp_t typ) {
	if (isLeaf(n)) leaf[n-maxRtr].nType = typ;
	else return false;
	return true;
}

/** Set the IP address of a leaf node.
 *  @param n is the node number of the leaf
 *  @param ip is its new IP address
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setLeafIpAdr(int n, ipa_t ip) {
	if (isLeaf(n)) leaf[n-maxRtr].ipAdr = ip;
	else return false;
	return true;
}

// Additional methods for working with routers //////////////////////

/** Determine if a given node number identifies a router.
 *  @param n is an integer node number
 *  @return true if n corresponds to a router in the network,
 *  else false
 */
inline bool NetInfo::isRouter(int n) const { return routers->isIn(n); }

/** Check to see if a given router interface is valid.
 *  @param r is the node number of a router
 *  @param iface is an integer interface n umber
 *  @return true if iface is a falid interface for r,
 *  else false
 */
inline bool NetInfo::validIf(int r, int iface) const {
	return isRouter(r) && (1 <= iface && iface <= rtr[r].numIf &&
					  rtr[r].iface[iface].ipAdr != 0);
}

/** Get the node number of the first router in the Forest network.
 *  Used for iterating through all the routers.
 *  @return the node number of the nominal first router, or 0 if
 *  no routers have been defined.
 */
inline int NetInfo::firstRouter() const { return routers->firstIn(); } 

/** Get the node number of the "next" router in the Forest network.
 *  Used for iterating through all the routers.
 *  @param r is the number of a router in the network
 *  @return the node number of the router following r, or 0 if there is
 *  no such router
 */
inline int NetInfo::nextRouter(int r) const { return routers->nextIn(r); }

/** Get the maximum router number for the network.
 *  @return the largest router number for this network
 */
inline int NetInfo::getMaxRouter() const { return maxRtr; }

/** Get the number of routers in this network.
 *  @return the largest router number for this network
 */
inline int NetInfo::getNumRouter() const { return routers->getNumIn(); }

/** Get the first address in a router's range of client addresses.
 *  Each router is assigned a range of Forest addresses for its clients.
 *  This method gets the first address in the range.
 *  @param r is the node number of a router
 *  @return the first client address for r or 0 if r is not a valid router
 */
inline fAdr_t NetInfo::getFirstCliAdr(int r) const {
	return (isRouter(r) ? rtr[r].firstCliAdr : 0);
}

/** Get the last address in a router's range of client addresses.
 *  Each router is assigned a range of Forest addresses for its clients.
 *  This method gets the last address in the range.
 *  @param r is the node number of a router
 *  @return the last client address for r or 0 if r is not a valid router
 */
inline fAdr_t NetInfo::getLastCliAdr(int r) const {
	return (isRouter(r) ? rtr[r].lastCliAdr : 0);
}

/** Get the number of interfaces defined for a router.
 *  @param r is the node number of a router
 *  @return the number of interfaces defined for router r or 0 if r
 *  is not a valid router
 */
inline int NetInfo::getNumIf(int r) const {
	return (isRouter(r) ? rtr[r].numIf : 0);
}

/** Get the IP address of a leaf node
 *  @param n is a leaf node number
 *  @return the IP address of n or 0 if n is not a valid leaf
 */
inline ipa_t NetInfo::getLeafIpAdr(int n) const {
	return (isLeaf(n) ? leaf[n-maxRtr].ipAdr : 0);
}

/** Get the Forest address of a node
 *  @param n is a node number
 *  @return the Forest address of n or 0 if n is not a valid node
 */
inline fAdr_t NetInfo::getNodeAdr(int n) const {
	return (isLeaf(n) ? leaf[n-maxRtr].fAdr :
		(isRouter(n) ? rtr[n].fAdr : 0));
}

/** Get the latitude of a node in the Forest network.
 *  Each node has a location which is represented using latitude and longitude.
 *  Latitudes are returned as double precision values in units of degrees.
 *  @param n is a node number
 *  @return the latitude defined for n
 */
inline double NetInfo::getNodeLat(int n) const {
	double x = (isLeaf(n) ? leaf[n-maxRtr].latitude :
		    (isRouter(n) ? rtr[n].latitude : UNDEF_LAT));
	return x/1000000;
}

/** Get the longitude of a node in the Forest network.
 *  Each node has a location which is represented using latitude and longitude.
 *  Latitudes are returned as double precision values in units of degrees.
 *  @param n is a node number
 *  @return the longitude defined for n
 */
inline double NetInfo::getNodeLong(int n) const {
	double x = (isLeaf(n) ? leaf[n-maxRtr].longitude :
		    (isRouter(n) ? rtr[n].longitude : UNDEF_LONG));
	return x/1000000;
}

/** Get the IP address of a specified router interface.
 *  @param r is the node number of a router in the network
 *  @param iface is an interface number for r
 *  @return the IP address of the specified interface or 0 if
 *  there is no such interface
 */
inline ipa_t NetInfo::getIfIpAdr(int n, int iface) const {
	return (validIf(n,iface) ? rtr[n].iface[iface].ipAdr : 0);
}

/** Get the first link in the range of links assigned to a given interface.
 *  Each router interface is assigned a consecutive range of link numbers.
 *  @param r is the node number of a router in the network
 *  @param iface is an interface number for r
 *  @return the first link number in the assigned range for this interface,
 *  or 0 if the interface number is invalid.
 */
inline int NetInfo::getIfFirstLink(int r, int iface) const {
	return (validIf(r,iface) ? rtr[r].iface[iface].firstLink : 0);
}

/** Get the last link in the range of links assigned to a given interface.
 *  Each router interface is assigned a consecutive range of link numbers.
 *  @param r is the node number of a router in the network
 *  @param iface is an interface number for r
 *  @return the last link number in the assigned range for this interface,
 *  or 0 if the interface number is invalid.
 */
inline int NetInfo::getIfLastLink(int r, int iface) const {
	return (validIf(r,iface) ? rtr[r].iface[iface].lastLink : 0);
}

/** Get the bit rate for a specified router interface.
 *  @param r is the node number of a router in the network
 *  @param iface is an interface number for r
 *  @return the bit rate (in kb/s) assigned to this interface,
 *  or 0 if the interface number is invalid.
 */
inline int NetInfo::getIfBitRate(int r, int iface) const {
	return (validIf(r,iface) ? rtr[r].iface[iface].bitRate : 0);
}

/** Get the packet rate for a specified router interface.
 *  @param r is the node number of a router in the network
 *  @param iface is an interface number for r
 *  @return the packet rate (in p/s) assigned to this interface,
 *  or 0 if the interface number is invalid.
 */
inline int NetInfo::getIfPktRate(int r, int iface) const {
	return (validIf(r,iface) ? rtr[r].iface[iface].pktRate : 0);
}

/** Set the first address in a router's range of assignable client addresses.
 *  @param r is the node number of the router
 *  @param adr is the forest address that defines the start of the range
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setFirstCliAdr(int r, fAdr_t adr) {
	if (isRouter(r)) rtr[r].firstCliAdr = adr;
	else return false;
	return true;
}

/** Set the last address in a router's range of assignable client addresses.
 *  @param r is the node number of the router
 *  @param adr is the forest address that defines the end of the range
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setLastCliAdr(int r, fAdr_t adr) {
	if (isRouter(r)) rtr[r].lastCliAdr = adr;
	else return false;
	return true;
}

/** Set the bit rate of a router interface.
 *  @param r is the node number of the router
 *  @param iface is the interface number
 *  @param br is the new bit rate for the interface
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setIfBitRate(int r, int iface, int br) {
	if (validIf(r,iface)) rtr[r].iface[iface].bitRate = br;
	else return false;
	return true;
}

/** Set the packet rate of a router interface.
 *  @param r is the node number of the router
 *  @param iface is the interface number
 *  @param pr is the new packet rate for the interface
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setIfPktRate(int r, int iface, int pr) {
	if (validIf(r,iface)) rtr[r].iface[iface].pktRate = pr;
	else return false;
	return true;
}

/** Set the first link in the range of links defined for a router interface.
 *  Each router interface is assigned a consecutive range of link numbers
 *  @param r is the node number of the router
 *  @param iface is the interface number
 *  @param lnk is the first link in the range of link numbers
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setIfFirstLink(int r, int iface, int lnk) {
	if (validIf(r,iface)) rtr[r].iface[iface].firstLink = lnk;
	else return false;
	return true;
}

/** Set the last link in the range of links defined for a router interface.
 *  Each router interface is assigned a consecutive range of link numbers
 *  @param r is the node number of the router
 *  @param iface is the interface number
 *  @param lnk is the last link in the range of link numbers
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setIfLastLink(int r, int iface, int lnk) {
	if (validIf(r,iface)) rtr[r].iface[iface].lastLink = lnk;
	else return false;
	return true;
}

/** Set the IP address of a router interface.
 *  @param r is the node number of a router
 *  @param iface is the interface number
 *  @param ip is the new ip address for the interface
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setIfIpAdr(int r, int iface, ipa_t ip) {
	if (validIf(r,iface)) rtr[r].iface[iface].ipAdr = ip;
	else return false;
	return true;
}

// Methods for working with links ///////////////////////////////////

/** Check to see if a given link number is valid.
 *  @param n is an integer link number
 *  @return true if n corresponds to a link in the network,
 *  else false
 */
inline bool NetInfo::validLink(int lnk) const {
	return netTopo->validEdge(lnk);
}

/** Get the number of the first link in the Forest network.
 *  @return the number of the first link or 0 if no links have
 *  been defined.
 */
inline int NetInfo::firstLink() const { return netTopo->first(); }

/** Get the number of the next link in the Forest network.
 *  @lnk is the number of a link in the network
 *  @return the number of the link following lnk or 0 if there is no
 *  such link
 */
inline int NetInfo::nextLink(int lnk) const { return netTopo->next(lnk); }

/** Get the number of the first link incident to a specified node.
 *  @param n is the node number of a node in the network
 *  @return the number of the first link incident to n or 0 if there
 *  are no links incident to n
 */
inline int NetInfo::firstLinkAt(int n) const {
	return (validNode(n) ? netTopo->first(n) : 0);
}

/** Get the number of the next link incident to a specified node.
 *  @param n is the node number of a node in the network
 *  @param lnk is a link incident to n
 *  @return the number of the next link incident to n or 0 if there
 *  is no such link
 */
inline int NetInfo::nextLinkAt(int n, int lnk) const {
	return (validNode(n) ? netTopo->next(n,lnk) : 0);
}

/** Get the maximum link number for the network.
 *  @return the largest link number for links in this network
 */
inline int NetInfo::getMaxLink() const { return maxLink; }

/** Get the node number for the "left" endpoint of a given link.
 *  The endpoints of a link are arbitrarily designated "left" and "right".
 *  @param lnk is a link number
 *  @return the node number of the left endpoint of lnk, or 0 if lnk
 *  is not a valid link number.
 */
inline int NetInfo::getLinkL(int lnk) const {
	return (validLink(lnk) ? netTopo->left(lnk) : 0);
}

/** Get the node number for the "right" endpoint of a given link.
 *  The endpoints of a link are arbitrarily designated "left" and "right".
 *  @param lnk is a link number
 *  @return the node number of the right endpoint of lnk, or 0 if lnk
 *  is not a valid link number.
 */
inline int NetInfo::getLinkR(int lnk) const {
	return (validLink(lnk) ? netTopo->right(lnk) : 0);
}

/** Get the node number for the "other" endpoint of a given link.
 *  @param r is the node number of a router
 *  @param lnk is a link number
 *  @return the node number of the endpoint of lnk that is not r, or
 *  0 if lnk is not a valid link number or r is not an endpoint of lnk
 */
inline int NetInfo::getPeer(int r, int lnk) const {
	return (validLink(lnk) ? netTopo->mate(r,lnk) : 0);
}

/** Get the local link number used by one of the endpoints of a link.
 *  Each router in a Forest network refers to links using local link numbers.
 *  @param lnk is a "global" link number
 *  @param r is the node number of router
 *  @return the local link number used by r or 0 if r is not a router
 *  or is not incident to lnk
 */
inline int NetInfo::getLocLink(int lnk, int r) const {
	return (!(validLink(lnk) && isRouter(r)) ? 0 :
		(r == netTopo->left(lnk) ? getLocLinkL(lnk) :
		(r == netTopo->right(lnk) ? getLocLinkR(lnk) : 0)));
}

/** Get the local link number used by the left endpoint of a link.
 *  Each router in a Forest network refers to links using local link numbers.
 *  @param lnk is a "global" link number
 *  @return the local link number used by the left endpoint, or 0 if
 *  the link number is invalid or the left endpoint is not a router.
 */
inline int NetInfo::getLocLinkL(int lnk) const {
	int r = getLinkL(lnk);
	return (isRouter(r) ? link[lnk].leftLnum : 0);
}

/** Get the local link number used by the right endpoint of a link.
 *  Each router in a Forest network refers to links using local link numbers.
 *  @param lnk is a "global" link number
 *  @return the local link number used by the right endpoint, or 0 if
 *  the link number is invalid or the left endpoint is not a router.
 */
inline int NetInfo::getLocLinkR(int lnk) const {
	int r = getLinkR(lnk);
	return (isRouter(r) ? link[lnk].rightLnum : 0);
}

/** Get the bit rate of a link in the Forest network.
 *  @param lnk is a link number
 *  @return the bit rate assigned to lnk or 0 if lnk is not a valid
 *  link number
 */
inline int NetInfo::getLinkBitRate(int lnk) const {
	return (validLink(lnk) ? link[lnk].bitRate : 0);
}

/** Get the packet rate of a link in the Forest network.
 *  @param lnk is a link number
 *  @return the packet rate assigned to lnk or 0 if lnk is not a valid
 *  link number
 */
inline int NetInfo::getLinkPktRate(int lnk) const {
	return (validLink(lnk) ? link[lnk].pktRate : 0);
}

/** Get the length of a link in the Forest network.
 *  @param lnk is a link number
 *  @return the assigned link length in kilometers, or 0 if lnk is not a valid
 *  link number
 */
inline int NetInfo::getLinkLength(int lnk) const {
	return (validLink(lnk) ? netTopo->length(lnk) : 0);
}

/** Get the number of the link incident to a leaf node.
 *  @param n is a node number for a leaf
 *  @return the number of its incident link or 0 if n is not a leaf
 *  or if it has no link
 */
inline int NetInfo::getLinkNum(int n) const {
	return (isLeaf(n) ? netTopo->first(n) : 0);
}

/** Get the global link number of a link incident to a router.
 *  @param r is a node number of a router
 *  @param llnk is a local link number of a link at r
 *  @return the global link number for local link llnk at r, or
 *  or 0 if r is not a router, or it has no such link
 */
inline int NetInfo::getLinkNum(int r, int llnk) const {
	return (isRouter(r) ? locLnk2lnk->lookup(ll2l_key(r,llnk))/2 : 0);
}

/** Set the local link number used by the left endpoint of a link.
 *  Each router in a Forest network refers to links using local link numbers.
 *  @param lnk is a "global" link number
 *  @param loc is the local link number to be used by the left endpoint of lnk
 *  @return true on success, else false
 */
inline bool NetInfo::setLocLinkL(int lnk, int loc) {
	if (validLink(lnk)) link[lnk].leftLnum = loc;
	else return false;
	return true;
}

/** Set the local link number used by the right endpoint of a link.
 *  Each router in a Forest network refers to links using local link numbers.
 *  @param lnk is a "global" link number
 *  @param loc is the local link number to be used by the right endpoint of lnk
 *  @return true on success, else false
 */
inline bool NetInfo::setLocLinkR(int lnk, int loc) {
	if (validLink(lnk)) link[lnk].rightLnum = loc;
	else return false;
	return true;
}

/** Set the bit rate of a link.
 *  @param lnk is a "global" link number
 *  @param br is the new bit rate
 *  @return true on success, else false
 */
inline bool NetInfo::setLinkBitRate(int lnk, int br) {
	if (validLink(lnk)) link[lnk].bitRate = br;
	else return false;
	return true;
}

/** Set the packet rate of a link.
 *  @param lnk is a "global" link number
 *  @param br is the new packet rate
 *  @return true on success, else false
 */
inline bool NetInfo::setLinkPktRate(int lnk, int pr) {
	if (validLink(lnk)) link[lnk].pktRate = pr;
	else return false;
	return true;
}

/** Set the length of a link.
 *  @param lnk is a "global" link number
 *  @param len is the new length
 *  @return true on success, else false
 */
inline bool NetInfo::setLinkLength(int lnk, int len) {
	if (validLink(lnk)) netTopo->setLength(lnk,len);
	else return false;
	return true;
}

// Methods for working with comtrees ////////////////////////////////

/** Check to see if a given comtree number is valid.
 *  @param comt is an integer comtree number
 *  @return true if comt corresponds to a comtree in the network,
 *  else false
 */
inline bool NetInfo::validComtree(int comt) const {
	return comtreeMap->validKey(comt);
}

/** Check to see if a given comtree index is valid.
 *  NetInfo uses integer indices in a restricted range to access
 *  comtree information efficiently. Users get the comtree index
 *  for a given comtree number using the lookupComtree() method.
 *  @param i is an integer comtree index
 *  @return true if n corresponds to a comtree in the network,
 *  else false
 */
inline bool NetInfo::validComtIndex(int i) const {
	return comtreeMap->validId(i);
}

/** Determine if a router is a core node in a specified comtree.
 *  @param i is an integer comtree index
 *  @param r is the node number of a router
 *  @return true if r is a core node in the comtree with index i,
 *  else false
 */
inline bool NetInfo::isComtCoreNode(int i, int r) const {
	return validComtIndex(i) &&
		comtree[i].coreSet->find(r) != comtree[i].coreSet->end();
}

/** Determine if a link is in a specified comtree.
 *  @param i is an integer comtree index
 *  @param lnk is a link number
 *  @return true if lnk is a link in the comtree with index i,
 *  else false
 */
inline bool NetInfo::isComtLink(int i, int lnk) const {
	return validComtIndex(i) &&
		comtree[i].linkMap->find(lnk) != comtree[i].linkMap->end();
}

/** Get the first valid comtree index.
 *  Used for iterating through all the comtrees.
 *  @return the index of the first comtree, or 0 if no comtrees defined
 */
inline int NetInfo::firstComtIndex() const {
	return comtreeMap->firstId();
}

/** Get the next valid comtree index.
 *  Used for iterating through all the comtrees.
 *  @param ctx is a comtree index
 *  @return the index of the next comtree after, or 0 if there is no
 *  next index
 */
inline int NetInfo::nextComtIndex(int ctx) const {
	return comtreeMap->nextId(ctx);
}

/** Get the node number of the first core node in a comtree with a given index.
 *  @param ctx is the comtree index
 *  @return the node number of the first core node in the comtree,
 *  or 0 if the comtree index is invalid or no core nodes have been defined
 */
inline int NetInfo::firstCore(int ctx) const {
	if (!validComtIndex(ctx)) return 0;
	set<int>::iterator p = comtree[ctx].coreSet->begin();
	return (p != comtree[ctx].coreSet->end() ? *p : 0);
}

/** Get the node number of the next core node in a comtree with a given index.
 *  @param r is the node number of a router
 *  @param ctx is the comtree index
 *  @return the node number of the first core node in the comtree,
 *  or 0 if the comtree index is invalid or no core nodes have been defined
 */
inline int NetInfo::nextCore(int r, int ctx) const {
	if (!validComtIndex(ctx)) return 0;
	set<int>::iterator p = comtree[ctx].coreSet->find(r);
	if (p == comtree[ctx].coreSet->end()) return 0;
	p++;
	return (p != comtree[ctx].coreSet->end() ? *p : 0);
}

/** Get the first link defined for a specified comtree.
 *  Used to iterate the links in a comtree.
 *  @param ctx is the comtree index
 *  @return the link number of the first link that belongs to the comtree,
 *  or 0 if the comtree index is invalid or contains no links
 */
inline int NetInfo::firstComtLink(int ctx) const {
	if (!validComtIndex(ctx)) return 0;
	map<int,RateSpec>::iterator p = comtree[ctx].linkMap->begin();
	return (p != comtree[ctx].linkMap->end() ? (*p).first : 0);
}

/** Get the next link defined for a specified comtree.
 *  Used to iterate the links in a comtree.
 *  @param lnk is a link in the comtree
 *  @param ctx is the comtree index
 *  @return the link number of the next link after lnk that belongs to the
 *  comtree, or 0 if the comtree index is invalid or contains no links
 */
inline int NetInfo::nextComtLink(int lnk, int ctx) const {
	if (!validComtIndex(ctx)) return 0;
	map<int,RateSpec>::iterator p = comtree[ctx].linkMap->find(lnk);
	if (p == comtree[ctx].linkMap->end()) return 0;
	p++;
	return (p != comtree[ctx].linkMap->end() ? (*p).first : 0);
}

/** Lookup the comtree index for a given comtree number.
 *  @param comt is a comtree number
 *  @param return the index used to efficiently access stored information
 *  for the comtree.
 */
inline int NetInfo::lookupComtree(int comt) const {
	return comtreeMap->getId(comt);
}

/** Get the comtree number for the comtree with a given index.
 *  @param ctx is a comtree index
 *  @param return the comtree number that is mapped to index c
 *  or 0 if c is not a valid index
 */
inline int NetInfo::getComtree(int ctx) const {
	return (validComtIndex(ctx) ? comtree[ctx].comtreeNum : 0);
}

/** Get the comtree root for the comtree with a given index.
 *  @param ctx is a comtree index
 *  @param return the specified root node for the comtree
 *  or 0 if c is not a valid index
 */
inline int NetInfo::getComtRoot(int ctx) const {
	return (validComtIndex(ctx) ? comtree[ctx].root : 0);
}

/** Get the downstream backbone bit rate for a comtree with a given index.
 *  @param ctx is a comtree index
 *  @param return the default downstream bit rate for backbone links in
 *  the comtree or 0 if c is not a valid index
 */
inline int NetInfo::getComtBrDown(int ctx) const {
	return (validComtIndex(ctx) ? comtree[ctx].bitRateDown : 0);
}

/** Get the upstream backbone bit rate for a comtree with a given index.
 *  @param ctx is a comtree index
 *  @param return the default upstream bit rate for backbone links in
 *  the comtree or 0 if c is not a valid index
 */
inline int NetInfo::getComtBrUp(int ctx) const {
	return (validComtIndex(ctx) ? comtree[ctx].bitRateUp : 0);
}

/** Get the downstream backbone packet rate for a comtree with a given index.
 *  @param ctx is a comtree index
 *  @param return the default downstream packet rate for backbone links in
 *  the comtree or 0 if c is not a valid index
 */
inline int NetInfo::getComtPrDown(int ctx) const {
	return (validComtIndex(ctx) ? comtree[ctx].pktRateDown : 0);
}

/** Get the upstream backbone packet rate for a comtree with a given index.
 *  @param ctx is a comtree index
 *  @param return the default upstream packet rate for backbone links in
 *  the comtree or 0 if c is not a valid index
 */
inline int NetInfo::getComtPrUp(int ctx) const {
	return (validComtIndex(ctx) ? comtree[ctx].pktRateUp : 0);
}

/** Get the downstream access bit rate for a comtree with a given index.
 *  @param ctx is a comtree index
 *  @param return the default downstream bit rate for access links in
 *  the comtree or 0 if c is not a valid index
 */
inline int NetInfo::getComtLeafBrDown(int ctx) const {
	return (validComtIndex(ctx) ? comtree[ctx].leafBitRateDown : 0);
}

/** Get the upstream access bit rate for a comtree with a given index.
 *  @param ctx is a comtree index
 *  @param return the default upstream bit rate for access links in
 *  the comtree or 0 if c is not a valid index
 */
inline int NetInfo::getComtLeafBrUp(int ctx) const {
	return (validComtIndex(ctx) ? comtree[ctx].leafBitRateUp : 0);
}

/** Get the downstream access packet rate for a comtree with a given index.
 *  @param ctx is a comtree index
 *  @param return the default downstream packet rate for access links in
 *  the comtree or 0 if c is not a valid index
 */
inline int NetInfo::getComtLeafPrDown(int ctx) const {
	return (validComtIndex(ctx) ? comtree[ctx].leafPktRateDown : 0);
}

/** Get the upstream access packet rate for a comtree with a given index.
 *  @param ctx is a comtree index
 *  @param return the default upstream packet rate for access links in
 *  the comtree or 0 if c is not a valid index
 */
inline int NetInfo::getComtLeafPrUp(int ctx) const {
	return (validComtIndex(ctx) ? comtree[ctx].leafPktRateUp : 0);
}

/** Add a new comtree.
 *  Defines a new comtree, with attributes left undefined.
 *  @param comt is the comtree number for the new comtree.
 *  @return true on success, false on failure
 */
inline bool NetInfo::addComtree(int comt) {
	int i = comtreeMap->addPair(comt);
	if (i == 0) return false;
	comtree[i].comtreeNum = comt;
	return true;
}

/** Remove a comtree.
 *  Defines a new comtree, with attributes left undefined.
 *  @param comt is the comtree number for the new comtree.
 *  @return true on success, false on failure
 */
inline bool NetInfo::removeComtree(int ctx) {
	if (!validComtIndex(ctx)) return false;
	comtreeMap->dropPair(comtree[ctx].comtreeNum);
	comtree[ctx].comtreeNum = 0;
	return true;
}

/** Add a new core node to a comtree.
 *  The new node is required to be a router.
 *  @param ctx is the comtree index
 *  @param r is the node number of the router
 *  @return true on success, false on failure
 */
inline bool NetInfo::addComtCoreNode(int ctx, int r) {
	if (!validComtIndex(ctx) || !isRouter(r)) return false;
	comtree[ctx].coreSet->insert(r);
	return true;
}

/** Remove a core node from a comtree.
 *  @param ctx is the comtree index
 *  @param r is the node number of the core node
 *  @return true on success, false on failure
 */
inline bool NetInfo::removeComtCoreNode(int ctx, int n) {
	if (!validComtIndex(ctx)) return false;
	comtree[ctx].coreSet->erase(n);
	return true;
}

/** Add a new link to a comtree.
 *  @param ctx is the comtree index
 *  @param lnk is the link number of the link to be added
 *  @return true on success, false on failure
 */
inline bool NetInfo::addComtLink(int ctx, int lnk) {
	if (!validComtIndex(ctx) || !validLink(lnk)) return false;
	pair<int,RateSpec> mv;
	mv.first = lnk;
	mv.second.bitRateLeft = mv.second.bitRateRight = 0;
	mv.second.pktRateLeft = mv.second.pktRateRight = 0;
	comtree[ctx].linkMap->insert(mv);
	return true;
}

/** Remove a link from a comtree.
 *  @param ctx is the comtree index
 *  @param lnk is the link number of the link to be removed
 *  @return true on success, false on failure
 */
inline bool NetInfo::removeComtLink(int ctx, int lnk) {
	if (!validComtIndex(ctx) || !validLink(lnk)) return false;
	comtree[ctx].linkMap->erase(lnk);
	return true;
}

/** Set the root node of a comtree.
 *  @param ctx is the index of the comtree
 *  @param r is the router that is to be the comtree root
 *  @return true on success, false on failure
 */
inline bool NetInfo::setComtRoot(int ctx, int r) {
	if (!validComtIndex(ctx) || !isRouter(r)) return false;
	comtree[ctx].root = r;
	return true;
}

/** Set the downstream bit rate for the backbone links of a comtree.
 *  @param ctx is the index of the comtree
 *  @param br is the new default downstream bit rate for backbone links
 *  @return true on success, false on failure
 */
inline bool NetInfo::setComtBrDown(int ctx, int br) {
	if (!validComtIndex(ctx)) return false;
	comtree[ctx].bitRateDown = br;
	return true;
}

/** Set the upstream bit rate for the backbone links of a comtree.
 *  @param ctx is the index of the comtree
 *  @param br is the new default upstream bit rate for backbone links
 *  @return true on success, false on failure
 */
inline bool NetInfo::setComtBrUp(int ctx, int br) {
	if (!validComtIndex(ctx)) return false;
	comtree[ctx].bitRateUp = br;
	return true;
}

/** Set the downstream packet rate for the backbone links of a comtree.
 *  @param ctx is the index of the comtree
 *  @param br is the new default downstream packet rate for backbone links
 *  @return true on success, false on failure
 */
inline bool NetInfo::setComtPrDown(int ctx, int pr) {
	if (!validComtIndex(ctx)) return false;
	comtree[ctx].pktRateDown = pr;
	return true;
}

/** Set the upstream packet rate for the backbone links of a comtree.
 *  @param ctx is the index of the comtree
 *  @param br is the new default upstream packet rate for backbone links
 *  @return true on success, false on failure
 */
inline bool NetInfo::setComtPrUp(int ctx, int pr) {
	if (!validComtIndex(ctx)) return false;
	comtree[ctx].pktRateUp = pr;
	return true;
}

/** Set the downstream bit rate for the access links of a comtree.
 *  @param ctx is the index of the comtree
 *  @param br is the new default downstream bit rate for access links
 *  @return true on success, false on failure
 */
inline bool NetInfo::setComtLeafBrDown(int ctx, int br) {
	if (!validComtIndex(ctx)) return false;
	comtree[ctx].leafBitRateDown = br;
	return true;
}

/** Set the upstream bit rate for the access links of a comtree.
 *  @param ctx is the index of the comtree
 *  @param br is the new default upstream bit rate for access links
 *  @return true on success, false on failure
 */
inline bool NetInfo::setComtLeafBrUp(int ctx, int br) {
	if (!validComtIndex(ctx)) return false;
	comtree[ctx].leafBitRateUp = br;
	return true;
}

/** Set the downstream packet rate for the access links of a comtree.
 *  @param ctx is the index of the comtree
 *  @param br is the new default downstream packet rate for access links
 *  @return true on success, false on failure
 */
inline bool NetInfo::setComtLeafPrDown(int ctx, int pr) {
	if (!validComtIndex(ctx)) return false;
	comtree[ctx].leafPktRateDown = pr;
	return true;
}

/** Set the upstream packet rate for the access links of a comtree.
 *  @param ctx is the index of the comtree
 *  @param br is the new default upstream packet rate for access links
 *  @return true on success, false on failure
 */
inline bool NetInfo::setComtLeafPrUp(int ctx, int pr) {
	if (!validComtIndex(ctx)) return false;
	comtree[ctx].leafPktRateUp = pr;
	return true;
}

/** Helper method used to define keys for internal locLnk2lnk hash table.
 *  @param r is the node number of a router
 *  @param llnk is a local link number at r
 *  @return a hash key appropriate for use with locLnk2lnk
 */
inline uint64_t NetInfo::ll2l_key(int r, int llnk) const {
	return (uint64_t(r) << 32) | (uint64_t(llnk) & 0xffffffff);
}

#endif
