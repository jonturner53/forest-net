/** @file NetInfo.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "NetInfo.h"

/** Constructor for NetInfo, allocates space and initializes private data.
 *  @param maxNode1 is the maximum number of nodes in this NetInfo object
 *  @param maxLink1 is the maximum number of links in this NetInfo object
 *  @param maxRtr1 is the maximum number of routers in this NetInfo object
 *  @param maxCtl1 is the maximum number of controllers in this NetInfo object
 *  @param maxComtree1 is the maximum number of comtrees in this NetInfo object
 */
NetInfo::NetInfo(int maxNode1, int maxLink1,
		 int maxRtr1, int maxCtl1 , int maxComtree1)
		 : maxNode(maxNode1), maxLink(maxLink1),
		   maxRtr(maxRtr1), maxCtl(maxCtl1), maxComtree(maxComtree1) {
	maxLeaf = maxNode-maxRtr;
	netTopo = new Wgraph(maxNode, maxLink);

	rtr = new RtrNodeInfo[maxRtr+1];
	routers = new UiSetPair(maxRtr);

	leaf = new LeafNodeInfo[maxLeaf+1];
	leaves = new UiSetPair(maxLeaf);
	nameNodeMap = new map<string,int>;
	adrNodeMap = new map<fAdr_t,int>;
	controllers = new set<int>();

	link = new LinkInfo[maxLink+1];
	locLnk2lnk = new UiHashTbl(2*min(maxLink,maxRtr*(maxRtr-1)/2)+1);

	comtree = new ComtreeInfo[maxComtree+1];
	comtreeMap = new IdMap(maxComtree);
}

/** Destructor for NetInfo class.
 *  Deallocates all dynamic storage
 */
NetInfo::~NetInfo() {
	delete netTopo;
	delete [] rtr; delete routers;
	delete [] leaf; delete leaves; delete controllers;
	delete nameNodeMap; delete adrNodeMap;
	delete [] link; delete locLnk2lnk;
	delete [] comtree; delete comtreeMap;
}

/** Add a new link to a comtree.
 *  @param ctx is the comtree index
 *  @param lnk is the link number of the link to be added
 *  @param parent is the parent endpoint of lnk
 *  @return true on success, false on failure
 */
bool NetInfo::addComtLink(int ctx, int lnk, int parent) {
	if (!validLink(lnk)) return false;
	pair<int,RateSpec> newPair;
	newPair.first = lnk;
	newPair.second.bitRateUp = newPair.second.bitRateDown = 0;
	newPair.second.pktRateUp = newPair.second.pktRateDown = 0;
	comtree[ctx].lnkMap->insert(newPair);

	int child = getPeer(parent,lnk);
	addComtNode(ctx,child); addComtNode(ctx,parent);
	map<int,ComtRtrInfo>::iterator pp;
	pp = comtree[ctx].rtrMap->find(child);
	pp->second.plnk = lnk; pp->second.lnkCnt += 1;
	pp = comtree[ctx].rtrMap->find(parent);
	pp->second.lnkCnt += 1;

	return true;
}

/** Remove a link from a comtree.
 *  @param ctx is the comtree index
 *  @param lnk is the link number of the link to be removed
 *  @return true on success, false on failure
 */
bool NetInfo::removeComtLink(int ctx, int lnk) {
	if (!validLink(lnk)) return false;
	map<int,RateSpec>::iterator lp;
	lp = comtree[ctx].lnkMap->find(lnk);
	if (lp == comtree[ctx].lnkMap->end())
		return true;
	comtree[ctx].lnkMap->erase(lp);

	int left = getLinkL(lnk); int right = getLinkR(lnk);
	map<int,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(left);
	if (rp != comtree[ctx].rtrMap->end()) {
		rp->second.lnkCnt -= 1;
		if (rp->second.lnkCnt <= 0 && left != getComtRoot(ctx))
			removeComtNode(ctx,left);
	}
	rp = comtree[ctx].rtrMap->find(right);
	if (rp != comtree[ctx].rtrMap->end()) {
		rp->second.lnkCnt -= 1;
		if (rp->second.lnkCnt <= 0 && right != getComtRoot(ctx))
			removeComtNode(ctx,right);
	}
	return true;
}

/** Perform a series of consistency checks.
 *  Print error message for each detected problem.
 *  @return true if all checks passed, else false
 */
