/** @file ComtInfo.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ComtInfo.h"

namespace forest {

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
	comtreeMap = new HashSet<comt_t,Hash::u32>(maxComtree);
}

/** Destructor for ComtInfo class.
 *  Deallocates all dynamic storage
 */
ComtInfo::~ComtInfo() {
	for (int ctx = comtreeMap->first(); ctx != 0;
		 ctx = comtreeMap->next(ctx)) {
		delete comtree[ctx].coreSet;
		delete comtree[ctx].leafMap;
		delete comtree[ctx].rtrMap;
	}
	delete [] comtree; delete comtreeMap;
}

/** Initialize locks and condition variables.  */
bool ComtInfo::init() {
	for (int ctx = 1; ctx <= maxComtree; ctx++) {
		comtree[ctx].busyBit = false;
	}
	return true;
}

/** Get the comtree index for a given comtree and lock the comtree.
 *  @param is a comtree number (not an index)
 *  @return the comtree index associated with the given comtree number,
 *  or 0 if the comtree number is not valid; on return the comtree
 *  is locked
 */
int ComtInfo::getComtIndex(int comt) {
	unique_lock<mutex> lck(mapLock);
	int ctx = comtreeMap->find(comt);
	if (ctx == 0) { return 0; }
	while (comtree[ctx].busyBit) { // wait until comtree is not busy
		comtree[ctx].busyCond.wait(lck);
		ctx = comtreeMap->find(comt);
		if (ctx == 0) return 0;
	}
	comtree[ctx].busyBit = true; // set busyBit to lock comtree
	return ctx;
}

/** Release a previously locked comtree.
 *  Waiting threads are signalled to let them proceed.
 *  @param ctx is the index of a locked comtree.
 */
void ComtInfo::releaseComtree(int ctx) {
	unique_lock<mutex> lck(mapLock);
	comtree[ctx].busyBit = false;
	comtree[ctx].busyCond.notify_one();
}

/** Get the first comtree in the list of valid comtrees and lock it.
 *  @return the ctx of the first valid comtree, or 0 if there is no
 *  valid comtree; on return, the comtree is locked
 */
int ComtInfo::firstComtree() {
	unique_lock<mutex> lck(mapLock);
	int ctx = comtreeMap->first();
	if (ctx == 0) return 0;
	while (comtree[ctx].busyBit) {
		comtree[ctx].busyCond.wait(lck);
		ctx = comtreeMap->first();
		if (ctx == 0) return 0;
	}
	comtree[ctx].busyBit = true;
	return ctx;
}

/** Get the next comtree in the list of valid comtrees and lock it.
 *  @param ctx is the index of a valid comtree
 *  @return the index of the next comtree in the list of valid comtrees
 *  or 0 if there is no next comtree; on return, the lock on ctx
 *  is released and the next comtree is locked.
 */
int ComtInfo::nextComtree(int ctx) {
	unique_lock<mutex> lck(mapLock);
	int nuCtx = comtreeMap->next(ctx);
	if (nuCtx == 0) {
		comtree[ctx].busyBit = false;
		comtree[ctx].busyCond.notify_one();
		return 0;
	}
	while (comtree[nuCtx].busyBit) {
		comtree[nuCtx].busyCond.wait(lck);
		nuCtx = comtreeMap->next(ctx);
		if (nuCtx == 0) {
			comtree[ctx].busyBit = false;
			comtree[ctx].busyCond.notify_one();
			return 0;
		}
	}
	comtree[nuCtx].busyBit = true;
	comtree[ctx].busyBit = false;
	comtree[ctx].busyCond.notify_one();
	return nuCtx;
}

/** Add a new comtree.
 *  Defines a new comtree, with attributes left undefined.
 *  @param comt is the comtree number for the new comtree.
 *  @return the index of the new comtree on success, else 0;
 *  on return, the new comtree is locked
 */
int ComtInfo::addComtree(int comt) {
	unique_lock<mutex> lck(mapLock);
	int ctx = comtreeMap->insert(comt);
	if (ctx == 0) return 0;
	// since this is a newly allocated comtree, it cannot be
	// locked by another thread
	comtree[ctx].busyBit = true;
	comtree[ctx].comtreeNum = comt;
	comtree[ctx].coreSet = new HashSet<fAdr_t,Hash::s32>;
	comtree[ctx].rtrMap = new HashMap<fAdr_t,ComtRtrInfo,Hash::s32>;
	comtree[ctx].leafMap = new HashMap<fAdr_t,ComtLeafInfo,Hash::s32>;
	return ctx;
}

/** Remove a comtree.
 *  Assumes that all bandwidth resources in underlying network
 *  have already been released and that the calling thread already
 *  holds a lock on the comtree.
 *  @param ctx is the comtree index for a valid comtree to be removed
 *  @return true on success, false on failure
 */
bool ComtInfo::removeComtree(int ctx) {
	unique_lock<mutex> lck(mapLock);
	if (!validComtIndex(ctx) || !comtree[ctx].busyBit) {
		return false;
	}
	comtreeMap->remove(comtree[ctx].comtreeNum);
	comtree[ctx].comtreeNum = 0;
	delete comtree[ctx].coreSet;
	delete comtree[ctx].leafMap;
	delete comtree[ctx].rtrMap;
	comtree[ctx].busyBit = false;
	comtree[ctx].busyCond.notify_one();
	return true;
}

/** Add a new node to a comtree.
 *  @param ctx is the comtree index
 *  @param fa is a valid forest address
 *  @return true on success, false on failure
 */
