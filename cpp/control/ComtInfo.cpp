/** @file ComtInfo.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ComtInfo.h"

/** Constructor for ComtInfo, allocates space and initializes private data.
 *  @param maxNode1 is the maximum number of nodes in this ComtInfo object
 *  @param maxLink1 is the maximum number of links in this ComtInfo object
 *  @param maxRtr1 is the maximum number of routers in this ComtInfo object
 *  @param maxCtl1 is the maximum number of controllers in this ComtInfo object
 *  @param maxComtree1 is the maximum number of comtrees in this ComtInfo object
 */
ComtInfo::ComtInfo(int maxComtree1, NetInfo& net1)
		 : maxComtree(maxComtree1), net(&net1) {
	comtree = new ComtreeInfo[maxComtree+1];
	comtreeMap = new IdMap(maxComtree);
}

/** Destructor for ComtInfo class.
 *  Deallocates all dynamic storage
 */
ComtInfo::~ComtInfo() {
// add code to delete other comtree info
	for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx)) {
		delete comtree[ctx].coreSet;
		delete comtree[ctx].leafMap;
		delete comtree[ctx].rtrMap;
	}
	delete [] comtree; delete comtreeMap;
}

/** Adjust the subtree rates along path to root.
 *  @param ctx is a valid comtree index
 *  @param rtr is the forest address of a router in the comtree
 *  @param rs is a RateSpec that is to be added to the subtree rates
 *  at all vertices from rtr to the root
 *  @param return true on success, false on failure
 */
bool ComtInfo::adjustSubtreeRates(int ctx, fAdr_t rtrAdr, RateSpec& rs) {
	int rtr = net->getNodeNum(rtrAdr);
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(rtrAdr);
	int count = 0;
	while (true) {
		rp->second.subtreeRates.add(rs);
		if (rp->second.plnk == 0) return true;
		rtr = net->getPeer(rtr,rp->second.plnk);
		rp = comtree[ctx].rtrMap->find(net->getNodeAdr(rtr));
		if (count++ > 50) {
			cerr << "ComtInfo::adjustSubtreeRates: excessively "
				"long path detected in comtree "
			     << getComtree(ctx) << ", probably a cycle\n";
			return false;
		}
	}
}

/** Read comtrees from an input stream.
 *  Comtree file contains one of more comtree definitions, where a
 *  comtree definition is as shown below
 *  <code>
 *  #      comtNum, owner,root,configMode,  backbone rates,      leaf rates
 *  comtree(1001,  netMgr,  r1,   manual, (1000,2000,1000,2000),(10,50,25,200),
 *              (r1,r2,r3),  # core nodes
 *     		(r1.1,r2.2,(1000,2000,1000,2000)),
 *     		(r1.2,r3.3,(1000,2000,1000,2000))
 *  )
 *  ; # required after last comtree 
 *  </code>
 *
 *  The list of core nodes and the list of links may both be omitted.
 *  To omit the list of core nodes, while providing the list of links,
 *  use a pair of commas after the leaf default rates.
 *  In the list of links, the first named node is assumed to be the child
 *  of the other node in the comtree. The RateSpec may be omitted, but
 *  if provided, the first number is the upstream bit rate, followed
 *  by downstream bit rate, upstream packet rate and downstream packet rate.
 */
bool ComtInfo::read(istream& in) {
	int comtNum = 1;	// comtNum = i for i-th comtree read

	string s, errMsg;
	while (!in.eof()) {
		if (!Util::skipBlank(in) || Util::verify(in,';')) break;
		if (!Util::readWord(in, s)) {
			cerr << "NetInfo:: read: syntax error: expected "
				"(;) or keyword (router,leaf,link)\n";
			return false;
		}
		if (s == "comtree") {
			if (readComtree(in, errMsg) == 0) {
				cerr << "ComtInfo::read: error when attempting "
					" to read " << comtNum << "-th comtree "
					"(" << errMsg << ")\n";
				return false;
			}
		} else {
			cerr << "ComtInfo::read: unrecognized word " << s
			     << " when attempting to read "
			     << comtNum << "-th comtree\n";
			return false;
		}
		comtNum++;
	}
	return check() && setAllComtRates();
}

/** Read a comtree description from an input stream.
 *  Expects the next nonblank input character to be the opening parenthesis
 *  of the comtree description. If a new comtree is read without error,
 *  proceeds to allocate and initialize the new comtree.
 *  @param in is an open input stream
 *  @param errMsg is a reference to a string used for returning an error
 *  message
 *  @return the comtree index for the new comtree, or 0 on error
 */
