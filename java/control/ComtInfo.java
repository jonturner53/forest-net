/** @file ComtInfo.cpp 
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

public class ComtInfo {

	/** Class used to represent information used to modify a link.  */
	private class LinkMod {
	public int lnk;		///< link number of the link
	public int child;	///< node # for "lower-end" of lnk
	public RateSpec rs;	///< rate spec for link
	LinkMod() { lnk = 0; child = 0; rs = new RateSpec(0); }
	LinkMod(int l, int c, RateSpec rs1) {
		lnk = l; child = c; rs.copyFrom(rs1);
	}
	};

	/** Utility class used by input methods */
	public class LinkModX extends LinkMod {
	public String errMsg;
	LinkModX() { super(); errMsg = null; }
	}

	/** Another utility class used by input methods */
	public class NodeDesc extends LinkMod {
	public int lnkCnt;
	NodeDesc() { super(); lnkCnt = 0; }
	}

	/** Another utility class used by input methods */
	public class LinkEndpoint {
	public String name; public int num;
	LinkEndpoint() { name = null; num = 0; }
	LinkEndpoint(String nam, int n) { name = nam; num = n; }
	}

	private int maxComtree;		///< maximum number of comtrees
	private NetInfo	net;		///< NetInfo data structure for network

	// used in rtrMap of ComtreeInfo
	private class ComtRtrInfo {
	public int	plnk;		// link to parent in comtree
	public int	lnkCnt;		// # of comtree links at this router
	public RateSpec subtreeRates;	// rates for subtree rooted at this node
	public boolean	frozen;		// true if plnk rate is frozen
	public RateSpec plnkRates;	// rates for link
	ComtRtrInfo() { plnk = lnkCnt = 0; frozen = false;
			subtreeRates = new RateSpec(0);
			plnkRates = new RateSpec(0); };		
	};

	// used in leafMap of ComtreeInfo objects
	private class ComtLeafInfo {
	public int	parent;		// forest address of parent of this leaf
	public int	llnk;		// local link # of parent link at parent
	public RateSpec plnkRates;	// rates for leaf and link to parent
	ComtLeafInfo() { parent = llnk = 0; plnkRates= new RateSpec(0); };
	};

	private class ComtreeInfo {
	public int	comtreeNum;	///< number of comtree
	public int	owner;		///< forest address of comtree owner
	public int	root;		///< forest address of root of comtree
	public boolean	autoConfig;	///< if true, set rates dynamically
	public RateSpec bbDefRates;	///< default backbone link rates
	public RateSpec leafDefRates;	///< default backbone link rates
	public Set<Integer> coreSet;	///< set of core nodes in comtree
	public Map<Integer,ComtRtrInfo> rtrMap; ///< map for routers in comtree
	public Map<Integer,ComtLeafInfo> leafMap; ///< map for leaf nodes
	};
	private ComtreeInfo [] comtree;	///< array of comtrees
	private IdMap comtreeMap;	///< maps comtree # to indices in array


	/** Constructor for ComtInfo, allocates space and initializes private
	 *  data.
	 *  @param maxNode1 is the max # of nodes in this ComtInfo object
	 *  @param maxLink1 is the max # of links in this ComtInfo object
	 *  @param maxRtr1 is the max # of routers in this ComtInfo object
	 *  @param maxCtl1 is the max # of controllers in this ComtInfo object
	 *  @param maxComtree1 is the max # of comtrees in this ComtInfo object
	 */
	public ComtInfo(int maxComtree, NetInfo net) {
		this.maxComtree = maxComtree; this.net = net;
		comtree = new ComtreeInfo[maxComtree+1];
		comtreeMap = new IdMap(maxComtree);
	}

	// Note that methods that take a comtree index as an argument
	// assume that the comtree index is valid. This was a conscious
	// decision to allow the use of ComtInfo in contexts where a program
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
	 *  ComtInfo uses integer indices in a restricted range to access
	 *  comtree information efficiently. Users get the comtree index
	 *  for a given comtree number using the lookupComtree() method.
	 *  @param ctx is an integer comtree index
	 *  @return true if n corresponds to a comtree in the network,
	 *  else false
	 */
	public boolean validComtIndex(int ctx) {
		return comtreeMap.validId(ctx);
	}
	
	/** Determine if a router is a core node in a specified comtree.
	 *  @param ctx is a valid comtree index
	 *  @param r is the forest address of a router
	 *  @return true if r is a core node in the comtree with index i,
	 *  else false
	 */
	public boolean isCoreNode(int ctx, int r) {
		return	comtree[ctx].coreSet.contains(r);
	}
	
	/** Determine if a node belongs to a comtree or not.
	 *  @param ctx is the comtree index of the comtree of interest
	 *  @param fa is the forest address of a node that is to be checked
	 *  for membership in the comtree
	 *  @return true if n is a node in the comtree, else false
	 */
	public boolean isComtNode(int ctx, int fa) {
		return isComtLeaf(ctx,fa) || isComtRtr(ctx,fa);
	}
	
	/** Determine if a node is a router in a comtree or not.
	 *  @param ctx is the comtree index of the comtree of interest
	 *  @param fa is the forest address of the node that we want to check 
	 *  in the comtree
	 *  @return true if fa is a node in the comtree, else false
	 */
	public boolean isComtRtr(int ctx, int fa) {
		return comtree[ctx].rtrMap.keySet().contains(fa);
	}
	
	/** Determine if a node is a leaf in a comtree.
	 *  @param ctx is the comtree index of the comtree of interest
	 *  @param ln is the forest address of a network node
	 *  @return true if ln is a leaf in the comtree, else false
	 */
	public boolean isComtLeaf(int ctx, int ln) {
		return comtree[ctx].leafMap.keySet().contains(ln);
	}
	
	/** Determine if a link is in a specified comtree.
	 *  @param ctx is a valid comtree index
	 *  @param lnk is a link number
	 *  @return true if lnk is a link in the comtree with index i,
	 *  else false
	 */
	public boolean isComtLink(int ctx, int lnk) {
		int left = net.getNodeAdr(net.getLeft(lnk));
		int right = net.getNodeAdr(net.getRight(lnk));
		
		return	(isComtNode(ctx,left) && right==getParent(ctx,left)) ||
		    	(isComtNode(ctx,right) && left==getParent(ctx,right));
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

	/** Get the set of core nodes for a given comtree.
	 *  This method is provided to enable a client program to iterate
	 *  over the set of core nodes. The client should not modify the core
	 *  set directly, as this can make the NetInfo object inconsistent.
	 *  @param ctx is a valid comtree index
	 *  @return a reference to the set of node numbers for the core nodes
	 */
	public Set<Integer> getCoreSet(int ctx) {
		return comtree[ctx].coreSet;
	}
	
	/** Get the comtree router map for a given comtree.
	 *  This method is provided to allow a client to iterate over the
	 *  routers in a comtree. It should not be used to modify the map,
	 *  as this can make the NetInfo object inconsistent.
	 *  @param ctx is a valid comtree index
	 *  @return a reference to the router map
	 */
	public Map<Integer,ComtRtrInfo> getRtrMap(int ctx) {
		return comtree[ctx].rtrMap;
	}
	
	/** Get the comtree leaf map for a given comtree.
	 *  This method is provided to allow a client to iterate over the
	 *  leaf nodes in a comtree. It should not be used to modify the map,
	 *  as this can make the NetInfo object inconsistent.
	 *  @param ctx is a valid comtree index
	 *  @return a reference to the leaf map
	 */
	public Map<Integer,ComtLeafInfo> getLeafMap(int ctx) {
		return comtree[ctx].leafMap;
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
	public int getRoot(int ctx) {
		return comtree[ctx].root;
	}
	
	/** Get the owner of a comtree with specified index.
	 *  @param ctx is a valid comtree index
	 *  @param return the forest address of the client that originally
	 *  created the comtree, or 0 if c is not a valid index
	 */
	public int getOwner(int ctx) {
		return comtree[ctx].owner;
	}
	
	/** Get the link to a parent of a node in a comtree.
	 *  @param ctx is a valid comtree index 
	 *  @param nfa is a forest address of a node in the comtree
	 *  @return the global link number of the link to nfa's parent,
	 *  if nfa is a router; return the local link number at nfa's
	 *  parent if nfa is a leaf; return 0 if nfa has no parent
	 **/
	public int getPlink(int ctx, int nfa) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(nfa);
		if (rtr != null) return rtr.plnk;
		ComtLeafInfo leaf = comtree[ctx].leafMap.get(nfa);
		return leaf.llnk;
	}
	
	/** Get the parent of a comtree node.
	 *  @param ctx is a valid comtree index 
	 *  @param fa is the number of a node in the comtree
	 *  @return the forest address of fa's parent, or 0 if fa has no parent
	 **/
	public int getParent(int ctx, int fa) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(fa);
		if (rtr != null) {
			if (rtr.plnk == 0) return 0;
			int parent = net.getPeer(net.getNodeNum(fa),rtr.plnk);
			return net.getNodeAdr(parent);
		}
		ComtLeafInfo leaf = comtree[ctx].leafMap.get(fa);
		return leaf.parent;
	}
	
	/** Get the child endpoint of a link in a comtree.
	 *  @param ctx is a valid comtree index 
	 *  @param lnk is a link number for a link in the comtree
	 *  @return the forest address of the child node, or 0 on failure
	 **/
	public int getChild(int ctx, int lnk) {
		int left = net.getLeft(lnk);
		int leftAdr = net.getNodeAdr(left);
		if (net.isLeaf(left)) return leftAdr;
		int right = net.getRight(lnk);
		int rightAdr = net.getNodeAdr(right);
		if (net.isLeaf(right)) return rightAdr;
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(leftAdr);
		return (rtr.plnk == lnk ? leftAdr : rightAdr);
	}
	
	/** Get the number of links in a comtree that are incident to a router.
	 *  @param ctx is a valid comtree index 
	 *  @param radr is the number of a router in the comtree
	 *  @return the number of links in the comtree incident to radr
	 *  or -1 if n is not a node in the comtree
	 **/
	public int getLinkCnt(int ctx, int radr) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(radr);
		return rtr.lnkCnt;
	}
	
	/** Get the default RateSpecs for a comtree.
	 *  @param ctx is a valid comtree index
	 *  @param bbRates is a reference to a RateSpec in which the default
	 *  backbone rates are returned
	 *  @param leafRates is a reference to a RateSpec in which the default
	 *  leaf rates are returned
	 */
	public void getDefRates(int ctx, RateSpec bbRates,
					 RateSpec leafRates) {
		bbRates.copyFrom(comtree[ctx].bbDefRates);
		leafRates.copyFrom(comtree[ctx].leafDefRates);
	}
	
	/** Determine if the rates for a comtree backbone link are frozen.
	 *  @param ctx is a valid comtree index
	 *  @param rtrAdr is a forest address for a node in the comtree
	 *  @return true if rtr has a parent and the link to its parent is
	 *  frozen, else false
	 */
	public boolean isFrozen(int ctx, int rtrAdr) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(rtrAdr);
		return rtr.plnk != 0 & rtr.frozen;
	}
	
	/** Get the RateSpec for a comtree link.
	 *  The rates represent the amount of network capacity required for
	 *  this link, given the subtree rates and the backbone configuration
	 *  mode.
	 *  @param ctx is a valid comtree index
	 *  @param fa is a forest address for a node in the comtree
	 *  @param rates is a reference to a rateSpec in which the rates for the
	 *  link from fa to its parent are returned
	 *  @return true on success, false on failure
	 */
	public boolean getLinkRates(int ctx, int fa, RateSpec rates) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(fa);
		if (rtr != null) {
			if (rtr.plnk == 0) return false;
			rates.copyFrom(rtr.plnkRates);
			return true;
		}
		ComtLeafInfo leaf = comtree[ctx].leafMap.get(fa);
		rates.copyFrom(leaf.plnkRates);
		return true;
	}
	
	/** Add a new comtree.
	 *  Defines a new comtree, with attributes left undefined.
	 *  @param comt is the comtree number for the new comtree.
	 *  @return the index of the new comtree on success, else 0
	 */
	public int addComtree(int comt) {
		int ctx = comtreeMap.addPair(comt);
		if (ctx == 0) return 0;
		if (comtree[ctx] == null) comtree[ctx] = new ComtreeInfo();
		comtree[ctx].comtreeNum = comt;
		comtree[ctx].bbDefRates = new RateSpec(0);
		comtree[ctx].leafDefRates = new RateSpec(0);
		comtree[ctx].coreSet = new java.util.HashSet<Integer>();
		comtree[ctx].rtrMap = new HashMap<Integer,ComtRtrInfo>();
		comtree[ctx].leafMap = new HashMap<Integer,ComtLeafInfo>();
		return ctx;
	}
	
	/** Remove a comtree.
	 *  Assumes that all bandwidth resources in underlying network
	 *  have already been released.
	 *  @param comt is the comtree number for the new comtree.
	 *  @return true on success, false on failure
	 */
	public boolean removeComtree(int ctx) {
		if (!validComtIndex(ctx)) return false;
		comtreeMap.dropPair(comtree[ctx].comtreeNum);
		comtree[ctx].bbDefRates = null;
		comtree[ctx].leafDefRates = null;
		comtree[ctx].coreSet = null;
		comtree[ctx].leafMap = null;
		comtree[ctx].rtrMap = null;
		comtree[ctx] = null;
		return true;
	}
	
	/** Add a new node to a comtree.
	 *  @param ctx is the comtree index
	 *  @param fa is a valid forest address
	 *  @return true on success, false on failure
	 */
	public boolean addNode(int ctx, int fa) {
		int nn = net.getNodeNum(fa);
		if (nn != 0 && net.isRouter(nn)) {
			ComtRtrInfo rtr = comtree[ctx].rtrMap.get(fa);
		    	if (rtr != null) return true;
			ComtRtrInfo nu = new ComtRtrInfo();
			comtree[ctx].rtrMap.put(fa,nu);
			return true;
		}
		ComtLeafInfo leaf = comtree[ctx].leafMap.get(fa);
		if (leaf != null) return true;
		ComtLeafInfo nu = new ComtLeafInfo();
		nu.plnkRates.copyFrom(comtree[ctx].leafDefRates);
		if (nn != 0) {
			int plnk = net.firstLinkAt(nn);
			int pnode = net.getPeer(nn,plnk);
			nu.parent = net.getNodeAdr(pnode);
			nu.llnk = net.getLLnum(plnk,pnode);
		}
		comtree[ctx].leafMap.put(fa,nu);
		return true;
	}
	
	/** Remove a node from a comtree.
	 *  This method will fail if the node is a router with links to children
	 *  Updates link counts at parent router.
	 *  @param ctx is the comtree index
	 *  @param fa is the forest address of a comtree node
	 *  @return true on success, false on failure
	 */
	public boolean removeNode(int ctx, int fa) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(fa);
		if (rtr != null) {
			int plnk = rtr.plnk;
			if ((plnk == 0 & rtr.lnkCnt != 0) ||
			    (plnk != 0 & rtr.lnkCnt != 1))
				return false;
			if (plnk != 0) {
				int parent = net.getPeer(
					     	 net.getNodeNum(fa),plnk);
				ComtRtrInfo prtr = comtree[ctx].rtrMap.get(
						   net.getNodeAdr(parent));
				prtr.lnkCnt--;
			}
			comtree[ctx].rtrMap.remove(fa);
			comtree[ctx].coreSet.remove(fa);
			return true;
		}
		ComtLeafInfo leaf = comtree[ctx].leafMap.get(fa);
		ComtRtrInfo prtr = comtree[ctx].rtrMap.get(leaf.parent);
		prtr.lnkCnt--;
		comtree[ctx].leafMap.remove(fa);
		return true;
	}
	
	/** Set the owner of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param owner is the forest address of the comtree owner
	 *  @return true on success, false on failure
	 */
	public boolean setOwner(int ctx, int owner) {
		if (net.getNodeNum(owner) == 0) return false;
		comtree[ctx].owner = owner;
		return true;
	}
	
	/** Set the root node of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param r is the forest address of the router the new comtree root
	 *  @return true on success, false on failure
	 */
	public boolean setRoot(int ctx, int r) {
		if (net.getNodeNum(r) == 0) return false;
		comtree[ctx].root = r;
		return true;
	}
	
	/** get the backbone configuration mode of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @return the backbone configuration mode
	 */
	public boolean getConfigMode(int ctx) {
		return comtree[ctx].autoConfig;
	}
	
	/** Set the backbone configuration mode of a comtree.
	 *  @param ctx is the index of the comtree
	 *  @param auto is the new configuration mode
	 *  @return true on success, false on failure
	 */
	public void setConfigMode(int ctx, boolean autoConfig) {
		comtree[ctx].autoConfig = autoConfig;
	}
	
	/** Set the default RateSpecs for a comtree.
	 *  @param ctx is a valid comtree index
	 *  @param bbRates is a reference to a RateSpec for the new
	 *  backbone rates
	 *  @param leafRates is a reference to a RateSpec for the new
	 *  leaf rates
	 */
	public void setDefRates(int ctx, RateSpec bbRates,
					 RateSpec leafRates) {
		comtree[ctx].bbDefRates.copyFrom(bbRates);
		comtree[ctx].leafDefRates.copyFrom(leafRates);
	}
	
	/** Add a new core node to a comtree.
	 *  The new node is required to be a router and if it is
	 *  not already in the comtree, it is added.
	 *  @param ctx is the comtree index
	 *  @param r is the forest address of the router
	 *  @return true on success, false on failure
	 */
	public boolean addCoreNode(int ctx, int rtrAdr) {
		int rtr = net.getNodeNum(rtrAdr);
		if (!net.isRouter(rtr)) return false;
		if (!isComtRtr(ctx,rtrAdr)) addNode(ctx,rtrAdr);
		comtree[ctx].coreSet.add(rtrAdr);
		return true;
	}
	
	/** Remove a core node from a comtree.
	 *  @param ctx is the comtree index
	 *  @param rtrAdr is the forest address of the core node
	 *  @return true on success, false on failure
	 */
	public boolean removeCoreNode(int ctx, int rtrAdr) {
		comtree[ctx].coreSet.remove(rtrAdr);
		return true;
	}
	
	/** Set the parent link of a router in the comtree.
	 *  Update link counts at router and its parent in the process.
	 *  @param ctx is a valid comtree index
	 *  @param radr is the forest address for a router in the comtree
	 *  @param plnk is the link number of a link in the comtree
	 *  @return true on success, false on failure
	 */
	public boolean setPlink(int ctx, int radr, int plnk) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(radr);
		rtr.plnk = plnk;
		rtr.lnkCnt++;
		if (plnk == 0) return true;
	
		int parent = net.getNodeAdr(
				net.getPeer(net.getNodeNum(radr),plnk));
		rtr = comtree[ctx].rtrMap.get(parent);
		rtr.lnkCnt++;
		return true;
	}
	
	/** Set the parent of a leaf in the comtree.
	 *  @param ctx is a valid comtree index
	 *  @param ladr is the forest address for a leaf in the comtree
	 *  @param parent is the forest address of the parent node in the
	 *  comtree
	 *  @param llnk is the local link number that the parent uses to
	 *  reach ladr
	 *  @return true on success, false on failure
	 */
	public boolean setParent(int ctx, int ladr, int parent, int llnk) {
		ComtLeafInfo leaf = comtree[ctx].leafMap.get(ladr);
		leaf.parent = parent;
		leaf.llnk = llnk;
	
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(parent);
		rtr.lnkCnt++;
		
		return true;
	}
	
	/** Freeze the rate of a comtree link.
	 *  @param ctx is a valid comtree index
	 *  @param radr is a forest address for a router in the comtree;
	 *  this method freezes the rate of the parent link at radr
	 */
	public void freeze(int ctx, int radr) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(radr);
		rtr.frozen = true;
	}
	
	/** Thaw (unfreeze) the rate of a comtree link.
	 *  @param ctx is a valid comtree index
	 *  @param radr is a forest address for a router in the comtree;
	 *  this method unfreezes the rate of the parent link at radr
	 */
	public void thaw(int ctx, int radr) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(radr);
		rtr.frozen = false;
	}
	
	/** Set the allocated RateSpec for a comtree link.
	 *  @param ctx is a valid comtree index
	 *  @param fa is the forest address for a node in the comtree;
	 *  this method sets the rates for the parent link at fa
	 *  @param newRates is the new allocated RateSpec for the link
	 *  @return true on success, false on failure
	 *  (if node has no parent link)
	 */
	public boolean setLinkRates(int ctx, int fa, RateSpec newRates) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(fa);
		if (rtr != null) {
			if (rtr.plnk == 0) return false;
			rtr.plnkRates.copyFrom(newRates);
			return true;
		}
		ComtLeafInfo leaf = comtree[ctx].leafMap.get(fa);
		leaf.plnkRates.copyFrom(newRates);
		return true;
	}
	
	/** Adjust the subtree rates along path to root.
	 *  @param ctx is a valid comtree index
	 *  @param rtr is the forest address of a router in the comtree
	 *  @param rs is a RateSpec that is to be added to the subtree rates
	 *  at all vertices from rtr to the root
	 *  @param return true on success, false on failure
	 */
	boolean adjustSubtreeRates(int ctx, int rtrAdr, RateSpec rs) {
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(rtrAdr);
		int count = 0;
		while (true) {
			rtr.subtreeRates.add(rs);
			if (rtr.plnk == 0) return true;
			int rn = net.getNodeNum(rtrAdr);
			rtrAdr = net.getNodeAdr(net.getPeer(rn,rtr.plnk));
			rtr = comtree[ctx].rtrMap.get(rtrAdr);
			if (count++ > 50) {
				System.err.println("adjustSubtreeRates: " +
					"excessively long path detected in " +
					"comtree " + getComtree(ctx) +
					", probably a cycle");
				return false;
			}
		}
	}
	
	/** Read comtrees from an input stream.
	 *  Comtree file contains one of more comtree definitions, where a
	 *  comtree definition is as shown below
	 *  <code>
	 *  #       num,owner,root,configMode,  backbone rates,    leaf rates
	 *  comtree(1001,netMgr,r1, manual,(1000,2000,1000,2000),(10,50,25,200),
	 *	      (r1,r2,r3),  # core nodes
	 *     		(r1.1,r2.2,(1000,2000,1000,2000)),
	 *     		(r1.2,r3.3,(1000,2000,1000,2000))
	 *  )
	 *  ; # required after last comtree 
	 *  </code>
	 *
	 *  The list of core nodes and the list of links may both be omitted.
	 *  To omit the list of core nodes, while providing the list of links,
	 *  use a pair of commas after the leaf default rates.
	 *  In the list of links, the first named node is assumed to be the 
	 *  child of the other node in the comtree. The RateSpec may be
	 *  omitted, but if provided, the first number is the upstream bit
	 *  rate, followed by downstream bit rate, upstream packet rate
	 *  and downstream packet rate.
	 */
	boolean read(PushbackReader in) {
		int comtNum = 1;	// comtNum = i for i-th comtree read
	
		String word, errMsg;
		while (true) {
			if (!Util.skipBlank(in) || Util.verify(in,';')) break;
			String s;
			if ((s = Util.readWord(in)) == null) {
				System.out.println("NetInfo.read: syntax "
					+ "error: expected (;) or keyword "
					+ "(router,leaf,link)");
				return false;
			}
			if (s.equals("comtree")) {
				errMsg = readComtree(in);
				if (errMsg != null) {
					System.out.println("ComtInfo.read: "
						+ "error when attempting to "
						+ "read " + comtNum
						+ "-th comtree (" + errMsg 
						+ ")");
					return false;
				}
			} else {
				System.out.println("ComtInfo.read: "
					+ "unrecognized word " + s
					+ " when attempting to read "
					+ comtNum + "-th comtree");
				return false;
			}
			comtNum++;
		}
		return check() && setAllComtRates();
	}
	
	/** Read a comtree description from an input stream.
	 *  Expects the next nonblank input character to be the opening
	 *  parenthesis of the comtree description. If a new comtree is read
	 *  without error, proceeds to allocate and initialize the new comtree.
	 *  @param in is an open input stream
	 *  @return a null object reference on success, a reference to an
	 *  error message on error
	 */
	public String readComtree(PushbackReader in) {
		int owner, root, core; boolean autoConfig;
		Vector<Integer> coreNodes = new Vector<Integer>();
		RateSpec bbRates = new RateSpec();
		RateSpec leafRates = new RateSpec();

		if (!Util.verify(in,'(')) {
			return "syntax error, expected left paren";
		}
		// read comtree number
		Util.skipBlank(in);
		Util.MutableInt comt = new Util.MutableInt();
		if (!Util.readNum(in,comt) || !Util.verify(in,',')) {
			return "could not read comtree number";
		}
		// read owner name
		Util.skipBlank(in);
		String s = Util.readWord(in);
		if (s == null || !Util.verify(in,',') ||
		    (owner = net.getNodeNum(s)) == 0) {
			return "could not read owner name";
		}
		// read root node name
		Util.skipBlank(in);
		s = Util.readWord(in);
		if (s == null || !Util.verify(in,',') ||
		    (root = net.getNodeNum(s)) == 0) {
			return "could not read root node name";
		}
		// read backbone configuration mode
		Util.skipBlank(in);
		if ((s = Util.readWord(in)) == null) {
			return "could not read backbone configuration mode";
		}
		if (s.equals("auto")) autoConfig = true;
		else if (s.equals("manual")) autoConfig = false;
		else {
			return "invalid backbone configuration mode " + s;
		}
		// read default rate specs
		Util.skipBlank(in);
		if (!Util.verify(in,',') || !readRateSpec(in,bbRates)) {
			return "could not backbone default rates";
		}
		Util.skipBlank(in);
		if (!Util.verify(in,',') || !readRateSpec(in,leafRates)) {
			return "could not read leaf default rates";
		}
		// read list of core nodes (may be empty)
		Util.skipBlank(in);
		if (Util.verify(in,',')) { // read list of additional core nodes
			Util.skipBlank(in);
			if (Util.verify(in,'(')) {
				if (!Util.verify(in,')')) {
					while (true) {
						Util.skipBlank(in);
						s = Util.readWord(in);
						if (s == null) {
							return "could not read "
							    + "core node name";
						}
				    		core = net.getNodeNum(s);
						if (core == 0) {
							return "invalid core "
							    + "node name " + s;
						}
						coreNodes.add(core);
						if (Util.verify(in,')')) break;
						if (!Util.verify(in,',')) {
							return "syntax error "
							    + "in list of core "
							    + "nodes after "
							    + s;
						}
					}
				}
			}
		}
		// read list of links (may be empty)
		Vector<LinkMod> links = new Vector<LinkMod>();
		LinkModX lm = null;
		if (Util.verify(in,',')) {
			int lnk; int child;
			Util.skipBlank(in);
			lm = readLink(in);
			if (lm.errMsg != null) return lm.errMsg;
			links.add(lm);
			while (Util.verify(in,',')) {
				Util.skipBlank(in);
				lm = readLink(in);
				if (lm.errMsg != null) return lm.errMsg;
				links.add(lm);
			}
		}
		Util.skipBlank(in);
		if (!Util.verify(in,')')) { 
			return "syntax error at end of link list, expected "
				+ "right paren";
		}
		int ctx;
		if ((ctx = addComtree(comt.val)) == 0) {
			return "could not allocate new comtree";
		}
		
		int ownerAdr = net.getNodeAdr(owner);
		int rootAdr  = net.getNodeAdr(root);

		// configure comtree
		if (!setOwner(ctx,ownerAdr) || !setRoot(ctx,rootAdr)) {
			return "could not configure comtree";
		}
		setConfigMode(ctx,autoConfig);
		setDefRates(ctx,bbRates,leafRates);
	
		if (!addNode(ctx,rootAdr) || !addCoreNode(ctx,rootAdr)) {
			return "could not add root to comtree";
		}
		for (int i = 0; i < coreNodes.size(); i++) {
			int cnAdr = net.getNodeAdr(coreNodes.get(i));
			if (!addNode(ctx,cnAdr) || !addCoreNode(ctx,cnAdr)) {
				return "could not add core node to comtree";
			}
		}
	
		for (int i = 0; i < links.size(); i++) {
			int lnk = links.get(i).lnk;
			int child = links.get(i).child;
			int parent = net.getPeer(child,lnk);
			int childAdr = net.getNodeAdr(child);
			int parentAdr = net.getNodeAdr(parent);
			RateSpec rs = links.get(i).rs;
	
			addNode(ctx,childAdr); addNode(ctx,parentAdr);
			if (net.isLeaf(child)) {
				ComtLeafInfo leaf;
				leaf = comtree[ctx].leafMap.get(childAdr);
				if (rs.bitRateUp == 0) rs = leafRates;
				leaf.plnkRates.copyFrom(rs);
			} else {
				ComtRtrInfo rtr;
				rtr = comtree[ctx].rtrMap.get(childAdr);
				rtr.plnk = lnk; rtr.lnkCnt++;
				if (rs.bitRateUp == 0) rs = bbRates;
				else rtr.frozen = true;
				rtr.plnkRates.copyFrom(rs);
				rs = rtr.subtreeRates;
			}
			adjustSubtreeRates(ctx,parentAdr,rs);
			comtree[ctx].rtrMap.get(parentAdr).lnkCnt++;
		}
		return null;
	}

	/** Read a comtree status string from an input stream.
	 *  Expects the next nonblank input character to be the opening
	 *  parenthesis of the comtree description. If the status is read
	 *  without error, it modifies the specified comtree to match.
	 *  @param in is an open input stream
	 *  @return the comtree number of the comtree read on success,
	 *  0 on error
	 */
	public int readComtStatus(PushbackReader in) {
		int owner, root, core; boolean autoConfig;
		RateSpec bbRates = new RateSpec();
		RateSpec leafRates = new RateSpec();

		Util.skipBlank(in);
		String s = Util.readWord(in);
		if (s == null || !s.equals("comtree") || !Util.verify(in,'('))
			return 0;
		// read comtree number
		Util.skipBlank(in);
		Util.MutableInt comt = new Util.MutableInt();
		if (!Util.readNum(in,comt) || !Util.verify(in,','))
			return 0;
		// read owner name
		Util.skipBlank(in);
		s = Util.readWord(in);
		if (s == null || !Util.verify(in,',') ||
		    (owner = net.getNodeNum(s)) == 0) 
			return 0;
		// read root node name
		Util.skipBlank(in);
		s = Util.readWord(in);
		if (s == null || !Util.verify(in,',') ||
		    (root = net.getNodeNum(s)) == 0) 
			return 0;
		// read backbone configuration mode
		Util.skipBlank(in);
		if ((s = Util.readWord(in)) == null) return 0;
		if (s.equals("auto")) autoConfig = true;
		else if (s.equals("manual")) autoConfig = false;
		else return 0;
		// read default rate specs
		Util.skipBlank(in);
		if (!Util.verify(in,',') || !readRateSpec(in,bbRates))
			return 0;
		Util.skipBlank(in);
		if (!Util.verify(in,',') || !readRateSpec(in,leafRates))
			return 0;
		// read list of core nodes (may be empty)
		java.util.HashSet<Integer> coreNodes =
			new java.util.HashSet<Integer>();
		Util.skipBlank(in);
		if (Util.verify(in,',')) { // read list of additional core nodes
			Util.skipBlank(in);
			if (Util.verify(in,'(')) {
				if (!Util.verify(in,')')) {
					while (true) {
						Util.skipBlank(in);
						s = Util.readWord(in);
						if (s == null) return 0;
				    		core = net.getNodeNum(s);
						if (core == 0) return 0;
						int cadr = net.getNodeAdr(core);
						coreNodes.add(cadr);
						if (Util.verify(in,')')) break;
						if (!Util.verify(in,','))
							return 0;
					}
				}
			}
		}
		// read list of nodes
		java.util.HashSet<NodeDesc> nodeSet =
			new java.util.HashSet<NodeDesc>();
		java.util.HashSet<Integer> nodeAdrSet =
			new java.util.HashSet<Integer>();
		if (Util.verify(in,',')) {
			Util.skipBlank(in);
			NodeDesc nd = new NodeDesc();
			if (!readNodeStatus(in,nd)) return 0;
			nodeSet.add(nd);
			nodeAdrSet.add(net.getNodeAdr(nd.child));
			while (Util.verify(in,',')) {
				Util.skipBlank(in);
				nd = new NodeDesc();
				if (!readNodeStatus(in,nd)) return 0;
				nodeSet.add(nd);
				nodeAdrSet.add(net.getNodeAdr(nd.child));
			}
		}
		Util.skipBlank(in);
		if (!Util.verify(in,')')) return 0;
		int ctx;
		if ((ctx = getComtIndex(comt.val)) == 0 &&
		    (ctx = addComtree(comt.val)) == 0)
			return 0;
		
		int ownerAdr = net.getNodeAdr(owner);
		int rootAdr  = net.getNodeAdr(root);

		// configure comtree
		if (!setOwner(ctx,ownerAdr) || !setRoot(ctx,rootAdr)) {
			return 0;
		}
		setConfigMode(ctx,autoConfig);
		setDefRates(ctx,bbRates,leafRates);

		// remove from comtree, routers that are not in nodeAdrSet
		// then add new nodes
		java.util.HashSet<Integer> dropSet =
			new java.util.HashSet<Integer>();
		for (int radr : comtree[ctx].rtrMap.keySet())
			if (!nodeAdrSet.contains(radr)) dropSet.add(radr);
		for (int ladr : comtree[ctx].leafMap.keySet())
			if (!nodeAdrSet.contains(ladr)) dropSet.add(ladr);
		for (int fadr : dropSet) {
			comtree[ctx].rtrMap.remove(fadr);
			comtree[ctx].leafMap.remove(fadr);
		}
		for (int fadr : nodeAdrSet)
			if (!isComtNode(ctx,fadr)) addNode(ctx,fadr);

		// remove core nodes that are not in coreNodes
		dropSet.clear();
		for (int radr : comtree[ctx].coreSet)
			if (radr != rootAdr && !coreNodes.contains(radr))
				dropSet.add(radr);
		for (int radr : dropSet)
			comtree[ctx].coreSet.remove(radr);

		if (!addCoreNode(ctx,rootAdr)) return 0;
		for (int cadr : coreNodes) {
			if (!addCoreNode(ctx,cadr)) return 0;
		}

		for (NodeDesc nd : nodeSet) {
			int childAdr = net.getNodeAdr(nd.child);
			if (isComtLeaf(ctx,childAdr)) {
				ComtLeafInfo leaf;
				leaf = comtree[ctx].leafMap.get(childAdr);
				leaf.plnkRates.copyFrom(nd.rs);
			} else {
				ComtRtrInfo rtr;
				rtr = comtree[ctx].rtrMap.get(childAdr);
				if (nd.child == root) {
					rtr.plnk = 0;
				} else {
					rtr.plnk = nd.lnk; 
					rtr.plnkRates.copyFrom(nd.rs);
				}
				rtr.lnkCnt = nd.lnkCnt;
			}
		}
		return comt.val;
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

	/** Read a link description from an input stream.
	 *  Expects the next nonblank input character to be the opening
	 *  parenthesis.
	 *  @param in is a PushbackReader
	 *  @return a reference to a LinkModX structure; on success, the
	 *  the errMsg reference is equal to null; on failure, it refers
	 *  to an error message
	 */
	public LinkModX readLink(PushbackReader in) {
		LinkModX lm = new LinkModX();
		lm.errMsg = null;
		// read the link endpoints
		LinkEndpoint left, right;
		if (!Util.verify(in,'(') ||
		    (left = readLinkEndpoint(in)) == null ||
		    !Util.verify(in,',')) {
			lm.errMsg = "could not read first link endpoint";
			return lm;
		}
		if ((right = readLinkEndpoint(in)) == null) {
			lm.errMsg = "could not read second link endpoint";
			return lm;
		}
		// find the link number in the NetInfo object
		if ((lm.child = net.getNodeNum(left.name)) == 0) {
			lm.errMsg = "invalid name for link endpoint "
				    + left.name;
			return lm;
		}
		int parent;
		if ((parent = net.getNodeNum(right.name)) == 0) {
			lm.errMsg = "invalid name for link endpoint "
				    + right.name;
			return lm;
		}
		if (!net.isRouter(parent)) {
			lm.errMsg = "invalid link: first node must be child "
				    + "in comtree";
			return lm;
		}
		lm.lnk = net.getLinkNum(lm.child,left.num);
		if (lm.lnk != net.getLinkNum(parent,right.num) || lm.lnk == 0) {
			lm.errMsg = "detected invalid link (" + left.name;
			if (left.num != 0) lm.errMsg += "." + left.num;
			lm.errMsg += "," + right.name;
			if (right.num != 0) lm.errMsg += "." + right.num;
			lm.errMsg += ")";
			return lm;
		}
		if (Util.verify(in,',')) {
			if (!readRateSpec(in,lm.rs)) {
				lm.errMsg = "could not read rate specification "
					    + "for link";
				return lm;
			}
		}
		if (!Util.verify(in,')')) {
			lm.errMsg = "syntax error, expected right paren";
			return lm;
		}
		return lm;
	}
	/** Read node status from an input stream.
	 *  @param in is a PushbackReader
	 *  @param nd is a NodeDesc object in which result is returned
	 *  @return true on success, false on failure.
	 */
	public boolean readNodeStatus(PushbackReader in, NodeDesc nd) {
		// read the link endpoints
		LinkEndpoint left, right;
		if (!Util.verify(in,'(') ||
		    (left = readLinkEndpoint(in)) == null ||
		    !Util.verify(in,','))
			return false;
		// find the link number in the NetInfo object
		if ((nd.child = net.getNodeNum(left.name)) == 0) 
			return false;
		Util.MutableInt lc = new Util.MutableInt();
		if (Util.readNum(in,lc)) { // special case of root
			nd.lnkCnt = lc.val; nd.lnk = 0;
			if (!Util.verify(in,')')) return false;
			return true;
		}
		if ((right = readLinkEndpoint(in)) == null)
			return false;
		int parent;
		if ((parent = net.getNodeNum(right.name)) == 0)
			return false;
		if (!net.isRouter(parent)) return false;
		nd.lnk = net.getLinkNum(nd.child,left.num);
		if (nd.lnk != net.getLinkNum(parent,right.num) || nd.lnk == 0)
			return false;
		if (!Util.verify(in,',') || !readRateSpec(in,nd.rs))
			return false;
		if (Util.verify(in,')')) { // no link count provided, default=0
			nd.lnkCnt = 0; return true;
		}
		if (!Util.verify(in,',') || !Util.readNum(in,lc)) return false;
		nd.lnkCnt = lc.val;
		if (!Util.verify(in,')')) return false;
		return true;
	}

	/** Read a link endpoint.
	 *  @param in is an open input stream
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

	/** Perform consistency check on all comtrees.
	 *  @return true if all comtrees pass all checks, else false
	 */
	public boolean check() {
		boolean status = true;
		for (int ctx = firstComtIndex(); ctx != 0;
			 ctx = nextComtIndex(ctx)) {
			int comt = getComtree(ctx);
			int rootAdr = getRoot(ctx);
			int root = net.getNodeNum(rootAdr);
	
			// first check that every leaf in comtree has a parent
			// that is a router in the comtree
			for (int leafAdr: comtree[ctx].leafMap.keySet()) {
				int padr = getParent(ctx,leafAdr);
				if (!isComtRtr(ctx,padr)) {
					int leaf = net.getNodeNum(leafAdr);
					String leafName;
					leafName = (leaf != 0 ?
						   net.getNodeName(leaf) :
						   Forest.fAdr2string(leafAdr));
					System.err.println("check: comtree "
						+ comt + " has leaf "
						+ leafName + " whose parent "
						+ "is not a router in comtree");
					status = false;
				}
			}
			// next, check that at most one router in comtree 
			// lacks a parent
			int cnt = 0;
			for (int rtrAdr : comtree[ctx].rtrMap.keySet()) {
				if (getParent(ctx,rtrAdr) == 0) cnt++;
			}
			if (cnt != 1) {
				System.err.println("check: comtree " + comt 
				     + " has " + cnt + " routers with no "
				     + "parent");
				status = false;
			}
	
			// check that the comtree backbone topology is really
			// a tree by doing a breadth-first search from the root;
			// while we're at it, make sure the parent of every
			// core node is a core node and that the zip codes
			// of routers within the comtree are contiguous
			LinkedList<Integer> pending = new LinkedList<Integer>();
			pending.add(root);
			HashMap<Integer,Integer> plink =
				new HashMap<Integer,Integer>();
			plink.put(root,0);
			// zipSet contains zipcodes we've seen in comt
			java.util.HashSet<Integer> zipSet;
			zipSet = new java.util.HashSet<Integer>();
			zipSet.add(Forest.zipCode(rootAdr));
			int nodeCount = 0;
			while (!pending.isEmpty()) {
				int u = pending.getFirst();
				pending.removeFirst();
				int uAdr = net.getNodeAdr(u);
				nodeCount++;
				boolean foundCycle = false;
				int uzip = Forest.zipCode(net.getNodeAdr(u));
				for (int lnk = net.firstLinkAt(u); lnk != 0;
					 lnk = net.nextLinkAt(u,lnk)) {
					int v = net.getPeer(u,lnk);
					if (!net.isRouter(v)) continue;
					int vAdr = net.getNodeAdr(v);
					if (!isComtRtr(ctx,vAdr)) continue;
					if (getPlink(ctx,vAdr) != lnk) continue;
					if (lnk == plink.get(u)) continue;
					int vzip = Forest.zipCode(vAdr);
					if (plink.get(v) != null) {
						System.err.println(
						    "checkComtrees: comtree "
						    + comt + " contains a "
						    + "cycle");
						foundCycle = true;
						break;
					}
					plink.put(v,lnk);
					pending.add(v);
					// check that if v is in core, so is u
					if (isCoreNode(ctx,vAdr) &&
					    !isCoreNode(ctx,uAdr)) {
						System.err.println("check: "
						    + "comtree "
						    + comt + " contains a core "
						    + "node "
						    + net.getNodeName(v)
						    + " whose parent is not a "
						    + "core node");
						status = false;
					}
					// now check that if v has a different
					// zip code than u, that we haven't
					// already seen this zip
					if (vzip != uzip) {
						if (zipSet.contains(vzip)) {
							System.err.println(
							    "check: zip "
							    + "code " + vzip
							    + " is non-"
							    + "contiguous in "
							    + "comtree " + comt
							    + ")");
							status = false;
						} else {
							zipSet.add(vzip);
						}
					}
				}
				if (foundCycle) { status = false; break; }
			}
			if (nodeCount != comtree[ctx].rtrMap.size()) {
				System.err.println("check: comtree " + comt
				     + " not connected");
				status = false;
			}
		}
		return status;
	}
	
	/** Set rates for links in all comtrees.
	 *  This involves computing the required rate for each comtree link
	 *  and allocating the required network capacity.
	 *  @return true if all comtrees configured successfully, else false
	 */
	boolean setAllComtRates() {
		for (int ctx = firstComtIndex(); ctx != 0;
			 ctx = nextComtIndex(ctx)) {
			if (!setComtRates(ctx)) return false;
		}
		return true;
	}
	
	/** Compute rates for all links in a comtree and allocate capacity of
	 *  network links.
	 *  @param ctx is a valid comtree index
	 *  @return true on success, false on failure
	 */
	boolean setComtRates(int ctx) {
		if (!validComtIndex(ctx)) return false;
		if (getConfigMode(ctx)) setAutoConfigRates(ctx);
		if (!checkComtRates(ctx)) {
			System.err.println("network lacks capacity for comtree "
			     + getComtree(ctx) + "\n");
			return false;
		}
		provision(ctx);
		return true;
	}
	
	/** Set the backbone link capacities for an auto-configured comtree.
	 *  @param ctx is a valid comtree index
	 *  @param u is a node in the comtree (used in recursive calls)
	 *  @return true on success, else false
	 */
	public void setAutoConfigRates(int ctx) {
		RateSpec rootRates;
		rootRates = comtree[ctx].rtrMap.get(getRoot(ctx)).subtreeRates;
		for (int radr : comtree[ctx].rtrMap.keySet()) {
			ComtRtrInfo rtr = comtree[ctx].rtrMap.get(radr);
			if (rtr.frozen) continue;
			int lnk = rtr.plnk;
			if (lnk == 0) continue;
			RateSpec rs = rtr.plnkRates;
			RateSpec srates = rtr.subtreeRates;
			RateSpec trates = new RateSpec(rootRates);
			trates.subtract(srates);
			if (isCoreNode(ctx,radr)) {
				rs.set(	srates.bitRateUp,trates.bitRateUp,
					srates.pktRateUp,trates.pktRateUp);
			} else {		
				rs.set(srates.bitRateUp,
				    Math.min(srates.bitRateDown,
						trates.bitRateUp),
				    srates.pktRateUp,
				    Math.min(srates.pktRateDown,
						trates.pktRateUp));
			}
		}
		return;
	}
	
	/** Check that there is sufficient capacity available for all links in a
	 *  comtree. Requires that rates have been set for all comtree links.
	 *  @param ctx is a valid comtree index
	 *  @param rootRates is a reference to the subtree rates at the root
	 *  @return true if the provisioned rates for the comtree are compatible
	 *  with the available capacity
	 */
	boolean checkComtRates(int ctx) {
		RateSpec availRates = new RateSpec(0);
		// first check parent links at routers
		for (int radr : comtree[ctx].rtrMap.keySet()) {
			ComtRtrInfo rtr = comtree[ctx].rtrMap.get(radr);
			int lnk = rtr.plnk;
			RateSpec rs = new RateSpec(rtr.plnkRates);
			if (net.getNodeNum(radr) != net.getLeft(lnk)) rs.flip();
			net.getAvailRates(lnk,availRates);
			if (!rs.leq(availRates)) return false;
		}
		// then check access links, for static leaf nodes
		for (int ladr : comtree[ctx].leafMap.keySet()) {
			ComtLeafInfo leaf = comtree[ctx].leafMap.get(ladr);
			int lnum = net.getNodeNum(ladr);
			if (lnum == 0) continue;
			int lnk = net.firstLinkAt(lnum);
			RateSpec rs = new RateSpec(leaf.plnkRates);
			if (lnum != net.getLeft(lnk)) rs.flip();
			net.getAvailRates(lnk,availRates);
			if (!rs.leq(availRates)) return false;
		}
		return true;
	}
	
	/** Provision all links in a comtree, reducing available link capacity.
	 *  Assumes that the required network capacity is available and that
	 *  comtree links are configured with required rates.
	 *  @param ctx is a valid comtree index
	 *  @return true on success, else false
	 */
	void provision(int ctx) {
		RateSpec availRates = new RateSpec(0);
		for (int radr : comtree[ctx].rtrMap.keySet()) {
			ComtRtrInfo rtr = comtree[ctx].rtrMap.get(radr);
			int lnk = rtr.plnk;
			if (lnk == 0) continue;
			RateSpec rs = new RateSpec(rtr.plnkRates);
			if (net.getNodeNum(radr) != net.getLeft(lnk)) rs.flip();
			net.getAvailRates(lnk,availRates);
			availRates.subtract(rs);
			net.setAvailRates(lnk,availRates);
		}
		// provision statically configured leaf nodes
		for (int ladr : comtree[ctx].leafMap.keySet()) {
			ComtLeafInfo leaf = comtree[ctx].leafMap.get(ladr);
			int lnum = net.getNodeNum(ladr);
			if (lnum == 0) continue;
			int lnk = net.firstLinkAt(lnum);
			RateSpec rs = new RateSpec(leaf.plnkRates);
			if (lnum != net.getLeft(lnk)) rs.flip();
			net.getAvailRates(lnk,availRates);
			availRates.subtract(rs);
			net.setAvailRates(lnk,availRates);
		}
	}
	
	/** Unprovision all links in a comtree, increasing available link
	 *  capacity.
	 *  @param ctx is a valid comtree index
	 *  @return true on success, else false
	 */
	void unprovision(int ctx) {
		RateSpec availRates = new RateSpec();
		for (int radr : comtree[ctx].rtrMap.keySet()) {
			ComtRtrInfo rtr = comtree[ctx].rtrMap.get(radr);
			int lnk = rtr.plnk; 
			if (lnk == 0) continue;
			RateSpec rs = new RateSpec(rtr.plnkRates);
			if (net.getNodeNum(radr) != net.getLeft(lnk)) rs.flip();
			net.getAvailRates(lnk,availRates);
			availRates.add(rs);
			net.setAvailRates(lnk,availRates);
		}
		// provision statically configured leaf nodes
		for (int ladr : comtree[ctx].leafMap.keySet()) {
			ComtLeafInfo leaf = comtree[ctx].leafMap.get(ladr);
			int lnum = net.getNodeNum(ladr);
			if (lnum == 0) continue;
			int lnk = net.firstLinkAt(lnum);
			RateSpec rs = new RateSpec(leaf.plnkRates);
			if (lnum != net.getLeft(lnk)) rs.flip();
			net.getAvailRates(lnk,availRates);
			availRates.add(rs);
			net.setAvailRates(lnk,availRates);
		}
	}
	
	/** Find a path from a vertex to a comtree.
	 *  This method builds a shortest path tree with a given source node.
	 *  The search is done over all network links that have the available
	 *  capacity to handle a provided RateSpec.
	 *  The search halts when it reaches any node in the comtree.
	 *  The path from source to this node is returned in path.
	 *  @param ctx is a valid comtree index for the comtree of interest
	 *  @param src is the router at which the path search starts
	 *  @param rs specifies the rates required on the links in the path
	 *  @param path is a reference to a list of LinkMod objects which
	 *  define the path when the method returns; the links are ordered
	 *  "bottom-up:.
	 *  @return the router number of the "branchRouter" at which the
	 *  path ends, or 0 if no path is found
	 */
	int findPath(int ctx, int src, RateSpec rs, LinkedList<LinkMod> path) {
		path.clear();
		int srcAdr = net.getNodeAdr(src);
		if (isComtNode(ctx,srcAdr)) return src;
		int n = 0;
		for (int r = net.firstRouter(); r != 0; r = net.nextRouter(r)) {
			n = Math.max(r,n);
		}
		Heap h = new Heap(n);
		int [] d = new int[n+1];
		int [] plnk = new int[n+1]; // note: reverse from direction
					    // in comtree
		for (int r = net.firstRouter(); r != 0; r = net.nextRouter(r)) {
			plnk[r] = 0; d[r] = Util.BIGINT;
		}
		RateSpec availRates = new RateSpec(0);
		d[src] = 0;
		h.insert(src,d[src]);
		while (!h.empty()) {
			int r = h.deletemin();
			for (int lnk = net.firstLinkAt(r); lnk != 0;
				 lnk = net.nextLinkAt(r,lnk)) {
				if (lnk == plnk[r]) continue;
				int peer = net.getPeer(r,lnk);
				if (!net.isRouter(peer)) continue;
				// if this link cannot take the default backbone
				// rate for this comtree, ignore it
				net.getAvailRates(lnk,availRates);
				if (r != net.getLeft(lnk)) availRates.flip();
				if (!rs.leq(availRates)) continue;
				if (isComtNode(ctx,net.getNodeAdr(peer))) {
					// done
					plnk[peer] = lnk;
					int u = peer;
					for (int pl = plnk[u]; pl != 0;
						 pl = plnk[u]) {
						int v = net.getPeer(u,plnk[u]);
						LinkMod lm;
						lm = new LinkMod(pl,v,rs);
						path.addFirst(lm);
						u = v;
					}
					return peer;
				}
				if (d[peer] > d[r] + net.getLinkLength(lnk)) {
					plnk[peer] = lnk;
					d[peer] = d[r] + net.getLinkLength(lnk);
					if (h.member(peer))
						h.changekey(peer,d[peer]);
					else h.insert(peer,d[peer]);
				}
			}
		}
		return 0;
	}
	
	/** Add a path from a router to a comtree.
	 *  This method adds a list of links to the backbone of an existing
	 *  comtree, sets their link rates and reserves capacity on the
	 *  network links; the links are assumed to have sufficient available
	 *  capacity.
	 *  @param ctx is a valid comtree index for the comtree of interest
	 *  @param path is a reference to a list of LinkMod objects defining
	 *  a path in the comtree; the links appear in "bottom-up" order
	 *  leading from some router outside the comtree to a router
	 *  in the comtree
	 */
	void addPath(int ctx, LinkedList<LinkMod> path) {
		if (path.isEmpty()) return;
		RateSpec availRates = new RateSpec(0);
		for (LinkMod lm : path) {
			int child = lm.child; int lnk = lm.lnk;
			RateSpec rs = lm.rs;
			int parent = net.getPeer(child,lnk);
			int childAdr = net.getNodeAdr(child);
			int parentAdr = net.getNodeAdr(parent);
			addNode(ctx,childAdr); addNode(ctx,parentAdr);
			setPlink(ctx,childAdr,lnk); thaw(ctx,childAdr);
			setLinkRates(ctx,childAdr,rs);
			if (child != net.getLeft(lnk)) rs.flip();
			net.getAvailRates(lnk,availRates);
			availRates.subtract(rs);
			net.setAvailRates(lnk,availRates);
		}
		return;
	}
	
	/** Remove a path from a router in a comtree.
	 *  @param ctx is a valid comtree index for the comtree of interest
	 *  @param path is a list of LinkMod objects defining the path
	 *  in bottom-up order; the path is assumed to have
	 *  no "branching links" incident to the path
	 */
	void removePath(int ctx, LinkedList<LinkMod> path) {
		if (path.isEmpty()) return;
		RateSpec availRates = new RateSpec(0);
		for (LinkMod lm : path) {
			removeNode(ctx,lm.child);
			RateSpec rs = lm.rs;
			if (lm.child != net.getLeft(lm.lnk)) rs.flip();
			net.getAvailRates(lm.lnk,availRates);
			availRates.add(rs);
			net.setAvailRates(lm.lnk,availRates);
		}
		return;
	}
	
	boolean computeMods(int ctx, LinkedList<LinkMod> modList) {
		int rootAdr = getRoot(ctx);
		RateSpec rs = comtree[ctx].rtrMap.get(rootAdr).subtreeRates;
		return computeMods(ctx,rootAdr,rs,modList);
	}
	
	/** Compute the change required in the RateSpecs for links in an
	 *  auto-configured comtree and verify that needed capacity is
	 *  available.
	 *  @param ctx is the comtree index of the comtree
	 *  @param radr is the Forest address of the root of the current
	 *  subtree; should be the comtree root in the top level recursive call
	 *  @param modList is a list of edges in the comtree and RateSpecs
	 *  representing the change in rates required at each link in the list;
	 *  returned RateSpecs may have negative values; list should be empty 
	 *  in top level recursive call
	 *  @return true if all links in comtree have sufficient available
	 *  capacity to accommodate the changes
	 */
	boolean computeMods(int ctx, int radr, RateSpec rootRates,
				     LinkedList<LinkMod> modList) {
		int rnum = net.getNodeNum(radr);
		if (!net.isRouter(rnum)) return true;
		ComtRtrInfo rtr = comtree[ctx].rtrMap.get(radr);
		int plnk = rtr.plnk;
		if (plnk != 0 & !isFrozen(ctx,plnk)) {
			// determine the required rates on link to parent
			LinkMod nuMod = new LinkMod();
			nuMod.child = rnum; nuMod.lnk = plnk;
			RateSpec srates = rtr.subtreeRates;
			RateSpec trates = rootRates; trates.subtract(srates);
			if (isCoreNode(ctx,radr)) {
				nuMod.rs.set(
					srates.bitRateUp,trates.bitRateUp,
					srates.pktRateUp,trates.pktRateUp);
			} else {		
				nuMod.rs.set(srates.bitRateUp,
				    Math.min(srates.bitRateDown,
						trates.bitRateUp),
				    srates.pktRateUp,
				    Math.min(srates.pktRateDown,
						trates.pktRateUp));
			}
			nuMod.rs.subtract(rtr.plnkRates);
			if (nuMod.rs.isZero()) return true; // no change needed
			RateSpec availRates = new RateSpec(0);
			net.getAvailRates(plnk,availRates);
			if (rnum != net.getLeft(plnk)) availRates.flip();
			if (!nuMod.rs.leq(availRates)) return false;
			modList.add(nuMod);
		}
	
		// Now, do subtrees; note that this method is going to be
		// slow for comtrees with very large backbones; only way to
		// fix it is to add child and sibling pointers; later, maybe
		for (int childAdr : comtree[ctx].rtrMap.keySet()) {
			if (radr != getParent(ctx,childAdr)) continue;
			if (!computeMods(ctx,childAdr,rootRates,modList))
				return false;
		}
		return true;
	}
	
	/** Add to the capacity of a list of comtree backbone links and
	 *  provision required link capacity in the network. Assumes that
	 *  network links have the required capacity and that both endpoints
	 *  of each link are routers.
	 *  @param ctx is the index of a comptree
	 *  @param modList is a list of link modifications; that is, comtree
	 *  links that are either to be added or have their rate adjusted.
	 *  and a RateSpec difference to be added to that link.
	 */
	void provision(int ctx, LinkedList<LinkMod> modList) {
		RateSpec delta = new RateSpec(0);
		RateSpec availRates = new RateSpec(0);
		for (LinkMod lm : modList) {
			int rtr = lm.child; int rtrAdr = net.getNodeAdr(rtr);
			int plnk = lm.lnk; delta.copyFrom(lm.rs);
			comtree[ctx].rtrMap.get(rtrAdr).plnkRates.add(delta);
			net.getAvailRates(plnk,availRates);
			if (rtr != net.getLeft(plnk)) delta.flip();
			availRates.subtract(delta);
			net.setAvailRates(plnk,availRates);
		}
	}
	
	/** Reduce the capacity of a list of comtree links and release required
	 *  link capacity in the network.
	 *  @param ctx is the index of a comptree
	 *  @param modList is a list of nodes, each representing a link in
	 *  the comtree and a RateSpec difference to be removed from that link.
	 */
	void unprovision(int ctx, LinkedList<LinkMod> modList) {
		RateSpec delta = new RateSpec(0);
		RateSpec availRates = new RateSpec(0);
		for (LinkMod lm : modList) {
			int rtr = lm.child; int rtrAdr = net.getNodeAdr(rtr);
			int plnk = lm.lnk; delta.copyFrom(lm.rs);
			comtree[ctx].rtrMap.get(rtrAdr).plnkRates.add(delta);
			net.getAvailRates(plnk,availRates);
			if (rtr != net.getLeft(plnk)) delta.flip();
			availRates.add(delta);
			net.setAvailRates(plnk,availRates);
		}
	}
	
	/** Create a String representation of the ComtInfo object.
	 *  @param s is a reference to a String in which the result is returned
	 *  @return a reference to s
	 */
	public String toString() {
		String s = "";
		for (int ctx = firstComtIndex(); ctx != 0;
			 ctx = nextComtIndex(ctx)) {
			s += comt2string(ctx);
		}
		return s + ";\n"; 
	}
	
	/** Create a String representation of a comtree link, including
	 *  RateSpec.
	 *  @param ctx is the comtree index for a comtree
	 *  @param lnk is a link number for a link in the comtree
	 *  @param s is a reference to a String in which result is returned
	 *  @return a reference to s
	 */
	public String link2string(int ctx, int lnk) {
		String s;
		int childAdr = getChild(ctx,lnk);
		int child = net.getNodeNum(childAdr);
		int parent = net.getPeer(child,lnk);
		s = "(" + net.getNodeName(child);
		if (net.isRouter(child))
			s += "." + net.getLLnum(lnk,child);
		s += "," + net.getNodeName(parent) + "."
		     	 + net.getLLnum(lnk,parent);
		RateSpec rs;
		if (net.isRouter(child)) {
			rs = comtree[ctx].rtrMap.get(childAdr).plnkRates;
		} else {
			rs = comtree[ctx].leafMap.get(childAdr).plnkRates;
		}
		s += "," + rs + ")";
		return s;
	}
	
	/** Create a String representation of a parent link for a leaf.
	 *  @param ctx is the comtree index for a comtree
	 *  @param leafAdr is a forest address of a leaf node
	 *  @param s is a reference to a String in which result is returned
	 *  @return a reference to s
	 */
	public String leafLink2string(int ctx, int leafAdr) {
		int lnum = net.getNodeNum(leafAdr);
		if (lnum != 0) return link2string(ctx,net.firstLinkAt(lnum));
	
		ComtLeafInfo leaf = comtree[ctx].leafMap.get(leafAdr);
		int parent = net.getNodeNum(leaf.parent);
		String s;
		s = "(" + Forest.fAdr2string(leafAdr) + ","
			+ net.getNodeName(parent) + "." + leaf.llnk;
		s += "," + leaf.plnkRates + ")";
		return s;
	}
	
	/** Create a String representation of a comtree.
	 *  @param ctx is the comtree index for a comtree
	 *  @param s is a reference to a String in which result is returned
	 *  @return a reference to s
	 */
	public String comt2string(int ctx) {
		String s = "";
		if (!validComtIndex(ctx)) return s;
		s = "comtree(" + getComtree(ctx) + ","
		  	+ net.getNodeName(net.getNodeNum(getOwner(ctx)))
			+ "," + net.getNodeName(net.getNodeNum(getRoot(ctx)))
			+ "," + (getConfigMode(ctx) ? "auto" : "manual") + ",";
		s += comtree[ctx].bbDefRates + "," + comtree[ctx].leafDefRates;
		int siz = comtree[ctx].rtrMap.size()
			+ comtree[ctx].leafMap.size();
		if (siz <= 1) {
			s += ")\n"; return s;
		} else if (comtree[ctx].coreSet.size() > 1) {
			s += ",\n\t(";
			// iterate through core nodes and print
			boolean first = true;
			for (int c: getCoreSet(ctx)) {
				if (c == getRoot(ctx)) continue;
				if (first) first = false;
				else	   s += ",";
				s += net.getNodeName(net.getNodeNum(c));
			}
			s += ")";
		} else {
			s += ",";
		}
		if (siz <= 1) {
			s += "\n)\n";
		} else {
			s += ",\n";
			// iterate through routers and print parent links
			Iterator<Integer> rp =
                                getRtrMap(ctx).keySet().iterator();
                        while (rp.hasNext()) {
                                int radr = rp.next();
				int plnk = getRtrMap(ctx).get(radr).plnk;
				if (plnk == 0) continue;
				s += "\t" + link2string(ctx,plnk);
				if (rp.hasNext()) s += ",";
				s += "\n";
			}
			// iterate through leaf nodes and print parent links
			Iterator<Integer> lp =
                                getLeafMap(ctx).keySet().iterator();
                        while (lp.hasNext()) {
                                int leaf = lp.next();
				s += "\t" + leafLink2string(ctx,leaf);
				if (lp.hasNext()) s += ",";
				s += "\n";
			}
			s += ")\n";
		}
		return s;
	}
}