bool ComtInfo::addNode(int ctx, fAdr_t fa) {
	int nn = net->getNodeNum(fa);
	if (nn != 0 && net->isRouter(nn)) {
	    	int x = comtree[ctx].rtrMap->find(fa);
	    	if (x != 0) return true;
		ComtRtrInfo cri;
		comtree[ctx].rtrMap->put(fa,cri);
		return true;
	}
	int x = comtree[ctx].leafMap->find(fa);
	if (x != 0) return true;
	ComtLeafInfo cli; cli.plnkRates = comtree[ctx].leafDefRates;
	if (nn != 0) {
		int plnk = net->firstLinkAt(nn);
		int parent = net->getPeer(nn,plnk);
		cli.parent = net->getNodeAdr(parent);
		cli.llnk = net->getLLnum(plnk,parent);
	}
	comtree[ctx].leafMap->put(fa,cli);
	return true;
}

/** Remove a node from a comtree.
 *  This method will fail if the node is a router with links to children
 *  Updates link counts at parent router.
 *  @param ctx is the comtree index
 *  @param fa is the forest address of a comtree node
 *  @return true on success, false on failure
 */
bool ComtInfo::removeNode(int ctx, fAdr_t fa) {
	int x = comtree[ctx].rtrMap->find(fa);
	if (x != 0) {
		ComtRtrInfo& cri = comtree[ctx].rtrMap->getValue(x);
		int plnk = cri.plnk;
		if ((plnk == 0 && cri.lnkCnt != 0) ||
		    (plnk != 0 && cri.lnkCnt != 1))
			return false;
		if (plnk != 0) {
			int parent = net->getPeer(net->getNodeNum(fa),plnk);
			int px = comtree[ctx].rtrMap->find(
					net->getNodeAdr(parent));
			comtree[ctx].rtrMap->getValue(px).lnkCnt--;
		}
		comtree[ctx].rtrMap->remove(fa);
		comtree[ctx].coreSet->remove(fa);
		return true;
	}
	int lx = comtree[ctx].leafMap->find(fa);
	ComtLeafInfo& cli = comtree[ctx].leafMap->getValue(lx);
	int rx = comtree[ctx].rtrMap->find(cli.parent);
	comtree[ctx].rtrMap->getValue(rx).lnkCnt--;
	comtree[ctx].leafMap->remove(fa);
	return true;
}


/** Adjust the subtree rates along path to root.
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is a valid comtree index
 *  @param rtr is the forest address of a router in the comtree
 *  @param rs is a RateSpec that is to be added to the subtree rates
 *  at all vertices from rtr to the root
 *  @param return true on success, false on failure
 */