bool NetInfo::check() {
	bool status = true;

	// make sure there is at least one router
	if (getNumRouters() == 0 || firstRouter() == 0) {
		cerr << "NetInfo::check: no routers in network, terminating\n";
		return false;
	}
	// make sure that no two links at a router have the
	// the same local link number
	for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
		for (int l1 = firstLinkAt(r); l1 != 0;
			 l1 = nextLinkAt(r,l1)) {
			for (int l2 = nextLinkAt(r,l1); l2 != 0;
				 l2 = nextLinkAt(r,l2)) {
				if (getLocLink(l1,r) == getLocLink(l2,r)) {
					string s1,s2;
					cerr << "NetInfo::check: detected two "
						"links with same local link "
						"number: "
					     << link2string(l1,s1) << " and "
					     << link2string(l2,s2) << "\n";
					status = false;
				}
			}
		}
	}

	// make sure that routers are all connected, by doing
	// a breadth-first search from firstRouter()
	set<int> seen; seen.insert(firstRouter());
	queue<int> pending; pending.push(firstRouter());
	while (!pending.empty()) {
		int u = pending.front(); pending.pop();
		for (int lnk = firstLinkAt(u); lnk != 0;
			 lnk = nextLinkAt(u,lnk)) {	
			int v = getPeer(u,lnk);
			if (getNodeType(v) != ROUTER) continue;
			if (seen.find(v) != seen.end()) continue;
			seen.insert(v);
			pending.push(v);
		}
	}
	if (seen.size() != getNumRouters()) {
		cerr << "NetInfo::check: network is not connected";
		status = false;
	}

	// check that no two nodes have the same address
	for (int n1 = firstNode(); n1 != 0; n1 = nextNode(n1)) {
		for (int n2 = nextNode(n1); n2 != 0; n2 = nextNode(n2)) {
			if (getNodeAdr(n1) == getNodeAdr(n2)) {
				string s1,s2;
				cerr << "NetInfo::check: detected two nodes "
				     << getNodeName(n1,s1) << " and "
				     << getNodeName(n2,s1) << " with the same "
					"forest address\n";
			status = false;
			}
		}
	}

	// check that the leaf address range for a router
	// is compatible with the router's address
	for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
		int rzip = Forest::zipCode(getNodeAdr(r));
		int flzip = Forest::zipCode(getFirstLeafAdr(r));
		int llzip = Forest::zipCode(getLastLeafAdr(r));
		if (rzip != flzip || rzip != llzip) {
			cerr << "netInfo::check: detected router " << r
			     << "with incompatible address and leaf address "
			     << "range\n";
			status = false;
		}
		if (getFirstLeafAdr(r) > getLastLeafAdr(r)) {
			cerr << "netInfo::check: detected router " << r
			     << "with empty leaf address range\n";
			status = false;
		}
	}

	// make sure that no two routers have overlapping leaf
	// address ranges
	for (int r1 = firstRouter(); r1 != 0; r1 = nextRouter(r1)) {
		int fla1 = getFirstLeafAdr(r1);
		int lla1 = getLastLeafAdr(r1);
		for (int r2 = nextRouter(r1); r2 != 0; r2 = nextRouter(r2)) {
			int fla2 = getFirstLeafAdr(r2);
			int lla2 = getLastLeafAdr(r2);
			if (fla2 <= lla1 && lla2 >= fla1) {
				cerr << "netInfo::check: detected two routers "
				     << r1 << " and " << r2
			     	     << " with overlapping address ranges\n";
				status = false;
			}
		}
	}

	// check that all leaf nodes have a single link that connects
	// to a router and that their address is in the range of their router
	for (int u = firstLeaf(); u != 0; u = nextLeaf(u)) {
		int lnk = firstLinkAt(u);
		if (lnk == 0) {
			string s;
			cerr << "NetInfo::check: detected a leaf node "
			     << getNodeName(u,s) << " with no links\n";
			status = false; continue;
		}
		if (nextLinkAt(u,lnk) != 0) {
			string s;
			cerr << "NetInfo::check: detected a leaf node "
			     << getNodeName(u,s)<< " with more than one link\n";
			status = false; continue;
		}
		if (getNodeType(getPeer(u,lnk)) != ROUTER) {
			string s;
			cerr << "NetInfo::check: detected a leaf node "
			     << getNodeName(u,s) << " with link to non-router"
				"\n";
			status = false; continue;
		}
		int rtr = getPeer(u,lnk);
		int adr = getNodeAdr(u);
		if (adr < getFirstLeafAdr(rtr) || adr > getLastLeafAdr(rtr)) {
			string s;
			cerr << "NetInfo::check: detected a leaf node "
			     << getNodeName(u,s) << " with an address outside "
				" the leaf address range of its router\n";
			status = false; continue;
		}
	}

	// check that link rates are within bounds
	for (int lnk = firstLink(); lnk != 0; lnk = nextLink(lnk)) {
		int br = getLinkBitRate(lnk);
		if (br < Forest::MINBITRATE || br > Forest::MAXBITRATE) {
			string s;
			cerr << "NetInfo::check: detected a link "
			     << link2string(lnk,s) << " with bit rate "
				"outside the allowed range\n";
			status = false;
		}
		int pr = getLinkPktRate(lnk);
		if (pr < Forest::MINPKTRATE || pr > Forest::MAXPKTRATE) {
			string s;
			cerr << "NetInfo::check: detected a link "
			     << link2string(lnk,s) << " with packet rate "
				"outside the allowed range\n";
			status = false;
		}
	}

	// check that routers' local link numbers fall within the range of
	// some valid interface
	for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
		for (int lnk = firstLinkAt(r); lnk != 0;
			 lnk = nextLinkAt(r,lnk)) {
			int llnk = getLocLink(lnk,r);
			if (getIface(llnk,r) == 0) {
				string s;
				cerr << "NetInfo::check: link " << llnk
				     << " at router " << getNodeName(r,s)
				     << " is not in the range assigned "
					"to any valid interface\n";
				status = false;
			}
		}
	}

	// check that router interface rates are within bounds
	for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
		for (int i = 1; i <= getNumIf(r); i++) {
			if (!validIf(r,i)) continue;
			int br = getIfBitRate(r,i);
			if (br < Forest::MINBITRATE || br >Forest::MAXBITRATE) {
				cerr << "NetInfo::check: interface " << i
				     << "at router " << r << " has bit rate "
					"outside the allowed range\n";
				status = false;
			}
			int pr = getIfPktRate(r,i);
			if (pr < Forest::MINPKTRATE || pr >Forest::MAXPKTRATE) {
				cerr << "NetInfo::check: interface " << i
				     << "at router " << r << " has packet rate "
					"outside the allowed range\n";
				status = false;
			}
		}
	}

	// verify that link rates at any router don't add up to more
	// than the interface rates
	for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
		int* ifbr = new int[getNumIf(r)+1];
		int* ifpr = new int[getNumIf(r)+1];
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
				string s;
				cerr << "NetInfo::check: links at interface "
				     << i  << " of router " << r
				     <<	" have total bit rate that exceeds "
					"interface bit rate\n";
			}
			if (ifpr[i] > getIfPktRate(r,i)) {
				string s;
				cerr << "NetInfo::check: links at interface "
				     << i  << " of router " << r
				     <<	" have total packet rate that exceeds "
					"interface packet rate\n";
			}
		}
	}

	// check that comtrees are in fact trees and that they satisfy
	// various other requirements
	for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx)) {
		int comt = getComtree(ctx);
		// first count nodes and links
		set<int> nodes; int lnkCnt = 0;
		for (int lnk = firstComtLink(ctx); lnk != 0;
			 lnk = nextComtLink(lnk,ctx)) {
			nodes.insert(getLinkL(lnk));
			nodes.insert(getLinkR(lnk));
			lnkCnt++;
		}
		if (lnkCnt != nodes.size() - 1) {
			cerr << "NetInfo::check: links in comtree "
			     << comt << " do not form a tree\n";
			status = false; continue;
		}
		// check that root and core nodes are in the set we've seen
		int root = getComtRoot(ctx);
		if (nodes.find(root) == nodes.end()) {
			cerr << "NetInfo::check: specified comtree root for "
				" comtree " << comt << " does not appear "
				"in any comtree link\n";
			status = false; continue;
		}
		bool seenRoot = false;
		for (int r = firstCore(ctx); r != 0; r = nextCore(r,ctx)) {
			if (r == root) seenRoot = true;
			if (nodes.find(r) == nodes.end()) {
				string s;
				cerr << "NetInfo::check: core node "
				     << getNodeName(r,s) << " for comtree "
				     << comt << " does not appear in any "
					"comtree link\n";
				status = false;
			}
		}
		if (!seenRoot) {
			string s;
			cerr << "NetInfo::check: root node does not appear "
				"among the core nodes for comtree " << comt
			     << "\n";
			status = false;
		}

		// now, check that the comtree topology is really a tree
		// by doing a breadth-first search from the root;
		// while we're at it, make sure the parent of every
		// core node is a core node and that the zip codes
		// of routers within the comtree are contiguous
		queue<int> pending; pending.push(root);
		map<int,int> plink; plink[root] = 0;
		map<int,int> pzip; pzip[Forest::zipCode(getNodeAdr(root))] = 0;
		while (!pending.empty()) {
			int u = pending.front(); pending.pop();
			bool foundCycle = false;
			int uzip = Forest::zipCode(getNodeAdr(u));
			for (int lnk = firstLinkAt(u); lnk != 0;
				 lnk = nextLinkAt(u,lnk)) {
				if (!isComtLink(ctx,lnk)) continue;
				if (lnk == plink[u]) continue;
				int v = getPeer(u,lnk);
				int vzip = Forest::zipCode(getNodeAdr(v));
				if (plink.find(v) != plink.end()) {
					cerr << "NetInfo::check: comtree "
					     <<	comt << " contains a cycle\n";
					foundCycle = true;
					break;
				}
				plink[v] = lnk;
				pending.push(v);
				// now check that if v is in core, so is u
				if (isComtCoreNode(ctx,v) &&
				    !isComtCoreNode(ctx,u)) {
					string s;
					cerr << "NetInfo::check: comtree "
					     << comt << " contains a core node "
					     << getNodeName(v,s) << " whose "
						"parent is not a core node\n";
					status = false;
				}
				// now check that if v has a different zip code
				// than u, that we haven't already seen this zip
				if (vzip != uzip) {
					if (pzip.find(v) != pzip.end()) {
						cerr << "NetInfo::check: zip "
							"code " << vzip <<
							" is non-contiguous in "
							"comtree " << comt
						     << "\n";
						status = false;
					} else {
						pzip[v] = u;
					}
				}
			}
			if (foundCycle) { status = false; break; }
		}
	}
	return status;
}

