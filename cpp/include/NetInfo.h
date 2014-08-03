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
#include <queue>
#include "Forest.h"
#include "HashMap.h"
#include "IdMap.h"
#include "UiSetPair.h"
#include "Wgraph.h"
#include "RateSpec.h"

namespace forest {


/** Maintains information about an entire Forest network.
 *  The NetInfo data structure is intended for use by network
 *  control elements that require a global view of the network.
 *  Currently, this includes the NetMgr and the ComtreeController
 *  components.
 *
 *  Internally, a NetInfo object uses a Wgraph to represent the
 *  network topology. It also has tables of attributes for all
 *  nodes and links. It provides a large number of
 *  methods for accessing the internal representation in a
 *  convenient way, It also has a read() method to read in a network
 *  description from a file and build the corresponding internal
 *  data structure. And it has a toString() method that can be used
 *  in an output stream.
 */
class NetInfo {
public:
		NetInfo(int,int,int,int);
		~NetInfo();

	// struct used by io routines
	struct LinkDesc {
	string	nameL, nameR;	///< names of link endpoints
	int	numL, numR;	///< local link numbers for routers
	int	length;		///< length of link
	RateSpec rates;		///< up means allowed rate from left endpoint
	};

	enum statusType { UP, DOWN, BOOTING };

	// methods for working with nodes
	// predicates
	bool	validNode(int) const;
	// iterating through nodes
	int	firstNode() const;
	int	nextNode(int) const;
	// access node attributes
	int	getMaxNode() const;
	string& getNodeName(int,string&)const ;
	int	getNodeNum(string&) const;
	int	getNodeNum(fAdr_t) const;
	Forest::ntyp_t getNodeType(int) const;
	fAdr_t	getNodeAdr(int) const;
	bool	getNodeLocation(int, pair<double,double>&) const;
	statusType getStatus(int) const;
	// modify node attributes
	bool	setNodeName(int, string&);
	bool	setNodeAdr(int, fAdr_t);
	bool	setNodeLocation(int, pair<double,double>&);
	void	setStatus(int, statusType);

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
	int	addLeaf(const string&, Forest::ntyp_t);
	bool	setLeafType(int, Forest::ntyp_t);
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
	int	getNumRouters() const;
	int	getIface(int, int) const;
	int	getNumIf(int) const;
	bool	getLeafRange(int,pair<fAdr_t,fAdr_t>&) const;
	ipa_t	getIfIpAdr(int,int) const;
	ipp_t	getIfPort(int,int) const;
	RateSpec& getIfRates(int,int) const;
	bool	getIfLinks(int,int,pair<int,int>&) const;
	// add/modify routers
	int	addRouter(const string&);
	bool	addInterfaces(int, int);
	bool	setLeafRange(int,pair<int,int>&);
	bool	setIfIpAdr(int,int,ipa_t);
	bool	setIfPort(int,int,ipp_t);
	bool	setIfLinks(int,int,pair<int,int>&);

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
	int	getLeft(int) const;
	int	getRight(int) const;
	int	getPeer(int,int) const;
	RateSpec& getLinkRates(int) const;
	RateSpec& getAvailRates(int) const;
	RateSpec& getDefLeafRates();
	int	getLinkLength(int) const;
	int	getLinkNum(int,int) const;
	int	getLLnum(int,int) const;
	int	getLeftLLnum(int) const;
	int	getRightLLnum(int) const;
	uint64_t getNonce(int) const;
	// add/modify links
	int	addLink(int,int,int,int);
	bool	setLeftLLnum(int,int);
	bool	setRightLLnum(int,int);
	bool	setLinkLength(int,int);
	bool	setNonce(int,uint64_t);

	// io routines
	bool read(istream&);
	string	link2string(int) const;
	string	linkProps2string(int) const;
	string	linkState2string(int) const;
	string	toString() const;

	// locking methods for mutually exclusive access
	void	lock();
	void	unlock();

private:
	int maxRtr;		///< max node number for a router;
				///< leaf nodes all have larger node numbers
	int maxNode;		///< max node number in netTopo graph
	int maxLink;		///< max link number in netTopo graph
	int maxLeaf;		///< max number of leafs
	int maxCtl;		///< maximum number of controllers

	/** NetTopo is a weighted graph defining the network topology.
	 *  Weights represent link costs */
	Wgraph	*netTopo;