bool ComtInfo::adjustSubtreeRates(int ctx, fAdr_t rtrAdr, RateSpec& rs) {
	int rtr = net->getNodeNum(rtrAdr);
	int x = comtree[ctx].rtrMap->find(rtrAdr);
	int count = 0;
	while (true) {
		ComtRtrInfo& cri = comtree[ctx].rtrMap->getValue(x);
		cri.subtreeRates.add(rs);
		if (cri.plnk == 0) return true;
		rtr = net->getPeer(rtr,cri.plnk);
		x = comtree[ctx].rtrMap->find(net->getNodeAdr(rtr));
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
 *
 *  This method assumes that no other thread currently has access to
 *  the ComtInfo object. It should only be used on startup, before other
 *  threads are launched.
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
 *  This thread is assumed to be the only one with access to the ComtInfo
 *  object at the moment. The method should only be used on startup, before
 *  other threads are launched.
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
	releaseComtree(ctx); 	// this is assumed to be only thread with
				// access to comtree, so locking not needed
	
	fAdr_t ownerAdr = net->getNodeAdr(owner);
	fAdr_t rootAdr  = net->getNodeAdr(root);

	// configure comtree
	if (!setOwner(ctx,ownerAdr) || !setRoot(ctx,rootAdr)) {
		errMsg = "could not configure comtree";
		return 0;
	}
	setConfigMode(ctx,autoConfig);
	getDefLeafRates(ctx) = leafRates;
	getDefBbRates(ctx) = bbRates;

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
		ComtreeInfo& c = comtree[ctx];

		addNode(ctx,childAdr); addNode(ctx,parentAdr);
		if (net->isLeaf(child)) {
			int x = c.leafMap->find(childAdr);
			ComtLeafInfo& cli = c.leafMap->getValue(x);
			if (rs.bitRateUp == -1) rs = leafRates;
			cli.plnkRates = rs;
		} else {
			int x = comtree[ctx].rtrMap->find(childAdr);
			ComtRtrInfo& cri = c.rtrMap->getValue(x);
			cri.plnk = lnk; cri.lnkCnt++;
			if (rs.bitRateUp == -1) rs = bbRates;
			else cri.frozen = true;
			cri.plnkRates = rs;
			rs = cri.subtreeRates;
		}
		adjustSubtreeRates(ctx,parentAdr,rs);
		int x = c.rtrMap->find(parentAdr);
		c.rtrMap->getValue(x).lnkCnt++;
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
	for (int ctx = firstComtree(); ctx != 0; ctx = nextComtree(ctx)) {
		ComtreeInfo& c = comtree[ctx];
		int comt = getComtree(ctx);
		fAdr_t rootAdr = getRoot(ctx);
		int root = net->getNodeNum(rootAdr);

		// first check that every leaf in comtree has a parent
		// that is a router in the comtree
		for (int x = c.leafMap->first(); x!=0; x = c.leafMap->next(x)) {
			fAdr_t leafAdr = c.leafMap->getKey(x);
			if (!isComtRtr(ctx,getParent(ctx,leafAdr))) {
				int leaf = net->getNodeNum(leafAdr);
				string leafName;
				if (leaf != 0)
					leafName = net->getNodeName(leaf);
				else Forest::fAdr2string(leafAdr);
				cerr << "ComtInfo::check: comtree " << comt 
				     << " has leaf " << leafName << " whose "
					"parent is not a router in comtree\n";
				status = false;
			}
		}
		// next, check that at most one router in comtree lacks a
		// parent
		int cnt = 0;
		for (int x = c.rtrMap->first(); x!=0; x = c.rtrMap->next(x)) {
			if (getParent(ctx,c.rtrMap->getKey(x)) == 0) cnt++;
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
					     << net->getNodeName(v)
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
		if (nodeCount != c.rtrMap->size()) {
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

	// first count links from leaf nodes
	for (fAdr_t leafAdr = firstLeaf(ctx); leafAdr != 0;
		    leafAdr = nextLeaf(ctx,leafAdr)) {
		fAdr_t padr = getParent(ctx,leafAdr);
		int parent = net->getNodeNum(padr);
		lnkCounts[parent]++;
	}
	// next, count links to other routers
	for (fAdr_t rtrAdr = firstRouter(ctx); rtrAdr != 0;
		    rtrAdr = nextRouter(ctx,rtrAdr)) {
		int rtr = net->getNodeNum(rtrAdr);
		fAdr_t padr = getParent(ctx,rtrAdr);
		if (padr == 0) continue;
		int parent = net->getNodeNum(padr);
		lnkCounts[parent]++;
		lnkCounts[rtr]++;
	}

	// now check stored link counts for routers
	for (fAdr_t rtrAdr = firstRouter(ctx); rtrAdr != 0;
		    rtrAdr = nextRouter(ctx,rtrAdr)) {
		int rtr = net->getNodeNum(rtrAdr);
		int lc = comtree[ctx].rtrMap->get(rtrAdr).lnkCnt;
		if (lnkCounts[rtr] != lc) {
			string s;
			cerr << "router " << net->getNodeName(rtr)
			     << " has " << lnkCounts[rtr]
			     << " links in comtree " << comt
			     << ", but recorded lnkCnt is " << lc;
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
	for (int x = comtree[ctx].leafMap->first(); x != 0;
		 x = comtree[ctx].leafMap->next(x)) {
		fAdr_t leafAdr = comtree[ctx].leafMap->getKey(x);
		ComtLeafInfo& cli = comtree[ctx].leafMap->getValue(x);
		RateSpec& prates = cli.plnkRates;
		if (prates.bitRateUp <= 0 || prates.bitRateDown <= 0 ||
		    prates.pktRateUp <= 0 || prates.pktRateDown <= 0) {
			int lnk = net->getLinkNum(
				  	net->getNodeNum(cli.parent),
					cli.llnk);
			cerr << "detected non-positive comtree link rate for "
			     << comt << " link " << net->link2string(lnk)
			     << " rateSpec=" << prates.toString() << endl;
			status = false;
		}
		fAdr_t padr = getParent(ctx,leafAdr);
		while (true) {
			int prtr = net->getNodeNum(padr);
			subtreeRates[prtr].add(prates);
			if (prtr == root) break;
			padr = getParent(ctx,padr);
		} 
	}

	// now check stored subtree rates
	for (int x = comtree[ctx].rtrMap->first(); x != 0;
		 x = comtree[ctx].rtrMap->next(x)) {
		fAdr_t radr = comtree[ctx].rtrMap->getKey(x);
		RateSpec& rs = comtree[ctx].rtrMap->getValue(x).subtreeRates;
		int rtr = net->getNodeNum(radr);
		if (!subtreeRates[rtr].equals(rs)) {
			cerr << "router " << net->getNodeName(rtr)
			     << " has subtree rate "
			     << subtreeRates[rtr].toString()
			     << " in comtree " << comt << ", but recorded"
			     << " value is "
			     << rs.toString() << endl;
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
	
	int x = comtree[ctx].rtrMap->find(rootAdr);
	RateSpec& rootRates = comtree[ctx].rtrMap->getValue(x).subtreeRates;

	for (int x = comtree[ctx].rtrMap->first(); x != 0;
		 x = comtree[ctx].rtrMap->next(x)) {
		ComtRtrInfo& cri = comtree[ctx].rtrMap->getValue(x);
		if (cri.frozen) continue;
		int lnk = cri.plnk;
		if (lnk == 0) continue;
		fAdr_t rtr = comtree[ctx].rtrMap->getKey(x);
		RateSpec rs(0);
		RateSpec& srates = cri.subtreeRates;
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
		if (!rs.equals(cri.plnkRates)) {
			cerr << "detected inconsistent comtree link rates in "
			     << comt << " link " << net->link2string(lnk)
			     << " computed rates: " << rs.toString()
			     << " and stored rates: "
			     << cri.plnkRates.toString() << endl;
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
	for (int ctx = firstComtree(); ctx != 0; ctx = nextComtree(ctx)) {
		if (!setComtRates(ctx)) return false;
	}
	return true;
}

/** Compute rates for all links in a comtree and allocate capacity of
 *  network links. This is for use when configuring an entire comtree,
 *  not when processing joins/leaves.
 *  Assumes that the current thread is the only one with access to
 *  object right now.
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
 *  Assumes that current thread holds a lock on comtree, if one is needed.
 *  @param ctx is a valid comtree index
 *  @param u is a node in the comtree (used in recursive calls)
 *  @return true on success, else false
 */
void ComtInfo::setAutoConfigRates(int ctx) {
	fAdr_t root = getRoot(ctx);
	int x = comtree[ctx].rtrMap->find(root);
	RateSpec& rootRates = comtree[ctx].rtrMap->getValue(x).subtreeRates;
	for (int x = comtree[ctx].rtrMap->first(); x != 0;
		 x = comtree[ctx].rtrMap->next(x)) {
		ComtRtrInfo& cri = comtree[ctx].rtrMap->getValue(x);
		if (cri.frozen) continue;
		int lnk = cri.plnk;
		if (lnk == 0) continue;
		fAdr_t rtr = comtree[ctx].rtrMap->getKey(x);
		RateSpec& rs = cri.plnkRates;
		RateSpec& srates = cri.subtreeRates;
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
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is a valid comtree index
 *  @return true if the provisioned rates for the comtree are compatible
 *  with the available capacity
 */
bool ComtInfo::checkComtRates(int ctx) {
	// first check parent links at routers
	for (int x = comtree[ctx].rtrMap->first(); x != 0;
		 x = comtree[ctx].rtrMap->next(x)) {
		ComtRtrInfo& cri = comtree[ctx].rtrMap->getValue(x);
		fAdr_t rtr = comtree[ctx].rtrMap->getKey(x);
		if (cri.plnk == 0) continue;
		RateSpec rs = cri.plnkRates;
		if (rtr != net->getLeft(cri.plnk)) rs.flip();
		RateSpec availRates = net->getAvailRates(cri.plnk);
		if (!rs.leq(availRates)) return false;
	}
	// then check access links, for static leaf nodes
	for (int x = comtree[ctx].leafMap->first(); x != 0;
		 x = comtree[ctx].leafMap->next(x)) {
		ComtLeafInfo& cli = comtree[ctx].leafMap->getValue(x);
		fAdr_t leafAdr = comtree[ctx].leafMap->getKey(x);
		int leaf = net->getNodeNum(leafAdr);
		if (leaf == 0) continue;
		int lnk = net->firstLinkAt(leaf);
		RateSpec rs = cli.plnkRates;
		if (leaf != net->getLeft(lnk)) rs.flip();
		RateSpec availRates = net->getAvailRates(lnk);
		if (!rs.leq(availRates)) { return false; }
	}
	return true;
}

/** Provision all links in a comtree, reducing available link capacity.
 *  Assumes that the required network capacity is available and that
 *  comtree links are configured with required rates.
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is a valid comtree index
 *  @return true on success, else false
 */
void ComtInfo::provision(int ctx) {
	for (int x = comtree[ctx].rtrMap->first(); x != 0;
		 x = comtree[ctx].rtrMap->next(x)) {
		ComtRtrInfo& cri = comtree[ctx].rtrMap->getValue(x);
		fAdr_t rtr = comtree[ctx].rtrMap->getKey(x);
		if (cri.plnk == 0) continue;
		RateSpec rs = cri.plnkRates;
		if (rtr != net->getLeft(cri.plnk)) rs.flip();
		net->getAvailRates(cri.plnk).subtract(rs);
	}
	// provision statically configured leaf nodes
	for (int x = comtree[ctx].leafMap->first(); x != 0;
		 x = comtree[ctx].leafMap->next(x)) {
		ComtLeafInfo& cli = comtree[ctx].leafMap->getValue(x);
		fAdr_t leafAdr = comtree[ctx].leafMap->getKey(x);
		int leaf = net->getNodeNum(leafAdr);
		if (leaf == 0) continue;
		int lnk = net->firstLinkAt(leaf);
		RateSpec rs = cli.plnkRates;
		if (leaf != net->getLeft(lnk)) rs.flip();
		net->getAvailRates(lnk).subtract(rs);
	}
}

/** Unprovision all links in a comtree,increasing available link capacity.
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is a valid comtree index
 *  @return true on success, else false
 */
void ComtInfo::unprovision(int ctx) {
	for (int x = comtree[ctx].rtrMap->first(); x != 0;
		 x = comtree[ctx].rtrMap->next(x)) {
		ComtRtrInfo& cri = comtree[ctx].rtrMap->getValue(x);
		fAdr_t rtr = comtree[ctx].rtrMap->getKey(x);
		if (cri.plnk == 0) continue;
		RateSpec rs = cri.plnkRates;
		if (rtr != net->getLeft(cri.plnk)) rs.flip();
		net->getAvailRates(cri.plnk).add(rs);
	}
	for (int x = comtree[ctx].leafMap->first(); x != 0;
		 x = comtree[ctx].leafMap->next(x)) {
		ComtLeafInfo& cli = comtree[ctx].leafMap->getValue(x);
		fAdr_t leafAdr = comtree[ctx].leafMap->getKey(x);
		int leaf = net->getNodeNum(leafAdr);
		if (leaf == 0) continue;
		int lnk = net->firstLinkAt(leaf);
		RateSpec rs = cli.plnkRates;
		if (leaf != net->getLeft(lnk)) rs.flip();
		net->getAvailRates(lnk).add(rs);
	}
}

/** Find a path from a vertex to a comtree.
 *  This method builds a shortest path tree with a given source node.
 *  The search is done over all network links that have the available capacity
 *  to handle a provided RateSpec.
 *  The search halts when it reaches any node in the comtree.
 *  The path from source to this node is returned in path.
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is a valid comtree index for the comtree of interest
 *  @param src is the router at which the path search starts
 *  @param rs specifies the rates required on the links in the path
 *  @param path is a reference to a list of LinkMod objects which
 *  define the path when the method returns; the links are ordered
 *  "bottom-up:.
 *  @return the router number of the "branchRouter" at which the
 *  path ends, or 0 if no path is found
 */
int ComtInfo::findPath(int ctx, int src, RateSpec& rs, Glist<LinkMod>& path) {
	path.clear();
	fAdr_t srcAdr = net->getNodeAdr(src);
	if (isComtNode(ctx,srcAdr)) return src;
	int n = 0;
	for (fAdr_t r = net->firstRouter(); r != 0; r = net->nextRouter(r)) {
		n = max(r,n);
	}
	Dheap<int> h(n); int d[n+1];
	int plnk[n+1]; // note: reverse from direction in comtree
	for (fAdr_t r = net->firstRouter(); r != 0; r = net->nextRouter(r)) {
		plnk[r] = 0; d[r] = INT_MAX;
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
			availRates = net->getAvailRates(lnk);
			if (r != net->getLeft(lnk)) availRates.flip();
			if (!rs.leq(availRates)) continue;
			if (isComtNode(ctx,net->getNodeAdr(peer))) { // done
				plnk[peer] = lnk;
				int u = peer;
				for (int pl = plnk[u]; pl != 0; pl = plnk[u]) {
					int v = net->getPeer(u,plnk[u]);
					LinkMod lm(pl,v,rs);
					path.addFirst(lm);
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

/** Find a path from a vertex to the root of a comtree.
 *  This method builds a shortest path tree with a given source node.
 *  The search is done over all network links that have the available capacity
 *  to handle a specified RateSpec. The search halts when it reaches any
 *  node in the comtree, then follows the path to the root.
 *  This method is not ideal, since it can return paths that do not
 *  provide for future growth. It is also not necessarily the best
 *  path from a cost standpoint.
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is a valid comtree index for the comtree of interest
 *  @param src is the router at which the path search starts
 *  @param rs specifies the rates required on the links in the path
 *  @param path is an integer vector in which the result returned;
 *  Values are router local link numbers that define the path in a bottom-up
 *  fashion.
 *  @return true on success, false on failure
 */
bool ComtInfo::findRootPath(int ctx, int src, RateSpec& rs, vector<int>& path) {
	fAdr_t srcAdr = net->getNodeAdr(src);

	path.clear();
	// if src is in comtree, just return path to root
	if (isComtNode(ctx,srcAdr)) {
		int u = src;
		int pl = getPlink(ctx,net->getNodeAdr(u));
		while (pl != 0) {
			path.push_back(net->getLLnum(pl,u));
			u = net->getPeer(u,pl);
			pl = getPlink(ctx,net->getNodeAdr(u));
		}
		return true;
	}
	
	int n = 0;
	for (fAdr_t r = net->firstRouter(); r != 0; r = net->nextRouter(r)) {
		n = max(r,n);
	}
	Dheap<int> h(n); int d[n+1];
	int plnk[n+1]; // note: reverse from direction in comtree
	for (fAdr_t r = net->firstRouter(); r != 0; r = net->nextRouter(r)) {
		plnk[r] = 0; d[r] = INT_MAX;
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
			// if this link cannot take the specified rate spec,
			// ignore it
			availRates = net->getAvailRates(lnk);
			if (r != net->getLeft(lnk)) availRates.flip();
			if (!rs.leq(availRates)) continue;
			if (isComtNode(ctx,net->getNodeAdr(peer))) { // done
				// copy spt path to path vector in reverse order
				plnk[peer] = lnk;
				int u = peer;
				int len = 0;
				for (int pl = plnk[u]; pl != 0; pl = plnk[u]) {
					len++; u = net->getPeer(u,pl);
				}
				path.resize(len);
				int i = len-1; u = peer;
				for (int pl = plnk[u]; pl != 0; pl = plnk[u]) {
					u = net->getPeer(u,pl);
					path[i--] = net->getLLnum(pl,u);
				}

				// continue up the comtree
				u = peer;
				int pl = getPlink(ctx,net->getNodeAdr(u));
				while (pl != 0) {
					path.push_back(net->getLLnum(pl,u));
					u = net->getPeer(u,pl);
					pl = getPlink(ctx,net->getNodeAdr(u));
				}
				return true;
			}
			if (d[peer] > d[r] + net->getLinkLength(lnk)) {
				plnk[peer] = lnk;
				d[peer] = d[r] + net->getLinkLength(lnk);
				if (h.member(peer)) h.changekey(peer,d[peer]);
				else h.insert(peer,d[peer]);
			}
		}
	}
	return false;
}

/** Add a path from a router to a comtree.
 *  This method adds a list of links to the backbone of an existing comtree,
 *  sets their link rates and reserves capacity on the network links;
 *  the links are assumed to have sufficient available capacity.
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is a valid comtree index for the comtree of interest
 *  @param path is a reference to a list of LinkMod objects defining a path
 *  in the comtree; the links appear in "bottom-up" order
 *  leading from some router outside the comtree to a router in the comtree
 */
void ComtInfo::addPath(int ctx, Glist<LinkMod>& path) {
	for (int x = path.first(); x != 0; x = path.next(x)) {
		// add lnk to the comtree in net
		LinkMod& lmx = path.value(x);
		RateSpec rs = lmx.rs;
		int parent = net->getPeer(lmx.child,lmx.lnk);
		fAdr_t childAdr = net->getNodeAdr(lmx.child);
		fAdr_t parentAdr = net->getNodeAdr(parent);
		addNode(ctx,childAdr); addNode(ctx,parentAdr);
		setPlink(ctx,childAdr,lmx.lnk); thaw(ctx,childAdr);
		getLinkRates(ctx,childAdr) = rs;
		if (lmx.child != net->getLeft(lmx.lnk)) rs.flip();
		net->getAvailRates(lmx.lnk).subtract(rs);
	}
	return;
}

/** Remove a path from a router in a comtree.
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is a valid comtree index for the comtree of interest
 *  @param path is a list of LinkMod objects defining the path
 *  in bottom-up order; the path is assumed to have
 *  no "branching links" incident to the path
 */
void ComtInfo::removePath(int ctx, Glist<LinkMod>& path) {
	if (path.empty()) return;
	for (int x = path.first(); x != 0; x = path.next(x)) {
		LinkMod& lmx = path.value(x);
		fAdr_t childAdr = net->getNodeAdr(lmx.child);
		RateSpec rs = getLinkRates(ctx,childAdr);
		if (lmx.child != net->getLeft(lmx.lnk)) rs.flip();
		net->getAvailRates(lmx.lnk).add(rs);
		removeNode(ctx,childAdr);
	}
	return;
}

/** Compute the change required in the RateSpecs for links in an
 *  auto-configured comtree and verify that needed capacity is available.
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is the comtree index of the comtree
 *  @param modList is a list of edges in the comtree and RateSpecs
 *  representing the change in rates required at each link in the list;
 *  returned RateSpecs may have negative values; list should be empty 
 *  in top level recursive call
 *  @return true if all links in comtree have sufficient available
 *  capacity to accommodate the changes
 */
bool ComtInfo::computeMods(int ctx, Glist<LinkMod>& modList) {
	fAdr_t root = getRoot(ctx);
	int x = comtree[ctx].rtrMap->find(root);
	RateSpec& rs = comtree[ctx].rtrMap->getValue(x).subtreeRates;
	modList.clear();
	bool status = computeMods(ctx,root,rs,modList);
	return status;
}

/** Compute the change required in the RateSpecs for links in an
 *  auto-configured comtree and verify that needed capacity is available.
 *  This is a recursive helper method called from the top-level computeMods
 *  method.
 *  Assumes that current thread holds a lock on comtree, and a lock on
 *  the network object.
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
			     Glist<LinkMod>& modList) {
	int rnum = net->getNodeNum(radr);
	if (!net->isRouter(rnum)) return true;
	int x = comtree[ctx].rtrMap->find(radr);
	ComtRtrInfo& cri = comtree[ctx].rtrMap->getValue(x);
	int plnk = cri.plnk;
	if (plnk != 0 && !isFrozen(ctx,radr)) {
		// determine the required rates on link to parent
		LinkMod nuMod;
		nuMod.child = rnum; nuMod.lnk = plnk;
		RateSpec& srates = cri.subtreeRates;
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
		nuMod.rs.subtract(cri.plnkRates);
		if (nuMod.rs.isZero()) return true; // no change needed
		RateSpec availRates = net->getAvailRates(plnk);
		if (rnum != net->getLeft(plnk)) availRates.flip();
		if (!nuMod.rs.leq(availRates)) return false;
		modList.addLast(nuMod);
	}

	// Now, do subtrees; note that this method is going to be
	// slow for comtrees with very large backbones; only way to
	// fix it is to add child and sibling pointers; later, maybe
	for (int x = comtree[ctx].rtrMap->first(); x != 0;
		 x = comtree[ctx].rtrMap->next(x)) {
		fAdr_t xadr = comtree[ctx].rtrMap->getKey(x);
		if (radr != getParent(ctx,xadr)) continue;
		if (!computeMods(ctx,xadr,rootRates,modList))
			return false;
	}
	return true;
}

/** Add to the capacity of a list of comtree backbone links and provision
 *  required link capacity in the network. Assumes that network links have
 *  the required capacity and that both endpoints of each link are routers.
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is the index of a comptree
 *  @param modList is a list of link modifications; that is, comtree links
 *  that are either to be added or have their rate adjusted.
 *  and a RateSpec difference to be added to that link.
 */
void ComtInfo::provision(int ctx, Glist<LinkMod>& modList) {
	RateSpec delta;
	for (int x = modList.first(); x != 0; x = modList.next(x)) {
		LinkMod& lmx = modList.value(x);
		int rtr = lmx.child; fAdr_t rtrAdr = net->getNodeAdr(rtr);
		int plnk = lmx.lnk; delta = lmx.rs;
		int rx = comtree[ctx].rtrMap->find(rtrAdr);
		comtree[ctx].rtrMap->getValue(rx).plnkRates.add(delta);
		if (rtr != net->getLeft(plnk)) delta.flip();
		net->getAvailRates(plnk).subtract(delta);
	}
}

/** Reduce the capacity of a list of comtree links and release required
 *  link capacity in the network.
 *  Assumes that current thread holds a lock on comtree and NetInfo object.
 *  @param ctx is the index of a comptree
 *  @param modList is a list of nodes, each representing a link in the comtree
 *  and a RateSpec difference to be removed from that link.
 */
void ComtInfo::unprovision(int ctx, Glist<LinkMod>& modList) {
	RateSpec delta;
	for (int x = modList.first(); x != 0; x = modList.next(x)) {
		LinkMod& lmx = modList.value(x);
		int rtr = lmx.child; fAdr_t rtrAdr = net->getNodeAdr(rtr);
		int plnk = lmx.lnk; delta = lmx.rs;
		int rx = comtree[ctx].rtrMap->find(rtrAdr);
		comtree[ctx].rtrMap->getValue(rx).plnkRates.add(delta);
		if (rtr != net->getLeft(plnk)) delta.flip();
		net->getAvailRates(plnk).add(delta);
	}
}


/** Create a string representation of the ComtInfo object.
 *  Does not do any locking, so may produce inconsistent results in
 *  multi-threaded environment.
 *  @param s is a reference to a string in which the result is returned
 *  @return a reference to s
 */
string ComtInfo::toString() {
	string s;
	for (int ctx = firstComtree(); ctx != 0; ctx = nextComtree(ctx)) {
		s += comt2string(ctx);
	}
	s += ";\n"; 
	return s;
}

/** Create a string representation of a comtree link, including RateSpec.
 *  Does not do any locking, so may produce inconsistent results in
 *  multi-threaded environment.
 *  @param ctx is the comtree index for a comtree
 *  @param lnk is a link number for a link in the comtree
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string ComtInfo::link2string(int ctx, int lnk) const {
	stringstream ss;
	fAdr_t childAdr = getChild(ctx,lnk);
	int child = net->getNodeNum(childAdr);
	int parent = net->getPeer(child,lnk);
	ss << "(" << net->getNodeName(child);
	if (net->isRouter(child))
		ss << "." << net->getLLnum(lnk,child);
	ss << "," << net->getNodeName(parent) << "."
	   << net->getLLnum(lnk,parent);
	RateSpec rs;
	if (net->isRouter(child)) {
		rs = comtree[ctx].rtrMap->getValue(
			comtree[ctx].rtrMap->find(childAdr)).plnkRates;
	} else {
		rs = comtree[ctx].leafMap->getValue(
			comtree[ctx].leafMap->find(childAdr)).plnkRates;
	}
	ss << ",(" << rs.bitRateUp << "," << rs.bitRateDown
	   << ","  << rs.pktRateUp << "," << rs.pktRateDown << "))";
	return ss.str();
}

/** Create a string representation of a parent link for a leaf.
 *  Does not do any locking, so may produce inconsistent results in
 *  multi-threaded environment.
 *  @param ctx is the comtree index for a comtree
 *  @param leafAdr is a forest address of a leaf node
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string ComtInfo::leafLink2string(int ctx, fAdr_t leafAdr) const {
	int leaf = net->getNodeNum(leafAdr);
	if (leaf != 0) return link2string(ctx,net->firstLinkAt(leaf));

	int x = comtree[ctx].leafMap->find(leafAdr);
	ComtLeafInfo& cli = comtree[ctx].leafMap->getValue(x);
	int parent = net->getNodeNum(cli.parent);
	stringstream ss;
	ss << "(" << Forest::fAdr2string(leafAdr)
	   << "," << net->getNodeName(parent) << "." << cli.llnk
	   << "," << cli.plnkRates.toString() << ")";
	return ss.str();
}

/** Create a string representation of a comtree.
 *  Does not do any locking, so may produce inconsistent results in
 *  multi-threaded environment.
 *  @param ctx is the comtree index for a comtree
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string ComtInfo::comt2string(int ctx) const {
	if (!validComtIndex(ctx)) return "";
	stringstream ss;
	ss << "comtree(" << getComtree(ctx) << ","
	   << net->getNodeName(net->getNodeNum(getOwner(ctx))) << ",";
	ss << net->getNodeName(net->getNodeNum(getRoot(ctx))) << ","
	   << (getConfigMode(ctx) ? "auto" : "manual") << ",";
	ss << comtree[ctx].bbDefRates.toString() << ",";
	ss << comtree[ctx].leafDefRates.toString();
	int numNodes = comtree[ctx].rtrMap->size()
			+ comtree[ctx].leafMap->size();
	if (numNodes <= 1) {
		ss << ")\n"; return ss.str();
	} else if (comtree[ctx].coreSet->size() > 1) {
		ss << ",\n\t(";
		// iterate through core nodes and print
		bool first = true;
		for (fAdr_t c = firstCore(ctx); c != 0; c = nextCore(ctx,c)) {
			if (c == getRoot(ctx)) continue;
			if (first) first = false;
			else	   ss << ",";
			ss << net->getNodeName(net->getNodeNum(c));
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
		for (int x = comtree[ctx].rtrMap->first(); x != 0;
			 x = comtree[ctx].rtrMap->next(x)) {
			int plnk = comtree[ctx].rtrMap->getValue(x).plnk;
			if (plnk == 0) continue;
			ss << "\t" << link2string(ctx,plnk);
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		// iterate through leaf nodes and print parent links
		for (int x = comtree[ctx].leafMap->first(); x != 0;
			 x = comtree[ctx].leafMap->next(x)) {
			fAdr_t leafAdr = comtree[ctx].rtrMap->getKey(x);
			ss << "\t" << leafLink2string(ctx,leafAdr);
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		ss << ")\n";
	}
	return ss.str();
}

/** Create a string representation of a the status of a comtree.
 *  Shows only the backbone links, but includes link counts for all routers.
 *  Does not do any locking, so may produce inconsistent results in
 *  multi-threaded environment.
 *  @param ctx is the comtree index for a comtree
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string ComtInfo::comtStatus2string(int ctx) const {
	if (!validComtIndex(ctx)) return "";
	stringstream ss;
	ss << "comtree(" << getComtree(ctx) << ","
	   << net->getNodeName(net->getNodeNum(getOwner(ctx))) << ","
	   << net->getNodeName(net->getNodeNum(getRoot(ctx))) << ","
	   << (getConfigMode(ctx) ? "auto" : "manual") << ","
	   << comtree[ctx].bbDefRates.toString() << ","
	   << comtree[ctx].leafDefRates.toString();
	int numNodes = comtree[ctx].rtrMap->size()
			+ comtree[ctx].leafMap->size();
	if (numNodes <= 1) {
		ss << ")\n"; return ss.str();
	} else if (comtree[ctx].coreSet->size() > 1) {
		ss << ",\n\t(";
		// iterate through core nodes and print
		bool first = true;
		for (fAdr_t c = firstCore(ctx); c != 0; c = nextCore(ctx,c)) {
			if (c == getRoot(ctx)) continue;
			if (first) first = false;
			else	   ss << ",";
			ss << net->getNodeName(net->getNodeNum(c));
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
		for (fAdr_t leafAdr = firstLeaf(ctx); leafAdr != 0;
			    leafAdr = nextLeaf(ctx,leafAdr)) {
			if (net->getNodeNum(leafAdr) !=0) num2go++;
		}
		// iterate through routers and print nodes, parents, rates
		// and link counts
		int numDone = 0;
		for (int x = comtree[ctx].rtrMap->first(); x != 0;
			 x = comtree[ctx].rtrMap->next(x)) {
			fAdr_t radr = comtree[ctx].rtrMap->getKey(x);
			ComtRtrInfo& cri = comtree[ctx].rtrMap->getValue(x);
			int rtr = net->getNodeNum(radr);
			ss << "\t(";
			if (cri.plnk == 0) { // special case of root
				ss << net->getNodeName(rtr) << ","
				   << cri.lnkCnt;
			} else {
				int parent = net->getPeer(rtr,cri.plnk);
				ss << net->getNodeName(rtr) << "."
				   << net->getLLnum(cri.plnk,rtr) << ",";
				ss << net->getNodeName(parent) << ".";
				ss << net->getLLnum(cri.plnk,parent) 
				   << "," << cri.plnkRates.toString()
				   << "," << cri.lnkCnt;
			}
			ss << ")";
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		// iterate through static leaf nodes and print parent links
		for (int x = comtree[ctx].leafMap->first(); x != 0;
			 x = comtree[ctx].leafMap->next(x)) {
			fAdr_t ladr = comtree[ctx].leafMap->getKey(x);
			ComtLeafInfo& cli = comtree[ctx].leafMap->getValue(x);
			int leaf = net->getNodeNum(ladr);
			if (leaf == 0) continue; // skip dynamic leafs
			ss << "\t(";
			int parent = net->getNodeNum(cli.parent);
			ss << net->getNodeName(leaf) << ","
			   << net->getNodeName(parent) << "." << cli.llnk
			   << "," << cli.plnkRates.toString() << ")";
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		ss << ")\n";
	}
	return ss.str();
}

string ComtInfo::comtStatus22string(int ctx) const {
	if (!validComtIndex(ctx)) return "";
	stringstream ss;
	ss << "comtree(" << getComtree(ctx) << ","
	   << net->getNodeName(net->getNodeNum(getOwner(ctx))) << ",";
	ss << net->getNodeName(net->getNodeNum(getRoot(ctx))) << ","
	   << (getConfigMode(ctx) ? "auto" : "manual") << ",";
	ss << comtree[ctx].bbDefRates.toString() << ",";
	ss << comtree[ctx].leafDefRates.toString();
	int numNodes = comtree[ctx].rtrMap->size()
			+ comtree[ctx].leafMap->size();
	if (numNodes <= 1) {
		ss << ")\n"; return ss.str();
	} else if (comtree[ctx].coreSet->size() > 1) {
		ss << ",\n\t(";
		// iterate through core nodes and print
		bool first = true;
		for (fAdr_t c = firstCore(ctx); c != 0; c = nextCore(ctx,c)) {
			if (c == getRoot(ctx)) continue;
			if (first) first = false;
			else	   ss << ",";
			ss << net->getNodeName(net->getNodeNum(c));
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
		for (int x = comtree[ctx].leafMap->first(); x != 0;
			 x = comtree[ctx].leafMap->next(x)) {
			fAdr_t ladr = comtree[ctx].leafMap->getKey(x);
			if (net->getNodeNum(ladr) != 0) num2go++;
		}
		// iterate through routers and print nodes, parents, rates
		// and link counts
		int numDone = 0;
		for (int x = comtree[ctx].rtrMap->first(); x != 0;
			 x = comtree[ctx].rtrMap->next(x)) {
			fAdr_t radr = comtree[ctx].rtrMap->getKey(x);
			ComtRtrInfo& cri = comtree[ctx].rtrMap->getValue(x);
			int rtr = net->getNodeNum(radr);
			ss << "\t(";
			if (cri.plnk == 0) { // special case of root
				ss << net->getNodeName(rtr) << ","
				   << cri.lnkCnt;
			} else {
				int parent = net->getPeer(rtr,cri.plnk);
				ss << net->getNodeName(rtr) << "."
				   << net->getLLnum(cri.plnk,rtr) << ","
				   << net->getNodeName(parent) << "."
				   << net->getLLnum(cri.plnk,parent) 
				   << "," << cri.plnkRates.toString()
				   << "," << cri.subtreeRates.toString()
				   << "," << cri.lnkCnt;
			}
			ss << ")";
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		// iterate through static leaf nodes and print parent links
		for (int x = comtree[ctx].leafMap->first(); x != 0;
			 x = comtree[ctx].leafMap->next(x)) {
			fAdr_t ladr = comtree[ctx].leafMap->getKey(x);
			ComtLeafInfo& cli = comtree[ctx].leafMap->getValue(x);
			int leaf = net->getNodeNum(ladr);
			if (leaf == 0) continue; // skip dynamic leafs
			ss << "\t(";
			int parent = net->getNodeNum(cli.parent);
			ss << net->getNodeName(leaf) << ","
			   << net->getNodeName(parent) << "." << cli.llnk
			   << "," << cli.plnkRates.toString() << ")";
			if (++numDone < num2go) ss << ",";
			ss << "\n";
		}
		ss << ")\n";
	}
	return ss.str();
}

} // ends namespace