/** Initialize auxiliary data structures for fast access to comtree information.
 *  Set comtree link rates to default values and check that total comtree rates
 *  don't exceed link rates. Also set the plnk and lnkCnt fields
 *  in the rtrMap for each comtree; note that we assume that the lnkCnt
 *  fields have been initialized to zero.
 */
bool NetInfo::setComtLnkNodeInfo() {
	bool status = true;
	for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx)) {
		// do breadth-first search over the comtree links
		int comt = getComtree(ctx); int root = getComtRoot(ctx);
		queue<int> pending; pending.push(root);
		map<int,int> plink; plink[root] = 0;
		while (!pending.empty()) {
			int u = pending.front(); pending.pop();
			for (int lnk = firstLinkAt(u); lnk != 0;
				 lnk = nextLinkAt(u,lnk)) {
				if (!isComtLink(ctx,lnk)) continue;
				incComtLnkCnt(ctx,u);
				if (lnk == plink[u]) continue;
				int v = getPeer(u,lnk);
				setComtPlink(ctx,v,lnk);
				plink[v] = lnk;
				pending.push(v);
				if (!setLinkRates(ctx,lnk,v)) {
					cerr << "NetInfo::setComtLinkRates: "
						"could not set comtree link "
						"rates as specified for "
						"comtree " << comt <<
						" lnk " << lnk << endl;
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
bool NetInfo::setLinkRates(int ctx, int lnk, int child) {
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
		brl = getComtBrUp(ctx,lnk);   prl = getComtPrUp(ctx,lnk);
		brr = getComtBrDown(ctx,lnk); prr = getComtPrDown(ctx,lnk);
	} else {
		brl = getComtBrDown(ctx,lnk); prl = getComtPrDown(ctx,lnk);
		brr = getComtBrUp(ctx,lnk);   prr = getComtPrUp(ctx,lnk);
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
int NetInfo::getIface(int llnk, int rtr) {
	for (int i = 1; i <= getNumIf(rtr); i++) {
		if (validIf(rtr,i) && llnk >= getIfFirstLink(rtr,i)
				   && llnk <= getIfLastLink(rtr,i)) {
			return i;
		}
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
int NetInfo::addRouter(const string& name) {
	int r = routers->firstOut();
	if (r == 0) return 0;
	if (nameNodeMap->find(name) != nameNodeMap->end()) return 0;
	routers->swap(r);

	rtr[r].name = name; (*nameNodeMap)[name] = r;
	rtr[r].nType = ROUTER;
	rtr[r].fAdr = -1;

	setNodeLat(r,UNDEF_LAT); setNodeLong(r,UNDEF_LONG);
	setFirstLeafAdr(r,-1); setLastLeafAdr(r,-1);
	rtr[r].numIf = 0; rtr[r].iface = 0;
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
bool NetInfo::addInterfaces(int r, int numIf) {
	if (!isRouter(r) || getNumIf(r) != 0) return false;
	rtr[r].iface = new IfInfo[numIf+1];
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
int NetInfo::addLeaf(const string& name, ntyp_t nTyp) {
	int ln = leaves->firstOut();
	if (ln == 0) return 0;
	if (nameNodeMap->find(name) != nameNodeMap->end()) return 0;
	leaves->swap(ln);

	int nodeNum = ln + maxRtr;
	leaf[ln].name = name;
	(*nameNodeMap)[name] = nodeNum;
	if (nTyp == CONTROLLER) controllers->insert(ln);
	leaf[ln].fAdr = -1;

	setLeafType(nodeNum,CLIENT); setLeafIpAdr(nodeNum,0); 
	setNodeLat(nodeNum,UNDEF_LAT); setNodeLong(nodeNum,UNDEF_LONG);
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
int NetInfo::addLink(int u, int v, int uln, int vln ) {
	int lnk = netTopo->join(u,v);
	if (lnk == 0) return 0;
	netTopo->setWeight(lnk,0);
	if (getNodeType(u) == ROUTER) {
		if (!locLnk2lnk->insert(ll2l_key(u,uln),2*lnk)) {
			netTopo->remove(lnk); return 0;
		}
		setLocLinkL(lnk,uln);
	}
	if (getNodeType(v) == ROUTER) {
		if (!locLnk2lnk->insert(ll2l_key(v,vln),2*lnk+1)) {
			locLnk2lnk->remove(ll2l_key(u,uln));
			netTopo->remove(lnk); return 0;
		}
		setLocLinkR(lnk,vln);
	}
	link[lnk].bitRate = Forest::MINBITRATE;
	link[lnk].pktRate = Forest::MINPKTRATE;
	link[lnk].availBitRateL = link[lnk].availBitRateR = Forest::MINBITRATE;
	link[lnk].availPktRateL = link[lnk].availPktRateR = Forest::MINPKTRATE;
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
 *       1     193.168.3.4  1	   50000    25000 ;
 *       2     193.168.3.5  2-30	40000    20000 ;
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
 *  comtree=1001 root=salt core=kans # may have multiple core=x assignments
 *  bitRateUp=5000 bitRateDown=10000 pktRateUp=3000 pktRateDown=6000
 *  leafBitRateUp=100 leafBitRateDown=200 leafPktRateUp=50 leafPktRateDown=200
 *  link=(salt.1,kans.1) link=(netMgr,kans.2) 	
 *  ;
 *  
 *  comtree=1002 root=kans core=salt 
 *  bitRateUp=5000 bitRateDown=10000 pktRateUp=3000 pktRateDown=6000
 *  leafBitRateUp=100 leafBitRateDown=200 leafPktRateUp=50 leafPktRateDown=200
 *  link=(salt.1,kans.1) link=(comtCtl,salt.2) 	
 *  ;
 *  ; # end of comtrees section
 */

/** Reads a network topology file and initializes the NetInfo data structure.
 *  @param in is an open input stream from which the topology file will
 *  be read
 *  @return true if the operation is successful, false if it fails
 *  (failures are typically accompanied by error messages on cerr)
 */
bool NetInfo::read(istream& in) {
	RtrNodeInfo cRtr;	// holds data for router being parsed
	LeafNodeInfo cLeaf;	// holds data for leaf being parsed
	LinkInfo cLink;		// hods data for link being parsed
	IfInfo iface[Forest::MAXINTF+1]; // holds data for router interfaces
	ComtreeInfo cComt;	// holds data comtree being parsed
	cComt.coreSet = 0; cComt.lnkMap = 0;

	int rtrNum = 1;		// rtrNum = i for i-th router in file
	int ifNum = 1;		// interface number for current interface
	int maxIfNum = 0;	// largest interface number specified
	int leafNum = 1;	// leafNum = i for i-th leaf in file 
	int linkNum = 1;	// linkNum = i for i-th link in file
	int comtNum = 1;	// comtNum = i for i-th comtree in file

	string leftName;	// name of a link's left end point
	string rightName;	// name of a link's right end point
	int linkLength = 0;	// length of a link

	// Parse contexts are used to keep track of where we are in the
	// parsed file. Based opn the context, we look for specific items
	// in the input stream. For example, the TOP context is the initial
   	// context, and will in the TOP context, we expect to see one of
	// keyworks, Routers, LeafNodes, Links or Comtrees.
	enum ParseContexts {
		TOP, ROUTER_SEC, ROUTER_CTXT , IFACES, IFACES_ENTRY,
		LEAF_SEC, LEAF, LINK_SEC, LINK, COMTREE_SEC, COMTREE_CTXT
	} context = TOP;
	
	char c; string s;
	while (!in.eof()) {
		if (!Misc::skipBlank(in)) { break; }
		s.clear();
		switch (context) {
		case TOP:
			if (!Misc::readWord(in, s)) {
				cerr << "NetInfo::read: can't read section "
				     << "name" << endl;
				return false;
			}
			if (s.compare("Routers") == 0)
				context = ROUTER_SEC;
			else if (s.compare("LeafNodes") == 0)
				context = LEAF_SEC;
			else if (s.compare("Links") == 0)
				context = LINK_SEC;
			else if (s.compare("Comtrees") == 0)
				context = COMTREE_SEC;
			else {
				cerr << "NetInfo::read: unexpected section "
				     << "name: " << s << endl;
				return false;
			}
			break;
		case ROUTER_SEC:
			if (in.peek() == ';') { // end of router section
				in.get(c); context = TOP; break;
			}
	
			// clear the current router structure
			cRtr.name = ""; cRtr.nType = UNDEF_NODE; cRtr.fAdr = 0;
			cRtr.latitude = 91000000; cRtr.longitude = 361000000; 
			cRtr.firstLeafAdr = cRtr.lastLeafAdr = 0;
			cRtr.numIf = 0; cRtr.iface = 0;
			context = ROUTER_CTXT;
			// falling through to ROUTER_CTXT context
		case ROUTER_CTXT:
			if (in.peek() == ';') { // end of router definition
				in.get(c);
				if (rtrNum > maxRtr) {
					cerr << "NetInfo::read: too many "
				     	     << "routers, max is " 
					     << maxRtr << endl;
					return false;
				}
				if (cRtr.name.compare("") == 0) {
					cerr << "NetInfo::read: no name for"
					     << " router number " << rtrNum
					     << endl;
					return false;
				}
				if (cRtr.nType == UNDEF_NODE) {
					cerr << "NetInfo::read: no type for"
					     << " router " << cRtr.name
					     << endl;
					return false;
				}
				if (!Forest::validUcastAdr(cRtr.fAdr)) {
					cerr << "NetInfo::read: no valid forest"
					     << " address for router "
					     << cRtr.name << endl;
					return false;
				}
				if (cRtr.latitude < -90000000 ||
				    cRtr.latitude > 90000000) {
					cerr << "NetInfo::read: no latitude "
					     << " for router "
					     << cRtr.name << endl;
					return false;
				}
				if (cRtr.longitude < -360000000 ||
				    cRtr.longitude > 360000000) {
					cerr << "NetInfo::read: no longitude "
					     << "for router "
					     << cRtr.name << endl;
					return false;
				}
				if (!Forest::validUcastAdr(cRtr.firstLeafAdr) ||
				    !Forest::validUcastAdr(cRtr.lastLeafAdr) ||
				     Forest::zipCode(cRtr.fAdr)
				      != Forest::zipCode(cRtr.firstLeafAdr) ||
				     Forest::zipCode(cRtr.fAdr)
				      != Forest::zipCode(cRtr.lastLeafAdr) ||
				    cRtr.firstLeafAdr > cRtr.lastLeafAdr) {
					cerr << "NetInfo::read: no valid client"
					     << " address range for router "
					     << cRtr.name << endl;
					return false;
				}
				if (cRtr.numIf == 0) {
					cerr << "NetInfo::read: no interfaces "
					     << "defined for router "
					     << cRtr.name << endl;
					return false;
				}
				// add new router and initialize attributes
				int r = addRouter(cRtr.name);
				if (r == 0) {
					cerr << "NetInfo::read: cannot add "
					     << "router "
					     << cRtr.name << endl;
					return false;
				}
				setNodeAdr(r,cRtr.fAdr);
				setNodeLat(r,cRtr.latitude/1000000.0);
				setNodeLong(r,cRtr.longitude/1000000.0);
				setFirstLeafAdr(r,cRtr.firstLeafAdr);
				setLastLeafAdr(r,cRtr.lastLeafAdr);
				addInterfaces(r,cRtr.numIf);
				for (int i = 1; i <= getNumIf(r); i++)
					rtr[r].iface[i] = iface[i];
				maxIfNum = 0;
				rtrNum++;
				context = ROUTER_SEC; break;
			}
			if (!Misc::readWord(in, s)) {
				cerr << "NetInfo::read: syntax error when "
				     << "reading router number " << rtrNum
				     << endl;
				return false;
			}
			if (s.compare("name") == 0 && Misc::verify(in,'=')) {
				if (!Misc::readWord(in,s)) {
					cerr << "NetInfo::read: can't read name"
					     << " for router number " << rtrNum
					     << endl;
					return false;
				}
				cRtr.name = s;
			} else if (s.compare("type") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readWord(in,s)) {
					cerr << "NetInfo::read: can't read type"
					     << " for router number " << rtrNum
					     << endl;
					return false;
				}
				cRtr.nType = Forest::getNodeType(s);
			} else if (s.compare("fAdr") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Forest::readForestAdr(in,cRtr.fAdr)) {
					cerr << "NetInfo::read: can't read "
					     << "forest address for router" 
					     << " number " << rtrNum << endl;
					return false;
				}
			} else if (s.compare("location")==0 &&
				   Misc::verify(in,'=')) {
				double x, y;
				if (!Misc::verify(in,'(') ||
				    !(in >> x) || ! Misc::verify(in,',') ||
				    !(in >> y) || ! Misc::verify(in,')')) {
					cerr << "NetInfo::read: can't read "
					     << "location for router"
					     << " number " << rtrNum << endl;
					return false;
				}
				cRtr.latitude  = (int) (x*1000000);
				cRtr.longitude = (int) (y*1000000);
			} else if (s.compare("leafAdrRange") == 0 &&
				   Misc::verify(in,'=')) {
				fAdr_t first, last;
				if (!Misc::verify(in,'(') ||
				    !Forest::readForestAdr(in,first) ||
				    ! Misc::verify(in,'-') ||
				    !Forest::readForestAdr(in,last) ||
				    ! Misc::verify(in,')')) {
					cerr << "NetInfo::read: can't read "
					     << "location for router"
					     << " number " << rtrNum << endl;
					return false;
				}
				cRtr.firstLeafAdr = first;
				cRtr.lastLeafAdr  = last;
			} else if (s.compare("interfaces") == 0) {
				context = IFACES;
			} else {
				cerr << "NetInfo::read: syntax error while "
				     << "reading router "
				     << rtrNum << endl;
				return false;
			}
			break;
		case IFACES:
			for (int i = 1; i <= Forest::MAXINTF; i++) {
				iface[i].ipAdr = 0;
				iface[i].bitRate = 0; iface[i].pktRate = 0;
				iface[i].firstLink = 0; iface[i].lastLink = 0;
			}
			context = IFACES_ENTRY;
			// falling through to ifaces entry context
		case IFACES_ENTRY:
			// first check for end keyword
			if (in.peek() == 'e') {
				if (!Misc::readWord(in,s)) {
					cerr << "NetInfo::read: syntax error "
					     << "in interfaces table for "
			   		     << "router " << rtrNum << endl;
					return false;
				}
				if (s.compare("end") != 0) {
					cerr << "NetInfo::read: syntax error "
					     << "in interfaces table for "
			   		     << "router " << rtrNum << endl;
					return false;
				}
				cRtr.numIf = maxIfNum;
				context = ROUTER_CTXT;
				break;
			}
			// next read interface number
			if (!Misc::readNum(in,ifNum)) {
				cerr << "NetInfo::read: syntax error "
				     << "in interfaces table for "
		   		     << "router " << rtrNum << endl;
				return false;
			}
			if (ifNum < 1 || ifNum > Forest::MAXINTF) {
				cerr << "NetInfo::read: interface number "
				     << ifNum << " is out of range" << endl;
				return false;
			}
			maxIfNum = max(maxIfNum, ifNum);
			// now read ip address and the first link
			if (!Np4d::readIpAdr(in,iface[ifNum].ipAdr) ||
			    !Misc::readNum(in,iface[ifNum].firstLink)) {
				cerr << "NetInfo::read: syntax error "
				     << "in interfaces table for "
		   		     << "router " << rtrNum 
				     << " interface " << ifNum << endl;
				return false;
			}
			// check for a dash, meaning that last link is present
			if (in.peek() == '-') {
				in.get(c);
				if (!Misc::readNum(in,iface[ifNum].lastLink)) {
					cerr << "NetInfo::read: syntax error "
					     << "in interfaces table for "
		   			     << "router " << rtrNum 
					     << " interface " << ifNum << endl;
					return false;
				}
			} else
				iface[ifNum].lastLink = iface[ifNum].firstLink;

			// and finally check for bitRate and pktRate
			if (!Misc::readNum(in,iface[ifNum].bitRate) ||
			    !Misc::readNum(in,iface[ifNum].pktRate)) {
				cerr << "NetInfo::read: syntax error "
				     << "in interfaces table for "
		   		     << "router " << rtrNum 
				     << " interface " << ifNum << endl;
				return false;
			}
			if (!Misc::skipBlank(in)) break;
			if (!Misc::verify(in,';')) {
				cerr << "NetInfo::read: syntax error "
				     << "in interfaces table for "
		   		     << "router " << rtrNum
				     << " interface " << ifNum << endl;
				return false;
			}
			break;
		case LEAF_SEC:
			if (in.peek() == ';') { // end of leaf node section
				in.get(c); context = TOP; break;
			}

			// clear the current leaf structure
			cLeaf.name = ""; cLeaf.nType = UNDEF_NODE; 
			cLeaf.ipAdr = 0; cLeaf.fAdr = 0;
			cLeaf.latitude = 91000000; cLeaf.longitude = 361000000; 
			context = LEAF;
			// falling through to LEAF context
		case LEAF:
			if (in.peek() == ';') { // end of leaf node definition
				in.get(c);
				if (leafNum > maxLeaf) {
					cerr << "NetInfo::read: too many "
				     	     << "leafNodes, max is " 
					     << maxLeaf << endl;
					return false;
				}
				if (cLeaf.name.compare("") == 0) {
					cerr << "NetInfo::read: no name for"
					     << " leaf node number " << leafNum
					     << endl;
					return false;
				}
				if (cLeaf.nType == UNDEF_NODE) {
					cerr << "NetInfo::read: no type for"
					     << " leaf node " << cLeaf.name
					     << endl;
					return false;
				}
				if (cLeaf.ipAdr == 0) {
					cerr << "NetInfo::read: no ip"
					     << " address for leaf node "
					     << cLeaf.name << endl;
					return false;
				}
				if (!Forest::validUcastAdr(cLeaf.fAdr)) {
					cerr << "NetInfo::read: no valid forest"
					     << " address for leaf node "
					     << cLeaf.name << endl;
					return false;
				}
				if (cLeaf.latitude < -90000000 ||
				    cLeaf.latitude > 90000000) {
					cerr << "NetInfo::read: no latitude "
					     << " for leaf node "
					     << cLeaf.name << endl;
					return false;
				}
				if (cLeaf.longitude < -360000000 ||
				    cLeaf.longitude > 360000000) {
					cerr << "NetInfo::read: no longitude "
					     << "for leaf node "
					     << cLeaf.name << endl;
					return false;
				}
				// add new leaf and initialize attributes
				int nodeNum = addLeaf(cLeaf.name,cLeaf.nType);
				if (nodeNum == 0) {
					cerr << "NetInfo::read: cannot add "
					     << "leaf "
					     << cLeaf.name << endl;
					return false;
				}
				setLeafType(nodeNum,cLeaf.nType);
				setLeafIpAdr(nodeNum,cLeaf.ipAdr);
				setNodeAdr(nodeNum,cLeaf.fAdr);
				setNodeLat(nodeNum,cLeaf.latitude/1000000.0);
				setNodeLong(nodeNum,cLeaf.longitude/1000000.0);
				leafNum++;
				context = LEAF_SEC; break;
			}
			if (!Misc::readWord(in, s)) {
				cerr << "NetInfo::read: syntax error when "
				     << "reading leaf node number " << leafNum
				     << endl;
				return false;
			}
			if (s.compare("name") == 0 && Misc::verify(in,'=')) {
				if (!Misc::readWord(in,s)) {
					cerr << "NetInfo::read: can't read name"
					     << " for leaf number " << leafNum
					     << endl;
					return false;
				}
				cLeaf.name = s;
			} else if (s.compare("type") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readWord(in,s)) {
					cerr << "NetInfo::read: can't read type"
					     << " for leaf number " << leafNum
					     << endl;
					return false;
				}
				cLeaf.nType = Forest::getNodeType(s);
			} else if (s.compare("ipAdr") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Np4d::readIpAdr(in,cLeaf.ipAdr)) {
					cerr << "NetInfo::read: can't read ip"
					     << " address for leaf number " 
					     << leafNum << endl;
					return false;
				}
			} else if (s.compare("fAdr") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Forest::readForestAdr(in,cLeaf.fAdr)) {
					cerr << "NetInfo::read: can't read "
					     << "forest address for router" 
					     << " number " << leafNum << endl;
					return false;
				}
			} else if (s.compare("location")==0 &&
				   Misc::verify(in,'=')) {
				double x, y;
				if (!Misc::verify(in,'(') ||
				    !(in >> x) || ! Misc::verify(in,',') ||
				    !(in >> y) || ! Misc::verify(in,')')) {
					cerr << "NetInfo::read: can't read "
					     << "location for leaf node"
					     << " number " << leafNum << endl;
					return false;
				}
				cLeaf.latitude  = (int) (x*1000000);
				cLeaf.longitude = (int) (y*1000000);
			} else {
				cerr << "NetInfo::read: syntax error while "
				     << "reading leaf node "
				     << leafNum << endl;
				return false;
			}
			break;
		case LINK_SEC:
			if (in.peek() == ';') { // end of links section
				in.get(c); context = TOP; break; 
			}

			// clear the current leaf structure
			cLink.leftLnum = 0; cLink.rightLnum = 0;
			cLink.bitRate = cLink.pktRate = 0;
			context = LINK;
			leftName.clear(); rightName.clear();
			linkLength = -1;
			// falling through to LINK context
		case LINK: {
			if (in.peek() == ';') { // end of link definition
				in.get(c);
				if (linkNum > maxLink) {
					cerr << "NetInfo::read: too many "
				     	     << "linkNodes, max is " 
					     << maxLink << endl;
					return false;
				}
				if (leftName == "") {
					cerr << "NetInfo::read: no left"
					     << " endpoint for link "
					     << linkNum << endl;
					return false;
				}
				if (rightName == "") {
					cerr << "NetInfo::read: no right"
					     << " endpoint for link "
					     << linkNum << endl;
					return false;
				}
				if (cLink.bitRate == 0) {
					cerr << "NetInfo::read: no bit rate for"
					     << " link "
					     << linkNum << endl;
					return false;
				}
				if (cLink.pktRate == 0) {
					cerr << "NetInfo::read: no pkt rate for"
					     << " link "
					     << linkNum << endl;
					return false;
				}
				if (linkLength == -1) {
					cerr << "NetInfo::read: no length for"
					     << " link "
					     << linkNum << endl;
					return false;
				}
				
				// add new link and set attributes
				int u = (*nameNodeMap)[leftName];
				int v = (*nameNodeMap)[rightName];
				int lnk = addLink(u,v,cLink.leftLnum,
						      cLink.rightLnum);
				if (lnk == 0) {
					cerr << "NetInfo::read: can't add "
						"link (" << leftName << "."
					     << cLink.leftLnum << ","
					     << rightName << "."
					     << cLink.rightLnum << ")" << endl;
					return false;
				}
				setLinkBitRate(lnk, cLink.bitRate);
				setLinkPktRate(lnk, cLink.pktRate);
				setLinkLength(lnk, linkLength);
				linkNum++;
				context = LINK_SEC; break;
			}
			if (!Misc::readWord(in, s)) {
				cerr << "NetInfo::read: syntax error when "
				     << "reading link number " << linkNum
				     << endl;
				return false;
			}
			if (s.compare("link") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::verify(in,'(') ||
				    !Misc::readWord(in,leftName)) {
					cerr << "NetInfo::read: syntax error "
					     << "while reading link "
					     << linkNum << endl;
					return false;
				}
				if (in.peek() == '.') {
					in.get(c);
					if (!Misc::readNum(in,cLink.leftLnum)) {
						cerr << "NetInfo::read: syntax "
						     << "error while reading "
						     << "link " << linkNum
						     << endl;
						return false;
					}
				}
			    	if (!Misc::verify(in,',') ||
			  	    !Misc::readWord(in,rightName)) {
					cerr << "NetInfo::read: syntax error "
					     << "while reading link "
					     << linkNum << endl;
					return false;
				}
				if (in.peek() == '.') {
					in.get(c);
					if (!Misc::readNum(in,cLink.rightLnum)){
						cerr << "NetInfo::read: syntax "
						     << "error while reading "
						     << "link " << linkNum
						     << endl;
						return false;
					}
				}
				if (!Misc::verify(in,')') ) {
					cerr << "NetInfo::read: syntax error "
					     << "while reading link "
					     << linkNum << endl;
					return false;
				}
				if (nameNodeMap->find(leftName)  
				     == nameNodeMap->end() ||
				    nameNodeMap->find(rightName)
				     == nameNodeMap->end()) {
					cerr << "NetInfo::read: link number "
					     << linkNum 
					     << " refers to unknown node name "
					     << endl;
					return false;
				}
				if (cLink.leftLnum == 0 &&
				    getNodeType((*nameNodeMap)[leftName])
				     ==ROUTER) {
					cerr << "NetInfo::read: missing local "
					     << "link number for router in "
					     << "link " << linkNum 
					     << endl;
					return false;
				}
				if (cLink.rightLnum == 0 &&
				    getNodeType((*nameNodeMap)[rightName])
				     == ROUTER) {
					cerr << "NetInfo::read: missing local "
					     << "link number for router in "
					     << "link " << linkNum 
					     << endl;
					return false;
				}
			} else if (s.compare("bitRate") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cLink.bitRate)) {
					cerr << "NetInfo::read: can't read "
					     << "bit rate for link "
					     << linkNum << endl;
					return false;
				}
			} else if (s.compare("pktRate") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cLink.pktRate)) {
					cerr << "NetInfo::read: can't read "
					     << "packet rate for link "
					     << linkNum << endl;
					return false;
				}
			} else if (s.compare("length") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,linkLength)) {
					cerr << "NetInfo::read: can't read "
					     << "length for link "
					     << linkNum << endl;
					return false;
				}
			} else {
				cerr << "NetInfo::read: syntax error while "
				     << "reading link "
				     << linkNum << endl;
				return false;
			}
				
			break;
			}
		case COMTREE_SEC:
			if (in.peek() == ';') { // end of comtrees section
				in.get(c); context = TOP; break; 
			}

			// clear the current leaf structure
			cComt.comtreeNum = 0; cComt.root = 0;
			cComt.ownerAdr = 0;
			cComt.bitRateDown = 0; cComt.bitRateUp = 0;
			cComt.pktRateDown = 0; cComt.pktRateUp = 0;
			cComt.leafBitRateDown = 0; cComt.leafBitRateUp = 0;
			cComt.leafPktRateDown = cComt.leafPktRateUp = 0;
			cComt.coreSet = new set<int>;
			cComt.lnkMap = new map<int,RateSpec>;
			cComt.rtrMap = new map<int,ComtRtrInfo>;
			context = COMTREE_CTXT;
			// falling through to COMTREE_CTXT context
		case COMTREE_CTXT: {
			if (in.peek() == ';') { // end of link definition
				in.get(c);
				if (comtNum > maxComtree) {
					cerr << "NetInfo::read: too many "
				     	     << "comtrees, max is " 
					     << maxComtree << endl;
					return false;
				}
				if (cComt.root == 0) {
					cerr << "NetInfo::read: no "
					     << "root for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (cComt.comtreeNum == 0) {
					cerr << "NetInfo::read: no "
					     << "comtree number for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (cComt.bitRateDown == 0) {
					cerr << "NetInfo::read: no "
					     << "bitRateDown for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (cComt.bitRateUp == 0) {
					cerr << "NetInfo::read: no "
					     << "bitRateUp for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (cComt.pktRateDown == 0) {
					cerr << "NetInfo::read: no "
					     << "pktRateDown for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (cComt.pktRateUp == 0) {
					cerr << "NetInfo::read: no "
					     << "pktRateUp for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (cComt.leafBitRateDown == 0) {
					cerr << "NetInfo::read: no "
					     << "leafBitRateDown for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (cComt.leafBitRateUp == 0) {
					cerr << "NetInfo::read: no "
					     << "leafBitRateUp for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (cComt.leafPktRateDown == 0) {
					cerr << "NetInfo::read: no "
					     << "leafPktRateDown for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (cComt.leafPktRateUp == 0) {
					cerr << "NetInfo::read: no "
					     << "leafPktRateUp for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				
				// get unused comtree index 
				int ctx = comtreeMap->addPair(cComt.comtreeNum);
				if (ctx == 0) {
					cerr << "NetInfo::read: too many "
						"comtrees" << endl;
					return false;
				}
				comtree[ctx] = cComt;
				cComt.coreSet = 0;
				cComt.lnkMap = 0; cComt.rtrMap = 0;
				comtNum++;
				context = COMTREE_SEC; break;
			}
			if (!Misc::readWord(in, s)) {
				cerr << "NetInfo::read: syntax error when "
				     << "reading " << comtNum
				     << "-th comtree" << endl;
				return false;
			}
			if (s.compare("comtree") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cComt.comtreeNum)) {
					cerr << "NetInfo::read: can't read "
					     << "comtree number for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
			} else if (s.compare("owner") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readWord(in,s)) {
					cerr << "NetInfo::read: can't read "
					     << "owner for " << comtNum 
					     << "-th comtree\n";
					return false;
				}
				int owner = getNodeNum(s);
				if (owner == 0) {
					cerr << "NetInfo::read: specified "
					     << "owner for " << comtNum 
					     << "-th comtree is not valid\n";
					return false;
				}
				cComt.ownerAdr = getNodeAdr(owner);
			} else if (s.compare("root") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readWord(in,s)) {
					cerr << "NetInfo::read: can't read "
					     << "root node for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (nameNodeMap->find(s) ==nameNodeMap->end()) {
					cerr << "NetInfo::read: root in "
					     << comtNum  << "-th comtree "
					     << " is an unknown node name"
					     << endl;
					return false;
				}
				cComt.root = (*nameNodeMap)[s];
				if (!isRouter(cComt.root)) {
					cerr << "NetInfo::read: root node "
					     << "is not a router in "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				cComt.coreSet->insert(cComt.root);
			} else if (s.compare("bitRateDown") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cComt.bitRateDown)) {
					cerr << "NetInfo::read: can't read "
					     << "bitRateDown for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
			} else if (s.compare("bitRateUp") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cComt.bitRateUp)) {
					cerr << "NetInfo::read: can't read "
					     << "bitRateUp for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
			} else if (s.compare("pktRateDown") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cComt.pktRateDown)) {
					cerr << "NetInfo::read: can't read "
					     << "pktRateDown for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
			} else if (s.compare("pktRateUp") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cComt.pktRateUp)) {
					cerr << "NetInfo::read: can't read "
					     << "pktRateUp for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
			} else if (s.compare("leafBitRateDown") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cComt.leafBitRateDown)) {
					cerr << "NetInfo::read: can't read "
					     << "leafBitRateDown for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
			} else if (s.compare("leafBitRateUp") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cComt.leafBitRateUp)) {
					cerr << "NetInfo::read: can't read "
					     << "leafBitRateUp for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
			} else if (s.compare("leafPktRateDown") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cComt.leafPktRateDown)) {
					cerr << "NetInfo::read: can't read "
					     << "leafPktRateDown for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
			} else if (s.compare("leafPktRateUp") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cComt.leafPktRateUp)) {
					cerr << "NetInfo::read: can't read "
					     << "leafPktRateUp for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
			} else if (s.compare("core") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readWord(in,s)) {
					cerr << "NetInfo::read: can't read "
					     << "core for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (nameNodeMap->find(s) ==nameNodeMap->end()) {
					cerr << "NetInfo::read: invalid router "
					     << "name for core in "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				int r = (*nameNodeMap)[s];
				if (!isRouter(r)) {
					cerr << "NetInfo::read: core node "
					     << "is not a router in "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				cComt.coreSet->insert(r);
			} else if (s.compare("link") == 0 &&
				   Misc::verify(in,'=')) {
				string leftName, rightName;
				int leftNum, rightNum;
				leftNum = rightNum = 0;
				if (!Misc::verify(in,'(') ||
				    !Misc::readWord(in,leftName)) {
					cerr << "NetInfo::read: syntax error "
					     << "while reading link in "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (in.peek() == '.') {
					in.get(c);
					if (!Misc::readNum(in,leftNum)) {
						cerr << "NetInfo::read: syntax "
						     << "error while reading "
						     << comtNum << "-th comtree"
						     << endl;
						return false;
					}
				}
				if (!Misc::verify(in,',') ||
				    !Misc::readWord(in,rightName)) {
					cerr << "NetInfo::read: syntax error "
					     << "while reading link in "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (in.peek() == '.') {
					in.get(c);
					if (!Misc::readNum(in,rightNum)) {
						cerr << "NetInfo::read: syntax "
						     << "error while reading "
						     << comtNum << "-th comtree"
						     << endl;
						return false;
					}
				}
				if (!Misc::verify(in,')') ) {
					cerr << "NetInfo::read: syntax error "
					     << "while reading "
					     << comtNum << "-th comtree"
					     << linkNum << endl;
					return false;
				}
				if (nameNodeMap->find(leftName)  
				     == nameNodeMap->end() ||
				    nameNodeMap->find(rightName)
				     == nameNodeMap->end()) {
					cerr << "NetInfo::read: " << comtNum
					     << "-th comtree refers to "
					     << "unknown node name "
					     << endl;
					return false;
				}
				int left = (*nameNodeMap)[leftName];
				if (leftNum == 0 && isRouter(left)) {
					cerr << "NetInfo::read: missing local "
					     << "link number for router in "
					     << comtNum << "-th comtree" 
					     << endl;
					return false;
				}
				int right = (*nameNodeMap)[rightName];
				if (rightNum == 0 && isRouter(right)) {
					cerr << "NetInfo::read: missing local "
					     << "link number for router in "
					     << comtNum << "-th comtree" 
					     << endl;
					return false;
				}
				int ll, lr;
				ll = (isLeaf(left)  ? getLinkNum(left) :
						    getLinkNum(left,leftNum));
				lr = (isLeaf(right) ? getLinkNum(right) :
						    getLinkNum(right,rightNum));
				if (ll == 0 || ll != lr ) {
					cerr << "NetInfo::read: reference to "
					     << "a non-existent link in "
					     << comtNum << "-th comtree" 
					     << endl;
					return false;
				}
				pair<int,RateSpec> newPair1;
				newPair1.first = ll;
				newPair1.second.bitRateUp = 0;
				newPair1.second.bitRateDown = 0;
				newPair1.second.pktRateUp = 0;
				newPair1.second.pktRateDown = 0;
				cComt.lnkMap->insert(newPair1);
				pair<int,ComtRtrInfo> newPair2;
				newPair2.second.plnk = 0;
				newPair2.second.lnkCnt = 0;
				if (getNodeType(left) == ROUTER) {
					newPair2.first = left;
					cComt.rtrMap->insert(newPair2);
				}
				if (getNodeType(right) == ROUTER) {
					newPair2.first = right;
					cComt.rtrMap->insert(newPair2);
				}
			} else {
				cerr << "NetInfo::read: syntax error while "
				     << "reading comtree "
				     << comtNum << endl;
				return false;
			}
				
			break;
			}
		default: fatal("NetInfo::read: undefined context");
		}
	}
	if (cComt.coreSet != 0) delete cComt.coreSet;
	if (cComt.lnkMap != 0) delete cComt.lnkMap;
	if (cComt.rtrMap != 0)  delete cComt.rtrMap;

	bool status = in.eof() && context == TOP && check()
	     	    && setComtLnkNodeInfo();