comt_t ComtInfo::readComtree(istream& in, string& errMsg) {
	comt_t comt; fAdr_t owner; fAdr_t root;
	int core; vector<int> coreNodes;
	bool autoConfig; RateSpec bbRates, leafRates;
	
	errMsg = "";
	if (!Util::verify(in,'(',50)) {
		errMsg = "syntax error, expected left paren";
		return 0;
	}
	// read comtree number
	Util::skipBlank(in);
	in >> comt;
	if (in.fail() || !Util::verify(in,',',20)) {
		errMsg = "could not read comtree number";
		return 0;
	}
	// read owner name
	Util::skipBlank(in);
	string s;
	if (!Util::readWord(in,s) || !Util::verify(in,',',20) ||
	    (owner = net->getNodeNum(s)) == 0) {
		errMsg = "could not read owner name";
		return 0;
	}
	// read root node name
	Util::skipBlank(in);
	if (!Util::readWord(in,s) || !Util::verify(in,',',20) ||
	    (root = net->getNodeNum(s)) == 0) {
		errMsg = "could not read root node name";
		return 0;
	}
	// read backbone configuration mode
	Util::skipBlank(in);
	if (!Util::readWord(in,s)) {
		errMsg = "could not read backbone configuration mode";
		return 0;
	}
	if (s == "auto") autoConfig = true;
	else if (s == "manual") autoConfig = false;
	else {
		errMsg = "invalid backbone configuration mode " + s;
		return 0;
	}
	// read default rate specs
	Util::skipBlank(in);
	if (!Util::verify(in,',',20) || !readRateSpec(in,bbRates)) {
		errMsg = "could not backbone default rates";
		return 0;
	}
	Util::skipBlank(in);
	if (!Util::verify(in,',',20) || !readRateSpec(in,leafRates)) {
		errMsg = "could not read leaf default rates";
		return 0;
	}
	// read list of core nodes (may be empty)
	Util::skipBlank(in);
	if (Util::verify(in,',',20)) { // read list of additional core nodes
		Util::skipBlank(in);
		if (Util::verify(in,'(',20)) {
			if (!Util::verify(in,')',20)) {
				while (true) {
					Util::skipBlank(in);
					string s;
			    		if (!Util::readWord(in,s)) {
						errMsg = "could not read core "
							 "node name";
						return 0;
					}
			    		if ((core = net->getNodeNum(s)) == 0) {
						errMsg = "invalid core node "
							 "name " + s;
						return 0;
					}
					coreNodes.push_back(core);
					if (Util::verify(in,')',20)) break;
					if (!Util::verify(in,',',20)) {
						errMsg = "syntax error in list "
							 "of core nodes "
							 "after " + s;
						return 0;
					}
				}
			}
		}
	}
	// read list of links (may be empty)
	vector<LinkMod> links;
	Util::skipBlank(in);
	LinkMod lm;
	if (Util::verify(in,',',20)) {
		int lnk; int child;
		RateSpec rs; rs.set(-1);
		if (!readLink(in,lnk,rs,child,errMsg)) return 0;
		lm.set(lnk,child,rs);
		links.push_back(lm);
		while (Util::verify(in,',',20)) {
			rs.set(-1);
			if (!readLink(in,lnk,rs,child,errMsg)) return 0;
			lm.set(lnk,child,rs);
			links.push_back(lm);
		}
	}
	if (!Util::verify(in,')',20)) { 
		errMsg = "syntax error at end of link list, expected "
			 "right paren";
		return 0;
	}
	int ctx;
	if ((ctx = addComtree(comt)) == 0) {
		errMsg = "could not allocate new comtree";
		return 0;
	}
	
	fAdr_t ownerAdr = net->getNodeAdr(owner);
	fAdr_t rootAdr  = net->getNodeAdr(root);

	// configure comtree
	if (!setOwner(ctx,ownerAdr) || !setRoot(ctx,rootAdr)) {
		errMsg = "could not configure comtree";
		return 0;
	}
	setConfigMode(ctx,autoConfig);
	setDefRates(ctx,bbRates,leafRates);

	if (!addNode(ctx,rootAdr) || !addCoreNode(ctx,rootAdr)) {
		errMsg = "could not add root to comtree";
		return 0;
	}
	for (unsigned int i = 0; i < coreNodes.size(); i++) {
		fAdr_t cnAdr = net->getNodeAdr(coreNodes[i]);
		if (!addNode(ctx,cnAdr) || !addCoreNode(ctx,cnAdr)) {
			errMsg = "could not add core node to comtree";
			return 0;
		}
	}

	for (unsigned int i = 0; i < links.size(); i++) {
		int lnk = links[i].lnk;
		int child = links[i].child;
		int parent = net->getPeer(child,lnk);
		fAdr_t childAdr = net->getNodeAdr(child);
		fAdr_t parentAdr = net->getNodeAdr(parent);
		RateSpec rs = links[i].rs;

		addNode(ctx,childAdr); addNode(ctx,parentAdr);
		if (net->isLeaf(child)) {
			map<fAdr_t,ComtLeafInfo>::iterator cp;
			cp = comtree[ctx].leafMap->find(childAdr);
			if (rs.bitRateUp == -1) rs = leafRates;
			cp->second.plnkRates = rs;
		} else {
			map<fAdr_t,ComtRtrInfo>::iterator cp;
			cp = comtree[ctx].rtrMap->find(childAdr);
			cp->second.plnk = lnk; cp->second.lnkCnt++;
			if (rs.bitRateUp == -1) rs = bbRates;
			else cp->second.frozen = true;
			cp->second.plnkRates = rs;
			rs = cp->second.subtreeRates;
		}
		adjustSubtreeRates(ctx,parentAdr,rs);
		map<fAdr_t,ComtRtrInfo>::iterator pp;
		pp = comtree[ctx].rtrMap->find(parentAdr);
		pp->second.lnkCnt++;
	}

	return true;
}

/** Read a rate specification.
 *  @param in is an open input stream
 *  @param rs is a reference to a RateSpec in which result is returned
 *  @return true on success, false on failure
 */
bool ComtInfo::readRateSpec(istream& in, RateSpec& rs) {
	int bru, brd, pru, prd;
	if (!Util::verify(in,'(',50)) return false;
	in >> bru; if (in.fail()) return false;
	if (!Util::verify(in,',',20)) return false;
	in >> brd; if (in.fail()) return false;
	if (!Util::verify(in,',',20)) return false;
	in >> pru; if (in.fail()) return false;
	if (!Util::verify(in,',',20)) return false;
	in >> prd; if (in.fail()) return false;
	if (!Util::verify(in,')',20)) return false;
	rs.set(bru,brd,pru,prd);
	return true;
}

/** Read a link description from an input stream.
 *  Expects the next nonblank input character to be the opening parenthesis.
 *  @param in is an open input stream
 *  @param lnk is used to return the link number for the link
 *  @param rs is used to return the RateSpec for this link
 *  @param child is used to return the "child" endpoint of the link
 *  @param errMsg is a reference to a string used for returning an error
 *  message
 *  @return true on success, false on failure
 */
bool ComtInfo::readLink(istream& in, int& lnk, RateSpec& rs, int& child,
			string& errMsg) {
	string nameL, nameR; int numL, numR;
	
	errMsg = "";
	// read the link endpoints
	if (!Util::verify(in,'(',50) ||
	    !readLinkEndpoint(in,nameL,numL) || !Util::verify(in,',',20)) {
		errMsg = "could not first link endpoint";
		return false;
	}
	if (!readLinkEndpoint(in,nameR,numR)) {
		errMsg = "could not read second link endpoint";
		return false;
	}
	// find the link number in the NetInfo object
	if ((child = net->getNodeNum(nameL)) == 0) {
		errMsg = "invalid name for link endpoint " + nameL;
		return false;
	}
	int parent;
	if ((parent = net->getNodeNum(nameR)) == 0) {
		errMsg = "invalid name for link endpoint " + nameR;
		return false;
	}
	if (!net->isRouter(parent)) {
		errMsg = "invalid link: first node must be child in comtree";
		return false;
	}
	lnk = net->getLinkNum(child,numL);
	if (lnk != net->getLinkNum(parent,numR) || lnk == 0) {
		stringstream ss;
		ss << "detected invalid link (" << nameL;
		if (numL != 0) ss << "." << numL;
		ss << "," << nameR;
		if (numR != 0) ss << "." << numR;
		ss << ")";
		errMsg = ss.str();
		return false;
	}
	if (Util::verify(in,',',20)) {
		if (!readRateSpec(in,rs)) {
			errMsg = "could not read rate specification for link";
			return false;
		}
	}
	if (!Util::verify(in,')',20)) {
		errMsg = "syntax error, expected right paren";
		return false;
	}
	return true;
}