	struct IfInfo {
	ipa_t	ipAdr;		///< ip address of forest interface
	ipp_t	port;		///< ip port number of forest interface
	int	firstLink;	///< number of first link assigned to interface
	int	lastLink;	///< number of last link assigned to interface
	RateSpec rates;		///< up denotes input, down denotes output
	};

	/** structure holding information used by leaf nodes */
	struct LeafNodeInfo {
	string	name;		///< leaf name
	Forest::ntyp_t nType;	///< leaf type
	ipa_t	ipAdr;		///< IP address of leaf
	fAdr_t	fAdr;		///< leaf's forest address
	int	latitude;	///< latitude of leaf (in micro-degrees, + or -)
	int	longitude;	///< latitude of leaf (in micro-degrees, + or -)
	statusType status;	///< active/inactive status
	};
	LeafNodeInfo *leaf;
	UiSetPair *leaves;	///< in-use and free leaf numbers
	set<int> *controllers;	///< set of controllers (by node #)

	static int const UNDEF_LAT = 91;	// invalid latitude
	static int const UNDEF_LONG = 361;	// invalid longitude

	/** structure holding information used by router nodes */
	struct RtrNodeInfo {
	string	name;		///< node name
	Forest::ntyp_t nType;	///< node type
	fAdr_t	fAdr;		///< router's forest address
	int	latitude;	///< latitude of node (in micro-degrees, + or -)
	int	longitude;	///< latitude of node (in micro-degrees, + or -)
	fAdr_t	firstLeafAdr;	///< router's first assignable leaf address
	fAdr_t  lastLeafAdr;	///< router's last assignable leaf address
	statusType status;	///< active/inactive status
	int	numIf;		///< number of interfaces
	IfInfo	*iface;		///< interface information
	};
	RtrNodeInfo *rtr;
	UiSetPair *routers;	///< tracks routers and unused router numbers

	/** maps a node name back to corresponding node number */
	map<string, int> *nameNodeMap;

	/** maps a forest address to a node number */
	map<fAdr_t, int> *adrNodeMap;

	HashMap *locLnk2lnk;	///< maps router/local link# to global link#
	uint64_t ll2l_key(int,int) const; ///< returns key used with locLnk2lnk

	struct LinkInfo {
	int	leftLnum;	///< local link number used by "left endpoint"
	int	rightLnum;	///< local link number used by "right endpoint"
	RateSpec rates;		///< use "up" to denote rates from "left"
	RateSpec availRates;	///< unused capacity
	uint64_t nonce;		///< used when initially connecting links
	};
	LinkInfo *link;

	RateSpec defaultLeafRates;	///< default link rates

	pthread_mutex_t glock;	///< for locking in multi-threaded contexts

	// helper methods for reading a NetInfo file
	bool	readRouter(istream&, RtrNodeInfo&, IfInfo*, string&);
	bool	readLocation(istream&, pair<double,double>&);
	bool	readAdrRange(istream&, pair<fAdr_t,fAdr_t>&);
	bool	readRateSpec(istream&, RateSpec&);
	int 	readIface(istream&, IfInfo*, string&);
	bool	readLeaf(istream&, LeafNodeInfo&, string&);
	bool	readLink(istream&, LinkDesc&, string&);
	bool	readLinkEndpoint(istream&, string&, int&);

	// post-input setup and validity checking
	bool 	check() const;
	bool 	checkLocalLinks() const;
	bool 	checkBackBone() const;
	bool 	checkAddresses() const;
	bool 	checkLeafRange() const;
	bool 	checkLeafNodes() const;
	bool 	checkLinkRates() const;
	bool 	checkRtrRates() const;