/*
cerr << "available bit rates for links\n";
for (int lnk = firstLink(); lnk != 0; lnk = nextLink(lnk)) {
	string ss;
	int left = getLinkL(lnk); int right = getLinkR(lnk);
	cerr << link2string(lnk,ss) << " " << getAvailBitRate(lnk,left)
	     << " kb/s " << getAvailPktRate(lnk,left) << " p/s "
	     << getAvailBitRate(lnk,right) << " kb/s "
	     << getAvailPktRate(lnk,right) << " p/s\n";
}
for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx)) {
	map<int,ComtRtrInfo>& rmap = (*comtree[ctx].rtrMap);
	map<int,ComtRtrInfo>::iterator rp;
	cerr << "parent links and link counts for routers in comtree "
	     << getComtree(ctx) << endl;
	for (rp = rmap.begin(); rp != rmap.end(); rp++) {
		string ss1, ss2;
		cerr << "[" << getNodeName(rp->first,ss1) << ": "
		     << link2string(rp->second.plnk,ss2) << " "
		     << rp->second.lnkCnt << "] ";
	}
	cerr << endl;
}
*/

	return status;
}

string& NetInfo::link2string(int lnk, string& s) const {
	if (lnk == 0) { s = "-"; return s; }
	int left = getLinkL(lnk); int right = getLinkR(lnk);
	string s1;
	s = "(" + getNodeName(left,s1);
	if (getNodeType(left) == ROUTER)  {
		s += "." + Misc::num2string(getLocLinkL(lnk),s1);
	}
	s += "," + getNodeName(right,s1);
	if (getNodeType(right) == ROUTER)  {
		s += "." + Misc::num2string(getLocLinkR(lnk),s1);
	}
	s += ")";
	return s;
}