/** Read a link endpoint.
 *  @param in is an open input stream
 *  @param name is a reference to a string used to return the endpoint name
 *  @param num is a reference to an int used to return the local link number
 *  @return true on success, false on failure
 */
bool ComtInfo::readLinkEndpoint(istream& in, string& name, int& num) {
	if (!Util::readWord(in,name)) return false;
	num = 0;
	if (Util::verify(in,'.')) {
		in >> num;
		if (in.fail() || num < 1) return false;
	}
	return true;	
}
/** Perform consistency check on all comtrees.
 *  @return true if all comtrees pass all checks, else false
 */
bool ComtInfo::check() {
	bool status = true;
	for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx)) {
		int comt = getComtree(ctx);
		fAdr_t rootAdr = getRoot(ctx);
		int root = net->getNodeNum(rootAdr);

		// first check that every leaf in comtree has a parent
		// that is a router in the comtree
		map<fAdr_t,ComtLeafInfo>::iterator lp;
		for (lp  = comtree[ctx].leafMap->begin();
		     lp != comtree[ctx].leafMap->end(); lp++) {
			if (!isComtRtr(ctx,getParent(ctx,lp->first))) {
				int leaf = net->getNodeNum(lp->first);
				string leafName;
				if (leaf != 0) net->getNodeName(leaf,leafName);
				else Forest::fAdr2string(leaf,leafName);
				cerr << "ComtInfo::check: comtree " << comt 
				     << " has leaf " << leafName << " whose "
					"parent is not a router in comtree\n";
				status = false;
			}
		}
		// next, check that at most one router in comtree lacks a
		// parent
		map<fAdr_t,ComtRtrInfo>::iterator rp;
		int cnt = 0;
		for (rp  = comtree[ctx].rtrMap->begin();
		     rp != comtree[ctx].rtrMap->end(); rp++) {
			if (getParent(ctx,rp->first) == 0) cnt++;
		}
		if (cnt != 1) {
			cerr << "ComtInfo::check: comtree " << comt 
			     << " has " << cnt << " routers with no parent\n";
				status = false;
		}

		// check that the comtree backbone topology is really a tree
		// by doing a breadth-first search from the root;
		// while we're at it, make sure the parent of every
		// core node is a core node and that the zip codes
		// of routers within the comtree are contiguous
		queue<int> pending; pending.push(root);
		map<fAdr_t,int> plink; plink[root] = 0;
		set<fAdr_t> zipSet;
		zipSet.insert(Forest::zipCode(rootAdr));
		unsigned int nodeCount = 0;
		while (!pending.empty()) {
			int u = pending.front(); pending.pop();
			fAdr_t uAdr = net->getNodeAdr(u);
			nodeCount++;
			bool foundCycle = false;
			int uzip = Forest::zipCode(net->getNodeAdr(u));
			for (int lnk = net->firstLinkAt(u); lnk != 0;
				 lnk = net->nextLinkAt(u,lnk)) {
				int v = net->getPeer(u,lnk);
				if (!net->isRouter(v)) continue;
				fAdr_t vAdr = net->getNodeAdr(v);
				if (!isComtNode(ctx,vAdr)) continue;
				if (getPlink(ctx,vAdr) != lnk) continue;
				if (lnk == plink[u]) continue;
				int vzip = Forest::zipCode(vAdr);
				if (plink.find(v) != plink.end()) {
					cerr << "ComtInfo::check: "
						"comtree " << comt
					     << " contains a cycle\n";
					foundCycle = true;
					break;
				}
				plink[v] = lnk;
				pending.push(v);
				// now check that if v is in core, so is u
				if (isCoreNode(ctx,vAdr) &&
				    !isCoreNode(ctx,uAdr)) {
					string s;
					cerr << "ComtInfo::check: comtree "
					     << comt << " contains a core node "
					     << net->getNodeName(v,s)
					     << " whose parent is not a "
						"core node\n";
					status = false;
				}
				// now check that if v has a different zip code
				// than u, that we haven't already seen this zip
				if (vzip != uzip) {
					if (zipSet.find(vzip) != zipSet.end()) {
						cerr << "ComtInfo::check: zip "
							"code " << vzip <<
							" is non-contiguous in "
							"comtree " << comt
						     << "\n";
						status = false;
					} else {
						zipSet.insert(vzip);
					}
				}
			}
			if (foundCycle) { status = false; break; }
		}
		if (nodeCount != comtree[ctx].rtrMap->size()) {
			cerr << "ComtInfo::check: comtree " << comt
			     << " not connected\n";
			status = false;
		}
	}
	return status;
}

bool ComtInfo::checkLinkCounts(int ctx) {
	int n = net->getMaxRouter();
	int *lnkCounts = new int[n+1];

	bool status = true;
	for (int i = 1; i <= n; i++) lnkCounts[i] = 0;

	int comt = getComtree(ctx);
	fAdr_t rootAdr = getRoot(ctx);
	int root = net->getNodeNum(rootAdr);

	// first count links from leaf nodes
	map<fAdr_t,ComtLeafInfo>::iterator lp;
	for (lp  = comtree[ctx].leafMap->begin();
	     lp != comtree[ctx].leafMap->end(); lp++) {
		fAdr_t padr = getParent(ctx,lp->first);
		int parent = net->getNodeNum(padr);
		lnkCounts[parent]++;
	}
	// next, count links to other routers
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	for (rp  = comtree[ctx].rtrMap->begin();
	     rp != comtree[ctx].rtrMap->end(); rp++) {
		fAdr_t radr = rp->first;
		int rtr = net->getNodeNum(radr);
		fAdr_t padr = getParent(ctx,radr);
		if (padr == 0) continue;
		int parent = net->getNodeNum(padr);
		lnkCounts[parent]++;
		lnkCounts[rtr]++;
	}

	// now check stored link counts for routers
	for (rp  = comtree[ctx].rtrMap->begin();
	     rp != comtree[ctx].rtrMap->end(); rp++) {
		fAdr_t radr = rp->first;
		int rtr = net->getNodeNum(radr);
		if (lnkCounts[rtr] != rp->second.lnkCnt) {
			string s;
			cerr << "router " << net->getNodeName(rtr,s)
			     << " has " << lnkCounts[rtr]
			     << " links in comtree " << comt
			     << ", but recorded lnkCnt is "
			     << rp->second.lnkCnt;
			status = false;
		}
	}
	return status;
}

