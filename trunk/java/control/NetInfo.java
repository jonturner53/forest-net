/** @file NetInfo.java 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

package forest.control;

import java.io.*;
import java.util.*;
import algoLib.misc.*;
import algoLib.dataStructures.basic.*;
import algoLib.dataStructures.graphs.*;
import forest.common.*;

public class NetInfo {
	private int maxRtr;	///< max node number for a router;
				///< leaf nodes all have larger node numbers
	private int maxNode;	///< max node number in netTopo graph
	private int maxLink;	///< max link number in netTopo graph
	private int maxLeaf;	///< max number of leafs

	/** Utility class used by input methods */
	public class LinkEndpoint {
	public String name; public int num;
	LinkEndpoint() { name = null; num = 0; }
	LinkEndpoint(String nam, int n) { name = nam; num = n; }
	}
	/** Another utility class used by input methods */
	public class LinkDesc {
	String	nameL, nameR;	///< names of link endpoints
	int	numL, numR;	///< local link numbers for routers
	int	length;		///< length of link
	RateSpec rates;		///< up means allowed rate from left endpoint
		public LinkDesc() {
			nameL = nameR = null; numL = numR = length = 0;
			rates = new RateSpec(0); 
		}
	};

	/** NetTopo is a weighted graph defining the network topology.
	 *  Weights represent link costs */
	private Wgraph netTopo;

	public class ComtRtrInfo {   ///< used in rtrMap of ComtreeInfo
        public int plnk;           ///< link to parent in comtree
        public int lnkCnt;         ///< number of comtree links at this router
		public ComtRtrInfo() {
			plnk = 0; lnkCnt = 0;
		}
	}

	private class IfInfo {
	int	ipAdr;		///< ip address of forest interface
	RateSpec rates;		///< rates for interface
	int	firstLink;	///< number of first link assigned to interface
	int	lastLink;	///< number of last link assigned to interface
		public IfInfo() {
			ipAdr = 0; rates = new RateSpec(0);
			firstLink = 0; lastLink = 0;
		}
		public void copyFrom(IfInfo iface) {
			ipAdr = iface.ipAdr; rates.copyFrom(iface.rates);
			firstLink = iface.firstLink; lastLink = iface.lastLink;
		}
	}

	private static final int UNDEF_LAT = 91;	// invalid latitude
	private static final int UNDEF_LONG = 361;	// invalid longitude

	/** class used for holding information used by leaf nodes */
	private class LeafNodeInfo {
	String	name;		///< leaf name
	Forest.NodeTyp nType;	///< leaf type
	int	ipAdr;		///< IP address of leaf
	int	fAdr;		///< leaf's forest address
	int	latitude;	///< latitude of leaf (in micro-degrees, + or -)
	int	longitude;	///< latitude of leaf (in micro-degrees, + or -)
		public LeafNodeInfo() {
			name = ""; nType = Forest.NodeTyp.UNDEF_NODE;
			ipAdr = 0; fAdr = 0;
			latitude = UNDEF_LAT; longitude = UNDEF_LONG;
		}
	}
	private LeafNodeInfo[] leaf;
	private UiSetPair leaves;	///< in-use and free leaf numbers
	private Set<Integer> controllers; ///< set of controllers (by node #)

	/** class for holding information used by router nodes */
	private class RtrNodeInfo {
	String	name;		///< node name
	Forest.NodeTyp nType;	///< node type
	int	fAdr;		///< router's forest address
	int	latitude;	///< latitude of node (in micro-degrees, + or -)
	int	longitude;	///< latitude of node (in micro-degrees, + or -)
	int	firstLeafAdr;	///< router's first assignable leaf address
	int  	lastLeafAdr;	///< router's last assignable leaf address
	int	numIf;		///< number of interfaces
	IfInfo[] iface;		///< interface information
		public RtrNodeInfo() {
			name = ""; nType = Forest.NodeTyp.UNDEF_NODE;
			fAdr = 0; latitude = UNDEF_LAT; longitude = UNDEF_LONG;
			firstLeafAdr = 0; lastLeafAdr = 0;
			numIf = 0; iface = null;
		}
	}

	private RtrNodeInfo[] rtr;
	private UiSetPair routers;	///< tracks routers & unused router #s

	/** maps a node name back to corresponding node number */
	private Map<String, Integer> nameNodeMap;

	/** maps a forest address to a node number */
	private Map<Integer, Integer> adrNodeMap;

	private UiHashTbl locLnk2lnk; ///< local link# to global link#

	class LinkInfo {
	int	leftLnum;	///< local link number used by "left endpoint"
	int	rightLnum;	///< local link number used by "right endpoint"
	RateSpec rates;		///< use "up" to denote rates from "left"
	RateSpec availRates;	///< unused capacity
		public LinkInfo() {
			leftLnum = 0; rightLnum = 0;
			rates = new RateSpec(0); availRates = new RateSpec(0);
		}
	};
	private LinkInfo[] link;

	RateSpec defaultLeafRates;

	/** Constructor for NetInfo, allocates space, initializes private data.
	 *  @param maxNode1 is the max # of nodes in this NetInfo object
	 *  @param maxLink1 is the max # of links in this NetInfo object
	 *  @param maxRtr1 is the max # of routers in this NetInfo object
	 *  @param maxComtree1 is the max # of comtrees in this NetInfo object
	 */
	public NetInfo(int maxNode, int maxLink, int maxRtr) {
		this.maxNode = maxNode; this.maxLink = maxLink;
		this.maxRtr = maxRtr;

		maxLeaf = maxNode-maxRtr;
		netTopo = new Wgraph(maxNode, maxLink);
	
		rtr = new RtrNodeInfo[maxRtr+1];
		routers = new UiSetPair(maxRtr);
	
		leaf = new LeafNodeInfo[maxLeaf+1];
		leaves = new UiSetPair(maxLeaf);
		nameNodeMap = new HashMap<String,Integer>();
		adrNodeMap  = new HashMap<Integer,Integer>();
		controllers = new java.util.HashSet<Integer>();
	
		link = new LinkInfo[maxLink+1];
		locLnk2lnk = new UiHashTbl(2*Math.min(maxLink,
					   maxRtr*(maxRtr-1)/2)+1);

		defaultLeafRates = new RateSpec(50,500,25,250);
	}

	// Methods for working with nodes ///////////////////////////////
	
	/** Check to see that a node number is valid.
	 *  @param n is an integer node number
	 *  @return true if n represents a valid node in the network,
	 *  else false
	 */
	public boolean validNode(int n) {
		return (isLeaf(n) || isRouter(n));
	}
	
	/** Get the number of the "first" node in the Forest network.
	 *  This method is used when iterating through all the nodes.
	 *  @return the node number of a nominal first node.
	 */
	public int firstNode() {
		return (firstRouter() != 0 ? firstRouter() : firstLeaf());
	}
	
	/** Get the node number of the "next" node in the Forest network.
	 *  Used for iterating through all the nodes.
	 *  @param n is the number of a node in the network
	 *  @return the node number of the node following n, or 0 if there is
	 *  no such node
	 */
	public int nextNode(int n) {
		return (isLeaf(n) ? nextLeaf(n) : (isRouter(n) ?
			 (nextRouter(n) != 0 ? nextRouter(n) : firstLeaf())
			: 0));
	}
	
	/** Get the maximum node number for the network.
	 *  @return the largest node number for nodes in this network
	 */
	public int getMaxNode() { return maxNode; }
	
	/** Get the name for a specified node.
	 *  @param n is a node number
	 *  @param s is a provided String in which the result is to be returned
	 *  @return a reference to s
	 */
	public String getNodeName(int n) {
		String s = (isLeaf(n) ? leaf[n-maxRtr].name : 
		     			(isRouter(n) ? rtr[n].name : ""));
		return s;
	}
	
	/** Get the node number corresponding to a give node name.
	 *  @param s is the name of a node
	 *  @return an integer node number or 0 if the given String does not
	 *  the name of any node
	 */
	public int getNodeNum(String s) {
		Integer nodeNum = nameNodeMap.get(s);
		return (nodeNum != null ? nodeNum : 0);
	}
	
	/** Get the node number corresponding to a give forest address.
	 *  @param adr is the forest address of a node
	 *  @return an integer node number or 0 if the given forest address
	 *  is not used by any node
	 */
	public int getNodeNum(int adr) {
		Integer nodeNum = adrNodeMap.get(adr);
		return (nodeNum != null ? nodeNum : 0);
	}
	
	/** Get the type of a specified node.
	 *  @param n is a node number
	 *  @return the node type of n or UNDEF_NODE if n is not a
	 *  valid node number
	 */
	public Forest.NodeTyp getNodeType(int n) {
		return (isLeaf(n) ? leaf[n-maxRtr].nType : (isRouter(n) ?
				rtr[n].nType : Forest.NodeTyp.UNDEF_NODE));
	}
	
	/** Set the name of a node.
	 *  @param n is the node number for the node
	 *  @param nam is its new name
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setNodeName(int n, String nam) {
		if (!validNode(n)) return false;
		if (isRouter(n)) rtr[n].name = nam;
		else 		 leaf[n-maxRtr].name = nam;
		nameNodeMap.put(nam,n);
		return true;
	}
	
	/** Set the forest address of a node.
	 *  @param n is the node number for the node
	 *  @param adr is its new forest address
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setNodeAdr(int n, int adr) {
		if (isLeaf(n)) leaf[n-maxRtr].fAdr = adr;
		else if (isRouter(n)) rtr[n].fAdr = adr;
		else { 
			return false;
		}
		adrNodeMap.put(adr,n);
		return true;
	}
	
	// Additional methods for working with leaf nodes ///////////////////
	
	/** Determine if a given node number identifies a leaf.
	 *  @param n is an integer node number
	 *  @return true if n corresponds to a leaf in the network,
	 *  else false
	 */
	public boolean isLeaf(int n) {
		return (n <= maxRtr ? false : leaves.isIn(n - maxRtr));
	}
	
	/** Get the node number of the first leaf node in the Forest network.
	 *  Used for iterating through all the leaf nodes.
	 *  @return the node number of the nominal first leaf, or 0 if
	 *  no leaves have been defined.
	 */
	public int firstLeaf() {
		return (leaves.firstIn() != 0 ? maxRtr + leaves.firstIn() : 0);
	}
	
	/** Get the node number of the "next" leaf node in the Forest network.
	 *  Used for iterating through all the leaves.
	 *  @param n is the number of a leaf in the network
	 *  @return the node number of the leaf following n, or 0 if there is
	 *  no such leaf
	 */
	public int nextLeaf(int n) {
		int nxt = leaves.nextIn(n-maxRtr);
		return (nxt != 0 ? maxRtr + nxt : 0);
	}

	/** Set the node type of a leaf node.
	 *  @param n is the node number of the leaf
	 *  @param typ is its new node type
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setLeafType(int n, Forest.NodeTyp typ) {
		if (isLeaf(n)) leaf[n-maxRtr].nType = typ;
		else return false;
		return true;
	}
	
	/** Set the IP address of a leaf node.
	 *  @param n is the node number of the leaf
	 *  @param ip is its new IP address
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setLeafIpAdr(int n, int ip) {
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
	public boolean isRouter(int n) { return routers.isIn(n); }
	
	/** Check to see if a given router interface is valid.
	 *  @param r is the node number of a router
	 *  @param iface is an integer interface n umber
	 *  @return true if iface is a falid interface for r,
	 *  else false
	 */
	public boolean validIf(int r, int iface) {
		return isRouter(r) && (1 <= iface && iface <= rtr[r].numIf &&
					  rtr[r].iface[iface].ipAdr != 0);
	}
	
	/** Get the node number of the first router in the Forest network.
	 *  Used for iterating through all the routers.
	 *  @return the node number of the nominal first router, or 0 if
	 *  no routers have been defined.
	 */
	public int firstRouter() { return routers.firstIn(); } 
	
	/** Get the node number of the "next" router in the Forest network.
	 *  Used for iterating through all the routers.
	 *  @param r is the number of a router in the network
	 *  @return the node number of the router following r, or 0 if there is
	 *  no such router
	 */
	public int nextRouter(int r) { return routers.nextIn(r); }
	
	/** Get the maximum router number for the network.
	 *  @return the largest router number for this network
	 */
	public int getMaxRouter() { return maxRtr; }
	
	/** Get the number of routers in this network.
	 *  @return the largest router number for this network
	 */
	public int getNumRouters() { return routers.getNumIn(); }

	/** Get the range of leaf addresses for a router.
	 *  @param r is the node number of the router
	 *  @param adr is the forest address that defines the start of the range
	 *  @return true if the operation succeeds, else false
	 */
	public boolean getLeafRange(int r, Pair<Integer,Integer> range) {
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
	public int getNumIf(int r) {
		return (isRouter(r) ? rtr[r].numIf : 0);
	}
	
	/** Get the IP address of a leaf node
	 *  @param n is a leaf node number
	 *  @return the IP address of n or 0 if n is not a valid leaf
	 */
	public int getLeafIpAdr(int n) {
		return (isLeaf(n) ? leaf[n-maxRtr].ipAdr : 0);
	}
	
	/** Get the Forest address of a node
	 *  @param n is a node number
	 *  @return the Forest address of n or 0 if n is not a valid node
	 */
	public int getNodeAdr(int n) {
		return (isLeaf(n) ? leaf[n-maxRtr].fAdr :
			(isRouter(n) ? rtr[n].fAdr : 0));
	}

	/** Get the location of a node in the Forest network.
	 *  @param n is a node number
	 *  @param loc is a reference to a pair of doubles in which the latitude
	 *  and longitude are returned
	 *  @return true on success, false on failure
	 */
	public boolean getNodeLocation(int n, Pair<Double,Double> loc) {
		if (isLeaf(n)) {
			loc.first  = leaf[n-maxRtr].latitude/1000000.0;
			loc.second = leaf[n-maxRtr].longitude/1000000.0;
		} else if (isRouter(n)) {
			loc.first  = rtr[n].latitude/1000000.0;
			loc.second = rtr[n].longitude/1000000.0;
		} else return false;
		return true;
	}

	/** Get the IP address of a specified router interface.
	 *  @param r is the node number of a router in the network
	 *  @param iface is an interface number for r
	 *  @return the IP address of the specified interface or 0 if
	 *  there is no such interface
	 */
	public int getIfIpAdr(int n, int iface) {
		return (validIf(n,iface) ? rtr[n].iface[iface].ipAdr : 0);
	}

	/** Get the RateSpec for a router interface.
	 *  @param r is the node number of a router in the network
	 *  @param iface is an interface number for r
	 *  @param rs is a reference to a RateSpec in which result is returned
	 *  @return true on success, else false
	 */
	public boolean getIfRates(int r, int iface, RateSpec rs) {
		if (!validIf(r,iface)) return false;
		rs.copyFrom(rtr[r].iface[iface].rates);
		return true;
	}
	
	/** Get the range of link numbers assigned to an interface.
	 *  Each router interface is assigned a consecutive range of link
	 *  numbers.
	 *  @param r is the node number of a router in the network
	 *  @param iface is an interface number for r
	 *  @return true on success, else false
	 */
	public boolean getIfLinks(int r, int iface,
				  Pair<Integer,Integer> links) {
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
	public boolean setLeafRange(int r, Pair<Integer,Integer> range) {
		if (!isRouter(r)) return false;
		rtr[r].firstLeafAdr  = range.first;
		rtr[r].lastLeafAdr = range.second;
		return true;
	}

	/** Set the location of a node in the Forest network.
	 *  @param n is a node number
	 *  @param loc is a reference to a pair of doubles which defines the new
	 *  latitude and longitude
	 *  @return true on success, false on failure
	 */
	public boolean setNodeLocation(int n, Pair<Double,Double> loc) {
		if (isLeaf(n)) {
			leaf[n-maxRtr].latitude  = (int) (1000000.0*loc.first);
			leaf[n-maxRtr].longitude = (int) (1000000.0*loc.second);
		} else if (isRouter(n)) {
			rtr[n].latitude  = (int) (1000000.0*loc.first);
			rtr[n].longitude = (int) (1000000.0*loc.second);
		} else return false;
		return true;
	}
	
	/** Set the RateSpec for a router interface.
	 *  @param r is the node number of a router in the network
	 *  @param iface is an interface number for r
	 *  @param rs is a reference to a RateSpec
	 *  @return true on success, else false
	 */
	public boolean setIfRates(int r, int iface, RateSpec rs) {
		if (!validIf(r,iface)) return false;
		rtr[r].iface[iface].rates.copyFrom(rs);
		return true;
	}
	
	/** Set the range of links defined for a router interface.
	 *  Each router interface is assigned a consecutive range of link
	 *  numbers
	 *  @param r is the node number of the router
	 *  @param iface is the interface number
	 *  @param links is a reference to a pair of link numbers that
	 *  define range
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setIfLinks(int r, int iface,
				  Pair<Integer,Integer> links) {
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
	public boolean setIfIpAdr(int r, int iface, int ip) {
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
	public boolean validLink(int lnk) {
		return netTopo.validEdge(lnk);
	}
	
	/** Get the number of the first link in the Forest network.
	 *  @return the number of the first link or 0 if no links have
	 *  been defined.
	 */
	public int firstLink() { return netTopo.first(); }
	
	/** Get the number of the next link in the Forest network.
	 *  @lnk is the number of a link in the network
	 *  @return the number of the link following lnk or 0 if there is no
	 *  such link
	 */
	public int nextLink(int lnk) { return netTopo.next(lnk); }
	
	/** Get the number of the first link incident to a specified node.
	 *  @param n is the node number of a node in the network
	 *  @return the number of the first link incident to n or 0 if there
	 *  are no links incident to n
	 */
	public int firstLinkAt(int n) {
		return (validNode(n) ? netTopo.firstAt(n) : 0);
	}
	
	/** Get the number of the next link incident to a specified node.
	 *  @param n is the node number of a node in the network
	 *  @param lnk is a link incident to n
	 *  @return the number of the next link incident to n or 0 if there
	 *  is no such link
	 */
	public int nextLinkAt(int n, int lnk) {
		return (validNode(n) ? netTopo.nextAt(n,lnk) : 0);
	}
	
	/** Get the maximum link number for the network.
	 *  @return the largest link number for links in this network
	 */
	public int getMaxLink() { return maxLink; }
	
	/** Get the node number for the "left" endpoint of a given link.
	 *  The endpoints of a link are arbitrarily designated "left" and
	 *  "right".
	 *  @param lnk is a link number
	 *  @return the node number of the left endpoint of lnk, or 0 if lnk
	 *  is not a valid link number.
	 */
	public int getLeft(int lnk) {
		return (validLink(lnk) ? netTopo.left(lnk) : 0);
	}
	
	/** Get the node number for the "right" endpoint of a given link.
	 *  The endpoints of a link are arbitrarily designated "left" and
	 *  "right".
	 *  @param lnk is a link number
	 *  @return the node number of the right endpoint of lnk, or 0 if lnk
	 *  is not a valid link number.
	 */
	public int getRight(int lnk) {
		return (validLink(lnk) ? netTopo.right(lnk) : 0);
	}
	
	/** Get the node number for the "other" endpoint of a given link.
	 *  @param n is a node number
	 *  @param lnk is a link number
	 *  @return the node number of the endpoint of lnk that is not n, or
	 *  0 if lnk is not a valid link number or n is not an endpoint of lnk
	 */
	public int getPeer(int n, int lnk) {
		return (validLink(lnk) ? netTopo.mate(n,lnk) : 0);
	}
	
	/** Get the local link number used by one of the endpoints of a link.
	 *  Each router in a Forest network refers to links using local link #s.
	 *  @param lnk is a "global" link number
	 *  @param r is the node number of router
	 *  @return the local link number used by r or 0 if r is not a router
	 *  or is not incident to lnk
	 */
	public int getLLnum(int lnk, int r) {
		return (!(validLink(lnk) && isRouter(r)) ? 0 :
			(r == netTopo.left(lnk) ? getLeftLLnum(lnk) :
			(r == netTopo.right(lnk) ? getRightLLnum(lnk) : 0)));
	}
	
	/** Get the local link number used by the left endpoint of a link.
	 *  Each router in a Forest network refers to links using local link #s.
	 *  @param lnk is a "global" link number
	 *  @return the local link number used by the left endpoint, or 0 if
	 *  the link number is invalid or the left endpoint is not a router.
	 */
	public int getLeftLLnum(int lnk) {
		int r = getLeft(lnk);
		return (isRouter(r) ? link[lnk].leftLnum : 0);
	}
	
	/** Get the local link number used by the right endpoint of a link.
	 *  Each router in a Forest network refers to links using local link #s.
	 *  @param lnk is a "global" link number
	 *  @return the local link number used by the right endpoint, or 0 if
	 *  the link number is invalid or the left endpoint is not a router.
	 */
	public int getRightLLnum(int lnk) {
		int r = getRight(lnk);
		return (isRouter(r) ? link[lnk].rightLnum : 0);
	}
	
	/** Get the RateSpec of a link in the Forest network.
	 *  @param lnk is a link number
	 *  @param rs is a reference to a RateSpec in which result is returned
	 *  @return true on success, false on failure
	 */
	public boolean getLinkRates(int lnk, RateSpec rs) {
		if (!validLink(lnk)) return false;
		rs.copyFrom(link[lnk].rates);
		return true;
	}
	
	/** Get the available rates of a link in the Forest network.
	 *  @param lnk is a link number
	 *  @param rs is a reference to a RateSpec in which result is returned
	 *  @return true on success, false on failure
	 */
	public boolean getAvailRates(int lnk, RateSpec rs) {
		if (!validLink(lnk)) return false;
		rs.copyFrom(link[lnk].availRates);
		return true;
	}
	
	/** Get the default rates for leaf nodes in a Forest network.
	 *  @param rs is a reference to a RateSpec
	 */
	public void getDefLeafRates(RateSpec rs) {
		rs.copyFrom(defaultLeafRates);
	}
	
	/** Get the length of a link in the Forest network.
	 *  @param lnk is a link number
	 *  @return the assigned link length in kilometers, or 0 if lnk is
	 *  not a valid link number
	 */
	public int getLinkLength(int lnk) {
		return (validLink(lnk) ? netTopo.weight(lnk) : 0);
	}
	
	/** Get the number of the link incident to a leaf node.
	 *  @param n is a node number for a leaf
	 *  @return the number of its incident link or 0 if n is not a leaf
	 *  or if it has no link
	 */
	public int getLinkNum(int n) {
		return (isLeaf(n) ? netTopo.firstAt(n) : 0);
	}
	
	/** Get the global link number of a link incident to a router.
	 *  @param r is a node number of a router
	 *  @param llnk is a local link number of a link at r
	 *  @return the global link number for local link llnk at r, or
	 *  or 0 if r is not a router, or it has no such link
	 */
	public int getLinkNum(int r, int llnk) {
		return (isRouter(r) ? locLnk2lnk.lookup(ll2l_key(r,llnk))/2 :
					(llnk == 0 ? getLinkNum(r) : 0));
	}

	/** Set the local link number used by the left endpoint of a link.
	 *  Each router in a Forest network refers to links using local link #s.
	 *  @param lnk is a "global" link number
	 *  @param loc is local link # to be used by the left endpoint of lnk
	 *  @return true on success, else false
	 */
	public boolean setLeftLLnum(int lnk, int loc) {
		if (validLink(lnk)) link[lnk].leftLnum = loc;
		else return false;
		return true;
	}
	
	/** Set the local link number used by the right endpoint of a link.
	 *  Each router in a Forest network refers to links using local link #s.
	 *  @param lnk is a "global" link number
	 *  @param loc is local link # to be used by the right endpoint of lnk
	 *  @return true on success, else false
	 */
	public boolean setRightLLnum(int lnk, int loc) {
		if (validLink(lnk)) link[lnk].rightLnum = loc;
		else return false;
		return true;
	}
	
	/** Set the RateSpec for a link.
	 *  @param lnk is a link number
	 *  @param rs is the new RateSpec for the link
	 *  @return true on success, false on failure
	 */
	public boolean setLinkRates(int lnk, RateSpec rs) {
		if (!validLink(lnk)) return false;
		link[lnk].rates.copyFrom(rs);
		return true;
	}
	
	/** Set avaiable capacity of a link.
	 *  @param lnk is a link number
	 *  @param rs is a RateSpec
	 */
	public boolean setAvailRates(int lnk, RateSpec rs) {
		if (!validLink(lnk)) return false;
		link[lnk].availRates.copyFrom(rs);
		return true;
	}
	
	/** Set the length of a link.
	 *  @param lnk is a "global" link number
	 *  @param len is the new length
	 *  @return true on success, else false
	 */
	public boolean setLinkLength(int lnk, int len) {
		if (validLink(lnk)) netTopo.setWeight(lnk,len);
		else return false;
		return true;
	}
	
	/** Get the set of nodes that are controllers.
	 *  This method is provided to enable a client program to iterate
	 *  over the set of controllers. The client should not modify the core
	 *  set directly, as this can make the NetInfo object inconsistent.
	 *  @return the node number of the first core node in the comtree,
	 *  or 0 if the comtree index is invalid or no core nodes have
	 *  been defined
	 */
	public Set<Integer> getControllers() {
		return controllers;
	}

	/** Helper method defines keys for internal locLnk2lnk hash table.
	 *  @param r is the node number of a router
	 *  @param llnk is a local link number at r
	 *  @return a hash key appropriate for use with locLnk2lnk
	 */
	public long ll2l_key(int r, int llnk) {
		return (((long) r) << 32) | (((long) llnk) & 0xffffffff);
	}
	
	/** Get the interface associated with a given local link number.
	 *  @param llnk is a local link number
	 *  @param rtr is a router
	 *  @return the number of the interface that hosts llnk
	 */
	public int getIface(int llnk, int rtr) {
		for (int i = 1; i <= getNumIf(rtr); i++) {
			if (!validIf(rtr,i)) continue;
			Pair<Integer,Integer> links = new
				Pair<Integer,Integer>(  new Integer(0),
							new Integer(0));
			getIfLinks(rtr,i,links);
			if (llnk >= links.first && llnk <= links.second)
				return i;
		}
		return 0;
	}

	/** Add a new router to the NetInfo object.
	 *  A new router object is allocated and assigned a name.
	 *  @param name is the name of the new router
	 *  @return the node number of the new router, or 0 if there
	 *  are no more available router numbers, or the given name is
	 *  already used by some other node.
	 */
	public int addRouter(String name) {
		int r = routers.firstOut();
		if (r == 0) return 0;
		if (nameNodeMap.get(name) != null) return 0;
		routers.swap(r);
	
		rtr[r] = new RtrNodeInfo();
		rtr[r].name = new String(name);
		nameNodeMap.put(name,r);
		rtr[r].nType = Forest.NodeTyp.ROUTER;
		rtr[r].fAdr = -1;
	
		Pair<Double,Double> loc;
		loc = new Pair<Double,Double>(	new Double(UNDEF_LAT),
						new Double(UNDEF_LONG));
		setNodeLocation(r,loc);
		Pair<Integer,Integer> range = new Pair<Integer,Integer>(-1,-1);
		setLeafRange(r,range);
		rtr[r].numIf = 0; rtr[r].iface = null;
		return r;
	}
	
	/** Add interfaces to a router.
	 *  Currently this operation can only be done once for a router,
	 *  typically during its initial initiialization.
	 *  @param r is the node number of the router
	 *  @param numIf is the number of interfaces that are to allocated
	 *  to the router
	 *  @return true on success, 0 on failure (the operation fails if
	 *  r is not a valid router number or if interfaces have already
	 *  been allocated to r)
	 */
	boolean addInterfaces(int r, int numIf) {
		if (!isRouter(r) || getNumIf(r) != 0) return false;
		rtr[r].iface = new IfInfo[numIf+1];
		for (int i = 1; i <= numIf; i++)
			rtr[r].iface[i] = new IfInfo();
		rtr[r].numIf = numIf;
		return true;
	}
	
	/** Add a leaf node to a Forest network
	 *  @param name is the name of the new leaf
	 *  @param nTyp is the desired node type (CLIENT or CONTROLLER)
	 *  @return the node number for the new leaf or 0 on failure
	 *  (the method fails if there are no available leaf records to
	 *  allocate to this leaf, or if the requested name is in use)
	 */
	int addLeaf(String name, Forest.NodeTyp nTyp) {
		int ln = leaves.firstOut();
		if (ln == 0) return 0;
		if (nameNodeMap.get(name) != null) return 0;
		leaves.swap(ln);
	
		leaf[ln] = new LeafNodeInfo();
		int nodeNum = ln + maxRtr;
		leaf[ln].name = name;
		nameNodeMap.put(name,nodeNum);
		if (nTyp == Forest.NodeTyp.CONTROLLER) controllers.add(nodeNum);
		leaf[ln].fAdr = -1;
	
		setLeafType(nodeNum,nTyp); setLeafIpAdr(nodeNum,0); 
		Pair<Double,Double> loc;
		loc = new Pair<Double,Double>(
			new Double(UNDEF_LAT), new Double(UNDEF_LONG));
		setNodeLocation(nodeNum,loc);
		return nodeNum;
	}
	
	/** Add a link to a forest network.
	 *  @param u is a node number of some node in the network
	 *  @param v is a node number of some node in the network
	 *  @param uln if u is a ROUTER, then uln is a local link number used by
	 *  u to identify the link - for leaf nodes, this argument is ignored
	 *  @param vln if v is a ROUTER, then vln is a local link number used by
	 *  v to identify the link - for leaf nodes,  this argument is ignored
	 *  @return the link number for the new link or 0 if the operation fails
	 *  (the operation fails if it is unable to associate a given local
	 *  link number with a specified router)
	 */
	public int addLink(int u, int v, int uln, int vln ) {
		int lnk = netTopo.join(u,v);
		netTopo.setWeight(lnk,0);
		if (lnk == 0) return 0;
		link[lnk] = new LinkInfo();
		if (getNodeType(u) == Forest.NodeTyp.ROUTER) {
			if (!locLnk2lnk.insert(ll2l_key(u,uln),2*lnk)) {
				netTopo.remove(lnk); return 0;
			}
			setLeftLLnum(lnk,uln);
		}
		if (getNodeType(v) == Forest.NodeTyp.ROUTER) {
			if (!locLnk2lnk.insert(ll2l_key(v,vln),2*lnk+1)) {
				locLnk2lnk.remove(ll2l_key(u,uln));
				netTopo.remove(lnk); return 0;
			}
			setRightLLnum(lnk,vln);
		}
		RateSpec rs = new RateSpec(Forest.MINBITRATE,Forest.MINBITRATE,
			    		   Forest.MINPKTRATE,Forest.MINPKTRATE);
		setLinkRates(lnk,rs);
		return lnk;
	}
	
	/** Reads a network topology file and initializes the NetInfo data
	 *  structure.
	 *
	 *  Example of NetInfo file format.
	 *  
	 *  <code>
	 *  # define salt router
	 *  router(salt,1.1000,(40,-50),(1.1-1.200),
	 *      # num,  ipAdr,    links, rate spec
	 *      [ 1,  193.168.3.4, 1,    (50000,30000,25000,15000) ],
	 *      [ 2,  193.168.3.5, 2-30, (50000,30000,25000,15000) ]
	 *  )
	 *  
	 *  leaf(netMgr,controller,192.168.1.3,2.900,(40,-50))
	 *  
	 *  link(netMgr,salt.2,1000,(3000,3000,5000,5000))
	 *  ; # semicolon to terminate
	 *  </code>
	 *
	 *  @param in is an open input stream from which topology file is read
	 *  @return true if the operation is successful, false if it fails
	 *  (failures are typically accompanied by error messages on err)
	 */
	public boolean read(PushbackReader in) {
		// objects to hold descriptions to current input item
		RtrNodeInfo cRtr = new RtrNodeInfo();
		LeafNodeInfo cLeaf = new LeafNodeInfo();
		LinkDesc cLink = new LinkDesc();
		IfInfo [] iface = new IfInfo[Forest.MAXINTF+1];
		for (int i = 0; i <= Forest.MAXINTF; i++)
			iface[i] = new IfInfo();
	
		int rtrNum = 1;		// rtrNum = i for i-th router in file
		int leafNum = 1;	// leafNum = i for i-th leaf in file 
		int linkNum = 1;	// linkNum = i for i-th link in file

		String s, errMsg;
		while (true) {
			if (!Util.skipBlank(in) || Util.verify(in,';')) break;
			s = Util.readWord(in);
			if (s == null) {
				System.err.println("read: syntax error: "
					+ "expected (;) or keyword "
					+ "(router,leaf,link)");
				return false;
			}
			if (s.equals("router")) {
				errMsg = readRouter(in, cRtr, iface);
				if (errMsg != null) {
					System.err.println("read: error when "
					    + "attempting to read " + rtrNum
					    + "-th router (" + errMsg + ")");
					return false;
				}
				int r = addRouter(cRtr.name);
				if (r == 0) {
					System.err.println("read: cannot add "
					    + "router " + cRtr.name);
					return false;
				}
				setNodeAdr(r,cRtr.fAdr);
				Pair<Double,Double> loc;
				loc = new Pair<Double,Double>(
					new Double(cRtr.latitude/1000000.0),
					new Double(cRtr.longitude/1000000.0));
				setNodeLocation(r,loc);
				Pair<Integer,Integer> range =
					new Pair<Integer,Integer>(
						  cRtr.firstLeafAdr,
						  cRtr.lastLeafAdr);
				setLeafRange(r,range);
				addInterfaces(r,cRtr.numIf);
				for (int i = 1; i <= getNumIf(r); i++) {
					rtr[r].iface[i].copyFrom(iface[i]);
				}
				rtrNum++;
			} else if (s.equals("leaf")) {
				errMsg = readLeaf(in, cLeaf);
				if (errMsg != null) {
					System.err.println("read: error when "
						+ "attempting to read "
						+ leafNum + "-th leaf ("
						+ errMsg + ")");
					return false;
				}
				int nodeNum = addLeaf(cLeaf.name,cLeaf.nType);
				if (nodeNum == 0) {
					System.err.println("read: cannot add "
					    + "leaf " + cLeaf.name);
					return false;
				}
				setLeafType(nodeNum,cLeaf.nType);
				setLeafIpAdr(nodeNum,cLeaf.ipAdr);
				setNodeAdr(nodeNum,cLeaf.fAdr);
				Pair<Double,Double> loc;
				loc = new Pair<Double,Double>(
					new Double(cLeaf.latitude/1000000.0),
					new Double(cLeaf.longitude/1000000.0));
				setNodeLocation(nodeNum,loc);
				leafNum++;
			} else if (s.equals("link")) {
				errMsg = readLink(in, cLink);
				if (errMsg != null) {
					System.err.println("read: error when "
						+ "attempting to read "
						+ linkNum + "-th link ("
						+ errMsg + ")");
					return false;
				}
				// add new link and set attributes
				Integer u = nameNodeMap.get(cLink.nameL);
				if (u == null) {
					System.err.println("read: error when "
						+ "attempting to read "
						+ linkNum + "-th link ("
					     	+ cLink.nameL
						+ " invalid node name)");
					return false;
				}
				Integer v = nameNodeMap.get(cLink.nameR);
				if (v == null) {
					System.err.println("read: error when "
						+ "attempting to read "
						+ linkNum + "-th link ("
					     	+ cLink.nameR
						+ " invalid node name)");
					return false;
				}
				int lnk = addLink(u.intValue(),v.intValue(),
						  cLink.numL,cLink.numR);
				if (lnk == 0) {
					System.err.println("read: can't add "
					    + "link (" + cLink.nameL + "."
					    + cLink.numL + "," + cLink.nameR
					    + "." + cLink.numR + ")");
					return false;
				}
				setLinkRates(lnk, cLink.rates);
				cLink.rates.scale(.9);
				setAvailRates(lnk, cLink.rates);
				setLinkLength(lnk, cLink.length);
				linkNum++;
			} else if (s.equals("defaultLeafRates")) {
				if (!readRateSpec(in,defaultLeafRates)) {
					System.err.println("read: can't read "
						+ "default rates for links");
					return false;
				}
			} else {
				System.err.println("read: unrecognized "
					+ "keyword (" + s + ")");
				return false;
			}
		}
		return check();
	}
	
	/** Read a router description from an input stream.
	 *  Expects the next nonblank input character to be the opening
	 *  parentheis of the router description.
	 *  @param in is an open input stream
	 *  @param rtr is a reference to a RtrNodeInfo struct used to
	 *  return the result
	 *  @param ifaces points to an array of IfInfo structs used to
	 *  return the interface definitions
	 *  @param errMsg is a reference to a string used for returning an error
	 *  message
	 *  @return a null reference on success and a reference to an
	 *  error message on failure
	 */
	String readRouter(PushbackReader in,RtrNodeInfo rtr,IfInfo [] ifaces) {
		String name; 
		Pair<Double,Double> loc = new
			Pair<Double,Double>(new Double(0), new Double(0));
		Pair<Integer,Integer> adrRange = new
			Pair<Integer,Integer>(new Integer(0),new Integer(0));
		
		if (!Util.verify(in,'(') ||
		    (name = Util.readWord(in)) == null ||
		    !Util.verify(in,',')) {
			return "could not read router name";
		}
		Util.MutableInt fadr = new Util.MutableInt();
		if (!Forest.readForestAdr(in,fadr) || !Util.verify(in,',')) {
			return "could not read Forest address for router "
				+ name;
		}
		if (!readLocation(in,loc) || !Util.verify(in,',')) {
			return "could not read location for router " + name;
		}
		if (!readAdrRange(in,adrRange) || !Util.verify(in,',')) {
			return "could not read address range for router "
				+ name;
		}
		Util.skipBlank(in);
		Util.MutableInt ifn = new Util.MutableInt();
		String errMsg = readIface(in,ifaces,ifn);
		if (errMsg != null) return errMsg;
		int maxif = ifn.val;
		while (Util.verify(in,',')) {
			Util.skipBlank(in);
			errMsg = readIface(in,ifaces,ifn);
			if (errMsg != null) return errMsg;
			maxif = Math.max(ifn.val,maxif);
		}
		Util.skipBlank(in);
		if (!Util.verify(in,')'))
			return "syntax error, expected right paren";
	
		rtr.name = name; rtr.fAdr = fadr.val;
		rtr.latitude = (int) (1000000*loc.first);
		rtr.longitude = (int) (1000000*loc.second);
		rtr.firstLeafAdr = adrRange.first;
		rtr.lastLeafAdr = adrRange.second;
		rtr.numIf = maxif;
		return null;
	}
	
	/** Read a (latitude,longitude) pair.
	 *  @param in is an open input stream
	 *  @param loc is a reference to a pair of doubles in which result
	 *  is returned
	 *  @return true on success, false on failure
	 */
	boolean readLocation(PushbackReader in, Pair<Double,Double> loc) {
		Util.MutableDouble x = new Util.MutableDouble();
		if (!Util.verify(in,'(') || !Util.readDouble(in,x))
			return false;
		loc.first = x.val;
		if (!Util.verify(in,',') || !Util.readDouble(in,x))
			return false;
		loc.second = x.val;
		if (!Util.verify(in,')')) return false;
		return true;
	}
	
	/** Read an address range pair. 
	 *  @param in is an open input stream
	 *  @param range is a reference to a pair of Forest addresses in which
	 *  result is returned
	 *  @return true on success, false on failure
	 */
	boolean readAdrRange(PushbackReader in, Pair<Integer,Integer> range) {
		Util.MutableInt x = new Util.MutableInt();
		if (!Util.verify(in,'(') || !Forest.readForestAdr(in,x))
			return false;
		range.first = x.val;
		if (!Util.verify(in,'-') || !Forest.readForestAdr(in,x))
			return false;
		range.second = x.val;
		return Util.verify(in,')');
	}
	
	/** Read a rate specification.
	 *  @param in is an open input stream
	 *  @param rs is a reference to a RateSpec in which result is returned
	 *  @return true on success, false on failure
	 */
	boolean readRateSpec(PushbackReader in, RateSpec rs) {
		Util.MutableInt bru = new Util.MutableInt();
		Util.MutableInt brd = new Util.MutableInt();
		Util.MutableInt pru = new Util.MutableInt();
		Util.MutableInt prd = new Util.MutableInt();
		if (!Util.verify(in,'(') || !Util.readNum(in,bru) ||
		    !Util.verify(in,',') || !Util.readNum(in,brd) ||
		    !Util.verify(in,',') || !Util.readNum(in,pru) ||
		    !Util.verify(in,',') || !Util.readNum(in,prd) ||
		    !Util.verify(in,')'))
			return false;
		rs.set(bru.val,brd.val,pru.val,prd.val);
		return true;
	}
	
	/** Read a router interface description from an input stream.
	 *  @param in is an open input stream
	 *  @param ifaces is an array of IfInfo structs used to return
	 *  the interface definition; this interface is written at the position
	 *  specified by its interface number
	 *  @param ifn is a MutableInt object which is equal to the
	 *  interface number following a successful return
	 *  @return a null reference on success and a reference to an
	 *  error message on failure
	 */
	public String readIface(PushbackReader in, IfInfo [] ifaces,
						   Util.MutableInt ifn) {
		int ip; int firstLink, lastLink;
		RateSpec rs = new RateSpec(0);
	
		if (!Util.verify(in,'[')) {
			return "syntax error: expected left bracket";
		}
		if (!Util.readNum(in,ifn)) {
			return "could not read interface number";
		}
		if (ifn.val < 1 || ifn.val > Forest.MAXINTF) {
			return "interface number " + ifn.val
				+ " exceeds allowed range";
		}
		if (!Util.verify(in,',') || (ip = Util.readIpAdr(in)) == 0) {
			return "could not read ip address for interface "
				+ ifn.val;
		}
		if (!Util.verify(in,',')) {
			return "syntax error in iface " + ifn.val
				+ ", expected comma";
		}
		Util.MutableInt x = new Util.MutableInt();
		if (!Util.readNum(in,x)) {
			return "could not read link range for iface " + ifn.val;
		}
		firstLink = x.val;
		lastLink = firstLink;
		if (Util.verify(in,'-')) {
			if (!Util.readNum(in,x)) {
				return "could not read link range for iface "
					+ ifn.val;
			}
			lastLink = x.val;
		}
		if (!Util.verify(in,',')) {
			return "syntax error in iface " + ifn.val
				+ ", expected comma";
		}
		if (!readRateSpec(in,rs)) {
			return "could not read ip address for interface "
				+ ifn.val;
		}
		if (!Util.verify(in,']')) {
			return "syntax error in iface " + ifn.val
			   	+ ", expected right bracket";
		}
		ifaces[ifn.val].ipAdr = ip;
		ifaces[ifn.val].firstLink = firstLink;
		ifaces[ifn.val].lastLink = lastLink;
		ifaces[ifn.val].rates.copyFrom(rs);
		return null;
	}
	
	/** Read a leaf node description from an input stream.
	 *  Expects the next nonblank input character to be the opening
	 *  parentheis of the leaf node description.
	 *  @param in is a PushbackReader for an open input stream
	 *  @param leaf is a reference to a LeafNodeInfo struct used to
	 *  return result
	 *  @return a null reference on success, and a reference to an
	 *  error message on failure
	 */
	public String readLeaf(PushbackReader in, LeafNodeInfo leaf) {
		String name; int ip;
		Forest.NodeTyp ntyp = Forest.NodeTyp.UNDEF_NODE;
		Pair<Double,Double> loc = new
			Pair<Double,Double>(new Double(0),new Double(0)); 
		
		if (!Util.verify(in,'(') ||
		    (name = Util.readWord(in)) == null ||
		    !Util.verify(in,',')) {
			return "could not read leaf node name";
		}
		String typstr;
		if ((typstr = Util.readWord(in)) == null ||
		    (ntyp = Forest.string2nodeTyp(typstr)) ==
			    Forest.NodeTyp.UNDEF_NODE ||
		    !Util.verify(in,',')) {
			return "could not read leaf type";
		}
		if ((ip = Util.readIpAdr(in)) == 0 ||
		    !Util.verify(in,',')) {
			return "could not read IP address for leaf " + name;
		}
		Util.MutableInt fadr = new Util.MutableInt();
		if (!Forest.readForestAdr(in,fadr) || !Util.verify(in,',')) {
			return "could not read Forest address for leaf " + name;
		}
		if (!readLocation(in,loc) || !Util.verify(in,')')) {
			return "could not read location for leaf node " + name;
		}
		leaf.name = name; leaf.nType = ntyp; leaf.ipAdr = ip;
		leaf.fAdr = fadr.val;
		leaf.latitude = (int) (1000000*loc.first);
		leaf.longitude = (int) (1000000*loc.second);
		return null;
	}
	
	/** Read a link description from an input stream.
	 *  Expects the next nonblank input character to be the opening
	 *  parentheis of the link description.
	 *  @param in is an open input stream
	 *  @param link is a reference to a LinkDesc struct used to
	 *  return the result
	 *  @param errMsg is a reference to a string used for returning an error
	 *  message
	 *  @return a null object reference on success, and an error message
	 *  on failure
	 */
	public String readLink(PushbackReader in, LinkDesc link) {
		LinkEndpoint ep1;
		if (!Util.verify(in,'(') ||
		    (ep1 = readLinkEndpoint(in)) == null ||
		    !Util.verify(in,',')) {
			return "could not first link endpoint";
		}
		LinkEndpoint ep2;
		if ((ep2 = readLinkEndpoint(in)) == null ||
		    !Util.verify(in,',')) {
			return "could not read second link endpoint";
		}
		Util.MutableInt length = new Util.MutableInt();
		if (!Util.readNum(in,length)) {
			return "could not read link length";
		}
		RateSpec rs = new RateSpec(0);
		if (Util.verify(in,')')) { // omitted rate spec
			rs = defaultLeafRates;
		} else {
			if (!Util.verify(in,',') || !readRateSpec(in,rs)) {
				return "could not read rate specification";
			}
			if (!Util.verify(in,')')) {
				return "syntax error, expected right paren";
			}
		}
		link.nameL = ep1.name; link.numL = ep1.num;
		link.nameR = ep2.name; link.numR = ep2.num;
		link.length = length.val;
		link.rates.copyFrom(rs);
		return null;
	}

	/** Read a link endpoint.
	 *  @param in is a PushbackReader for an open input stream
	 *  @return a reference to a LinkEndpoint object on success,
	 *  null on failure
	 */
	public LinkEndpoint readLinkEndpoint(PushbackReader in) {
		String name = Util.readWord(in);
		if (name == null) return null;
		Util.MutableInt num = new Util.MutableInt();
		if (Util.verify(in,'.')) {
			if (!Util.readNum(in,num)) return null;
		}
		return new LinkEndpoint(name,num.val);	
	}
	
	/** Perform a series of consistency checks.
	 *  Print error message for each detected problem.
	 *  @return true if all checks passed, else false
	 */
	boolean check() {
		boolean status = true;
	
		// make sure there is at least one router
		if (getNumRouters() == 0 || firstRouter() == 0) {
			System.err.println("check: no routers in network, "
				+ "terminating");
			return false;
		}
		// make sure that local link numbers
		// the same local link number
		if (!checkLocalLinks()) status = false;
	
		// make sure that routers are all connected, by doing
		// a breadth-first search from firstRouter()
		if (!checkBackBone()) status = false;
	
		// check that node addresses are consistent
		if (!checkAddresses()) status = false;
	
		// check that the leaf address ranges are consistent
		if (!checkLeafRange()) status = false;
	
		// check that all leaf nodes are consistent.
		if (!checkLeafNodes()) status = false;
	
		// check that link rates are within bounds
		if (!checkLinkRates()) status = false;
	
		// check that router interface rates are within bounds
		if (!checkRtrRates()) status = false;
	
		return status;
	}
	
	/** Check that the local link numbers at all routers are consistent.
	 *  Write message to stderr for every error detected.
	 *  @return true if all local link numbers are consistent, else false
	 */
	boolean checkLocalLinks() {
		boolean status = true;
		for (int rtr = firstRouter(); rtr != 0; rtr = nextRouter(rtr)) {
			for (int l1 = firstLinkAt(rtr); l1 != 0;
				 l1 = nextLinkAt(rtr,l1)) {
				for (int l2 = nextLinkAt(rtr,l1); l2 != 0;
					 l2 = nextLinkAt(rtr,l2)) {
					if (getLLnum(l1,rtr) ==
					    getLLnum(l2,rtr)) {
						System.err.println(
						    "checkLocalLinks: "
						    + "detected two links at "
						    + "router " + rtr
						    + " with same local link "
						    + "number: "
						    + link2string(l1) + " and "
						    + link2string(l2));
						status = false;
					}
				}
				// check that local link numbers fall within the
				// range of some valid interface
				int llnk = getLLnum(l1,rtr);
				if (getIface(llnk,rtr) == 0) {
					System.err.println("checkLocalLinks: "
					    + "link " + llnk + " at "
					    + getNodeName(rtr) + " is not "
					    + "in the range assigned "
					    + "to any valid interface");
					status = false;
				}
			}
		}
		return status;
	}
	
	/** Check that the routers are all connected.
	 *  Write message to stderr for every error detected.
	 *  @return true if the routers form a connected network, else false
	 */
	boolean checkBackBone() {
		Set<Integer> seen = new java.util.HashSet<Integer>();
		seen.add(firstRouter());
		Queue<Integer> pending = new LinkedList<Integer>();
		pending.add(firstRouter());
		while (!pending.isEmpty()) {
			int u = pending.remove(); 
			for (int lnk = firstLinkAt(u); lnk != 0;
				 lnk = nextLinkAt(u,lnk)) {	
				int v = getPeer(u,lnk);
				if (getNodeType(v) != Forest.NodeTyp.ROUTER)
					continue;
				if (seen.contains(v)) continue;
				seen.add(v);
				pending.add(v);
			}
		}
		if ((int) seen.size() == getNumRouters()) return true;
		System.err.println("checkBackbone: network is not connected");
		return false;
	}
	
	/** Check that no two nodes have the same Forest address.
	 *  Write message to stderr for every error detected.
	 *  @return true if all addresses are unique, else false
	 */
	boolean checkAddresses() {
		boolean status = true;
		for (int n1 = firstNode(); n1 != 0; n1 = nextNode(n1)) {
			for (int n2 = nextNode(n1); n2!=0; n2 = nextNode(n2)) {
				if (getNodeAdr(n1) == getNodeAdr(n2)) {
					System.err.println("check: detected "
					    + "two nodes " + getNodeName(n1)
					    + " and " + getNodeName(n2)
					    + " with the same forest address");
					status = false;
				}
			}
		}
		return status;
	}
	
	/** Check leaf address ranges of all routers.
	 *  Write message to stderr for every error detected.
	 *  @return true if each router's leaf address range is consistent with
	 *  its address (must all lie in its zip code) and no two routers have
	 *  overlapping leaf address ranges, otherwise return false
	 */
	public boolean checkLeafRange() {
		boolean status = true;
		// check that the leaf address range for a router
		// is compatible with the router's address
		for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
			int rzip = Forest.zipCode(getNodeAdr(r));
			Pair<Integer,Integer> range =
				new Pair<Integer,Integer>(0,0);
			getLeafRange(r,range);
			int flzip = Forest.zipCode(range.first);
			int llzip = Forest.zipCode(range.second);
			if (rzip != flzip || rzip != llzip) {
				System.err.println("netInfo.checkLeafRange: "
				    + "detected router " + r + "with "
				    + "incompatible address and leaf address "
				    + "range");
				status = false;
			}
			if (range.first > range.second) {
				System.err.println("netInfo.check: detected "
				    + "router " + r + "with empty leaf "
				    + "address range");
				status = false;
			}
		}
	
		// make sure that no two routers have overlapping leaf
		// address ranges
		for (int r1 = firstRouter(); r1 != 0; r1 = nextRouter(r1)) {
			Pair<Integer,Integer> range1 =
				new Pair<Integer,Integer>(0,0);
			getLeafRange(r1,range1);
			for (int r2 = nextRouter(r1); r2 != 0;
				 r2 = nextRouter(r2)) {
				Pair<Integer,Integer> range2 =
					new Pair<Integer,Integer>(0,0);
				getLeafRange(r2,range2);
				if (range1.first > range2.first) continue;
				if (range2.first <= range1.second) {
					System.err.println(
					    "netInfo.checkLeafRange: detected "
					    + "two routers " + r1 + " and "
					    + r2 + " with overlapping "
					    + "address ranges");
					status = false;
				}
			}
		}
		return status;
	}
	
	/** Check consistency of leaf nodes.
	 *  Write message to stderr for every error detected.
	 *  @return true if leaf nodes all pass consistency checks, else false
	 */
	public boolean checkLeafNodes() {
		boolean status = true;
		for (int u = firstLeaf(); u != 0; u = nextLeaf(u)) {
			int lnk = firstLinkAt(u);
			if (lnk == 0) {
				System.err.println("checkLeafNodes: detected "
				    + "a leaf node " + getNodeName(u)
				    + " with no links");
				status = false; continue;
			}
			if (nextLinkAt(u,lnk) != 0) {
				System.err.println("checkLeafNodes: detected "
				    + "a leaf node " + getNodeName(u)
				    + " with more than one link");
				status = false; continue;
			}
			if (getNodeType(getPeer(u,lnk)) !=
			    Forest.NodeTyp.ROUTER) {
				System.err.println("checkLeafNodes: detected "
				    + "a leaf node " + getNodeName(u)
				    + " with link to non-router");
				status = false; continue;
			}
			int rtr = getPeer(u,lnk);
			int adr = getNodeAdr(u);
			Pair<Integer,Integer> range = new
				Pair<Integer,Integer>(	new Integer(0),
							new Integer(0));
			getLeafRange(rtr,range);
			if (adr < range.first || adr > range.second) {
				System.err.println("checkLeafNodes: detected "
				    + "a leaf node " + getNodeName(u)
				    + " with an address outside the leaf "
				    + "address range of its router");
				status = false; continue;
			}
		}
		return status;
	}
	
	/** Check that link rates are consistent with Forest requirements.
	 *  @return true if all link rates are consistent, else false
	 */
	public boolean checkLinkRates() {
		boolean status = true;
		for (int lnk = firstLink(); lnk != 0; lnk = nextLink(lnk)) {
			RateSpec rs = new RateSpec(0); getLinkRates(lnk,rs);
			if (rs.bitRateUp < Forest.MINBITRATE ||
			    rs.bitRateUp > Forest.MAXBITRATE ||
			    rs.bitRateDown < Forest.MINBITRATE ||
			    rs.bitRateDown > Forest.MAXBITRATE) {
				System.err.println("check: detected a link "
				     + link2string(lnk) + " with bit rate "
				     + "outside the allowed range");
				status = false;
			}
			if (rs.pktRateUp < Forest.MINPKTRATE ||
			    rs.pktRateUp > Forest.MAXPKTRATE ||
			    rs.pktRateDown < Forest.MINPKTRATE ||
			    rs.pktRateDown > Forest.MAXPKTRATE) {
				System.err.println("check: detected a link "
				     + link2string(lnk) + " with packet rate "
				     + "outside the allowed range");
				status = false;
			}
		}
		return status;
	}
	
	/** Check interface and link rates at all routers for consistency.
	 *  Write an error message to stderr for each error detected
	 *  @return true if all check pass, else false
	 */
	public boolean checkRtrRates() {
		boolean status =true;
		for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
			// check all interfaces at r
			for (int i = 1; i <= getNumIf(r); i++) {
				if (!validIf(r,i)) continue;
				RateSpec rs = new RateSpec(0);
				getIfRates(r,i,rs);
				if (rs.bitRateUp < Forest.MINBITRATE ||
				    rs.bitRateUp >Forest.MAXBITRATE ||
				    rs.bitRateDown < Forest.MINBITRATE ||
				    rs.bitRateDown >Forest.MAXBITRATE) {
					System.err.println("checkRtrRates: "
					    + "interface " + i + "at router "
					    + r + " has bit rate outside the "
					    + "allowed range");
					status = false;
				}
				if (rs.pktRateUp < Forest.MINPKTRATE ||
				    rs.pktRateUp >Forest.MAXPKTRATE ||
				    rs.pktRateDown < Forest.MINPKTRATE ||
				    rs.pktRateDown >Forest.MAXPKTRATE) {
					System.err.println("checkRtrRates: "
					    + "interface " + i + "at router "
					    + r + " has packet rate outside "
					    + "the allowed range");
					status = false;
				}
			}
			// check that the link rates at each interface do not
			// exceed interface rate
			RateSpec [] ifrates = new RateSpec[getNumIf(r)+1];
			for (int i = 0; i <= getNumIf(r); i++) {
				ifrates[i] = new RateSpec(0);
			}
			for (int lnk = firstLinkAt(r); lnk != 0;
				 lnk = nextLinkAt(r,lnk)) {
				int llnk = getLLnum(lnk,r);
				int iface = getIface(llnk,r);
				RateSpec rs = new RateSpec(0);
				getLinkRates(lnk,rs);
				if (r == getLeft(lnk)) rs.flip();
				ifrates[iface].add(rs);
			}
			for (int i = 0; i <= getNumIf(r); i++) {
				if (!validIf(r,i)) continue;
				RateSpec ifrs = new RateSpec(0);
				getIfRates(r,i,ifrs);
				if (!ifrates[i].leq(ifrs)) {
					System.err.println("check: links at "
					    + "interface " + i + " of router "
					    + getNodeName(r)
					    + " exceed its capacity");
				}
			}
		}
		return status;
	}
	
	/** Create a String representation of a link.
	 *  @param lnk is a link number
	 *  @return the string representing the link
	 */
	public String link2string(int lnk) {
		String s = "";
		if (lnk == 0) { s += "-"; return s; }
		int left = getLeft(lnk); int right = getRight(lnk);
		s += "(" + getNodeName(left);
		if (getNodeType(left) == Forest.NodeTyp.ROUTER) 
			s += "." + getLeftLLnum(lnk);
		s += "," + getNodeName(right);
		if (getNodeType(right) == Forest.NodeTyp.ROUTER)
			s += "." + getRightLLnum(lnk);
		s += ")";
		return s;
	}
	
	/** Create a string representation of a link and its properties.
	 *  @param link is a link number
	 *  @return the string representing the link and its properties
	 */
	public String linkProps2string(int lnk) {
		String s = "";
		if (lnk == 0) { s += "-"; return s; }
		int left = getLeft(lnk); int right = getRight(lnk);
		s += "(" + getNodeName(left);
		if (getNodeType(left) == Forest.NodeTyp.ROUTER)
			s += "." + getLeftLLnum(lnk);
		s += "," + getNodeName(right);
		if (getNodeType(right) == Forest.NodeTyp.ROUTER)
			s += "." + getRightLLnum(lnk);
		s += "," + getLinkLength(lnk) + ",";
		RateSpec rs = new RateSpec(0);
		getLinkRates(lnk,rs); s += rs.toString() + ")";
		return s;
	}
	
	/** Create a string representation of a link and its current state.
	 *  @param link is a link number
	 *  @return the string representing the link and its state
	 */
	public String linkState2string(int lnk) {
		String s = "";
		if (lnk == 0) { s += "-"; return s; }
		int left = getLeft(lnk); int right = getRight(lnk);
		s += "(" + getNodeName(left);
		if (getNodeType(left) == Forest.NodeTyp.ROUTER) 
			s += "." + getLeftLLnum(lnk);
		s += "," + getNodeName(right);
		if (getNodeType(right) == Forest.NodeTyp.ROUTER) 
			s += "." + getRightLLnum(lnk);
		s += "," + getLinkLength(lnk) + ",";
		RateSpec rs = new RateSpec(0);
		getLinkRates(lnk,rs); s += rs.toString();
		getAvailRates(lnk,rs); s += "," + rs.toString() + ")";
		return s;
	}
	
	/** Create a string representation of the NetInfo object.
	 *  @return the string representing the object
	 */
	public String toString() {
		// First write the routers section
		String s = "";
		for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
			s += rtr2string(r);
		}
	
		// Next write the leaf nodes, starting with the controllers
		for (int c : controllers) {
			s += leaf2string(c);
		}
		for (int n = firstLeaf(); n != 0; n = nextLeaf(n)) {
			if (getNodeType(n) != Forest.NodeTyp.CONTROLLER)
				s += leaf2string(n);
		}
		s += '\n';
	
		// Now, write the links
		for (int lnk = firstLink(); lnk != 0; lnk = nextLink(lnk)) {
			s += "link" + linkProps2string(lnk) + "\n";
		}
	
		// and finally, the default link rates
		s += "defaultLeafRates" + (defaultLeafRates.toString()) + "\n";
	
		s += ";\n"; return s;
	}
	
	/** Create a string representation of a router.
	 *  @param rtr is a node number for a router
	 *  @return a string representing the router
	 */
	public String rtr2string(int rtr) {
		String s = "";
		s += "router(" + getNodeName(rtr) + ", ";
		s += Forest.fAdr2string(getNodeAdr(rtr)) + ", ";
		Pair<Double,Double> loc = new
			Pair<Double,Double>(new Double(0), new Double(0));
		getNodeLocation(rtr,loc);
		s += "(" + loc.first + "," + loc.second + "), ";
		Pair<Integer,Integer> range = new
			Pair<Integer,Integer>(new Integer(0), new Integer(0));
		getLeafRange(rtr,range);
		s += "(" + Forest.fAdr2string(range.first);
		s += "-" + Forest.fAdr2string(range.second) + "),\n";
		for (int i = 1; i <= getNumIf(rtr); i++) {
			if (!validIf(rtr,i)) continue;
			s += "\t[ " + i + ",  "
			   + Util.ipAdr2string(getIfIpAdr(rtr,i)) + ", "; 
			Pair<Integer,Integer> links =
				new Pair<Integer,Integer>(0,0);
			getIfLinks(rtr,i,links);
			if (links.first == links.second) {
				s += links.first + ", ";
			} else {
				s += links.first + "-" + links.second + ", ";
			}
			RateSpec rs = new RateSpec(0);
			getIfRates(rtr,i,rs);
			s += rs.toString() + " ]\n";
		}
		s += ")\n";
		return s;
	}
	
	/** Create a string representation of a leaf node.
	 *  @param leaf is a node number for a leaf node
	 *  @return a string representing the leaf
	 */
	public String leaf2string(int leaf) {
		String s = "";
		s += "leaf(" + getNodeName(leaf) + ", ";
		s += Forest.nodeTyp2string(getNodeType(leaf)) + ", ";
		s += Util.ipAdr2string(getLeafIpAdr(leaf)) + ", "; 
		s += Forest.fAdr2string(getNodeAdr(leaf)) + ", ";
		Pair<Double,Double>loc =
			new Pair<Double,Double>(new Double(0),new Double(0));
		getNodeLocation(leaf,loc);
		s += "(" + loc.first + "," + loc.second + ")";
		s += ")\n";
		return s;
	}
}