/** Write the contents of a NetInfo object to an output stream.
 *  The object is written out in a form that allows it to be read in again
 *  using the read method. Thus programs that modify the interal representation
 *  can save the result to an output file from which it can later restore the
 *  original configuration. Note that on re-reading, node numbers and link
 *  numbers may change, but semantically, the new version will be equivalent
 *  to the old one.
 *  @param out is an open output stream
 */
void NetInfo::write(ostream& out) const {
	string s = ""; out << toString(s);
}

string& NetInfo::toString(string& s) const {
	// First write the "Routers" section
	string s1 = "";
	s = "Routers\n\n";
	for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
		s += rtr2string(r,s1);
	}
	s += ";\n\n";

	// Next write the LeafNodes, starting with the controllers
	s += "LeafNodes\n\n";
	for (int c = firstController(); c != 0; c = nextController(c)) {
		s += leaf2string(c,s1);
	}
	for (int n = firstLeaf(); n != 0; n = nextLeaf(n)) {
		if (getNodeType(n) != CONTROLLER) s += leaf2string(n,s1);
	}
	s += ";\n\n";

	// Now, write the Links
	s += "Links\n\n"; 
	for (int lnk = firstLink(); lnk != 0; lnk = nextLink(lnk)) {
		s += netlink2string(lnk,s1);
	}
	s += ";\n\n"; 

	// And finally, the Comtrees
	s += "Comtrees\n\n"; 
	for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx)) {
		s += comt2string(ctx,s1);
	}
	s += ";\n"; 
	return s;
}