bool ComtInfo::checkSubtreeRates(int ctx) {
	int n = net->getMaxRouter();
	RateSpec *subtreeRates = new RateSpec[n+1];

	bool status = true;
	for (int i = 1; i <= n; i++) subtreeRates[i].set(0);

	int comt = getComtree(ctx);
	fAdr_t rootAdr = getRoot(ctx);
	int root = net->getNodeNum(rootAdr);

	// compute bottom-up from leaf nodes
	// check for negative rates while we're at it
	map<fAdr_t,ComtLeafInfo>::iterator lp;
	for (lp  = comtree[ctx].leafMap->begin();
	     lp != comtree[ctx].leafMap->end(); lp++) {
		RateSpec& prates = lp->second.plnkRates;
		if (prates.bitRateUp <= 0 || prates.bitRateDown <= 0 ||
		    prates.pktRateUp <= 0 || prates.pktRateDown <= 0) {
			string s1, s2;
			int lnk = net->getLinkNum(
				  	net->getNodeNum(lp->second.parent),
					lp->second.llnk);
			cerr << "detected non-positive comtree link rate for "
			     << comt << " link " << net->link2string(lnk,s1)
			     << " rateSpec=" << prates.toString(s2) << endl;
			status = false;
		}
		fAdr_t radr = getParent(ctx,lp->first);
		while (true) {
			int rtr = net->getNodeNum(radr);
			subtreeRates[rtr].add(prates);
			if (rtr == root) break;
			radr = getParent(ctx,radr);
		} 
	}

	// now check stored subtree rates
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	for (rp  = comtree[ctx].rtrMap->begin();
	     rp != comtree[ctx].rtrMap->end(); rp++) {
		fAdr_t radr = rp->first;
		int rtr = net->getNodeNum(radr);
		if (!subtreeRates[rtr].equals(rp->second.subtreeRates)) {
			string s1, s2, s3;
			cerr << "router " << net->getNodeName(rtr,s1)
			     << " has subtree rate "
			     << subtreeRates[rtr].toString(s2)
			     << " in comtree " << comt << ", but recorded"
			     << " value is "
			     << rp->second.subtreeRates.toString(s3) << endl;
			status = false;
		}
	}
	return status;
}