	// toString helper methods
	string& rtr2string(int,string&) const;
	string& leaf2string(int,string&) const;
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
inline string& NetInfo::getNodeName(int n, string& s) const {
	if (isLeaf(n)) s = leaf[n-maxRtr].name;
	else if (isRouter(n)) s = rtr[n].name;
	else s = "";
	return s;
}

/** Get the node number corresponding to a give node name.
 *  @param s is the name of a node
 *  @return an integer node number or 0 if the given string does not
 *  the name of any node
 */
inline int NetInfo::getNodeNum(string& s) const {
	map<string,int>::iterator p = nameNodeMap->find(s);
	return ((p != nameNodeMap->end()) ? p->second : 0);
}

/** Get the node number corresponding to a give forest address.
 *  @param adr is the forest address of a node
 *  @return an integer node number or 0 if the given forest address
 *  is not used by any node
 */
inline int NetInfo::getNodeNum(fAdr_t adr) const {
	map<fAdr_t,int>::iterator p = adrNodeMap->find(adr);
	return ((p != adrNodeMap->end()) ? p->second : 0);
}

/** Get the type of a specified node.
 *  @param n is a node number
 *  @return the node type of n or UNDEF_NODE if n is not a valid node number
 */
inline Forest::ntyp_t NetInfo::getNodeType(int n) const {
	return (isLeaf(n) ? leaf[n-maxRtr].nType : 
		(isRouter(n) ? rtr[n].nType : Forest::UNDEF_NODE));
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
	nameNodeMap->erase(getNodeName(n,s));
	(*nameNodeMap)[nam] = n;
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
	adrNodeMap->erase(getNodeAdr(n));
	(*adrNodeMap)[adr] = n;
	return true;
}
/** Set the location of a node in the Forest network.
 *  @param n is a node number
 *  @param loc is a reference to a pair of doubles which defines the new
 *  latitude and longitude
 *  @return true on success, false on failure
 */
inline bool NetInfo::setNodeLocation(int n, pair<double,double>& loc) {
	if (isLeaf(n)) {
		leaf[n-maxRtr].latitude  = (int) 1000000*loc.first;
		leaf[n-maxRtr].longitude = (int) 1000000*loc.second;
	} else if (isRouter(n)) {
		rtr[n].latitude  = (int) 1000000*loc.first;
		rtr[n].longitude = (int) 1000000*loc.second;
	} else return false;
	return true;
}

/** Set the status of a node.
 *  @param n is a node number
 *  @param status is the node status
 */
inline void NetInfo::setStatus(int n, statusType status) {
	if (isLeaf(n)) leaf[n-maxRtr].status = status;
	else rtr[n].status = status;
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
inline bool NetInfo::setLeafType(int n, Forest::ntyp_t typ) {
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
inline int NetInfo::getNumRouters() const { return routers->getNumIn(); }


/** Get the range of leaf addresses for a router.
 *  @param r is the node number of the router
 *  @param adr is the forest address that defines the start of the range
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::getLeafRange(int r, pair<fAdr_t,fAdr_t>& range) const {
	if (!isRouter(r)) return false;
	range.first  = rtr[r].firstLeafAdr;
	range.second = rtr[r].lastLeafAdr;
	return true;
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

/** Get the location of a node in the Forest network.
 *  @param n is a node number
 *  @param loc is a reference to a pair of doubles in which the latitude
 *  and longitude are returned
 *  @return true on success, false on failure
 */
inline bool NetInfo::getNodeLocation(int n, pair<double,double>& loc) const {
	if (isLeaf(n)) {
		loc.first  = leaf[n-maxRtr].latitude/1000000.0;
		loc.second = leaf[n-maxRtr].longitude/1000000.0;
	} else if (isRouter(n)) {
		loc.first  = rtr[n].latitude/1000000.0;
		loc.second = rtr[n].longitude/1000000.0;
	} else return false;
	return true;
}

/** Get the status of a node.
 *  @param n is a node number
 *  @return true on success, false on failure
 */
inline NetInfo::statusType NetInfo::getStatus(int n) const {
	if (isLeaf(n)) return leaf[n-maxRtr].status;
	else return rtr[n].status;
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

/** Get the IP port of a specified router interface.
 *  @param r is the node number of a router in the network
 *  @param iface is an interface number for r
 *  @return the port number of the specified interface or 0 if
 *  there is no such interface
 */
inline ipp_t NetInfo::getIfPort(int n, int iface) const {
	return (validIf(n,iface) ? rtr[n].iface[iface].port : 0);
}

/** Get the RateSpec for a router interface.
 *  @param r is the node number of a router in the network
 *  @param iface is an interface number for r
 *  @return a reference to the RateSpec for the specified router interface
 */
inline RateSpec& NetInfo::getIfRates(int r, int iface) const {
	return rtr[r].iface[iface].rates;
}

/** Get the range of link numbers assigned to an interface.
 *  Each router interface is assigned a consecutive range of link numbers.
 *  @param r is the node number of a router in the network
 *  @param iface is an interface number for r
 *  @return true on success, else false
 */
inline bool NetInfo::getIfLinks(int r, int iface, pair<int,int>& links) const {
	if (!validIf(r,iface)) return false;
	links.first  = rtr[r].iface[iface].firstLink;
	links.second = rtr[r].iface[iface].lastLink;
	return true;
}

/** Set the range of assignable client addresses for this router.
 *  @param r is the node number of the router
 *  @param adr is the forest address that defines the start of the range
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setLeafRange(int r, pair<fAdr_t,fAdr_t>& range) {
	if (!isRouter(r)) return false;
	rtr[r].firstLeafAdr  = range.first;
	rtr[r].lastLeafAdr = range.second;
	return true;
}

/** Set the RateSpec for a router interface.
 *  @param r is the node number of a router in the network
 *  @param iface is an interface number for r
 *  @param rs is a reference to a RateSpec
 *  @return true on success, else false
inline bool NetInfo::setIfRates(int r, int iface, RateSpec& rs) {
	if (!validIf(r,iface)) return false;
	rtr[r].iface[iface].rates = rs;
	return true;
}
 */

/** Set the range of links defined for a router interface.
 *  Each router interface is assigned a consecutive range of link numbers
 *  @param r is the node number of the router
 *  @param iface is the interface number
 *  @param links is a reference to a pair of link numbers that define range
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setIfLinks(int r, int iface, pair<int,int>& links) {
	if (!validIf(r,iface)) return false;
	rtr[r].iface[iface].firstLink  = links.first;
	rtr[r].iface[iface].lastLink = links.second;
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

/** Set the IP address of a router interface.
 *  @param r is the node number of a router
 *  @param iface is the interface number
 *  @param port is the port number for the interface
 *  @return true if the operation succeeds, else false
 */
inline bool NetInfo::setIfPort(int r, int iface, ipp_t port) {
	if (validIf(r,iface)) rtr[r].iface[iface].port = port;
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
	return (validNode(n) ? netTopo->firstAt(n) : 0);
}

/** Get the number of the next link incident to a specified node.
 *  @param n is the node number of a node in the network
 *  @param lnk is a link incident to n
 *  @return the number of the next link incident to n or 0 if there
 *  is no such link
 */
inline int NetInfo::nextLinkAt(int n, int lnk) const {
	return (validNode(n) ? netTopo->nextAt(n,lnk) : 0);
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
inline int NetInfo::getLeft(int lnk) const {
	return (validLink(lnk) ? netTopo->left(lnk) : 0);
}

/** Get the node number for the "right" endpoint of a given link.
 *  The endpoints of a link are arbitrarily designated "left" and "right".
 *  @param lnk is a link number
 *  @return the node number of the right endpoint of lnk, or 0 if lnk
 *  is not a valid link number.
 */
inline int NetInfo::getRight(int lnk) const {
	return (validLink(lnk) ? netTopo->right(lnk) : 0);
}

/** Get the node number for the "other" endpoint of a given link.
 *  @param n is a node number
 *  @param lnk is a link number
 *  @return the node number of the endpoint of lnk that is not n, or
 *  0 if lnk is not a valid link number or n is not an endpoint of lnk
 */
inline int NetInfo::getPeer(int n, int lnk) const {
	return (validLink(lnk) ? netTopo->mate(n,lnk) : 0);
}

/** Get the local link number used by one of the endpoints of a link.
 *  Each router in a Forest network refers to links using local link numbers.
 *  @param lnk is a "global" link number
 *  @param r is the node number of router
 *  @return the local link number used by r or 0 if r is not a router
 *  or is not incident to lnk
 */
inline int NetInfo::getLLnum(int lnk, int r) const {
	return (!(validLink(lnk) && isRouter(r)) ? 0 :
		(r == netTopo->left(lnk) ? getLeftLLnum(lnk) :
		(r == netTopo->right(lnk) ? getRightLLnum(lnk) : 0)));
}

/** Get the local link number used by the left endpoint of a link.
 *  Each router in a Forest network refers to links using local link numbers.
 *  @param lnk is a "global" link number
 *  @return the local link number used by the left endpoint, or 0 if
 *  the link number is invalid or the left endpoint is not a router.
 */
inline int NetInfo::getLeftLLnum(int lnk) const {
	int r = getLeft(lnk);
	return ((lnk != 0 && isRouter(r)) ? link[lnk].leftLnum : 0);
}

/** Get the local link number used by the right endpoint of a link.
 *  Each router in a Forest network refers to links using local link numbers.
 *  @param lnk is a "global" link number
 *  @return the local link number used by the right endpoint, or 0 if
 *  the link number is invalid or the left endpoint is not a router.
 */
inline int NetInfo::getRightLLnum(int lnk) const {
	int r = getRight(lnk);
	return ((lnk != 0 && isRouter(r)) ? link[lnk].rightLnum : 0);
}

/** Get the nonce for a link.
 *  @param lnk is the number of a link.
 *  @return the nonce used to identify an endpoint when connecting a link
 */
inline uint64_t NetInfo::getNonce(int lnk) const {
	return (validLink(lnk) ? link[lnk].nonce : 0);
}

/** Get the RateSpec of a link in the Forest network.
 *  @param lnk is a link number
 *  @return a reference to the link rate spec
 */
inline RateSpec& NetInfo::getLinkRates(int lnk) const {
	return link[lnk].rates;
}

/** Get the available rates of a link in the Forest network.
 *  @param lnk is a link number
 *  @return a reference to the available rate spec
 */
inline RateSpec& NetInfo::getAvailRates(int lnk) const {
	return link[lnk].availRates;
}

/** Get the default rates for leaf nodes in a Forest network.
 *  @return a reference to RateSpec for leaf rates
 */
inline RateSpec& NetInfo::getDefLeafRates() {
	return defaultLeafRates;
}

/** Get the length of a link in the Forest network.
 *  @param lnk is a link number
 *  @return the assigned link length in kilometers, or 0 if lnk is not a valid
 *  link number
 */
inline int NetInfo::getLinkLength(int lnk) const {
	return (validLink(lnk) ? netTopo->weight(lnk) : 0);
}

/** Get the global link number of a link incident to a node.
 *  @param nn is a valid node number
 *  @param llnk is an optional local link number
 *  @return the link number of the node; if the node is a router, the
 *  link is the one with the specified local link number; return 0 on error
 */
inline int NetInfo::getLinkNum(int nn, int llnk) const {
	if (!validNode(nn)) return 0;
	if (isRouter(nn)) return locLnk2lnk->get(ll2l_key(nn,llnk))/2;
	return firstLinkAt(nn);
}

/** Set the local link number used by the left endpoint of a link.
 *  Each router in a Forest network refers to links using local link numbers.
 *  @param lnk is a "global" link number
 *  @param loc is the local link number to be used by the left endpoint of lnk
 *  @return true on success, else false
 */
inline bool NetInfo::setLeftLLnum(int lnk, int loc) {
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
inline bool NetInfo::setRightLLnum(int lnk, int loc) {
	if (validLink(lnk)) link[lnk].rightLnum = loc;
	else return false;
	return true;
}

/** Set the nonce for a link.
 *  @param lnk is the number of a link.
 *  @param nonce is a nonce used to identify an endpoint when connecting a link
 *  @return true on success, else false
 */
inline bool NetInfo::setNonce(int lnk, uint64_t nonce) {
	if (!validLink(lnk)) return false;
	link[lnk].nonce = nonce;
	return true;
}

/** Set the RateSpec for a link.
 *  @param lnk is a link number
 *  @param rs is the new RateSpec for the link
 *  @return true on success, false on failure
inline bool NetInfo::setLinkRates(int lnk, RateSpec& rs) {
	if (!validLink(lnk)) return false;
	link[lnk].rates = rs;
	return true;
}
 */

/** Set the available capacity of a link.
 *  @param lnk is a link number
 *  @param rs is a RateSpec
inline bool NetInfo::setAvailRates(int lnk, RateSpec& rs) {
	if (!validLink(lnk)) return false;
	link[lnk].availRates = rs;
	return true;
}
 */

/** Set the length of a link.
 *  @param lnk is a "global" link number
 *  @param len is the new length
 *  @return true on success, else false
 */
inline bool NetInfo::setLinkLength(int lnk, int len) {
	if (validLink(lnk)) netTopo->setWeight(lnk,len);
	else return false;
	return true;
}

/** Helper method used to define keys for internal locLnk2lnk HashMap.
 *  @param r is the node number of a router
 *  @param llnk is a local link number at r
 *  @return a hash key appropriate for use with locLnk2lnk
 */
inline uint64_t NetInfo::ll2l_key(int r, int llnk) const {
	return (uint64_t(r) << 32) | (uint64_t(llnk) & 0xffffffff);
}

/** Lock the data structure.
 *  Use in multi-threaded applications that dynamically modify the
 *  data structure. For now, just a single global lock. 
 */
inline void NetInfo::lock() { pthread_mutex_lock(&glock); }

/** Unlock the data structure.
 */
inline void NetInfo::unlock() { pthread_mutex_unlock(&glock); }

} // ends namespace


#endif