string& NetInfo::rtr2string(int rtr, string& s) const {
	string s1,s2,s3;
	s = "";
	s += "name=" + getNodeName(rtr,s1) + " "
	  +  "type=" + Forest::nodeType2string(getNodeType(rtr),s2)
	  + " fAdr=" + Forest::fAdr2string(getNodeAdr(rtr),s3);
	s += " leafAdrRange=("
	  + Forest::fAdr2string(getFirstLeafAdr(rtr),s1) + "-"
	  + Forest::fAdr2string(getLastLeafAdr(rtr),s2) + ")";
	char buf[100];
	sprintf(buf,"%f",getNodeLat(rtr));
	s1 = buf; s += "\n\tlocation=(" + s1 + ",";
	sprintf(buf,"%f",getNodeLong(rtr));
	s1 = buf; s += s1 + ")\n";
	s += "interfaces\n";
	s += "# iface#   ipAdr  linkRange  bitRate  pktRate\n";
	for (int i = 1; i <= getNumIf(rtr); i++) {
		if (!validIf(rtr,i)) continue;
		s += "   " + Misc::num2string(i,s1) + "  "
		  +  Np4d::ip2string(getIfIpAdr(rtr,i),s2); 
		if (getIfFirstLink(rtr,i) == getIfLastLink(rtr,i)) 
			s += " " + Misc::num2string(getIfFirstLink(rtr,i),s1)
			  +  " ";
		else
			s += " " + Misc::num2string(getIfFirstLink(rtr,i),s1)
			  +  "-" + Misc::num2string(getIfLastLink(rtr,i), s2)
			  +  "  ";
		s += Misc::num2string(getIfBitRate(rtr,i),s1) + "  "
		  +  Misc::num2string(getIfPktRate(rtr,i),s2) + ";\n";
	}
	s += "end\n;\n";
	return s;
}

