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
	private int maxComtree;	///< maximum number of comtrees

	/** NetTopo is a weighted graph defining the network topology.
	 *  Weights represent link costs */
	private Wgraph netTopo;

	public class RateSpec {            ///< used in lnkMap of ComtreeInfo
        public int bitRateUp;      ///< upstream bit rate on comtree link
        public int bitRateDown;    ///< downstream bit rate on comtree link
        public int pktRateUp;      ///< upstream packet rate on comtree link
        public int pktRateDown;    ///< downstream packet rate on comtree link
	}

	public class ComtRtrInfo {   ///< used in rtrMap of ComtreeInfo
        public int plnk;           ///< link to parent in comtree
        public int lnkCnt;         ///< number of comtree links at this router
		public ComtRtrInfo() {
			plnk = 0; lnkCnt = 0;
		}
	}

	private class IfInfo {
	int	ipAdr;		///< ip address of forest interface
	int	bitRate;	///< max bit rate for interface (Kb/s)
	int	pktRate;	///< max pkt rate (packets/s)
	int	firstLink;	///< number of first link assigned to interface
	int	lastLink;	///< number of last link assigned to interface
		public IfInfo() {
			ipAdr = 0; bitRate = 0; pktRate = 0;
			firstLink = 0; lastLink = 0;
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
	int	bitRate;	///< max bit rate for link
	int	pktRate;	///< max packet rate for link
	int	availBitRateL;	///< available bit rate from left endpoint
	int	availPktRateL;	///< available packet rate from left endpoint
	int	availBitRateR;	///< available bit rate from right endpoint
	int	availPktRateR;	///< available packet rate from right endpoint
		public LinkInfo() {
			leftLnum = 0; rightLnum = 0;
			bitRate = 0; pktRate = 0;
			availBitRateL = 0; availPktRateL = 0;
			availBitRateR = 0; availPktRateR = 0;
		}
	};
	private LinkInfo[] link;

	public class ComtreeInfo {
	int	comtreeNum;	///< number of comtree
	int	root;		///< root node of comtree
	int	ownerAdr;	///< address of client that created comtree
	int	bitRateDown;	///< downstream bit rate for backbone links
	int	bitRateUp;	///< upstream bit rate for backbone links
	int	pktRateDown;	///< downstream packet rate for backbone links
	int	pktRateUp;	///< upstream packet rate for backbone links
	int	leafBitRateDown;///< downstream bit rate for leaf links
	int	leafBitRateUp;	///< upstream bit rate for leaf links
	int	leafPktRateDown;///< downstream packet rate for leaf links
	int	leafPktRateUp;	///< upstream packet rate for leaf links
	Set<Integer> coreSet;	///< set of core nodes in comtree
	Map<Integer,RateSpec> lnkMap; ///< map for links in comtree
	Map<Integer,ComtRtrInfo> rtrMap; ///< map for nodes in comtree
		public ComtreeInfo() {
			comtreeNum = 0; root = 0; ownerAdr = 0;
			bitRateDown = 0; bitRateUp = 0;
			pktRateDown = 0; pktRateUp = 0;
			leafBitRateDown = 0; leafBitRateUp = 0;
			leafPktRateDown = 0; leafPktRateUp = 0;
			coreSet = null; lnkMap = null; rtrMap = null;
		}
	};
	private ComtreeInfo[] comtree;	///< array of comtrees
	private IdMap comtreeMap;	///< maps comtree #s to indices in array

	/** Constructor for NetInfo, allocates space, initializes private data.
	 *  @param maxNode1 is the max # of nodes in this NetInfo object
	 *  @param maxLink1 is the max # of links in this NetInfo object
	 *  @param maxRtr1 is the max # of routers in this NetInfo object
	 *  @param maxComtree1 is the max # of comtrees in this NetInfo object
	 */
	public NetInfo(int maxNode, int maxLink, int maxRtr, int maxComtree) {

		this.maxNode = maxNode; this.maxLink = maxLink;
		this.maxRtr = maxRtr;
		this.maxComtree = maxComtree;

		maxLeaf = maxNode-maxRtr;
		netTopo = new Wgraph(maxNode, maxLink);
	
		rtr = new RtrNodeInfo[maxRtr+1];
		routers = new UiSetPair(maxRtr);
	
		leaf = new LeafNodeInfo[maxLeaf+1];
		leaves = new UiSetPair(maxLeaf);
		nameNodeMap = new HashMap<String,Integer>();
		adrNodeMap  = new HashMap<Integer,Integer>();
		controllers = new HashSet<Integer>();
	
		link = new LinkInfo[maxLink+1];
		locLnk2lnk = new UiHashTbl(2*Math.min(maxLink,
					   maxRtr*(maxRtr-1)/2)+1);
	
		comtree = new ComtreeInfo[maxComtree+1];
		comtreeMap = new IdMap(maxComtree);
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
		else return false;
		adrNodeMap.put(n,adr);
		return true;
	}
	
	/** Set the latitude of a node.
	 *  @param n is the node number for the node
	 *  @param lat is its new latitude
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setNodeLat(int n, double lat) {
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
	public boolean setNodeLong(int n, double longg) {
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
	
	/** Get the first address in a router's range of client addresses.
	 *  Each router is assigned a range of Forest addresses for its clients.
	 *  This method gets the first address in the range.
	 *  @param r is the node number of a router
	 *  @return the first client address for r or 0 if r is not
	 *  a valid router
	 */
	public int getFirstLeafAdr(int r) {
		return (isRouter(r) ? rtr[r].firstLeafAdr : 0);
	}
	
	/** Get the last address in a router's range of client addresses.
	 *  Each router is assigned a range of Forest addresses for its clients.
	 *  This method gets the last address in the range.
	 *  @param r is the node number of a router
	 *  @return the last client address for r or 0 if r is not
	 *  a valid router
	 */
	public int getLastLeafAdr(int r) {
		return (isRouter(r) ? rtr[r].lastLeafAdr : 0);
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
	
	/** Get the latitude of a node in the Forest network.
	 *  Each node has a location which is represented using latitude
	 *  and longitude. Latitudes are returned as double precision values
	 *  in units of degrees.
	 *  @param n is a node number
	 *  @return the latitude defined for n
	 */
	public double getNodeLat(int n) {
		double x = (isLeaf(n) ? leaf[n-maxRtr].latitude :
			    (isRouter(n) ? rtr[n].latitude : UNDEF_LAT));
		return x/1000000;
	}
	
	/** Get the longitude of a node in the Forest network.
	 *  Each node has a location which is represented using latitude
	 *  and longitude. Latitudes are returned as double precision values
	 *  in units of degrees.
	 *  @param n is a node number
	 *  @return the longitude defined for n
	 */
	public double getNodeLong(int n) {
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
	public int getIfIpAdr(int n, int iface) {
		return (validIf(n,iface) ? rtr[n].iface[iface].ipAdr : 0);
	}
	
	/** Get the first link in range of links assigned to a given interface.
	 *  Each router interface is assigned a consecutive range of link #s.
	 *  @param r is the node number of a router in the network
	 *  @param iface is an interface number for r
	 *  @return the first link # in the assigned range for this interface,
	 *  or 0 if the interface number is invalid.
	 */
	public int getIfFirstLink(int r, int iface) {
		return (validIf(r,iface) ? rtr[r].iface[iface].firstLink : 0);
	}
	
	/** Get the last link in the range assigned to a given interface.
	 *  Each router interface is assigned a consecutive range of link #s.
	 *  @param r is the node number of a router in the network
	 *  @param iface is an interface number for r
	 *  @return the last link # in the assigned range for this interface,
	 *  or 0 if the interface # is invalid.
	 */
	public int getIfLastLink(int r, int iface) {
		return (validIf(r,iface) ? rtr[r].iface[iface].lastLink : 0);
	}
	
	/** Get the bit rate for a specified router interface.
	 *  @param r is the node number of a router in the network
	 *  @param iface is an interface number for r
	 *  @return the bit rate (in kb/s) assigned to this interface,
	 *  or 0 if the interface number is invalid.
	 */
	public int getIfBitRate(int r, int iface) {
		return (validIf(r,iface) ? rtr[r].iface[iface].bitRate : 0);
	}
	
	/** Get the packet rate for a specified router interface.
	 *  @param r is the node number of a router in the network
	 *  @param iface is an interface number for r
	 *  @return the packet rate (in p/s) assigned to this interface,
	 *  or 0 if the interface number is invalid.
	 */
	public int getIfPktRate(int r, int iface) {
		return (validIf(r,iface) ? rtr[r].iface[iface].pktRate : 0);
	}

	/** Set first address in a router's range of assignable leaf addresses.
	 *  @param r is the node number of the router
	 *  @param adr is the forest address that defines the start of the range
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setFirstLeafAdr(int r, int adr) {
		if (isRouter(r)) rtr[r].firstLeafAdr = adr;
		else return false;
		return true;
	}
	
	/** Set last address in a router's range of assignable client addresses.
	 *  @param r is the node number of the router
	 *  @param adr is the forest address that defines the end of the range
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setLastLeafAdr(int r, int adr) {
		if (isRouter(r)) rtr[r].lastLeafAdr = adr;
		else return false;
		return true;
	}
	
	/** Set the bit rate of a router interface.
	 *  @param r is the node number of the router
	 *  @param iface is the interface number
	 *  @param br is the new bit rate for the interface
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setIfBitRate(int r, int iface, int br) {
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
	public boolean setIfPktRate(int r, int iface, int pr) {
		if (validIf(r,iface)) rtr[r].iface[iface].pktRate = pr;
		else return false;
		return true;
	}
	
	/** Set first link in the range of links defined for a router interface.
	 *  Each router interface is assigned a consecutive range of link #s.
	 *  @param r is the node number of the router
	 *  @param iface is the interface number
	 *  @param lnk is the first link in the range of link numbers
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setIfFirstLink(int r, int iface, int lnk) {
		if (validIf(r,iface)) rtr[r].iface[iface].firstLink = lnk;
		else return false;
		return true;
	}
	
	/** Set last link in the range of links defined for a router interface.
	 *  Each router interface is assigned a consecutive range of link #s.
	 *  @param r is the node number of the router
	 *  @param iface is the interface number
	 *  @param lnk is the last link in the range of link numbers
	 *  @return true if the operation succeeds, else false
	 */
	public boolean setIfLastLink(int r, int iface, int lnk) {
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
	public int getLinkL(int lnk) {
		return (validLink(lnk) ? netTopo.left(lnk) : 0);
	}
	
	/** Get the node number for the "right" endpoint of a given link.
	 *  The endpoints of a link are arbitrarily designated "left" and
	 *  "right".
	 *  @param lnk is a link number
	 *  @return the node number of the right endpoint of lnk, or 0 if lnk
	 *  is not a valid link number.
	 */
	public int getLinkR(int lnk) {
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
	public int getLocLink(int lnk, int r) {
		return (!(validLink(lnk) && isRouter(r)) ? 0 :
			(r == netTopo.left(lnk) ? getLocLinkL(lnk) :
			(r == netTopo.right(lnk) ? getLocLinkR(lnk) : 0)));
	}
	
	/** Get the local link number used by the left endpoint of a link.
	 *  Each router in a Forest network refers to links using local link #s.
	 *  @param lnk is a "global" link number
	 *  @return the local link number used by the left endpoint, or 0 if
	 *  the link number is invalid or the left endpoint is not a router.
	 */
	public int getLocLinkL(int lnk) {
		int r = getLinkL(lnk);
		return (isRouter(r) ? link[lnk].leftLnum : 0);
	}
	
	/** Get the local link number used by the right endpoint of a link.
	 *  Each router in a Forest network refers to links using local link #s.
	 *  @param lnk is a "global" link number
	 *  @return the local link number used by the right endpoint, or 0 if
	 *  the link number is invalid or the left endpoint is not a router.
	 */
	public int getLocLinkR(int lnk) {
		int r = getLinkR(lnk);
		return (isRouter(r) ? link[lnk].rightLnum : 0);
	}
	
	/** Get the bit rate of a link in the Forest network.
	 *  @param lnk is a link number
	 *  @return the bit rate assigned to lnk or 0 if lnk is not a valid
	 *  link number
	 */
	public int getLinkBitRate(int lnk) {
		return (validLink(lnk) ? link[lnk].bitRate : 0);
	}
	
	/** Get the packet rate of a link in the Forest network.
	 *  @param lnk is a link number
	 *  @return the packet rate assigned to lnk or 0 if lnk is not a valid
	 *  link number
	 */
	public int getLinkPktRate(int lnk) {
		return (validLink(lnk) ? link[lnk].pktRate : 0);
	}
	
	/** Get the available bit rate of a link in the Forest network.
	 *  @param lnk is a link number
	 *  @param n is the node number of one of lnk's endpoints
	 *  @return the bit rate assigned to lnk or 0 if lnk is not a valid
	 *  link number
	 */
	public int getAvailBitRate(int lnk, int n) {
		if (!validLink(lnk)) return 0;
		     if (n == getLinkL(lnk)) return link[lnk].availBitRateL;
		else if (n == getLinkR(lnk)) return link[lnk].availBitRateR;
		else return 0;
	}
	
	/** Get the available packet rate of a link in the Forest network.
	 *  @param lnk is a link number
	 *  @param n is the node number for one of lnk's endpoints
	 *  @return the available packet rate leaving node n on lnk
	 */
	public int getAvailPktRate(int lnk, int n) {
		if (!validLink(lnk)) return 0;
		     if (n == getLinkL(lnk)) return link[lnk].availPktRateL;
		else if (n == getLinkR(lnk)) return link[lnk].availPktRateR;
		else return 0;
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
		return (isRouter(r) ? locLnk2lnk.lookup(ll2l_key(r,llnk))/2 :0);
	}

	/** Set the local link number used by the left endpoint of a link.
	 *  Each router in a Forest network refers to links using local link #s.
	 *  @param lnk is a "global" link number
	 *  @param loc is local link # to be used by the left endpoint of lnk
	 *  @return true on success, else false
	 */
	public boolean setLocLinkL(int lnk, int loc) {
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
	public boolean setLocLinkR(int lnk, int loc) {
		if (validLink(lnk)) link[lnk].rightLnum = loc;
		else return false;
		return true;
	}
	
	/** Set the bit rate of a link.
	 *  @param lnk is a "global" link number
	 *  @param br is the new bit rate
	 *  @return true on success, else false
	 */
	public boolean setLinkBitRate(int lnk, int br) {
		if (!validLink(lnk)) return false;
		int dbr = br - link[lnk].bitRate;
		link[lnk].availBitRateL += dbr;
		link[lnk].availBitRateR += dbr;
		link[lnk].bitRate = br;
		return true;
	}
	
	/** Set the packet rate of a link.
	 *  @param lnk is a "global" link number
	 *  @param pr is the new packet rate
	 *  @return true on success, else false
	 */
	public boolean setLinkPktRate(int lnk, int pr) {
		if (!validLink(lnk)) return false;
		int dpr = pr - link[lnk].pktRate;
		link[lnk].availPktRateL += dpr;
		link[lnk].availPktRateR += dpr;
		link[lnk].pktRate = pr;
		return true;
	}
	
	/** Add to the available bit rate for a link.
	 *  @param lnk is a "global" link number
	 *  @param n is the node number for one of lnk's endpoints; the method
	 *  adds to the bit rate leaving n
	 *  @param br is the amount to add to the available bit rate
	 *  leaving node n
	 *  @return true on success, else false
	 */
	public boolean addAvailBitRate(int lnk, int n, int br) {
		if (!validLink(lnk)) return false;
		if (n == getLinkL(lnk)) {
			if (br + link[lnk].availBitRateL < 0) return false;
			link[lnk].availBitRateL += br;
			link[lnk].availBitRateL = Math.min(
				link[lnk].availBitRateL,link[lnk].bitRate);
		} else if (n == getLinkR(lnk)) {
			if (br + link[lnk].availBitRateR < 0) return false;
			link[lnk].availBitRateR += br;
			link[lnk].availBitRateR = Math.min(
				link[lnk].availBitRateR,link[lnk].bitRate);
		} else return false;
		return true;
	}
	
	/** Add to the available packet rate for a link.
	 *  @param lnk is a "global" link number
	 *  @param n is the node number for one of lnk's endpoints
	 *  @param pr is the amount to add to the available packet rate
	 *  leaving node n
	 *  @return true on success, else false
	 */
	public boolean addAvailPktRate(int lnk, int n, int pr) {
		if (!validLink(lnk)) return false;
		if (n == getLinkL(lnk)) {
			if (pr + link[lnk].availPktRateL < 0) return false;
			link[lnk].availPktRateL += pr;
			link[lnk].availPktRateL = Math.min(
				link[lnk].availPktRateL,link[lnk].pktRate);
		} else if (n == getLinkR(lnk)) {
			if (pr + link[lnk].availPktRateR < 0) return false;
			link[lnk].availPktRateR += pr;
			link[lnk].availPktRateR = Math.min(
				link[lnk].availPktRateR,link[lnk].pktRate);
		} else return false;
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
	
	// Methods for working with comtrees ////////////////////////////////
	// Note that methods that take a comtree index as an argument
	// assume that the comtree index is valid. This was a conscious
	// decision to allow the use of NetInfo in contexts where a program
	// needs to define per-comtree locks. Any operation that uses or
	// changes comtreeMap must be protected by a "global" lock.
	// This includes the validComtIndex() method, so if we checked
	// the comtree index in methods that just affect one comtree,
	// client programs would need to acquire a global lock for any
	// of these operations. Hence, we don't check and we trust the client
	// program to never pass an invalid comtree index.
	// Of course, locks are only required in multi-threaded programs.
	
	/** Check to see if a given comtree number is valid.
	 *  @param comt is an integer comtree number
	 *  @return true if comt corresponds to a comtree in the network,
	 *  else false
	 */
	public boolean validComtree(int comt) {
		return comtreeMap.validKey(comt);
	}
	
	/** Check to see if a given comtree index is valid.
	 *  NetInfo uses integer indices in a restricted range to access
	 *  comtree information efficiently. Users get the comtree index
	 *  for a given comtree number using the getComtIndex() method.
	 *  @param ctx is an integer comtree index
	 *  @return true if n corresponds to a comtree in the network,
	 *  else false
	 */
	public boolean validComtIndex(int ctx) {
		return comtreeMap.validId(ctx);
	}
	
	/** Determine if a router is a core node in a specified comtree.
	 *  @param ctx is a valid comtree index
	 *  @param r is the node number of a router
	 *  @return true if r is a core node in the comtree with index i,
	 *  else false
	 */
	public boolean isComtCoreNode(int ctx, int r) {
		return comtree[ctx].coreSet.contains(r);
	}
	
	/** Determine if a node belongs to a comtree or not.
	 *  @param ctx is the comtree index of the comtree of interest
	 *  @param n is number of the node that we want to check for membership
	 *  in the comtree
	 *  @return true if n is a node in the comtree, else false
	 */
	public boolean isComtNode(int ctx, int n) {
		int lnk = firstLinkAt(n);
	        return (isLeaf(n) ? 
			comtree[ctx].lnkMap.get(lnk) != null
			: comtree[ctx].rtrMap.get(n) != null);
	}
	
	/** Determine if a link is in a specified comtree.
	 *  @param ctx is a valid comtree index
	 *  @param lnk is a link number
	 *  @return true if lnk is a link in the comtree with index i,
	 *  else false
	 */
	public boolean isComtLink(int ctx, int lnk) {
		return comtree[ctx].lnkMap.get(lnk) != null;
	}
	
	/** Get the first valid comtree index.
	 *  Used for iterating through all the comtrees.
	 *  @return the index of the first comtree, or 0 if no comtrees defined
	 */
	public int firstComtIndex() {
		return comtreeMap.firstId();
	}
	
	/** Get the next valid comtree index.
	 *  Used for iterating through all the comtrees.
	 *  @param ctx is a comtree index
	 *  @return the index of the next comtree after, or 0 if there is no
	 *  next index
	 */
	public int nextComtIndex(int ctx) {
		return comtreeMap.nextId(ctx);
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

	/** Get the set of core nodes for a given comtree.
	 *  This method is provided to enable a client program to iterate
	 *  over the set of core nodes. The client should not modify the core
	 *  set directly, as this can make the NetInfo object inconsistent.
	 *  @param ctx is a valid comtree index
	 *  @return the node number of the first core node in the comtree,
	 *  or 0 if the comtree index is invalid or no core nodes have
	 *  been defined
	 */
	public Set<Integer> getCoreSet(int ctx) {
		return comtree[ctx].coreSet;
	}
	
	/** Get the comtree link map for a given comtree.
	 *  This method is provided to allow a client to iterate over the
	 *  links in a comtree. It should not be used to modify the set of
	 *  links, as this can make the NetInfo object inconsistent.
	 *  @param ctx is a valid comtree index
	 *  @return the link # of the first link that belongs to the comtree,
	 *  or 0 if the comtree index is invalid or contains no links
	 */
	public Map<Integer,RateSpec> getComtLinks(int ctx) {
		return comtree[ctx].lnkMap;
	}
	
	/** Get the comtree index for a given comtree number.
	 *  @param comt is a comtree number
	 *  @param return the index used to efficiently access stored
	 *  information for the comtree.
	 */
	public int getComtIndex(int comt) {
		return comtreeMap.getId(comt);
	}
	
	/** Get the comtree number for the comtree with a given index.
	 *  @param ctx is a valid comtree index
	 *  @param return the comtree number that is mapped to index c
	 *  or 0 if c is not a valid index
	 */
	public int getComtree(int ctx) {
		return comtree[ctx].comtreeNum;
	}
	
	/** Get the comtree root for the comtree with a given index.
	 *  @param ctx is a valid comtree index
	 *  @param return the specified root node for the comtree
	 *  or 0 if c is not a valid index
	 */
	public int getComtRoot(int ctx) {
		return comtree[ctx].root;
	}
	
	/** Get the owner of a comtree with specified index.
	 *  @param ctx is a valid comtree index
	 *  @param return the forest address of the client that originally
	 *  created the comtree, or 0 if c is not a valid index
	 */
	public int getComtOwner(int ctx) {
		return comtree[ctx].ownerAdr;
	}
	
	/** Get the link to a parent of a given node in the comtree.
	 *  @param ctx is a valid comtree index 
	 *  @param n is # of the node that whose parent link is to be returned
	 *  @return the (global) link # of the link to n's parent, or 0 if n
	 *  has no parent, or -1 if n is not a node the comtree
	 **/
	public int getComtPlink(int ctx, int n) {
		if (!isComtNode(ctx,n)) return -1;
		if (isLeaf(n)) return firstLinkAt(n);
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(n);
		return (rtr != null ? rtr.plnk : -1);
	}
	
	/** Get the number of links in a comtree that are incident to a node.
	 *  @param ctx is a valid comtree index 
	 *  @param n is the number of a node in the comtree
	 *  @return the number of links in the comtree incident to n
	 *  or -1 if n is not a node in the comtree
	 **/
	public int getComtLnkCnt(int ctx, int n) {
		if (!isComtNode(ctx,n)) return -1;
		if (isLeaf(n)) return 1;
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(n);
		return (rtr != null ? rtr.lnkCnt : -1);
	}
	
	/** Get the downstream bit rate for a comtree link.
	 *  @param ctx is a valid comtree index
	 *  @param lnk is an optional link number for a link in the comtree
	 *  @param if the lnk argument is provided and non-zero, return the
	 *  downstream bit rate on lnk, otherwise return the default downstream
	 *  backbone link rate
	 */
	public int getComtBrDown(int ctx, int lnk) {
		if (lnk == 0) return comtree[ctx].bitRateDown;
		RateSpec rs = comtree[ctx].lnkMap.get(lnk);
		return (rs != null ? rs.bitRateDown : 0);
	}
	public int getComtBrDown(int ctx) { return getComtBrDown(ctx,0); }
	
	/** Get the upstream bit rate for a comtree link.
	 *  @param ctx is a comtree index
	 *  @param lnk is an optional link number for a link in the comtree
	 *  @param if the lnk argument is provided and non-zero, return the
	 *  upstream bit rate on lnk, otherwise return the default upstream
	 *  backbone link rate
	 */
	public int getComtBrUp(int ctx, int lnk) {
		if (lnk == 0) return comtree[ctx].bitRateUp;
		RateSpec rs = comtree[ctx].lnkMap.get(lnk);
		return (rs != null ? rs.bitRateUp : 0);
	}
	public int getComtBrUp(int ctx) { return getComtBrUp(ctx,0); }
	
	/** Get the downstream backbone packet rate for a comtree link.
	 *  @param ctx is a valid comtree index
	 *  @param lnk is an optional link number for a link in the comtree
	 *  @param if the lnk argument is provided and non-zero, return the
	 *  downstream packet rate on lnk, otherwise return default downstream
	 *  backbone link rate
	 */
	public int getComtPrDown(int ctx, int lnk) {
		if (lnk == 0) return comtree[ctx].pktRateDown;
		RateSpec rs = comtree[ctx].lnkMap.get(lnk);
		return (rs != null ? rs.pktRateDown : 0);
	}
	public int getComtPrDown(int ctx) { return getComtPrDown(ctx,0); }
	
	/** Get the upstream backbone packet rate for a comtree link.
	 *  @param ctx is a comtree index
	 *  @param lnk is an optional link # for a backbone link in the comtree
	 *  @param if the lnk argument is provided and non-zero, return the
	 *  upstream packet rate on lnk, otherwise return the default upstream
	 *  backbone link rate
	 */
	public int getComtPrUp(int ctx, int lnk) {
		if (lnk == 0) return comtree[ctx].pktRateUp;
		RateSpec rs = comtree[ctx].lnkMap.get(lnk);
		return (rs != null ? rs.pktRateUp : 0);
	}
	public int getComtPrUp(int ctx) { return getComtPrUp(ctx,0); }
	
	/** Get the default downstream access bit rate for a comtree.
	 *  @param ctx is a valid comtree index
	 *  @param return the default downstream bit rate for access links in
	 *  the comtree
	 */
	public int getComtLeafBrDown(int ctx) {
		return comtree[ctx].leafBitRateDown;
	}
	
	/** Get the default upstream access bit rate for a comtree.
	 *  @param ctx is a valid comtree index
	 *  @param return the default upstream bit rate for access links in
	 *  the comtree
	 */
	public int getComtLeafBrUp(int ctx) {
		return comtree[ctx].leafBitRateUp;
	}
	
	/** Get the default downstream access packet rate for a comtree.
	 *  @param ctx is a valid comtree index
	 *  @param return the default downstream packet rate for access links in
	 *  the comtree
	 */
	public int getComtLeafPrDown(int ctx) {
		return comtree[ctx].leafPktRateDown;
	}
	
	/** Get the default upstream access packet rate for a comtree.
	 *  @param ctx is a comtree index
	 *  @param return the default upstream packet rate for access links in
	 *  the comtree
	 */
	public int getComtLeafPrUp(int ctx) {
		return comtree[ctx].leafPktRateUp;
	}
	
	/** Add a new comtree.
	 *  Defines a new comtree, with attributes left undefined.
	 *  @param nuComt is a ComtreeInfo object for the new comtree
	 *  @return true on success, false on failure
	 */
	public boolean addComtree(ComtreeInfo nuComt) {
		int ctx = comtreeMap.addPair(nuComt.comtreeNum);
		if (ctx == 0) return false;
		comtree[ctx] = nuComt;
		return true;
	}
	
	/** Update a comtree.
	 *  Replaces the comtree information for an existing comtree.
	 *  @param nuComt is a ComtreeInfo object
	 *  @return true on success, false on failure
	 */
	public boolean updateComtree(ComtreeInfo nuComt) {
		int ctx = comtreeMap.getId(nuComt.comtreeNum);
		if (ctx == 0) return false;
		comtree[ctx] = nuComt;
		return true;
	}
	
	/** Remove a comtree.
	 *  Defines a new comtree, with attributes left undefined.
	 *  @param comt is the comtree number for the new comtree.
	 *  @return true on success, false on failure
	 */
	public boolean removeComtree(int ctx) {
		if (!validComtIndex(ctx)) return false;
		comtreeMap.dropPair(comtree[ctx].comtreeNum);
		comtree[ctx].comtreeNum = 0;
		comtree[ctx].coreSet = null;
		comtree[ctx].lnkMap = null;
		comtree[ctx].rtrMap = null;
		return true;
	}
	
	/** Add a new router to a comtree.
	 *  If the specified node is a leaf, we do nothing and return false,
	 *  as leaf nodes are not explictly represented in the data class
	 *  for comtrees.
	 *  @param ctx is the comtree index
	 *  @param r is the node number of the router
	 *  @return true on success, false on failure
	 */
	public boolean addComtNode(int ctx, int r) {
		if (!isRouter(r)) return false;
		if (comtree[ctx].rtrMap.get(r) == null) {
			ComtRtrInfo rtr = new ComtRtrInfo();
			rtr.lnkCnt = 0; rtr.plnk = 0;
			comtree[ctx].rtrMap.put(r,rtr);
		}
		return true;
	}
	
	/** Add a new router to a ComtreeInfo object.
	 *  If the specified node is a leaf, we do nothing and return false,
	 *  as leaf nodes are not explictly represented in the data class
	 *  for comtrees.
	 *  @param cti is a ComtreeInfo object
	 *  @param r is the node number of the router
	 *  @return true on success, false on failure
	 */
	public boolean addComtNode(ComtreeInfo cti, int r) {
		if (!isRouter(r)) return false;
		if (cti.rtrMap.get(r) == null) {
			ComtRtrInfo rtr = new ComtRtrInfo();
			rtr.lnkCnt = 0; rtr.plnk = 0;
			cti.rtrMap.put(r,rtr);
		}
		return true;
	}
	
	/** Remove a router from a comtree.
	 *  This method will fail if the stored link count for this router
	 *  in the comtree is non-zero.
	 *  @param ctx is the comtree index
	 *  @param r is the node number of the router
	 *  @return true on success, false on failure
	 */
	public boolean removeComtNode(int ctx, int r) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(r);
		if (rtr == null) return true;
		if (rtr.lnkCnt != 0) return false;
		comtree[ctx].rtrMap.remove(r);
		comtree[ctx].coreSet.remove(r);
		return true;
	}
	
	/** Add a new core node to a comtree.
	 *  The new node is required to be a router and if it is
	 *  not already in the comtree, it is added.
	 *  @param ctx is the comtree index
	 *  @param r is the node number of the router
	 *  @return true on success, false on failure
	 */
	public boolean addComtCoreNode(int ctx, int r) {
		if (!isRouter(r)) return false;
		if (!isComtNode(ctx,r)) addComtNode(ctx,r);
		comtree[ctx].coreSet.add(r);
		return true;
	}
	
	/** Remove a core node from a comtree.
	 *  @param ctx is the comtree index
	 *  @param r is the node number of the core node
	 *  @return true on success, false on failure
	 */
	public boolean removeComtCoreNode(int ctx, int n) {
		comtree[ctx].coreSet.remove(n);
		return true;
	}
	
	/** Set the link count for a router in a comtree.
	 *  This method is provided to allow link counts to be used to
	 *  track the presence of leaf nodes that may not be represented
	 *  explicitly in the comtree. The client program must take care
	 *  to maintain these counts appropriately.
	 *  @param ctx is a valid comtree index
	 *  @param r is a router in the comtree
	 *  @param cnt is the new value for the link count
	 *  @return true on success, false on failure
	 */
	public boolean setComtLnkCnt(int ctx, int r, int cnt) {
		if (isLeaf(r)) return true;
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(r);
		if (rtr == null) return false;
		rtr.lnkCnt = cnt;
		return true;
	}
	
	/** Increment the link count for a router in a comtree.
	 *  This method is provided to allow link counts to be used to
	 *  track the presence of leaf nodes that may not be represented
	 *  explicitly in the comtree. The client program must take care
	 *  to maintain these counts appropriately.
	 *  @param ctx is a valid comtree index
	 *  @param r is a router in the comtree
	 *  @return true on success, false on failure
	 */
	public boolean incComtLnkCnt(int ctx, int r) {
		if (isLeaf(r)) return true;
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(r);
		if (rtr == null) return false;
		rtr.lnkCnt += 1;
		return true;
	}
	
	/** Decrement the link count for a router in the comtree.
	 *  This method is provided to allow link counts to be used to
	 *  track the presence of leaf nodes that may not be represented
	 *  explicitly in the comtree. The client program must take care
	 *  to maintain these counts appropriately.
	 *  @param ctx is a valid comtree index
	 *  @param r is a router in the comtree
	 *  @return true on success, false on failure
	 */
	public boolean decComtLnkCnt(int ctx, int r) {
		if (isLeaf(r)) return true;
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(r);
		if (rtr == null) return false;
		rtr.lnkCnt += 1;
		return true;
	}
	
	/** Set the root node of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param r is the router that is to be the comtree root
	 *  @return true on success, false on failure
	 */
	public boolean setComtRoot(int ctx, int r) {
		if (!isRouter(r)) return false;
		comtree[ctx].root = r;
		return true;
	}
	
	/** Set the owner of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param owner is the forest address of the new comtree owner
	 *  @return true on success, false on failure
	 */
	public boolean setComtOwner(int ctx, int owner) {
		if (!Forest.validUcastAdr(owner)) return false;
		comtree[ctx].ownerAdr = owner;
		return true;
	}
	
	/** Set the parent link of a node in the comtree.
	 *  @param ctx is a valid comtree index
	 *  @param n is a node in the comtree
	 *  @return true on success, false on failure
	 */
	public boolean setComtPlink(int ctx, int n, int plnk) {
		if (isLeaf(n)) return true; // for leaf, plnk defined implicitly
		ComtRtrInfo rtr= comtree[ctx].rtrMap.get(n);
		if (rtr == null) return false;
		rtr.plnk = plnk;
		return true;
	}
	
	/** Set the downstream bit rate for the backbone links of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param lnk is an optional link # for a backbone link in the comtree
	 *  @param if the lnk argument is provided and non-zero, set the
	 *  downstream bit rate on lnk, otherwise set the default
	 *  downstream backbone link rate
	 */
	public boolean setComtBrDown(int ctx, int br, int lnk) {
		if (lnk == 0) { comtree[ctx].bitRateDown = br; return true; }
		RateSpec rs = comtree[ctx].lnkMap.get(lnk);
		if (rs == null) return false;
		rs.bitRateDown = br;
		return true;
	}
	
	/** Set the upstream bit rate for the backbone links of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param lnk is an optional link # for a backbone link in the comtree
	 *  @param if the lnk argument is provided and non-zero, set the
	 *  upstream bit rate on lnk, otherwise set the default
	 *  upstream backbone link rate
	 */
	public boolean setComtBrUp(int ctx, int br, int lnk) {
		if (lnk == 0) { comtree[ctx].bitRateUp = br; return true; }
		RateSpec rs = comtree[ctx].lnkMap.get(lnk);
		if (rs == null) return false;
		rs.bitRateUp = br;
		return true;
	}
	
	/** Set the downstream packet rate for the backbone links of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param lnk is a link number for a backbone link in the comtree
	 *  @param if the lnk argument is non-zero, set the
	 *  downstream packet rate on lnk, otherwise set the default
	 *  downstream backbone link rate
	 */
	public boolean setComtPrDown(int ctx, int pr, int lnk) {
		if (lnk == 0) { comtree[ctx].pktRateDown = pr; return true; }
		RateSpec rs = comtree[ctx].lnkMap.get(lnk);
		if (rs == null) return false;
		rs.pktRateDown = pr;
		return true;
	}
	
	/** Set the upstream packet rate for the backbone links of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param lnk is a link # for a backbone link in the comtree
	 *  @param if the lnk argument is non-zero, set the
	 *  upstream packet rate on lnk, otherwise set the default
	 *  upstream backbone link rate
	 */
	public boolean setComtPrUp(int ctx, int pr, int lnk) {
		if (lnk == 0) { comtree[ctx].pktRateUp = pr; return true; }
		RateSpec rs = comtree[ctx].lnkMap.get(lnk);
		if (rs == null) return false;
		rs.pktRateUp = pr;
		return true;
	}
	
	/** Set default downstream bit rate for the access links of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param br is the new default downstream bit rate for access links
	 *  @return true on success, false on failure
	 */
	public boolean setComtLeafBrDown(int ctx, int br) {
		comtree[ctx].leafBitRateDown = br;
		return true;
	}
	
	/** Set the default upstream bit rate for the access links of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param br is the new default upstream bit rate for access links
	 *  @return true on success, false on failure
	 */
	public boolean setComtLeafBrUp(int ctx, int br) {
		comtree[ctx].leafBitRateUp = br;
		return true;
	}
	
	/** Set default downstream packet rate for access links of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param br is the new default downstream packet rate for access links
	 *  @return true on success, false on failure
	 */
	public boolean setComtLeafPrDown(int ctx, int pr) {
		comtree[ctx].leafPktRateDown = pr;
		return true;
	}
	
	/** Set default upstream packet rate for the access links of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param br is the new default upstream packet rate for access links
	 *  @return true on success, false on failure
	 */
	public boolean setComtLeafPrUp(int ctx, int pr) {
		comtree[ctx].leafPktRateUp = pr;
		return true;
	}
	
	/** Helper method defines keys for internal locLnk2lnk hash table.
	 *  @param r is the node number of a router
	 *  @param llnk is a local link number at r
	 *  @return a hash key appropriate for use with locLnk2lnk
	 */
	public long ll2l_key(int r, int llnk) {
		return (((long) r) << 32) | (((long) llnk) & 0xffffffff);
	}
	
	/** Add a new link to a comtree.
	 *  @param ctx is the comtree index
	 *  @param lnk is the link number of the link to be added
	 *  @param parent is the parent endpoint of lnk
	 *  @return true on success, false on failure
	 */
	public boolean addComtLink(int ctx, int lnk, int parent) {
		if (!validLink(lnk)) return false;
		RateSpec rs = new RateSpec();
		rs.bitRateUp = rs.bitRateDown = 0;
		rs.pktRateUp = rs.pktRateDown = 0;
		comtree[ctx].lnkMap.put(lnk,rs);
	
		int child = getPeer(parent,lnk);
		addComtNode(ctx,child); addComtNode(ctx,parent);
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(child);
		rtr.plnk = lnk; rtr.lnkCnt += 1;
		rtr = comtree[ctx].rtrMap.get(parent);
		rtr.lnkCnt += 1;
	
		return true;
	}
	
	/** Remove a link from a comtree.
	 *  @param ctx is the comtree index
	 *  @param lnk is the link number of the link to be removed
	 *  @return true on success, false on failure
	 */
	boolean removeComtLink(int ctx, int lnk) {
		if (!validLink(lnk)) return false;
		RateSpec rs = comtree[ctx].lnkMap.get(lnk);
		if (rs == null) return true;
		comtree[ctx].lnkMap.remove(lnk);
	
		int left = getLinkL(lnk); int right = getLinkR(lnk);
		ComtRtrInfo cr = comtree[ctx].rtrMap.get(left);
		if (cr != null) {
			cr.lnkCnt -= 1;
			if (cr.lnkCnt <= 0 && left != getComtRoot(ctx))
				removeComtNode(ctx,left);
		}
		cr = comtree[ctx].rtrMap.get(right);
		if (cr != null) {
			cr.lnkCnt -= 1;
			if (cr.lnkCnt <= 0 && right != getComtRoot(ctx))
				removeComtNode(ctx,right);
		}
		return true;
	}
	
	/** Perform a series of consistency checks.
	 *  Print error message for each detected problem.
	 *  @return true if all checks passed, else false
	 */
	public boolean check() {
		boolean status = true;
	
		// make sure there is at least one router
		if (getNumRouters() == 0 || firstRouter() == 0) {
			System.err.println("check: no routers in network, "
					    + "terminating");
			return false;
		}
		// make sure that no two links at a router have the
		// the same local link number
		for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
			for (int l1 = firstLinkAt(r); l1 != 0;
				 l1 = nextLinkAt(r,l1)) {
				for (int l2 = nextLinkAt(r,l1); l2 != 0;
					 l2 = nextLinkAt(r,l2)) {
					if (getLocLink(l1,r)
					     == getLocLink(l2,r)) {
						System.err.println("check: "
						 + "detected two links with "
						 + "same local link number: "
						 + link2string(l1) + " and "
						 + link2string(l2));
						status = false;
					}
				}
			}
		}
	
		// make sure that routers are all connected, by doing
		// a breadth-first search from firstRouter()
		Set<Integer> seen = new HashSet<Integer>();
		seen.add(firstRouter());
		LinkedList<Integer> pending = new LinkedList<Integer>();
		pending.add(firstRouter());
		Integer u;
		while ((u = pending.peekFirst()) != null) {
			pending.removeFirst();
			for (int lnk = firstLinkAt(u); lnk != 0;
				 lnk = nextLinkAt(u,lnk)) {	
				int v = getPeer(u,lnk);
				if (getNodeType(v) != Forest.NodeTyp.ROUTER)
					continue;
				if (seen.contains(v)) continue;
				seen.add(v);
				pending.addLast(v);
			}
		}
		if (seen.size() != getNumRouters()) {
			System.err.println("check: network is not connected");
			status = false;
		}
	
		// check that no two nodes have the same address
		for (int n1 = firstNode(); n1 != 0; n1 = nextNode(n1)) {
			for (int n2 = nextNode(n1); n2!= 0; n2 = nextNode(n2)) {
				if (getNodeAdr(n1) == getNodeAdr(n2)) {
					System.err.println("check: detected two"
						+ " nodes " + getNodeName(n1)
						+ " and " + getNodeName(n2) 
						+ " with the same forest "
						+ "address");
				status = false;
				}
			}
		}
	
		// check that the leaf address range for a router
		// is compatible with the router's address
		for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
			int rzip = Forest.zipCode(getNodeAdr(r));
			int flzip = Forest.zipCode(getFirstLeafAdr(r));
			int llzip = Forest.zipCode(getLastLeafAdr(r));
			if (rzip != flzip || rzip != llzip) {
				System.err.println( "netInfo.check: detected "
					+ "router " + r + "with incompatible "
					+ "address and leaf address "
					+ "range");
				status = false;
			}
			if (getFirstLeafAdr(r) > getLastLeafAdr(r)) {
				System.err.println( "netInfo.check: detected "						+ "router " + r + " with empty leaf "
					+ "address range");
				status = false;
			}
		}
	
		// make sure that no two routers have overlapping leaf
		// address ranges
		for (int r1 = firstRouter(); r1 != 0; r1 = nextRouter(r1)) {
			int fla1 = getFirstLeafAdr(r1);
			int lla1 = getLastLeafAdr(r1);
			for (int r2 = nextRouter(r1); r2 != 0;
				 r2 = nextRouter(r2)) {
				int fla2 = getFirstLeafAdr(r2);
				int lla2 = getLastLeafAdr(r2);
				if (fla2 <= lla1 && lla2 >= fla1) {
					System.err.println( "netInfo.check: "
						+ "detected two routers " + r1
						+ " and " + r2 + " with "
						+ "overlapping address ranges");
					status = false;
				}
			}
		}
	
		// check that all leaf nodes have a single link that connects
		// to a router and that their address is in their router's range
		for (int v = firstLeaf(); v != 0; v = nextLeaf(v)) {
			int lnk = firstLinkAt(v);
			if (lnk == 0) {
				System.err.println( "check: detected a leaf "
					+ " node " + getNodeName(v)
					+ " with no links");
				status = false; continue;
			}
			if (nextLinkAt(v,lnk) != 0) {
				System.err.println( "check: detected a leaf "
					+ "node " + getNodeName(v)
					+ " with more than one link");
				status = false; continue;
			}
			if (getNodeType(getPeer(v,lnk)) !=	
						Forest.NodeTyp.ROUTER) {
				System.err.println( "check: detected a leaf "
					+ "node " + getNodeName(v)
					+ " with link to non-router");
				status = false; continue;
			}
			int rtr = getPeer(v,lnk);
			int adr = getNodeAdr(v);
			if (adr < getFirstLeafAdr(rtr) ||
			    adr > getLastLeafAdr(rtr)) {
				System.err.println( "check: detected a leaf "
					+ "node " + getNodeName(v) 
					+ " with an address outside the leaf "
					+ "address range of its router");
				status = false; continue;
			}
		}
	
		// check that link rates are within bounds
		for (int lnk = firstLink(); lnk != 0; lnk = nextLink(lnk)) {
			int br = getLinkBitRate(lnk);
			if (br < Forest.MINBITRATE ||
			    br > Forest.MAXBITRATE) {
				System.err.println( "check: detected a link "
				     	+ link2string(lnk) + " with bit rate "
					+ "outside the allowed range");
				status = false;
			}
			int pr = getLinkPktRate(lnk);
			if (pr < Forest.MINPKTRATE ||
			    pr > Forest.MAXPKTRATE) {
				System.err.println( "check: detected a link "
				     	+ link2string(lnk) + " with packet "
					+ "rate outside the allowed range");
				status = false;
			}
		}
	
		// check that routers' local link numbers fall within range of
		// some valid interface
		for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
			for (int lnk = firstLinkAt(r); lnk != 0;
				 lnk = nextLinkAt(r,lnk)) {
				int llnk = getLocLink(lnk,r);
				if (getIface(llnk,r) == 0) {
					System.err.println( "check: link "
						+ llnk + " at router " 
						+ getNodeName(r) + " is not "
						+ "in the range assigned "
						+ "to any valid interface");
					status = false;
				}
			}
		}
	
		// check that router interface rates are within bounds
		for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
			for (int i = 1; i <= getNumIf(r); i++) {
				if (!validIf(r,i)) continue;
				int br = getIfBitRate(r,i);
				if (br < Forest.MINBITRATE ||
				    br >Forest.MAXBITRATE) {
					System.err.println( "check: interface "
						+ i + "at router " + r
						+ " has bit rate outside the "
						+ "allowed range");
					status = false;
				}
				int pr = getIfPktRate(r,i);
				if (pr < Forest.MINPKTRATE ||
				    pr >Forest.MAXPKTRATE) {
					System.err.println("check: interface "
						+ i + "at router " + r
						+ " has packet rate outside "
						+ "the allowed range");
					status = false;
				}
			}
		}
	
		// verify that link rates at any router don't add up to more
		// than the interface rates
		for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
			int[] ifbr = new int[getNumIf(r)+1];
			int[] ifpr = new int[getNumIf(r)+1];
			for (int i = 0; i <= getNumIf(r); i++) {
				ifbr[i] = 0; ifpr[i] = 0;
			}
			for (int lnk = firstLinkAt(r); lnk != 0;
				 lnk = nextLinkAt(r,lnk)) {
				int llnk = getLocLink(lnk,r);
				int iface = getIface(llnk,r);
				ifbr[iface] += getLinkBitRate(lnk);
				ifpr[iface] += getLinkPktRate(lnk);
			}
			for (int i = 0; i <= getNumIf(r); i++) {
				if (!validIf(r,i)) continue;
				if (ifbr[i] > getIfBitRate(r,i)) {
					System.err.println("check: links at "
						+ "interface " + i + " of "
						+ "router " + r + " have total "
						+ "bit rate that exceeds "
						+ "interface bit rate");
				}
				if (ifpr[i] > getIfPktRate(r,i)) {
					System.err.println("check: links at "
						+ "interface " + i  + " of "
						+ "router " + r + " have total "
						+ "packet rate that exceeds "
						+ "interface packet rate");
				}
			}
		}
	
		// check that comtrees are in fact trees and that they satisfy
		// various other requirements
		for (int ctx = firstComtIndex(); ctx != 0;
			 ctx = nextComtIndex(ctx)) {
			int comt = getComtree(ctx);
			// first count nodes and links
			Set<Integer> nodes = new HashSet<Integer>();
			int lnkCnt = 0;
			Iterator<Integer> clp =
				getComtLinks(ctx).keySet().iterator();
			while (clp.hasNext()) {
				Integer lnk = clp.next();
				nodes.add(getLinkL(lnk));
				nodes.add(getLinkR(lnk));
				lnkCnt++;
			}
			if (lnkCnt != nodes.size() - 1) {
				System.err.println( "check: links in comtree "
				     + comt + " do not form a tree");
				status = false; continue;
			}
			// check that root and core nodes are in set we've seen
			int root = getComtRoot(ctx);
			if (!nodes.contains(root)) {
				System.err.println("check: specified comtree "
					+ "root for comtree " + comt + " does "
					+ "not appear in any comtree link");
				status = false; continue;
			}
			boolean seenRoot = false;
			Iterator<Integer> csp = getCoreSet(ctx).iterator();
			while (csp.hasNext()) {
				Integer r = csp.next();
				if (r == root) seenRoot = true;
				if (!nodes.contains(r)) {
					System.err.println("check: core node "
					     	+ getNodeName(r) + " for "
						+ "comtree " + comt + " does "
						+ "not appear in any comtree "
						+ "link");
					status = false;
				}
			}
			if (!seenRoot) {
				System.err.println("check: root node does not "
					+ "appear among the core nodes for "
					+ "comtree " + comt);
				status = false;
			}
	
			// now, check that the comtree topology is really a tree
			// by doing a breadth-first search from the root;
			// while we're at it, make sure the parent of every
			// core node is a core node and that the zip codes
			// of routers within the comtree are contiguous
			pending.clear(); pending.addLast(root);
			Map<Integer,Integer> plink;
			plink = new HashMap<Integer,Integer>();
			plink.put(root,0);
			Map<Integer,Integer> pzip;
			pzip = new HashMap<Integer,Integer>();
			pzip.put(Forest.zipCode(getNodeAdr(root)),0);
			while ((u = pending.peekFirst()) != null) {
				pending.removeFirst();
				boolean foundCycle = false;
				int uzip = Forest.zipCode(getNodeAdr(u));
				for (int lnk = firstLinkAt(u); lnk != 0;
					 lnk = nextLinkAt(u,lnk)) {
					if (!isComtLink(ctx,lnk)) continue;
					if (lnk == plink.get(u)) continue;
					int v = getPeer(u,lnk);
					int vzip =Forest.zipCode(getNodeAdr(v));
					if (plink.get(v) != null) {
						System.err.println("check: "
							+ "comtree " + comt
							+ " contains a cycle");
						foundCycle = true;
						break;
					}
					plink.put(v,lnk);
					pending.addLast(v);
					// now check that if v is in core,
					// so is u
					if (isComtCoreNode(ctx,v) &&
					    !isComtCoreNode(ctx,u)) {
						System.err.println( "check: "
							+ "comtree " + comt 
							+ " contains a core "
							+ "node "
						     	+ getNodeName(v)
							+ " whose parent is "
							+ "not a core node");
						status = false;
					}
					// now check that if v has a different
					// zip code than u, that we haven't 
					// already seen this zip
					if (vzip != uzip) {
						if (pzip.get(v) != null) {
							System.err.println(
							      "check: zip code "
								+ vzip + " is "
								+ "non-contigu"
								+ "ous in "
							      	+ "comtree "
								+ comt);
							status = false;
						} else {
							pzip.put(v,u);
						}
					}
				}
				if (foundCycle) { status = false; break; }
			}
		}
		return status;
	}
	
	/** Initialize auxiliary data class for fast access to comtree 
	 *  information. Set comtree link rates to default values and 
	 *  check that total comtree rates don't exceed link rates.
	 *  Also set the plnk and lnkCnt fields in the rtrMap for each comtree;
	 *  note that we assume that the lnkCnt fields have been initialized
	 *  to zero.
	 */
	public boolean setComtLnkNodeInfo() {
		boolean status = true;
		for (int ctx = firstComtIndex(); ctx != 0;
			 ctx = nextComtIndex(ctx)) {
			// do breadth-first search over the comtree links
			int comt = getComtree(ctx); int root = getComtRoot(ctx);
			LinkedList<Integer> pending = new LinkedList<Integer>();
			pending.addLast(root);
			Map<Integer,Integer> plink =
				new HashMap<Integer,Integer>();
			setComtPlink(ctx,root,0);
			plink.put(root,0);
			Integer u;
			while ((u = pending.peekFirst()) != null) {
				pending.removeFirst();
				for (int lnk = firstLinkAt(u); lnk != 0;
					 lnk = nextLinkAt(u,lnk)) {
					if (!isComtLink(ctx,lnk)) continue;
					incComtLnkCnt(ctx,u);
					if (lnk == plink.get(u)) continue;
					int v = getPeer(u,lnk);
					setComtPlink(ctx,v,lnk);
					plink.put(v,lnk);
					pending.addLast(v);
					if (!setLinkRates(ctx,lnk,v)) {
						System.err.println(
						    "setComtLnkNodeInfo: could "
						    + "not set comtree link "
						    + "rates as specified for "
						    + "comtree " + comt
						    + " lnk " + lnk);
						status = false;
					}
				}
			}
			plink.clear();
		}
		return status;
	}
	
	/** Set the rates on a specific comtree link and adjust available rates.
	 *  @param ctx is a valid comtree index
	 *  @param lnk is a link in the comtree
	 *  @param child is the "child endpoint" of lnk, in the comtree
	 *  @return true on success, false on failure;
	 */
	public boolean setLinkRates(int ctx, int lnk, int child) {
		// first set the rates on the comtree links
		if (isLeaf(child)) {
			if (!setComtBrDown(ctx,getComtLeafBrDown(ctx),lnk) ||
			    !setComtBrUp(ctx,getComtLeafBrUp(ctx),lnk) ||
			    !setComtPrDown(ctx,getComtLeafPrDown(ctx),lnk) ||
			    !setComtPrUp(ctx,getComtLeafPrUp(ctx),lnk))
				return false;
		} else {
			if (!setComtBrDown(ctx,getComtBrDown(ctx),lnk) ||
			    !setComtBrUp(ctx,getComtBrUp(ctx),lnk) ||
			    !setComtPrDown(ctx,getComtPrDown(ctx),lnk) ||
			    !setComtPrUp(ctx,getComtPrUp(ctx),lnk))
				return false;
		}
		// next, adjust the available rates on the network links
		int brl, prl, brr, prr;
		if (child == getLinkL(lnk)) {
			brl = getComtBrUp(ctx,lnk);  
			prl = getComtPrUp(ctx,lnk);
			brr = getComtBrDown(ctx,lnk);
			prr = getComtPrDown(ctx,lnk);
		} else {
			brl = getComtBrDown(ctx,lnk);
			prl = getComtPrDown(ctx,lnk);
			brr = getComtBrUp(ctx,lnk);  
			prr = getComtPrUp(ctx,lnk);
		}
		int left = getLinkL(lnk); int right = getLinkR(lnk);
		if (!addAvailBitRate(lnk,left, -brl)) return false;
		if (!addAvailPktRate(lnk,left, -prl)) return false;
		if (!addAvailBitRate(lnk,right,-brr)) return false;
		if (!addAvailPktRate(lnk,right,-prr)) return false;
		return true;
	}
	
	/** Get the interface associated with a given local link number.
	 *  @param llnk is a local link number
	 *  @param rtr is a router
	 *  @return the number of the interface that hosts llnk
	 */
	public int getIface(int llnk, int rtr) {
		for (int i = 1; i <= getNumIf(rtr); i++) {
			if (validIf(rtr,i) && llnk >= getIfFirstLink(rtr,i)
					   && llnk <= getIfLastLink(rtr,i))
				return i;
		}
		return 0;
	}
	
	/** Add a new router to the NetInfo object.
	 *  @param nuRtr is an appropriately initialized RtrNodeInfo object.
	 *  @return the node number of the new router, or 0 if there
	 *  are no more available router numbers, or if nuRtr's name is
	 *  already used by some other node.
	 */
	public int addRouter(RtrNodeInfo nuRtr) {
		int r = routers.firstOut();
		if (r == 0) return 0;
		if (nameNodeMap.get(nuRtr.name) != null) return 0;
		routers.swap(r);
		nameNodeMap.put(nuRtr.name,r);
		rtr[r] = nuRtr;
		return r;
	}
	
	/** Add a leaf node to a Forest network
	 *  @param nuLeaf is an appropriately initialized LeafNodeInfo object
	 *  which is to be added to the NetInfo object
	 *  @return the node number for the new leaf or 0 on failure
	 *  (the method fails if there are no available leaf records to
	 *  allocate to this leaf, or if the requested name is in use)
	 */
	public int addLeaf(LeafNodeInfo nuLeaf) {
		int ln = leaves.firstOut();
		if (ln == 0) return 0;
		if (nameNodeMap.get(nuLeaf.name) != null) return 0;
		leaves.swap(ln);
		int nodeNum = ln + maxRtr;
		leaf[ln] = nuLeaf;
		nameNodeMap.put(nuLeaf.name,nodeNum);
		if (nuLeaf.nType == Forest.NodeTyp.CONTROLLER)
			controllers.add(ln);
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
		if (getNodeType(u) == Forest.NodeTyp.ROUTER) {
			if (!locLnk2lnk.insert(ll2l_key(u,uln),2*lnk)) {
				netTopo.remove(lnk); return 0;
			}
			setLocLinkL(lnk,uln);
		}
		if (getNodeType(v) == Forest.NodeTyp.ROUTER) {
			if (!locLnk2lnk.insert(ll2l_key(v,vln),2*lnk+1)) {
				locLnk2lnk.remove(ll2l_key(u,uln));
				netTopo.remove(lnk); return 0;
			}
			setLocLinkR(lnk,vln);
		}
		link[lnk].bitRate = Forest.MINBITRATE;
		link[lnk].pktRate = Forest.MINPKTRATE;
		link[lnk].availBitRateL = Forest.MINBITRATE;
		link[lnk].availBitRateR = Forest.MINBITRATE;
		link[lnk].availPktRateL = Forest.MINPKTRATE;
		link[lnk].availPktRateR = Forest.MINPKTRATE;
		return lnk;
	}
	
	public int addLink(int u, int v, int lnkLen, LinkInfo nuLnk) {
		int lnk = netTopo.join(u,v);
		netTopo.setWeight(lnk,lnkLen);
		if (lnk == 0) return 0;
		int uln = nuLnk.leftLnum;
		if (getNodeType(u) == Forest.NodeTyp.ROUTER) {
			if (!locLnk2lnk.insert(ll2l_key(u,uln),2*lnk)) {
				netTopo.remove(lnk); return 0;
			}
		}
		int vln = nuLnk.rightLnum;
		if (getNodeType(v) == Forest.NodeTyp.ROUTER) {
			if (!locLnk2lnk.insert(ll2l_key(v,vln),2*lnk+1)) {
				locLnk2lnk.remove(ll2l_key(u,uln));
				netTopo.remove(lnk); return 0;
			}
		}
		link[lnk] = nuLnk;

		return lnk;
	}
	
	/* Example of NetInfo input file format.
	 *  
	 *  Routers # starts section that defines routers
	 *  
	 *  # define salt router
	 *  name=salt type=router ipAdr=2.3.4.5 fAdr=1.1000
	 *  location=(40,-50) leafAdrRange=(1.1-1.200)
	 *  
	 *  interfaces # router interfaces
	 *  # ifaceNum ifaceIp      ifaceLinks  bitRate  pktRate
	 *       1     193.168.3.4     1	50000    25000 ;
	 *       2     193.168.3.5     2-30	40000    20000 ;
	 *  end
	 *  ;
	 *  
	 *  # define kans router
	 *  name=kans type=router ipAdr=6.3.4.5 fAdr=2.1000
	 *  location=(40,-40) leafAdrRange=(2.1-2.200)
	 *  
	 *  interfaces
	 *  # ifaceNum ifaceIp      ifaceLinks  bitRate  pktRate
	 *       1     193.168.5.6  1	   50000    25000 ;
	 *       2     193.168.5.7  2-30	40000    20000 ;
	 *  end
	 *  ;
	 *  ;
	 *  
	 *  LeafNodes # starts section defining other leaf nodes
	 *  
	 *  name=netMgr type=controller ipAdr=192.168.8.2 fAdr=2.900
	 *  location=(42,-50);
	 *  name=comtCtl type=controller ipAdr=192.168.7.5 fAdr=1.902
	 *  location=(42,-40);
	 *  ;
	 *  
	 *  Links # starts section that defines links
	 *  
	 *  # name.link#:name.link# packetRate bitRate
	 *  link=(salt.1,kans.1) bitRate=20000 pktRate=40000 ;
	 *  link=(netMgr,kans.2) bitRate=3000 pktRate=5000 ;
	 *  link=(comtCtl,salt.2) bitRate=3000 pktRate=5000 ;
	 *  ;
	 *  
	 *  Comtrees # starts section that defines comtrees
	 *  
	 *  comtree=1001 root=salt core=kans # may have >1 core=x assignments
	 *  bitRateUp=5000 bitRateDown=10000 pktRateUp=3000 pktRateDown=6000
	 *  leafBitRateUp=100 leafBitRateDown=200
	 *  leafPktRateUp=50 leafPktRateDown=200
	 *  link=(salt.1,kans.1) link=(netMgr,kans.2) 	
	 *  ;
	 *  
	 *  comtree=1002 root=kans core=salt 
	 *  bitRateUp=5000 bitRateDown=10000 pktRateUp=3000 pktRateDown=6000
	 *  leafBitRateUp=100 leafBitRateDown=200
	 *  leafPktRateUp=50 leafPktRateDown=200
	 *  link=(salt.1,kans.1) link=(comtCtl,salt.2) 	
	 *  ;
	 *  ; # end of comtrees section
	 */

	// Exceptions for use with read.
	// These exceptions are all caught locally.
	// Using them as a way to reduce code repetion.
	private class TopParseError extends RuntimeException {
		public TopParseError() { super(""); }
		public TopParseError(String auxMsg) {
			super("(" + auxMsg + ")");
		}
	}
	private class RouterParseError extends RuntimeException {
		public RouterParseError() { super(""); }
		public RouterParseError(int rtr, String auxMsg) {
			super(" " + rtr + "-th router (" + auxMsg + ")");
		}
	}
	private class IfaceParseError extends RuntimeException {
		public IfaceParseError() { super(""); }
		public IfaceParseError(int rtr, String auxMsg) {
			super(" " + rtr + "-th router (" + auxMsg + ")");
		}
	}
	private class LeafParseError extends RuntimeException {
		public LeafParseError() { super(""); }
		public LeafParseError(int node, String auxMsg) {
			super(" " + node + "-th leaf node (" + auxMsg + ")");
		}
	}
	private class LinkParseError extends RuntimeException {
		public LinkParseError() { super(""); }
		public LinkParseError(int lnk, String auxMsg) {
			super(" " + lnk + "-th link (" + auxMsg + ")");
		}
	}
	private class ComtreeParseError extends RuntimeException {
		public ComtreeParseError() { super(""); }
		public ComtreeParseError(int comt, String auxMsg) {
			super(" " + comt + "-th comtree (" + auxMsg + ")");
		}
	}
	/** Parse contexts are used in read method to keep track of where we
	 *  are in the parsed file. Based upon the context, look for specific 
	 *  items in the input stream. For example, TOP context is the initial
   	 *  context, and in the TOP context, we expect to see one of
	 *  keyworks, Routers, LeafNodes, Links or Comtrees.
	 *  The associated description for each defined constant
	 *  is used in error messages
	 */
	private enum ParseContexts {
		TOP("in top level context"),
		ROUTER_SEC("in router section"),
		ROUTER_CTXT("in router section"),
		IFACES("in interface table"),
		IFACES_ENTRY("in interface table entry"),
		LEAF_SEC("in leaf node definition"),
		LEAF("in leaf node definition"),
		LINK_SEC("in link definition"),
		LINK("in link definition"),
		COMTREE_SEC("in comtree definition"),
		COMTREE_CTXT("in comtree definition");
		String desc;
		ParseContexts(String desc) {
			this.desc = desc;
		}
	};
	
	/** Reads a network topology file and initializes the NetInfo object.
	 *  @param in is an open input stream from which the topology file will
	 *  be read
	 *  @return true if the operation is successful, false if it fails
	 *  (failures are typically accompanied by error messages on cerr)
	 */
	public boolean read(InputStream in) {
		int rtrNum = 1;		// rtrNum = i for i-th router in file
		int ifNum = 1;		// interface number for current iface
		int leafNum = 1;	// leafNum = i for i-th leaf in file 
		int lnkNum = 1;		// lnkNum = i for i-th link in file
		int comtNum = 1;	// comtNum = i for i-th comtree in file

		String errMsg = "";
	
		ParseContexts context = ParseContexts.TOP;
		Scanner s = new Scanner(in);
		
		boolean status = false; // indicates abnormal terination
		while (s.hasNext()) {
			// skip over comments
			while (s.hasNext("#.*")) s.nextLine();
			if (!s.hasNext()) { status = true; break; }
			if (s.hasNext(";")) { // end of section
				if (context == ParseContexts.TOP) {
					status = true; break;
				}
				context = ParseContexts.TOP;
				s.next(); continue; 
			}
			// try block extends over entire switch statement
			try { switch (context) {
			case TOP:
				String secName = s.next();
				if (secName.equals("Routers"))
					context = ParseContexts.ROUTER_SEC;
				else if (secName.equals("LeafNodes"))
					context = ParseContexts.LEAF_SEC;
				else if (secName.equals("Links"))
					context = ParseContexts.LINK_SEC;
				else if (secName.equals("Comtrees"))
					context = ParseContexts.COMTREE_SEC;
				else {
					throw new TopParseError(
					    "invalid section name: " + secName);
				}
				break;
			case ROUTER_SEC:
				if (rtrNum > maxRtr)
					throw new RouterParseError(rtrNum,
							"too many routers");
				errMsg = readRouter(s);
				if (errMsg.length() != 0) {
					throw new
					RouterParseError(rtrNum,errMsg);
				}
				rtrNum++; break;
			case LEAF_SEC:
				if (leafNum > maxLeaf)
					throw new LeafParseError(leafNum,
							"too many leaf nodes");
				errMsg = readLeaf(s);
				if (errMsg.length() != 0) {
					throw new
					LeafParseError(leafNum,errMsg);
				}
				leafNum++; break;
			case LINK_SEC:
				if (lnkNum > maxLink)
					throw new LinkParseError(lnkNum,
							"too many links");
				errMsg = readLink(s);
				if (errMsg.length() != 0) {
					throw new
					LinkParseError(lnkNum,errMsg);
				}
				lnkNum++; break;
			case COMTREE_SEC:
				if (comtNum > maxComtree)
					throw new ComtreeParseError(comtNum,
							"too many comtrees");
				errMsg = readComt(s);
				if (errMsg.length() != 0) {
					throw new
					ComtreeParseError(comtNum,errMsg);
				}
				comtNum++; break;
			default: Util.fatal("read: undefined context");
			}} catch(Exception e) {
				System.err.println("NetInfo.read: error when "
				    + "parsing input " + context.desc
				    + "\n" + e);
				return false;
			}
		}
		return status && context == ParseContexts.TOP &&
				 check() && setComtLnkNodeInfo();
	}

	public String readRouter(Scanner s) {
		RtrNodeInfo nuRtr = new RtrNodeInfo();
		String inWord = null;
		String[] part;
		while (true) {
			// skip comments
			while (s.hasNext("#.*")) s.nextLine();
			if (s.hasNext(";")) { // end of router def'n
				s.next();
				if (nuRtr.name.equals(""))
					return "missing router name";
				if (nuRtr.nType == Forest.NodeTyp.UNDEF_NODE)
					return "missing node type for " +
						nuRtr.name;
				if (!Forest.validUcastAdr(nuRtr.fAdr)) 
					return "missing forest address for " +
						nuRtr.name;
				if (nuRtr.latitude == UNDEF_LAT)
					return "missing latitude for " +
						nuRtr.name;
				if (nuRtr.longitude == UNDEF_LONG)
					return "missing longitude for " +
						nuRtr.name;
				if (!Forest.validUcastAdr(nuRtr.firstLeafAdr) ||
				    !Forest.validUcastAdr(nuRtr.lastLeafAdr) ||
				     Forest.zipCode(nuRtr.fAdr)
				      != Forest.zipCode(nuRtr.firstLeafAdr) ||
				     Forest.zipCode(nuRtr.fAdr)
				      != Forest.zipCode(nuRtr.lastLeafAdr) ||
				    nuRtr.firstLeafAdr > nuRtr.lastLeafAdr)
					return "no valid client address " +
					    	"range for " + nuRtr.name;
				if (nuRtr.numIf == 0)
					return "no interfaces defined for " +
						nuRtr.name;
				// add new router, initialize attributes
				int r = addRouter(nuRtr);
				if (r == 0) return "cannot add " + nuRtr.name;
				return "";
			}
			
			if (!s.hasNext()) return "incomplete router spec";
			inWord = s.next();
			part = inWord.split("=",2);
			if (part[0].equals("name")) {
				nuRtr.name = part[1];
			} else if (part[0].equals("type")) {
				nuRtr.nType = Forest.string2nodeTyp(part[1]);
			} else if (part[0].equals("fAdr")) {
				nuRtr.fAdr = Forest.forestAdr(part[1]);
			} else if (part[0].equals("location")) {
				part = part[1].split("[(,)]",4);
				if (part.length != 4)
					return "invalid token: " + inWord;
				nuRtr.latitude  = (int) (1000000
				    * Double.parseDouble(part[1]));
				nuRtr.longitude  = (int) (1000000
				    * Double.parseDouble(part[2]));

			} else if (part[0].equals("leafAdrRange")) {
				part = part[1].split("[(-.)]",6);
				if (part.length != 6)
				   	return "invalid token: " + inWord;
				int zip = Integer.parseInt(part[1]);
				int loc = Integer.parseInt(part[2]);
				nuRtr.firstLeafAdr = Forest.forestAdr(zip,loc);
				zip = Integer.parseInt(part[3]);
				loc = Integer.parseInt(part[4]);
				nuRtr.lastLeafAdr = Forest.forestAdr(zip,loc);
			} else if (part[0].equals("interfaces")) {
				String errMsg = readIfaces(s,nuRtr);
				if (errMsg.length() != 0) return errMsg;
			} else {
				return "invalid token: " + inWord;
			}
		}
	}

	public String readIfaces(Scanner s, RtrNodeInfo nuRtr) {
		String inWord = null;
		String[] part;
		IfInfo[] iface = new IfInfo[Forest.MAXINTF+1];
		for (int i = 1; i <= Forest.MAXINTF; i++) iface[i] = null;
		int maxIfNum = 0;
		while (true) {
			// skip comments
			while (s.hasNext("#.*")) s.nextLine();

			// first check for end keyword
			if (s.hasNext("end")) {
				s.next();
				nuRtr.numIf = maxIfNum;
				nuRtr.iface = new IfInfo[nuRtr.numIf+1];
				for (int i = 1; i <= nuRtr.numIf; i++)
					nuRtr.iface[i] = iface[i];
				return "";
			}
			if (!s.hasNextInt()) return "missing interface number";
			int ifNum = s.nextInt();
			if (ifNum < 1 || ifNum > Forest.MAXINTF)
				return "interface num is out of range";
			if (iface[ifNum] != null)
				return "repeating iface number " + ifNum;
			iface[ifNum] = new IfInfo();
			maxIfNum = Math.max(maxIfNum, ifNum);

			// now read ip address
			if (!s.hasNext()) return "missing IP address";
			inWord = s.next();
			int ipa = Util.string2ipAdr(inWord);
			if (ipa == 0) return "bad token: " + inWord;
			iface[ifNum].ipAdr = ipa;

			// now read the link number (or range)
			if (!s.hasNext()) return "missing link number range";
			inWord = s.next();
			part = inWord.split("-",2);
			iface[ifNum].firstLink = Integer.parseInt(part[0]);
			iface[ifNum].lastLink = iface[ifNum].firstLink;
			if (part.length == 2) 
				iface[ifNum].lastLink =
				    	Integer.parseInt(part[1]);
			if (!s.hasNext()) return "missing bit rate";
			iface[ifNum].bitRate = s.nextInt();
			if (!s.hasNext()) return "missing packet rate";
			iface[ifNum].pktRate = s.nextInt();
			if (s.hasNext(";")) s.next();
			else return "missing semicolon";
		}
	}

	public String readLeaf(Scanner s) {
		LeafNodeInfo nuLeaf = new LeafNodeInfo();
		String inWord = null;
		String[] part;

		while (true) {
			// skip comments
			while (s.hasNext("#.*")) s.nextLine();
			if (s.hasNext(";")) {
				s.next();
				if (nuLeaf.name.equals(""))
					return "missing node name";
				if (nuLeaf.nType == Forest.NodeTyp.UNDEF_NODE)
					return "missing node type for leaf"
						+ " node " + nuLeaf.name;
				if (nuLeaf.ipAdr == 0)
					return "missing IP address for leaf "
						+ "node " + nuLeaf.name;
				if (!Forest.validUcastAdr(nuLeaf.fAdr))
					return "missing forest address for"
					    	+ " leaf node " + nuLeaf.name;
				if (nuLeaf.latitude == UNDEF_LAT) 
					return "missing latitude for leaf node "
					    	+ nuLeaf.name;
				if (nuLeaf.longitude == UNDEF_LONG)
					return "missing longitude for leaf"
					    	+ " node " + nuLeaf.name;
				// add new leaf, initialize attributes
				int nodeNum = addLeaf(nuLeaf);
				if (nodeNum == 0)
					return "cannot add leaf " + nuLeaf.name;
				return "";
			}
			if (!s.hasNext("[a-zA-Z_]+=.*")) {
				return "invalid token: " + s.next();
			}
			inWord = s.next();
			part = inWord.split("=",2);
			if (part[0].equals("name")) {
				nuLeaf.name = part[1];
			} else if (part[0].equals("type")) {
				nuLeaf.nType = Forest.string2nodeTyp(part[1]);
				if (nuLeaf.nType == Forest.NodeTyp.UNDEF_NODE)
					return "bad type: " + part[1];
			} else if (part[0].equals("ipAdr")) {
				int ipa = Util.string2ipAdr(part[1]);
				if (ipa == 0) return "bad token: " + inWord;
				nuLeaf.ipAdr = ipa;
			} else if (part[0].equals("fAdr")) {
				int fAdr = Forest.forestAdr(part[1]);
				if (fAdr == 0) return "bad token: " + inWord;
				nuLeaf.fAdr = fAdr;
			} else if (part[0].equals("location")) {
				part = part[1].split("[(,)]",4);
				if (part.length != 4)
					return "invalid token: " + inWord;
				nuLeaf.latitude  = (int) (1000000
				    * Double.parseDouble(part[1]));
				nuLeaf.longitude  = (int) (1000000
				    * Double.parseDouble(part[2]));
			} else {
				return "invalid token: " + inWord;
			}
		}
	}

	public String readLink(Scanner s) {
		LinkInfo nuLnk = new LinkInfo();
		String inWord = null;
		String[] part;
		String leftName = null; String rightName = null;
		int linkLength = -1;

		while (true) {
			// skip comments
			while (s.hasNext("#.*")) s.nextLine();
			if (s.hasNext(";")) { // end of link definition
				s.next();
				if (leftName == "") 
					return "missing left endpoint";
				if (rightName == "") 
					return "missing right endpoint";
				if (nuLnk.bitRate == 0)
					return "missing bit rate";
				if (nuLnk.pktRate == 0)
					return "missing packet rate";
				if (linkLength == -1)
					return "missing length";
				
				// add new link and set attributes
				int u = nameNodeMap.get(leftName);
				int v = nameNodeMap.get(rightName);
				int lnk = addLink(u,v,linkLength,nuLnk);
				if (lnk == 0) 
					return "can't add link (" + leftName
						+ "." + nuLnk.leftLnum + ","
					        + rightName + "."
					        + nuLnk.rightLnum +")";
				return "";
			}
			if (!s.hasNext("[a-zA-Z_]+=.*"))
				return "invalid token: " + s.next();
			inWord = s.next();
			part = inWord.split("=",2);
			if (part[0].equals("link")) {
				part = part[1].split("[(.,)]",6);
				if (part.length < 5) {
					return "invalid token: " + inWord;
				}
				leftName = part[1];
				int left = nameNodeMap.get(leftName);
				if (left == 0)
					return "unrecognized node name in "
					    	+ inWord;
				int rpos = 2;
				if (getNodeType(left) ==Forest.NodeTyp.ROUTER) {
					nuLnk.leftLnum =
					    Integer.parseInt(part[2]);
					rpos = 3;
				}
				rightName = part[rpos];
				int right = nameNodeMap.get(rightName);
				if (right == 0)
					return "unrecognized node name in "
					    	+ inWord;
				if (getNodeType(right)==Forest.NodeTyp.ROUTER)
					nuLnk.rightLnum =
					     Integer.parseInt(part[rpos+1]);
			} else if (part[0].equals("bitRate")) {
				nuLnk.bitRate = Integer.parseInt(part[1]);
				nuLnk.availBitRateL = nuLnk.bitRate;
				nuLnk.availBitRateR = nuLnk.bitRate;
			} else if (part[0].equals("pktRate")) {
				nuLnk.pktRate = Integer.parseInt(part[1]);
				nuLnk.availPktRateL = nuLnk.pktRate;
				nuLnk.availPktRateR = nuLnk.pktRate;
			} else if (part[0].equals("length")) {
				linkLength = Integer.parseInt(part[1]);
			} else {
				return "invalid token: " + inWord;
			}
		}
	}

	public String readComt(Scanner s) {
		ComtreeInfo nuComt = new ComtreeInfo();
		nuComt.coreSet = new HashSet<Integer>();
		nuComt.lnkMap = new HashMap<Integer,RateSpec>();
		nuComt.rtrMap = new HashMap<Integer,ComtRtrInfo>();

		String inWord;
		String[] part;
		while (true) {
			// skip comments
			while (s.hasNext("#.*")) s.nextLine();
			if (s.hasNext(";")) { // end of comtree definition
				s.next();
				if (nuComt.root == 0)
					return "missing root";
				if (nuComt.comtreeNum == 0)
					return "missing comtree number";
				if (nuComt.bitRateDown == 0)
					return "missing bitRateDown for " +
						"comtree " + nuComt.comtreeNum;
				if (nuComt.bitRateUp == 0) 
					return "missing bitRateUp for comtree "
					    	+ nuComt.comtreeNum;
				if (nuComt.pktRateDown == 0)
					return "missing pktRateDown for " +
						"comtree " + nuComt.comtreeNum;
				if (nuComt.pktRateUp == 0)
					return "missing pktRateUp for comtree "
					    	+ nuComt.comtreeNum;
				if (nuComt.leafBitRateDown == 0)
					return "missing leafBitRateDown for " +
					    	"comtree " + nuComt.comtreeNum;
				if (nuComt.leafBitRateUp == 0) 
					return "missing leafBitRateUp for " +
					    	"comtree " + nuComt.comtreeNum;
				if (nuComt.leafPktRateDown == 0)
					return "missing leafPktRateDown for " +
					    	"comtree " + nuComt.comtreeNum;
				if (nuComt.leafPktRateUp == 0) {
					return "missing leafPktRateUp for " +
					    	"comtree " + nuComt.comtreeNum;
				}
				if (!addComtree(nuComt))
					return "cannot add comtree " +
						nuComt.comtreeNum;
				return "";
			}
			// parse next chunk of comtree definition
			if (!s.hasNext("[a-zA-Z_]+=.*")) 
				return "invalid token: " + s.next();
			inWord = s.next();
			part = inWord.split("=",2);
			if (part[0].equals("comtree")) {
				nuComt.comtreeNum = Integer.parseInt(part[1]);
			} else if (part[0].equals("owner")) {
				int owner = getNodeNum(part[1]);
				if (owner == 0) 
					return "invalid token: " + inWord;
				nuComt.ownerAdr = getNodeAdr(owner);
			} else if (part[0].equals("root")) {
				Integer root = nameNodeMap.get(part[1]);
				if (root == null)
					return "root " + part[1] +
						" is not a valid node";
				nuComt.root = root;
				if (!isRouter(nuComt.root))
					return "root " + part[1] +
						" is not a router";
				nuComt.coreSet.add(nuComt.root);
			} else if (part[0].equals("core")) {
				Integer r = nameNodeMap.get(part[1]);
				if (r == null) 
					return "core " + part[1]
						+ " is not a valid node";
				if (!isRouter(r))
					return "core " + part[1]
					    	+ " is not a router";
				nuComt.coreSet.add(r);
			} else if (part[0].equals("bitRateDown")) {
				nuComt.bitRateDown = Integer.parseInt(part[1]);
			} else if (part[0].equals("bitRateUp")) {
				nuComt.bitRateUp = Integer.parseInt(part[1]);
			} else if (part[0].equals("pktRateDown")) {
				nuComt.pktRateDown = Integer.parseInt(part[1]);
			} else if (part[0].equals("pktRateUp")) {
				nuComt.pktRateUp = Integer.parseInt(part[1]);
			} else if (part[0].equals("leafBitRateDown")) {
				nuComt.leafBitRateDown =
					Integer.parseInt(part[1]);
			} else if (part[0].equals("leafBitRateUp")) {
				nuComt.leafBitRateUp =
					Integer.parseInt(part[1]);
			} else if (part[0].equals("leafPktRateDown")) {
				nuComt.leafPktRateDown =
					Integer.parseInt(part[1]);
			} else if (part[0].equals("leafPktRateUp")) {
				nuComt.leafPktRateUp =
					Integer.parseInt(part[1]);
			} else if (part[0].equals("link")) {
				int lnk = parseLink(part[1]);
				if (lnk <= 0) 
					return "cannot parse link " + part[1];
				RateSpec rs = new RateSpec();
				rs.bitRateUp = 0; rs.bitRateDown = 0;
				rs.pktRateUp = 0; rs.pktRateDown = 0;
				nuComt.lnkMap.put(lnk,rs);
				int left = netTopo.left(lnk);
				if (getNodeType(left) ==Forest.NodeTyp.ROUTER) {
					ComtRtrInfo cri = new ComtRtrInfo();
					nuComt.rtrMap.put(left,cri);
				}
				int right = netTopo.right(lnk);
				if (getNodeType(right)==Forest.NodeTyp.ROUTER) {
					ComtRtrInfo cri = new ComtRtrInfo();
					nuComt.rtrMap.put(right,cri);
				}
			} else {
				return "invalid token " + inWord;
			}
		}
	}

	/** Read comtree status information from an input stream.
	 */
	public ComtreeInfo readComtStatus(InputStream in) {
		ComtreeInfo nuComt = new ComtreeInfo();
		nuComt.coreSet = new HashSet<Integer>();
		nuComt.lnkMap = new HashMap<Integer,RateSpec>();
		nuComt.rtrMap = new HashMap<Integer,ComtRtrInfo>();

		Scanner s = new Scanner(in);
		String inWord;
		String[] part;
		while (true) {
			// skip comments
			while (s.hasNext("#.*")) {
				System.out.println(s.nextLine());
			}
			if (s.hasNext(";")) { // end of comtree definition
				s.next(); break;
			}
			// parse next chunk of comtree definition
			if (!s.hasNext("[a-zA-Z_]+=.*")) {
				nuComt.comtreeNum = 0; return nuComt;
			}
			inWord = s.next();
			part = inWord.split("=",2);
			if (part[0].equals("comtree")) {
				nuComt.comtreeNum = Integer.parseInt(part[1]);
			} else if (part[0].equals("owner")) {
				int owner = getNodeNum(part[1]);
				if (owner == 0) break;
				nuComt.ownerAdr = getNodeAdr(owner);
			} else if (part[0].equals("root")) {
				Integer root = nameNodeMap.get(part[1]);
				if (root == null) break;
				nuComt.root = root;
				if (!isRouter(nuComt.root)) break;
				nuComt.coreSet.add(nuComt.root);
			} else if (part[0].equals("core")) {
				Integer r = nameNodeMap.get(part[1]);
				if (r == null) break;
				if (!isRouter(r)) break;
				nuComt.coreSet.add(r);
			} else if (part[0].equals("bitRateDown")) {
				nuComt.bitRateDown = Integer.parseInt(part[1]);
			} else if (part[0].equals("bitRateUp")) {
				nuComt.bitRateUp = Integer.parseInt(part[1]);
			} else if (part[0].equals("pktRateDown")) {
				nuComt.pktRateDown = Integer.parseInt(part[1]);
			} else if (part[0].equals("pktRateUp")) {
				nuComt.pktRateUp = Integer.parseInt(part[1]);
			} else if (part[0].equals("leafBitRateDown")) {
				nuComt.leafBitRateDown =
					Integer.parseInt(part[1]);
			} else if (part[0].equals("leafBitRateUp")) {
				nuComt.leafBitRateUp =
					Integer.parseInt(part[1]);
			} else if (part[0].equals("leafPktRateDown")) {
				nuComt.leafPktRateDown =
					Integer.parseInt(part[1]);
			} else if (part[0].equals("leafPktRateUp")) {
				nuComt.leafPktRateUp =
					Integer.parseInt(part[1]);
			} else if (part[0].equals("link")) {
				int lnk = parseLink(part[1]);
				if (lnk <= 0) break;
				RateSpec rs = new RateSpec();
				rs.bitRateUp = 0; rs.bitRateDown = 0;
				rs.pktRateUp = 0; rs.pktRateDown = 0;
				nuComt.lnkMap.put(lnk,rs);
				int left = netTopo.left(lnk);
				if (getNodeType(left) ==Forest.NodeTyp.ROUTER) {
					ComtRtrInfo cri = new ComtRtrInfo();
					nuComt.rtrMap.put(left,cri);
				}
				int right = netTopo.right(lnk);
				if (getNodeType(right)==Forest.NodeTyp.ROUTER) {
					ComtRtrInfo cri = new ComtRtrInfo();
					nuComt.rtrMap.put(right,cri);
				}
			} else if (part[0].equals("node")) {
				part = part[1].split("[(,]",4);
				if (part.length < 4) break;
				String nodeName = part[1];
				Integer node = nameNodeMap.get(nodeName);
				if (node == null) break;
				int lnkCnt = Integer.parseInt(part[2]);
				int plnk;
				if (part[3].charAt(0) == '-') plnk = 0;
				else plnk = parseLink(part[3]);
				if (plnk < 0) break;
				if (getNodeType(node) != Forest.NodeTyp.ROUTER)
					break;
				addComtNode(nuComt,node);
				ComtRtrInfo cri = new ComtRtrInfo();
				cri.plnk = plnk; cri.lnkCnt = lnkCnt;
				nuComt.rtrMap.put(node,cri);
			} else {
				nuComt.comtreeNum = 0; break;
			}
		}
		if (nuComt.root == 0 || nuComt.comtreeNum == 0 ||
		    nuComt.bitRateDown == 0 || nuComt.bitRateUp == 0 ||
		    nuComt.pktRateDown == 0 || nuComt.pktRateUp == 0 ||
		    nuComt.leafBitRateDown == 0 || nuComt.leafBitRateUp == 0 ||
		    nuComt.leafPktRateDown == 0 || nuComt.leafPktRateUp == 0) {
			nuComt.comtreeNum = 0;
		}
		return nuComt;
	}

	/** Extract a link from a given string.
	 *  @param s is a string that represents a link
	 *  @return the link number of the link represented by s;
	 *  if the link is not already part of the network, add it;
	 *  return -1 on failure
	 */
	public int parseLink(String s) {
		if (s.equals("-")) return 0;
		String[] part;
		part = s.split("[(.,)]",6);
		if (part.length < 3) return -1;
		String leftName = part[1];
		Integer left = nameNodeMap.get(leftName);
		if (left == null) return -1;
		int rpos = 2;
		int leftLnum = 0;
		if (getNodeType(left) == Forest.NodeTyp.ROUTER) {
			leftLnum = Integer.parseInt(part[2]);
			rpos = 3;
		}
		if (part.length < rpos) return -1;
		String rightName = part[rpos];
		Integer right = nameNodeMap.get(rightName);
		if (right == null) return -1;
		int rightLnum = 0;
		if (getNodeType(right) == Forest.NodeTyp.ROUTER) {
			if (part.length < rpos+1) return -1;
			rightLnum = Integer.parseInt(part[rpos+1]);
		}
		int ll = (isLeaf(left) ? getLinkNum(left)
				: getLinkNum(left,leftLnum));
		int lr = (isLeaf(right) ? getLinkNum(right)
				: getLinkNum(right,rightLnum));
		if (ll != 0) return (ll == lr ? ll : -1);
		ll = addLink(left,right,leftLnum,rightLnum);
		return ll;	
	}
	
	public String link2string(int lnk) {
		String s = "";
		if (lnk == 0) { s = "-"; return s; }
		int left = getLinkL(lnk); int right = getLinkR(lnk);
		s = "(" + getNodeName(left);
		if (getNodeType(left) == Forest.NodeTyp.ROUTER)  {
			s += "." + getLocLinkL(lnk);
		}
		s += "," + getNodeName(right);
		if (getNodeType(right) == Forest.NodeTyp.ROUTER)  {
			s += "." + getLocLinkR(lnk);
		}
		s += ")";
		return s;
	}
	
	/** Write the contents of a NetInfo object to an output stream.
	 *  The object is written out in a form that allows it to be read in 
	 *  again using the read method. Thus programs that modify the interal 
	 *  representation can save the result to an output file from which 
	 *  it can later restore the original configuration. Note that on
	 *  re-reading, node numbers and link numbers may change, but 
	 *  semantically, the new version will be equivalent to the old one.
	 *  @param out is an open output stream
	 */
	public String toString() {
		String s;
		// Start with the "Routers" section
		s = "Routers\n";
		for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
			s += rtr2string(r);
		}
		s += ";\n";
	
		// Next write the LeafNodes, starting with the controllers
		s += "LeafNodes\n";

		Iterator<Integer> cp = getControllers().iterator();
		while (cp.hasNext()) {
			Integer c = cp.next(); s += leaf2string(c);
		}
		for (int n = firstLeaf(); n != 0; n = nextLeaf(n)) {
			if (getNodeType(n) != Forest.NodeTyp.CONTROLLER)
				s += leaf2string(n);
		}
		s += ";\n";
	
		// Now, write the Links
		s += "Links\n"; 
		for (int lnk = firstLink(); lnk != 0; lnk = nextLink(lnk)) {
			s += netlink2string(lnk);
		}
		s += ";\n"; 
	
		// And finally, the Comtrees
		s += "Comtrees\n"; 
		for (int ctx = firstComtIndex(); ctx != 0;
			 ctx = nextComtIndex(ctx)) {
			s += comt2string(ctx);
		}
		s += ";\n"; 
		return s;
	}
	
	public String rtr2string(int rtr) {
		String s;
		s = "name=" + getNodeName(rtr) + " "
		    + "type=" + Forest.nodeTyp2string(getNodeType(rtr))
		    + " fAdr=" + Forest.fAdr2string(getNodeAdr(rtr));
		s += " leafAdrRange=("
		    + Forest.fAdr2string(getFirstLeafAdr(rtr)) + "-"
		    + Forest.fAdr2string(getLastLeafAdr(rtr)) + ")";
		s += "\n\tlocation=(" + getNodeLat(rtr) + ","
		    		     + getNodeLong(rtr) + ")";
		s += "\ninterfaces\n"
		    + "# iface#   ipAdr  linkRange  bitRate  pktRate\n";
		for (int i = 1; i <= getNumIf(rtr); i++) {
			if (!validIf(rtr,i)) continue;
			s += "   " + i + "  "
			    + Util.ipAdr2string(getIfIpAdr(rtr,i)); 
			if (getIfFirstLink(rtr,i) == getIfLastLink(rtr,i)) 
				s += " " + getIfFirstLink(rtr,i) + " ";
			else
				s += " " + getIfFirstLink(rtr,i) + "-"
				    + getIfLastLink(rtr,i) + "  ";
			s += getIfBitRate(rtr,i) + "  "
			    + getIfPktRate(rtr,i) + " ;\n";
		}
		s += "end\n;\n";
		return s;
	}
	
	public String leaf2string(int leaf) {
		String s;
		s = "name=" + getNodeName(leaf) + " "
		    + "type=" + Forest.nodeTyp2string(getNodeType(leaf))
		    + " ipAdr=" + Util.ipAdr2string(getLeafIpAdr(leaf)) 
		    + " fAdr=" + Forest.fAdr2string(getNodeAdr(leaf));
		s += "\n\tlocation=(" + getNodeLat(leaf) + ","
		    		     + getNodeLong(leaf) + ") ;\n";
		return s;
	}
	
	public String netlink2string(int lnk) {
		String s;
		s = "link=" + link2string(lnk)
		    + " bitRate=" + getLinkBitRate(lnk)
		    + " pktRate=" + getLinkPktRate(lnk)
		    + " length=" + getLinkLength(lnk) + " ;\n"; 
		return s;
	}
	
	public String comt2string(int ctx) {
		if (!validComtIndex(ctx)) return "-";
		String s;
		s = "comtree=" + getComtree(ctx)
		    + " root=" + getNodeName(getComtRoot(ctx))
		    + "\nbitRateDown=" + getComtBrDown(ctx)
		    + " bitRateUp=" + getComtBrUp(ctx)
		    + " pktRateDown=" + getComtPrDown(ctx)
		    + " pktRateUp=" + getComtPrUp(ctx)
		    + "\nleafBitRateDown=" + getComtLeafBrDown(ctx)
		    + " leafBitRateUp=" + getComtLeafBrUp(ctx)
		    + " leafPktRateDown=" + getComtLeafPrDown(ctx)
		    + " leafPktRateUp=" + getComtLeafPrUp(ctx); 
	
		// iterate through core nodes and print
		s += "\n"; 
		Iterator<Integer> csp = getCoreSet(ctx).iterator();
		while (csp.hasNext()) {
			Integer c = csp.next();
			if (c != getComtRoot(ctx)) 
				s += "core=" + getNodeName(c) + " ";
		}
		s += "\n"; 
		// iterate through links and print
		Iterator<Integer> clp = getComtLinks(ctx).keySet().iterator();
		while (clp.hasNext()) {
			Integer lnk = clp.next();
			s += "link=" + link2string(lnk) + " ";
		}
		s += "\n;\n"; 
		return s;
	}
}