bool ComtInfo::checkLinkRates(int ctx) {
	if (!getConfigMode(ctx)) return true;

	bool status = true;
	int comt = getComtree(ctx);
	fAdr_t rootAdr = getRoot(ctx);
	int root = net->getNodeNum(rootAdr);
	
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(rootAdr);
	RateSpec& rootRates = rp->second.subtreeRates;

	for (rp  = comtree[ctx].rtrMap->begin();
	     rp != comtree[ctx].rtrMap->end(); rp++) {
		if (rp->second.frozen) continue;
		int lnk = rp->second.plnk;
		if (lnk == 0) continue;
		fAdr_t rtr = rp->first;
		RateSpec rs(0);
		RateSpec& srates = rp->second.subtreeRates;
		RateSpec trates = rootRates; trates.subtract(srates);
		if (isCoreNode(ctx,rtr)) {
			rs.set(	srates.bitRateUp,trates.bitRateUp,
				srates.pktRateUp,trates.pktRateUp);
		} else {		
			rs.set(	srates.bitRateUp,
			    	min(srates.bitRateDown,trates.bitRateUp),
			    	srates.pktRateUp,
			    	min(srates.pktRateDown,trates.pktRateUp));
		}
		if (!rs.equals(rp->second.plnkRates)) {
			string s1, s2, s3;
			cerr << "detected inconsistent comtree link rates in "
			     << comt << " link " << net->link2string(lnk,s1)
			     << " computed rates: " << rs.toString(s2)
			     << " and stored rates: "
			     << rp->second.plnkRates.toString(s3) << endl;
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
bool ComtInfo::setAllComtRates() {
	for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx)) {
		if (!setComtRates(ctx)) return false;
	}
	return true;
}

/** Compute rates for all links in a comtree and allocate capacity of
 *  network links. This is for use when configuring an entire comtree,
 *  not when processing joins/leaves.
 *  @param ctx is a valid comtree index
 *  @return true on success, false on failure
 */
bool ComtInfo::setComtRates(int ctx) {
	if (!validComtIndex(ctx)) return false;
	if (getConfigMode(ctx)) setAutoConfigRates(ctx);
	if (!checkComtRates(ctx)) {
		cerr << "network lacks capacity for comtree "
		     << getComtree(ctx) << endl;
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
void ComtInfo::setAutoConfigRates(int ctx) {
	fAdr_t root = getRoot(ctx);
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(root);
	RateSpec& rootRates = rp->second.subtreeRates;
	for (rp  = comtree[ctx].rtrMap->begin();
	     rp != comtree[ctx].rtrMap->end(); rp++) {
		if (rp->second.frozen) continue;
		int lnk = rp->second.plnk;
		if (lnk == 0) continue;
		fAdr_t rtr = rp->first;
		RateSpec& rs = rp->second.plnkRates;
		RateSpec& srates = rp->second.subtreeRates;
		RateSpec trates = rootRates; trates.subtract(srates);
		if (isCoreNode(ctx,rtr)) {
			rs.set(	srates.bitRateUp,trates.bitRateUp,
				srates.pktRateUp,trates.pktRateUp);
		} else {		
			rs.set(	srates.bitRateUp,
			    	min(srates.bitRateDown,trates.bitRateUp),
			    	srates.pktRateUp,
			    	min(srates.pktRateDown,trates.pktRateUp));
		}
	}
	return;
}

/** Check that there is sufficient capacity available for all links in a
 *  comtree. Requires that rates have been set for all comtree links.
 *  @param ctx is a valid comtree index
 *  @return true if the provisioned rates for the comtree are compatible
 *  with the available capacity
 */
bool ComtInfo::checkComtRates(int ctx) {
	// first check parent links at routers
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	for (rp  = comtree[ctx].rtrMap->begin();
	     rp != comtree[ctx].rtrMap->end(); rp++) {
		fAdr_t rtr = rp->first; int lnk = rp->second.plnk;
		RateSpec rs = rp->second.plnkRates;
		if (rtr != net->getLeft(lnk)) rs.flip();
		RateSpec availRates;
		net->getAvailRates(lnk,availRates);
		if (!rs.leq(availRates)) return false;
	}
	// then check access links, for static leaf nodes
	map<fAdr_t,ComtLeafInfo>::iterator lp;
	for (lp  = comtree[ctx].leafMap->begin();
	     lp != comtree[ctx].leafMap->end(); lp++) {
		fAdr_t leafAdr = lp->first;
		int leaf = net->getNodeNum(leafAdr);
		if (leaf == 0) continue;
		int lnk = net->firstLinkAt(leaf);
		RateSpec rs = lp->second.plnkRates;
		if (leaf != net->getLeft(lnk)) rs.flip();
		RateSpec availRates;
		net->getAvailRates(lnk,availRates);
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
void ComtInfo::provision(int ctx) {
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	for (rp  = comtree[ctx].rtrMap->begin();
	     rp != comtree[ctx].rtrMap->end(); rp++) {
		fAdr_t rtr = rp->first; int lnk = rp->second.plnk;
		if (lnk == 0) continue;
		RateSpec rs = rp->second.plnkRates;
		if (rtr != net->getLeft(lnk)) rs.flip();
		RateSpec availRates;
		net->getAvailRates(lnk,availRates);
		availRates.subtract(rs);
		net->setAvailRates(lnk,availRates);
	}
	// provision statically configured leaf nodes
	map<fAdr_t,ComtLeafInfo>::iterator lp;
	for (lp  = comtree[ctx].leafMap->begin();
	     lp != comtree[ctx].leafMap->end(); lp++) {
		fAdr_t leafAdr = lp->first;
		int leaf = net->getNodeNum(leafAdr);
		if (leaf == 0) continue;
		int lnk = net->firstLinkAt(leaf);
		RateSpec rs = lp->second.plnkRates;
		if (leaf != net->getLeft(lnk)) rs.flip();
		RateSpec availRates;
		net->getAvailRates(lnk,availRates);
		availRates.subtract(rs);
		net->setAvailRates(lnk,availRates);
	}
}

/** Unprovision all links in a comtree,increasing available link capacity.
 *  @param ctx is a valid comtree index
 *  @return true on success, else false
 */
void ComtInfo::unprovision(int ctx) {
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	for (rp  = comtree[ctx].rtrMap->begin();
	     rp != comtree[ctx].rtrMap->end(); rp++) {
		fAdr_t rtr = rp->first; int lnk = rp->second.plnk;
		if (lnk == 0) continue;
		RateSpec rs = rp->second.plnkRates;
		if (rtr != net->getLeft(lnk)) rs.flip();
		RateSpec availRates;
		net->getAvailRates(lnk,availRates);
		availRates.add(rs);
		net->setAvailRates(lnk,availRates);
	}
	map<fAdr_t,ComtLeafInfo>::iterator lp;
	for (lp  = comtree[ctx].leafMap->begin();
	     lp != comtree[ctx].leafMap->end(); lp++) {
		fAdr_t leafAdr = lp->first;
		int leaf = net->getNodeNum(leafAdr);
		if (leaf == 0) continue;
		int lnk = net->firstLinkAt(leaf);
		RateSpec rs = lp->second.plnkRates;
		if (leaf != net->getLeft(lnk)) rs.flip();
		RateSpec availRates;
		net->getAvailRates(lnk,availRates);
		availRates.add(rs);
		net->setAvailRates(lnk,availRates);
	}
}

/** Find a path from a vertex to a comtree.
 *  This method builds a shortest path tree with a given source node.
 *  The search is done over all network links that have the available capacity
 *  to handle a provided RateSpec.
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
int ComtInfo::findPath(int ctx, int src, RateSpec& rs, list<LinkMod>& path) {
	path.clear();
	fAdr_t srcAdr = net->getNodeAdr(src);
	if (isComtNode(ctx,srcAdr)) return src;
	int n = 0;
	for (fAdr_t r = net->firstRouter(); r != 0; r = net->nextRouter(r)) {
		n = max(r,n);
	}
	Heap h(n); int d[n+1];
	int plnk[n+1]; // note: reverse from direction in comtree
	for (fAdr_t r = net->firstRouter(); r != 0; r = net->nextRouter(r)) {
		plnk[r] = 0; d[r] = BIGINT;
	}
	RateSpec availRates;
	d[src] = 0;
	h.insert(src,d[src]);
	while (!h.empty()) {
		fAdr_t r = h.deletemin();
		for (int lnk = net->firstLinkAt(r); lnk != 0;
			 lnk = net->nextLinkAt(r,lnk)) {
			if (lnk == plnk[r]) continue;
			int peer = net->getPeer(r,lnk);
			if (!net->isRouter(peer)) continue;
			// if this link cannot take the default backbone
			// rate for this comtree, ignore it
			net->getAvailRates(lnk,availRates);
			if (r != net->getLeft(lnk)) availRates.flip();
			if (!rs.leq(availRates)) continue;
			if (isComtNode(ctx,net->getNodeAdr(peer))) { // done
				plnk[peer] = lnk;
				int u = peer;
				for (int pl = plnk[u]; pl != 0; pl = plnk[u]) {
					int v = net->getPeer(u,plnk[u]);
					LinkMod lm(pl,v,rs);
					path.push_front(lm);
					u = v;
				}
				return peer;
			}
			if (d[peer] > d[r] + net->getLinkLength(lnk)) {
				plnk[peer] = lnk;
				d[peer] = d[r] + net->getLinkLength(lnk);
				if (h.member(peer)) h.changekey(peer,d[peer]);
				else h.insert(peer,d[peer]);
			}
		}
	}
	return 0;
}

/** Add a path from a router to a comtree.
 *  This method adds a list of links to the backbone of an existing comtree,
 *  sets their link rates and reserves capacity on the network links;
 *  the links are assumed to have sufficient available capacity.
 *  @param ctx is a valid comtree index for the comtree of interest
 *  @param path is a reference to a list of LinkMod objects defining a path
 *  in the comtree; the links appear in "bottom-up" order
 *  leading from some router outside the comtree to a router in the comtree
 */
void ComtInfo::addPath(int ctx, list<LinkMod>& path) {
	if (path.begin() == path.end()) return;
	list<LinkMod>::iterator pp;
	for (pp = path.begin(); pp != path.end(); pp++) {
		// add lnk to the comtree in net
		int child = pp->child; int lnk = pp->lnk;
		RateSpec rs = pp->rs;
		int parent = net->getPeer(child,lnk);
		fAdr_t childAdr = net->getNodeAdr(child);
		fAdr_t parentAdr = net->getNodeAdr(parent);
		addNode(ctx,childAdr); addNode(ctx,parentAdr);
		setPlink(ctx,childAdr,lnk); thaw(ctx,childAdr);
		setLinkRates(ctx,childAdr,rs);
		if (child != net->getLeft(lnk)) rs.flip();
		RateSpec availRates;
		net->getAvailRates(lnk,availRates);
		availRates.subtract(rs);
		net->setAvailRates(lnk,availRates);
	}
	return;
}

/** Remove a path from a router in a comtree.
 *  @param ctx is a valid comtree index for the comtree of interest
 *  @param path is a list of LinkMod objects defining the path
 *  in bottom-up order; the path is assumed to have
 *  no "branching links" incident to the path
 */
void ComtInfo::removePath(int ctx, list<LinkMod>& path) {
	if (path.begin() == path.end()) return;
	list<LinkMod>::iterator pp;
	for (pp = path.begin(); pp != path.end(); pp++) {
		fAdr_t childAdr = net->getNodeAdr(pp->child);
		RateSpec rs; getLinkRates(ctx,childAdr,rs);
		if (pp->child != net->getLeft(pp->lnk)) rs.flip();
		RateSpec availRates;
		net->getAvailRates(pp->lnk,availRates);
		availRates.add(rs);
		net->setAvailRates(pp->lnk,availRates);
		removeNode(ctx,childAdr);
	}
	return;
}

bool ComtInfo::computeMods(int ctx, list<LinkMod>& modList) {
	fAdr_t root = getRoot(ctx);
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(root);
	modList.clear();
	return computeMods(ctx,root,rp->second.subtreeRates,modList);
}

/** Compute the change required in the RateSpecs for links in an
 *  auto-configured comtree and verify that needed capacity is available.
 *  @param ctx is the comtree index of the comtree
 *  @param radr is the Forest address of the root of the current subtree;
 *  should be the comtree root in the top level recursive call
 *  @param modList is a list of edges in the comtree and RateSpecs
 *  representing the change in rates required at each link in the list;
 *  returned RateSpecs may have negative values; list should be empty 
 *  in top level recursive call
 *  @return true if all links in comtree have sufficient available
 *  capacity to accommodate the changes
 */
bool ComtInfo::computeMods(int ctx, fAdr_t radr, RateSpec& rootRates,
			     list<LinkMod>& modList) {
	int rnum = net->getNodeNum(radr);
	if (!net->isRouter(rnum)) return true;
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(radr);
	int plnk = rp->second.plnk;
	if (plnk != 0 && !isFrozen(ctx,radr)) {
		// determine the required rates on link to parent
		LinkMod nuMod;
		nuMod.child = rnum; nuMod.lnk = plnk;
		RateSpec& srates = rp->second.subtreeRates;
		RateSpec trates = rootRates; trates.subtract(srates);
		if (isCoreNode(ctx,radr)) {
			nuMod.rs.set(srates.bitRateUp,trates.bitRateUp,
					srates.pktRateUp,trates.pktRateUp);
		} else {		
			nuMod.rs.set(srates.bitRateUp,
			    	min(srates.bitRateDown,trates.bitRateUp),
			    	srates.pktRateUp,
			    	min(srates.pktRateDown,trates.pktRateUp));
		}
		nuMod.rs.subtract(rp->second.plnkRates);
		if (nuMod.rs.isZero()) return true; // no change needed
		RateSpec availRates; net->getAvailRates(plnk,availRates);
		if (rnum != net->getLeft(plnk)) availRates.flip();
		if (!nuMod.rs.leq(availRates)) return false;
		modList.push_back(nuMod);
	}

	// Now, do subtrees; note that this method is going to be
	// slow for comtrees with very large backbones; only way to
	// fix it is to add child and sibling pointers; later, maybe
	for (rp  = comtree[ctx].rtrMap->begin();
	     rp != comtree[ctx].rtrMap->end(); rp++) {
		if (radr != getParent(ctx,rp->first)) continue;
		if (!computeMods(ctx,rp->first,rootRates,modList))
			return false;
	}
	return true;
}

/** Add to the capacity of a list of comtree backbone links and provision
 *  required link capacity in the network. Assumes that network links have
 *  the required capacity and that both endpoints of each link are routers.
 *  @param ctx is the index of a comptree
 *  @param modList is a list of link modifications; that is, comtree links
 *  that are either to be added or have their rate adjusted.
 *  and a RateSpec difference to be added to that link.
 */
void ComtInfo::provision(int ctx, list<LinkMod>& modList) {
	list<LinkMod>::iterator mlp;
	RateSpec delta, availRates;
	for (mlp = modList.begin(); mlp != modList.end(); mlp++) {
		int rtr = mlp->child; fAdr_t rtrAdr = net->getNodeAdr(rtr);
		int plnk = mlp->lnk; delta = mlp->rs;
		map<fAdr_t,ComtRtrInfo>::iterator rp;
		rp = comtree[ctx].rtrMap->find(rtrAdr);
		rp->second.plnkRates.add(delta);
		net->getAvailRates(plnk,availRates);
		if (rtr != net->getLeft(plnk)) delta.flip();
		availRates.subtract(delta);
		net->setAvailRates(plnk,availRates);
	}
}

/** Reduce the capacity of a list of comtree links and release required
 *  link capacity in the network.
 *  @param ctx is the index of a comptree
 *  @param modList is a list of nodes, each representing a link in the comtree
 *  and a RateSpec difference to be removed from that link.
 */
void ComtInfo::unprovision(int ctx, list<LinkMod>& modList) {
	list<LinkMod>::iterator mlp;
	RateSpec delta, availRates;
	for (mlp = modList.begin(); mlp != modList.end(); mlp++) {
		int rtr = mlp->child; fAdr_t rtrAdr = net->getNodeAdr(rtr);
		int plnk = mlp->lnk; delta = mlp->rs;
		map<fAdr_t,ComtRtrInfo>::iterator rp;
		rp = comtree[ctx].rtrMap->find(rtrAdr);
		rp->second.plnkRates.add(delta);
		net->getAvailRates(plnk,availRates);
		if (rtr != net->getLeft(plnk)) delta.flip();
		availRates.add(delta);
		net->setAvailRates(plnk,availRates);
	}
}


/** Create a string representation of the ComtInfo object.
 *  @param s is a reference to a string in which the result is returned
 *  @return a reference to s
 */
string& ComtInfo::toString(string& s) const {
	s = "";
	for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx)) {
		string s1;
		s += comt2string(ctx,s1);
	}
	s += ";\n"; 
	return s;
}

/** Create a string representation of a comtree link, including RateSpec.
 *  @param ctx is the comtree index for a comtree
 *  @param lnk is a link number for a link in the comtree
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& ComtInfo::link2string(int ctx, int lnk, string& s) const {
	stringstream ss;
	fAdr_t childAdr = getChild(ctx,lnk);
	int child = net->getNodeNum(childAdr);
	int parent = net->getPeer(child,lnk);
	ss << "(" << net->getNodeName(child,s);
	if (net->isRouter(child))
		ss << "." << net->getLLnum(lnk,child);
	ss << "," << net->getNodeName(parent,s) << "."
	   << net->getLLnum(lnk,parent);
	RateSpec rs;
	if (net->isRouter(child)) {
		map<fAdr_t,ComtRtrInfo>::iterator rp;
		rp = comtree[ctx].rtrMap->find(childAdr);
		rs = rp->second.plnkRates;
	} else {
		map<fAdr_t,ComtLeafInfo>::iterator lp;
		lp = comtree[ctx].leafMap->find(childAdr);
		rs = lp->second.plnkRates;
	}
	ss << ",(" << rs.bitRateUp << "," << rs.bitRateDown
	   << ","  << rs.pktRateUp << "," << rs.pktRateDown << "))";
	s = ss.str();
	return s;
}

/** Create a string representation of a parent link for a leaf.
 *  @param ctx is the comtree index for a comtree
 *  @param leafAdr is a forest address of a leaf node
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& ComtInfo::leafLink2string(int ctx, fAdr_t leafAdr, string& s) const {
	int leaf = net->getNodeNum(leafAdr);
	if (leaf != 0) return link2string(ctx,net->firstLinkAt(leaf),s);

	map<fAdr_t,ComtLeafInfo>::iterator lp;
	lp = comtree[ctx].leafMap->find(leafAdr);
	int parent = net->getNodeNum(lp->second.parent);
	stringstream ss;
	ss << "(" << Forest::fAdr2string(leafAdr,s);
	ss << "," << net->getNodeName(parent,s) << "." << lp->second.llnk;
	ss << "," << lp->second.plnkRates.toString(s) << ")";
	s = ss.str();
	return s;
}

/** Create a string representation of a comtree.
 *  @param ctx is the comtree index for a comtree
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& ComtInfo::comt2string(int ctx, string& s) const {
	s = "";
	if (!validComtIndex(ctx)) return s;
	stringstream ss;
	ss << "comtree(" << getComtree(ctx) << ","
	   << net->getNodeName(net->getNodeNum(getOwner(ctx)),s) << ",";
	ss << net->getNodeName(net->getNodeNum(getRoot(ctx)),s) << ","
	   << (getConfigMode(ctx) ? "auto" : "manual") << ",";
	ss << comtree[ctx].bbDefRates.toString(s) << ",";
	ss << comtree[ctx].leafDefRates.toString(s);
	int numNodes = comtree[ctx].rtrMap->size()
			+ comtree[ctx].leafMap->size();
	if (numNodes <= 1) {
		ss << ")\n"; s = ss.str(); return s;
	} else if (comtree[ctx].coreSet->size() > 1) {
		ss << ",\n\t(";
		// iterate through core nodes and print
		bool first = true;
		for (fAdr_t c = firstCore(ctx); c != 0; c = nextCore(ctx,c)) {
			if (c == getRoot(ctx)) continue;
			if (first) first = false;
			else	   ss << ",";
			ss << net->getNodeName(net->getNodeNum(c),s);
		}
		ss << ")";
	} else {
		ss << ",";
	}
	if (numNodes <= 1) {
		ss << "\n)\n";
	} else {
		ss << ",\n";
		int num2go = numNodes - 1;
		// iterate through routers and print parent links
		int numDone = 0;
		map<fAdr_t,ComtRtrInfo>::iterator rp;
		for (rp  = comtree[ctx].rtrMap->begin();
		     rp != comtree[ctx].rtrMap->end(); rp++) {
			if (rp->second.plnk == 0) continue;
			ss << "\t" << link2string(ctx,rp->second.plnk,s);
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		// iterate through leaf nodes and print parent links
		map<fAdr_t,ComtLeafInfo>::iterator lp;
		for (lp  = comtree[ctx].leafMap->begin();
		     lp != comtree[ctx].leafMap->end(); lp++) {
			ss << "\t"
			   << leafLink2string(ctx,lp->first,s);
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		ss << ")\n";
	}
	s = ss.str();
	return s;
}

/** Create a string representation of a the status of a comtree.
 *  Shows only the backbone links, but includes link counts for all routers.
 *  @param ctx is the comtree index for a comtree
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& ComtInfo::comtStatus2string(int ctx, string& s) const {
	s = "";
	if (!validComtIndex(ctx)) return s;
	stringstream ss;
	ss << "comtree(" << getComtree(ctx) << ","
	   << net->getNodeName(net->getNodeNum(getOwner(ctx)),s) << ",";
	ss << net->getNodeName(net->getNodeNum(getRoot(ctx)),s) << ","
	   << (getConfigMode(ctx) ? "auto" : "manual") << ",";
	ss << comtree[ctx].bbDefRates.toString(s) << ",";
	ss << comtree[ctx].leafDefRates.toString(s);
	int numNodes = comtree[ctx].rtrMap->size()
			+ comtree[ctx].leafMap->size();
	if (numNodes <= 1) {
		ss << ")\n"; s = ss.str(); return s;
	} else if (comtree[ctx].coreSet->size() > 1) {
		ss << ",\n\t(";
		// iterate through core nodes and print
		bool first = true;
		for (fAdr_t c = firstCore(ctx); c != 0; c = nextCore(ctx,c)) {
			if (c == getRoot(ctx)) continue;
			if (first) first = false;
			else	   ss << ",";
			ss << net->getNodeName(net->getNodeNum(c),s);
		}
		ss << ")";
	} else {
		ss << ",";
	}
	if (numNodes <= 1) {
		ss << "\n)\n";
	} else {
		ss << ",\n";
		// count number of nodes to be added to string
		int num2go = comtree[ctx].rtrMap->size();
		map<fAdr_t,ComtLeafInfo>::iterator lp;
		for (lp  = comtree[ctx].leafMap->begin();
		     lp != comtree[ctx].leafMap->end(); lp++) {
			if (net->getNodeNum(lp->first) != 0) num2go++;
		}
		// iterate through routers and print nodes, parents, rates
		// and link counts
		int numDone = 0;
		map<fAdr_t,ComtRtrInfo>::iterator rp;
		for (rp  = comtree[ctx].rtrMap->begin();
		     rp != comtree[ctx].rtrMap->end(); rp++) {
			fAdr_t radr = rp->first;
			int rtr = net->getNodeNum(radr);
			ss << "\t(";
			if (rp->second.plnk == 0) { // special case of root
				ss << net->getNodeName(rtr,s) << ","
				   << rp->second.lnkCnt;
			} else {
				int parent = net->getPeer(rtr,rp->second.plnk);
				ss << net->getNodeName(rtr,s) << "."
				   << net->getLLnum(rp->second.plnk,rtr) << ",";
				ss << net->getNodeName(parent,s) << ".";
				ss << net->getLLnum(rp->second.plnk,parent) 
				   << "," << rp->second.plnkRates.toString(s)
				   << "," << rp->second.lnkCnt;
			}
			ss << ")";
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		// iterate through static leaf nodes and print parent links
		for (lp  = comtree[ctx].leafMap->begin();
		     lp != comtree[ctx].leafMap->end(); lp++) {
			fAdr_t ladr = lp->first;
			int leaf = net->getNodeNum(ladr);
			if (leaf == 0) continue; // skip dynamic leafs
			ss << "\t(";
			int parent = net->getNodeNum(lp->second.parent);
			ss << net->getNodeName(leaf,s) << ",";
			ss << net->getNodeName(parent,s) << "."
			   << lp->second.llnk ;
			ss << "," << lp->second.plnkRates.toString(s) << ")";
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		ss << ")\n";
	}
	s = ss.str();
	return s;
}

string& ComtInfo::comtStatus22string(int ctx, string& s) const {
	s = "";
	if (!validComtIndex(ctx)) return s;
	stringstream ss;
	ss << "comtree(" << getComtree(ctx) << ","
	   << net->getNodeName(net->getNodeNum(getOwner(ctx)),s) << ",";
	ss << net->getNodeName(net->getNodeNum(getRoot(ctx)),s) << ","
	   << (getConfigMode(ctx) ? "auto" : "manual") << ",";
	ss << comtree[ctx].bbDefRates.toString(s) << ",";
	ss << comtree[ctx].leafDefRates.toString(s);
	int numNodes = comtree[ctx].rtrMap->size()
			+ comtree[ctx].leafMap->size();
	if (numNodes <= 1) {
		ss << ")\n"; s = ss.str(); return s;
	} else if (comtree[ctx].coreSet->size() > 1) {
		ss << ",\n\t(";
		// iterate through core nodes and print
		bool first = true;
		for (fAdr_t c = firstCore(ctx); c != 0; c = nextCore(ctx,c)) {
			if (c == getRoot(ctx)) continue;
			if (first) first = false;
			else	   ss << ",";
			ss << net->getNodeName(net->getNodeNum(c),s);
		}
		ss << ")";
	} else {
		ss << ",";
	}
	if (numNodes <= 1) {
		ss << "\n)\n";
	} else {
		ss << ",\n";
		// count number of nodes to be added to string
		int num2go = comtree[ctx].rtrMap->size();
		map<fAdr_t,ComtLeafInfo>::iterator lp;
		for (lp  = comtree[ctx].leafMap->begin();
		     lp != comtree[ctx].leafMap->end(); lp++) {
			if (net->getNodeNum(lp->first) != 0) num2go++;
		}
		// iterate through routers and print nodes, parents, rates
		// and link counts
		int numDone = 0;
		map<fAdr_t,ComtRtrInfo>::iterator rp;
		for (rp  = comtree[ctx].rtrMap->begin();
		     rp != comtree[ctx].rtrMap->end(); rp++) {
			fAdr_t radr = rp->first;
			int rtr = net->getNodeNum(radr);
			ss << "\t(";
			if (rp->second.plnk == 0) { // special case of root
				ss << net->getNodeName(rtr,s) << ","
				   << rp->second.lnkCnt;
			} else {
				int parent = net->getPeer(rtr,rp->second.plnk);
				ss << net->getNodeName(rtr,s) << "."
				   << net->getLLnum(rp->second.plnk,rtr) << ",";
				ss << net->getNodeName(parent,s) << ".";
				ss << net->getLLnum(rp->second.plnk,parent) 
				   << "," << rp->second.plnkRates.toString(s);
				ss << "," << rp->second.subtreeRates.toString(s)
				   << "," << rp->second.lnkCnt;
			}
			ss << ")";
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		// iterate through static leaf nodes and print parent links
		for (lp  = comtree[ctx].leafMap->begin();
		     lp != comtree[ctx].leafMap->end(); lp++) {
			fAdr_t ladr = lp->first;
			int leaf = net->getNodeNum(ladr);
			if (leaf == 0) continue; // skip dynamic leafs
			ss << "\t(";
			int parent = net->getNodeNum(lp->second.parent);
			ss << net->getNodeName(leaf,s) << ",";
			ss << net->getNodeName(parent,s) << "."
			   << lp->second.llnk ;
			ss << "," << lp->second.plnkRates.toString(s) << ")";
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		ss << ")\n";
	}
	s = ss.str();
	return s;
}