string& NetInfo::leaf2string(int leaf, string& s) const {
	string s0, s1, s2, s3;
	s = "";
	s += "name=" + getNodeName(leaf,s0) + " "
	  +  "type=" + Forest::nodeType2string(getNodeType(leaf),s1)
	  +  " ipAdr=" + Np4d::ip2string(getLeafIpAdr(leaf),s2) 
	  +  " fAdr=" + Forest::fAdr2string(getNodeAdr(leaf),s3);
	char buf[100];
	sprintf(buf,"%f",getNodeLat(leaf)); 
	s1 = buf; s += "\n\tlocation=(" + s1 + ",";
	sprintf(buf,"%f",getNodeLong(leaf));
	s1 = buf; s += s1 + ")\n";
	return s;
}

string& NetInfo::netlink2string(int lnk, string& s) const {
	string s0,s1,s2,s3;
	s = "";
	s += "link=" + link2string(lnk,s0) + " bitRate="
	  +  Misc::num2string(getLinkBitRate(lnk),s1) +  " pktRate="
	  +  Misc::num2string(getLinkPktRate(lnk),s2) +  " length=" 
	  +  Misc::num2string(getLinkLength(lnk),s3) + ";\n"; 
	return s;
}

string& NetInfo::comt2string(int ctx, string& s) const {
	s = "";
	if (!validComtIndex(ctx)) return s;
	stringstream ss;
	ss << "comtree=" << getComtree(ctx)
	   << " root=" << getNodeName(getComtRoot(ctx),s)
	   << "\nbitRateDown=" << getComtBrDown(ctx)
	   << " bitRateUp=" << getComtBrUp(ctx)
	   << " pktRateDown=" << getComtPrDown(ctx)
	   << " pktRateUp=" << getComtPrUp(ctx)
	   << "\nleafBitRateDown=" << getComtLeafBrDown(ctx)
	   << " leafBitRateUp=" << getComtLeafBrUp(ctx)
	   << " leafPktRateDown=" << getComtLeafPrDown(ctx)
	   << " leafPktRateUp=" << getComtLeafPrUp(ctx) << "\n"; 

	// iterate through core nodes and print
	for (int c = firstCore(ctx); c != 0; c = nextCore(c,ctx)) {
		if (c != getComtRoot(ctx)) 
			ss << "core=" << getNodeName(c,s) << " ";
	}
	ss << "\n"; 
	// iterate through links and print
	for (int lnk = firstComtLink(ctx); lnk != 0;
		 lnk = nextComtLink(lnk,ctx)) {
		ss << "link=" << link2string(lnk,s) << " ";
	}
	ss << "\n;\n"; 
	s = ss.str();
	return s;
}

string& NetInfo::comtStatusString(int ctx, string& s) const {
	s = "";
	if (!validComtIndex(ctx)) return s;
	stringstream ss;
	ss << "comtree=" << getComtree(ctx)
	   << " root=" << getNodeName(getComtRoot(ctx),s)
	   << "\nbitRateDown=" << getComtBrDown(ctx)
	   << " bitRateUp=" << getComtBrUp(ctx)
	   << " pktRateDown=" << getComtPrDown(ctx)
	   << " pktRateUp=" << getComtPrUp(ctx)
	   << "\nleafBitRateDown=" << getComtLeafBrDown(ctx)
	   << " leafBitRateUp=" << getComtLeafBrUp(ctx)
	   << " leafPktRateDown=" << getComtLeafPrDown(ctx)
	   << " leafPktRateUp=" << getComtLeafPrUp(ctx) << "\n"; 

	// iterate through core nodes and print
	for (int c = firstCore(ctx); c != 0; c = nextCore(c,ctx)) {
		if (c != getComtRoot(ctx)) 
			ss << "core=" << getNodeName(c,s) << " ";
	}
	ss << "\n"; 
	// iterate through links and print
	for (int lnk = firstComtLink(ctx); lnk != 0;
		 lnk = nextComtLink(lnk,ctx)) {
		ss << "link=" << link2string(lnk,s) << " ";
	}
	ss << "\n"; 
	// iterate through nodes and print
	map<int,ComtRtrInfo>::iterator p = comtree[ctx].rtrMap->begin();
	int i = 1;
	while (p != comtree[ctx].rtrMap->end()) {
		int r = p->first;
		int plnk = p->second.plnk;
		ss << "node=(" << getNodeName(r,s) << ","
		   << p->second.lnkCnt << ",";
		ss << link2string(plnk,s) << ") ";
		if ((++i%10) == 0) ss << "\n";
		p++;
	}
	ss << "\n;\n"; 
	s = ss.str();
	return s;
}
