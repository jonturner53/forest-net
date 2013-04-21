/** @file ComtInfo.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef COMTINFO_H
#define COMTINFO_H

#include "NetInfo.h"
#include "Heap.h"

/** Class used to represent information used to modify a link.  */
class LinkMod {
public:
	int lnk;		///< link number of the link
	int child;		///< node number for rtr at "lower-end" of lnk
	RateSpec rs;		///< rate spec for link
	LinkMod() { lnk = 0; child = 0; rs.set(0); }
	LinkMod(int l, int c, RateSpec& rs1) { set(l,c,rs1); }
	LinkMod(const LinkMod& lm) { (*this) = lm; }
	void set(int l, int c, RateSpec& rs1) {
		lnk = l; child = c; rs = rs1;
	}
};

/** Maintains information about the comtrees in a Forest network.
 */
class ComtInfo {
public:
		ComtInfo(int,NetInfo&);
		~ComtInfo();

	// predicates
	bool	validComtree(int) const;
	bool	validComtIndex(int) const;
	bool	isComtNode(int,int) const;
	bool	isComtRtr(int,int) const;
	bool	isComtLeaf(int,int) const;
	bool	isCoreNode(int,fAdr_t) const;
	bool	isComtLink(int,int) const;
	// methods for iterating through comtrees and comtree components
	int	firstComtIndex() const;
	int	nextComtIndex(int) const;
	fAdr_t	firstCore(int) const;
	fAdr_t	nextCore(int, fAdr_t) const;
	fAdr_t	firstRouter(int) const;
	fAdr_t	nextRouter(int, fAdr_t) const;
	fAdr_t	firstLeaf(int) const;
	fAdr_t	nextLeaf(int, fAdr_t) const;
	// access comtree attributes
	int	getComtIndex(int) const;
	int	getComtree(int) const;
	fAdr_t	getOwner(int) const;
	fAdr_t	getRoot(int) const;
	bool	getConfigMode(int) const;
	RateSpec& getDefLeafRates(int);
	RateSpec& getDefBbRates(int);
	fAdr_t	getChild(int,int) const;
	int	getPlink(int,fAdr_t) const;
	fAdr_t	getParent(int,fAdr_t) const;
	int	getLinkCnt(int,fAdr_t) const;
	bool	isFrozen(int,fAdr_t) const;
	RateSpec& getLinkRates(int,int) const;
	// add/remove/modify comtrees
	int	addComtree(int);
	bool	removeComtree(int);
	bool	setRoot(int,fAdr_t);
	bool	setOwner(int,fAdr_t);
	void	setConfigMode(int,bool);
	//void	setDefRates(int,RateSpec&,RateSpec&);
	bool	addNode(int,fAdr_t);
	bool	removeNode(int,fAdr_t);
	bool	addCoreNode(int,fAdr_t);
	bool	removeCoreNode(int,fAdr_t);
	bool	setPlink(int,fAdr_t,int);
	bool	setParent(int,fAdr_t,fAdr_t,int);
	void	freeze(int,fAdr_t);
	void	thaw(int,fAdr_t);
	//bool 	setLinkRates(int,fAdr_t,RateSpec&);
	// compute rates and provision network capacity
	bool	setAllComtRates();
	bool	setComtRates(int);
	void	setAutoConfigRates(int);
	bool	checkComtRates(int);
	int	findPath(int,int,RateSpec&,list<LinkMod>&);
	void	addPath(int,list<LinkMod>&);
	void	removePath(int,list<LinkMod>&);
	bool	adjustSubtreeRates(int,int,RateSpec&);
	bool	computeMods(int,list<LinkMod>&);
	bool	computeMods(int,fAdr_t,RateSpec&,list<LinkMod>&);
	void	provision(int);
	void	provision(int,list<LinkMod>&);
	void	unprovision(int);
	void	unprovision(int,list<LinkMod>&);

	// io methods
	bool	read(istream&);
	comt_t	readComtree(istream&,string&);
	bool	readRateSpec(istream&,RateSpec&);
	bool	readLink(istream&,int&,RateSpec&,int&,string&);
	bool	readLinkEndpoint(istream&,string&,int&);
	string& link2string(int,int,string&) const;
	string& leafLink2string(int,fAdr_t,string&) const;
	string& comt2string(int,string&) const;
	string& comtStatus2string(int,string&) const;
	string& comtStatus22string(int,string&) const;
	string& toString(string&) const;

	// verification methods
	bool	check();
	bool	checkLinkCounts(int);
	bool	checkSubtreeRates(int);
	bool	checkLinkRates(int);

private:
	int maxComtree;		///< maximum number of comtrees
	NetInfo	*net;		///< NetInfo data structure for network

	// used in rtrMap of ComtreeInfo
	struct ComtRtrInfo {
	int	plnk;		// link to parent in comtree
	int	lnkCnt;		// number of comtree links at this router
	RateSpec subtreeRates;	// rates for subtree rooted at this node
	bool	frozen;		// true if plnk rate is frozen
	RateSpec plnkRates;	// rates for link
	ComtRtrInfo() { plnk = lnkCnt = 0; frozen = false;
			 subtreeRates.set(0); plnkRates.set(0); };		
	ComtRtrInfo(const ComtRtrInfo& cr) { (*this) = cr; }
	};
	struct ComtLeafInfo {
	fAdr_t	parent;		// forest address of parent of this leaf
	int	llnk;		// local link number of parent link at parent
	RateSpec plnkRates;	// rates for leaf and link to parent
	ComtLeafInfo() { parent = llnk = 0; plnkRates.set(0); };		
	ComtLeafInfo(const ComtLeafInfo& cl) { (*this) = cl; }
	};

	struct ComtreeInfo {
	int	comtreeNum;	///< number of comtree
	fAdr_t	owner;		///< forest address of comtree owner
	fAdr_t	root;		///< forest address of root node of comtree
	bool	autoConfig;	///< if true, set comtree rates dynamically
	RateSpec bbDefRates;	///< default backbone link rates
	RateSpec leafDefRates;	///< default backbone link rates
	set<fAdr_t> *coreSet;	///< set of core nodes in comtree
	map<fAdr_t,ComtRtrInfo> *rtrMap; ///< map for routers in comtree
	map<fAdr_t,ComtLeafInfo> *leafMap; ///< map for leaf nodes in comtree
	};
	ComtreeInfo *comtree;	///< array of comtrees
	IdMap *comtreeMap;	///< maps comtree numbers to indices in array
};

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
inline bool ComtInfo::validComtree(int comt) const {
	return comtreeMap->validKey(comt);
}

/** Check to see if a given comtree index is valid.
 *  ComtInfo uses integer indices in a restricted range to access
 *  comtree information efficiently. Users get the comtree index
 *  for a given comtree number using the lookupComtree() method.
 *  @param ctx is an integer comtree index
 *  @return true if n corresponds to a comtree in the network,
 *  else false
 */
inline bool ComtInfo::validComtIndex(int ctx) const {
	return comtreeMap->validId(ctx);
}

/** Determine if a router is a core node in a specified comtree.
 *  @param ctx is a valid comtree index
 *  @param r is the forest address of a router
 *  @return true if r is a core node in the comtree with index i,
 *  else false
 */
inline bool ComtInfo::isCoreNode(int ctx, fAdr_t r) const {
	return comtree[ctx].coreSet->find(r) != comtree[ctx].coreSet->end();
}

/** Determine if a node belongs to a comtree or not.
 *  @param ctx is the comtree index of the comtree of interest
 *  @param fa is the forest address of a node that is to be checked
 *  for membership in the comtree
 *  @return true if n is a node in the comtree, else false
 */
inline bool ComtInfo::isComtNode(int ctx, fAdr_t fa) const {
        return isComtLeaf(ctx,fa) || isComtRtr(ctx,fa);
}

/** Determine if a node is a router in a comtree or not.
 *  @param ctx is the comtree index of the comtree of interest
 *  @param fa is the forest address of the node that we want to check 
 *  in the comtree
 *  @return true if fa is a node in the comtree, else false
 */
inline bool ComtInfo::isComtRtr(int ctx, fAdr_t fa) const {
        return comtree[ctx].rtrMap->find(fa) != comtree[ctx].rtrMap->end();
}

/** Determine if a node is a leaf in a comtree.
 *  @param ctx is the comtree index of the comtree of interest
 *  @param ln is the forest address of a network node
 *  @return true if ln is a leaf in the comtree, else false
 */
inline bool ComtInfo::isComtLeaf(int ctx, fAdr_t ln) const {
        return comtree[ctx].leafMap->find(ln) != comtree[ctx].leafMap->end();
}

/** Determine if a link is in a specified comtree.
 *  @param ctx is a valid comtree index
 *  @param lnk is a link number
 *  @return true if lnk is a link in the comtree with index i,
 *  else false
 */
inline bool ComtInfo::isComtLink(int ctx, int lnk) const {
	fAdr_t left = net->getNodeAdr(net->getLeft(lnk));
	fAdr_t right = net->getNodeAdr(net->getRight(lnk));
	
	return	(isComtNode(ctx,left) && right == getParent(ctx,left)) ||
	    	(isComtNode(ctx,right) && left == getParent(ctx,right));
}

/** Get the first valid comtree index.
 *  Used for iterating through all the comtrees.
 *  @return the index of the first comtree, or 0 if no comtrees defined
 */
inline int ComtInfo::firstComtIndex() const {
	return comtreeMap->firstId();
}

/** Get the next valid comtree index.
 *  Used for iterating through all the comtrees.
 *  @param ctx is a comtree index
 *  @return the index of the next comtree after, or 0 if there is no
 *  next index
 */
inline int ComtInfo::nextComtIndex(int ctx) const {
	return comtreeMap->nextId(ctx);
}

/** Get the forest address of the first core node in a comtree.
 *  @param ctx is a valid comtree index
 *  @return the forest address of the first core node in the comtree,
 *  or 0 if the comtree index is invalid or no core nodes have been defined
 */
inline int ComtInfo::firstCore(int ctx) const {
	set<fAdr_t>::iterator p = comtree[ctx].coreSet->begin();
	return (p != comtree[ctx].coreSet->end() ? *p : 0);
}

/** Get the forest address of the next core node in a comtree.
 *  @param ctx is the comtree index
 *  @param rtr is the forest address of a core node in the comtree
 *  @return the forest address of the next core node in the comtree,
 *  or 0 if there is no next core node
 */
inline int ComtInfo::nextCore(int ctx, fAdr_t rtr) const {
	set<fAdr_t>::iterator p = comtree[ctx].coreSet->find(rtr);
	p++;
	return (p != comtree[ctx].coreSet->end() ? *p : 0);
}

/** Get the forest address of the first router in a comtree.
 *  @param ctx is a valid comtree index
 *  @return the forest address of the first router in the comtree,
 *  or 0 if there is no router in the comtree
 */
inline int ComtInfo::firstRouter(int ctx) const {
	map<fAdr_t,ComtRtrInfo>::iterator p = comtree[ctx].rtrMap->begin();
	return (p != comtree[ctx].rtrMap->end() ? p->first : 0);
}

/** Get the forest address of the next router in a comtree.
 *  @param ctx is a valid comtree index
 *  @param rtr is the forest address of a router in the comtree
 *  @return the forest address of the next router in the comtree,
 *  or 0 if there is no next router
 */
inline int ComtInfo::nextRouter(int ctx, fAdr_t rtr) const {
	map<fAdr_t,ComtRtrInfo>::iterator p = comtree[ctx].rtrMap->find(rtr);
	p++;
	return (p != comtree[ctx].rtrMap->end() ? p->first : 0);
}

/** Get the forest address of the first leaf in a comtree.
 *  @param ctx is a valid comtree index
 *  @return the forest address of the first leaf in the comtree,
 *  or 0 if there is no leaf in the comtree
 */
inline int ComtInfo::firstLeaf(int ctx) const {
	map<fAdr_t,ComtLeafInfo>::iterator p = comtree[ctx].leafMap->begin();
	return (p != comtree[ctx].leafMap->end() ? p->first : 0);
}

/** Get the forest address of the next leaf in a comtree.
 *  @param ctx is a valid comtree index
 *  @param leaf is the forest address of a leaf in the comtree
 *  @return the forest address of the next leaf in the comtree,
 *  or 0 if there is no next leaf
 */
inline int ComtInfo::nextLeaf(int ctx, fAdr_t leaf) const {
	map<fAdr_t,ComtLeafInfo>::iterator p = comtree[ctx].leafMap->find(leaf);
	p++;
	return (p != comtree[ctx].leafMap->end() ? p->first : 0);
}

/** Get the comtree index for a given comtree number.
 *  @param comt is a comtree number
 *  @param return the index used to efficiently access stored information
 *  for the comtree.
 */
inline int ComtInfo::getComtIndex(int comt) const {
	return comtreeMap->getId(comt);
}

/** Get the comtree number for the comtree with a given index.
 *  @param ctx is a valid comtree index
 *  @param return the comtree number that is mapped to index c
 *  or 0 if c is not a valid index
 */
inline int ComtInfo::getComtree(int ctx) const {
	return comtree[ctx].comtreeNum;
}

/** Get the comtree root for the comtree with a given index.
 *  @param ctx is a valid comtree index
 *  @param return the specified root node for the comtree
 *  or 0 if c is not a valid index
 */
inline fAdr_t ComtInfo::getRoot(int ctx) const {
	return comtree[ctx].root;
}

/** Get the owner of a comtree with specified index.
 *  @param ctx is a valid comtree index
 *  @param return the forest address of the client that originally
 *  created the comtree, or 0 if c is not a valid index
 */
inline fAdr_t ComtInfo::getOwner(int ctx) const {
	return comtree[ctx].owner;
}

/** Get the link to a parent of a node in a comtree.
 *  @param ctx is a valid comtree index 
 *  @param nfa is a forest address of a node in the comtree
 *  @return the global link number of the link to nfa's parent,
 *  if nfa is a router; return the local link number at nfa's
 *  parent if nfa is a leaf; return 0 if nfa has no parent
 **/
inline int ComtInfo::getPlink(int ctx, fAdr_t nfa) const {
        map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(nfa);
	if (rp != comtree[ctx].rtrMap->end())
		return rp->second.plnk;
        map<fAdr_t,ComtLeafInfo>::iterator lp;
	lp = comtree[ctx].leafMap->find(nfa);
	return lp->second.llnk;
}

/** Get the parent of a comtree node.
 *  @param ctx is a valid comtree index 
 *  @param fa is the number of a node in the comtree
 *  @return the forest address of fa's parent, or 0 if fa has no parent
 **/
inline fAdr_t ComtInfo::getParent(int ctx, fAdr_t fa) const {
        map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(fa);
	if (rp != comtree[ctx].rtrMap->end()) {
		if (rp->second.plnk == 0) return 0;
		int parent = net->getPeer(net->getNodeNum(fa),rp->second.plnk);
		return net->getNodeAdr(parent);
	}
        map<fAdr_t,ComtLeafInfo>::iterator lp;
	lp = comtree[ctx].leafMap->find(fa);
	return lp->second.parent;
}

/** Get the child endpoint of a link in a comtree.
 *  @param ctx is a valid comtree index 
 *  @param lnk is a link number for a link in the comtree
 *  @return the forest address of the child node, or 0 on failure
 **/
inline fAdr_t ComtInfo::getChild(int ctx, int lnk) const {
	int left = net->getLeft(lnk);
	fAdr_t leftAdr = net->getNodeAdr(left);
	if (net->isLeaf(left)) return leftAdr;
	int right = net->getRight(lnk);
	fAdr_t rightAdr = net->getNodeAdr(right);
	if (net->isLeaf(right)) return rightAdr;
        map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(leftAdr);
	return (rp->second.plnk == lnk ? leftAdr : rightAdr);
}

/** Get the number of links in a comtree that are incident to a router.
 *  @param ctx is a valid comtree index 
 *  @param rtr is the number of a router in the comtree
 *  @return the number of links in the comtree incident to rtr
 *  or -1 if n is not a node in the comtree
 **/
inline int ComtInfo::getLinkCnt(int ctx, fAdr_t rtr) const {
        map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(rtr);
        return rp->second.lnkCnt;
}

/** Get the default RateSpecs for access links in a comtree.
 *  @param ctx is a valid comtree index
 *  @return a reference to the default rate spec for access links
 */
inline RateSpec& ComtInfo::getDefLeafRates(int ctx) {
	return comtree[ctx].leafDefRates;
}

/** Get the default RateSpecs for backbone links in a comtree.
 *  @param ctx is a valid comtree index
 *  @return a reference to the default rate spec for backbone links
 */
inline RateSpec& ComtInfo::getDefBbRates(int ctx) {
	return comtree[ctx].bbDefRates;
}

/** Determine if the rates for a comtree backbone link are frozen.
 *  @param ctx is a valid comtree index
 *  @param rtr is a forest address for a node in the comtree
 *  @return true on if rtr has a parent and the link to its parent is frozen,
 *  else false
 */
inline bool ComtInfo::isFrozen(int ctx, fAdr_t rtr) const {
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(rtr);
	return rp->second.plnk != 0 && rp->second.frozen;
}

/** Get the RateSpec for a comtree link.
 *  The rates represent the amount of network capacity required for
 *  this link, given the subtree rates and the backbone configuration mode.
 *  @param ctx is a valid comtree index
 *  @param fa is a forest address for a node in the comtree
 *  @return a reference to a rateSpec in for the
 *  link from fa to its parent
 *  @return true on success, false on failure
 */
inline RateSpec& ComtInfo::getLinkRates(int ctx, fAdr_t fa) const {
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(fa);
	if (rp != comtree[ctx].rtrMap->end()) return rp->second.plnkRates;
	map<fAdr_t,ComtLeafInfo>::iterator lp;
	lp = comtree[ctx].leafMap->find(fa);
	return lp->second.plnkRates;
}

/** Add a new comtree.
 *  Defines a new comtree, with attributes left undefined.
 *  @param comt is the comtree number for the new comtree.
 *  @return the index of the new comtree on success, else 0
 */
inline int ComtInfo::addComtree(int comt) {
	int ctx = comtreeMap->addPair(comt);
	if (ctx == 0) return 0;
	comtree[ctx].comtreeNum = comt;
	comtree[ctx].coreSet = new set<fAdr_t>;
	comtree[ctx].rtrMap = new map<fAdr_t,ComtRtrInfo>;
	comtree[ctx].leafMap = new map<fAdr_t,ComtLeafInfo>;
	return ctx;
}

/** Remove a comtree.
 *  Assumes that all bandwidth resources in underlying network
 *  have already been released.
 *  @param comt is the comtree number for the new comtree.
 *  @return true on success, false on failure
 */
inline bool ComtInfo::removeComtree(int ctx) {
	if (!validComtIndex(ctx)) return false;
	comtreeMap->dropPair(comtree[ctx].comtreeNum);
	comtree[ctx].comtreeNum = 0;
	delete comtree[ctx].coreSet;
	delete comtree[ctx].leafMap;
	delete comtree[ctx].rtrMap;
	return true;
}

/** Add a new node to a comtree.
 *  @param ctx is the comtree index
 *  @param fa is a valid forest address
 *  @return true on success, false on failure
 */
inline bool ComtInfo::addNode(int ctx, fAdr_t fa) {
	int nn = net->getNodeNum(fa);
	if (nn != 0 && net->isRouter(nn)) {
		map<fAdr_t,ComtRtrInfo>::iterator rp;
	    	rp = comtree[ctx].rtrMap->find(fa);
	    	if (rp != comtree[ctx].rtrMap->end()) return true;
		pair<fAdr_t,ComtRtrInfo> newPair;
		newPair.first = fa;
		comtree[ctx].rtrMap->insert(newPair);
		return true;
	}
	map<fAdr_t,ComtLeafInfo>::iterator lp;
	lp = comtree[ctx].leafMap->find(fa);
	if (lp != comtree[ctx].leafMap->end()) return true;
	pair<fAdr_t,ComtLeafInfo> newPair;
	newPair.first = fa;
	newPair.second.plnkRates = comtree[ctx].leafDefRates;
	if (nn != 0) {
		int plnk = net->firstLinkAt(nn);
		int parent = net->getPeer(nn,plnk);
		newPair.second.parent = net->getNodeAdr(parent);
		newPair.second.llnk = net->getLLnum(plnk,parent);
	}
	comtree[ctx].leafMap->insert(newPair);
	return true;
}

/** Remove a node from a comtree.
 *  This method will fail if the node is a router with links to children
 *  Updates link counts at parent router.
 *  @param ctx is the comtree index
 *  @param fa is the forest address of a comtree node
 *  @return true on success, false on failure
 */
inline bool ComtInfo::removeNode(int ctx, fAdr_t fa) {
	map<fAdr_t,ComtRtrInfo>::iterator np;
	np = comtree[ctx].rtrMap->find(fa);
	if (np != comtree[ctx].rtrMap->end()) {
		int plnk = np->second.plnk;
		if ((plnk == 0 && np->second.lnkCnt != 0) ||
		    (plnk != 0 && np->second.lnkCnt != 1))
			return false;
		if (plnk != 0) {
			int parent = net->getPeer(net->getNodeNum(fa),plnk);
			map<fAdr_t,ComtRtrInfo>::iterator pp;
			pp = comtree[ctx].rtrMap->find(net->getNodeAdr(parent));
			pp->second.lnkCnt--;
		}
		comtree[ctx].rtrMap->erase(np);
		comtree[ctx].coreSet->erase(fa);
		return true;
	}
	map<fAdr_t,ComtLeafInfo>::iterator lp;
	lp = comtree[ctx].leafMap->find(fa);
	map<fAdr_t,ComtRtrInfo>::iterator pp;
	pp = comtree[ctx].rtrMap->find(lp->second.parent);
	pp->second.lnkCnt--;
	comtree[ctx].leafMap->erase(lp);
	return true;
}

/** Set the owner of a comtree.
 *  @param ctx is the index of the comtree
 *  @param owner is the forest address of the comtree owner
 *  @return true on success, false on failure
 */
inline bool ComtInfo::setOwner(int ctx, fAdr_t owner) {
	if (net->getNodeNum(owner) == 0) return false;
	comtree[ctx].owner = owner;
	return true;
}

/** Set the root node of a comtree.
 *  @param ctx is the index of the comtree
 *  @param r is the forest address of the router the new comtree root
 *  @return true on success, false on failure
 */
inline bool ComtInfo::setRoot(int ctx, fAdr_t r) {
	if (net->getNodeNum(r) == 0) return false;
	comtree[ctx].root = r;
	return true;
}

/** get the backbone configuration mode of a comtree.
 *  @param ctx is the index of the comtree
 *  @return the backbone configuration mode
 */
inline bool ComtInfo::getConfigMode(int ctx) const {
	return comtree[ctx].autoConfig;
}

/** Set the backbone configuration mode of a comtree.
 *  @param ctx is the index of the comtree
 *  @param auto is the new configuration mode
 *  @return true on success, false on failure
 */
inline void ComtInfo::setConfigMode(int ctx, bool autoConfig) {
	comtree[ctx].autoConfig = autoConfig;
}

/** Set the default RateSpecs for a comtree.
 *  @param ctx is a valid comtree index
 *  @param bbRates is a reference to a RateSpec for the new
 *  backbone rates
 *  @param leafRates is a reference to a RateSpec for the new
 *  leaf rates
inline void ComtInfo::setDefRates(int ctx, RateSpec& bbRates,
					      RateSpec& leafRates) {
	comtree[ctx].bbDefRates   = bbRates;
	comtree[ctx].leafDefRates = leafRates;
}
 */

/** Add a new core node to a comtree.
 *  The new node is required to be a router and if it is
 *  not already in the comtree, it is added.
 *  @param ctx is the comtree index
 *  @param r is the forest address of the router
 *  @return true on success, false on failure
 */
inline bool ComtInfo::addCoreNode(int ctx, fAdr_t rtrAdr) {
	int rtr = net->getNodeNum(rtrAdr);
	if (!net->isRouter(rtr)) return false;
	if (!isComtRtr(ctx,rtrAdr)) addNode(ctx,rtrAdr);
	comtree[ctx].coreSet->insert(rtrAdr);
	return true;
}

/** Remove a core node from a comtree.
 *  @param ctx is the comtree index
 *  @param rtrAdr is the forest address of the core node
 *  @return true on success, false on failure
 */
inline bool ComtInfo::removeCoreNode(int ctx, fAdr_t rtrAdr) {
	comtree[ctx].coreSet->erase(rtrAdr);
	return true;
}

/** Set the parent link of a router in the comtree.
 *  Update link counts at router and its parent in the process.
 *  @param ctx is a valid comtree index
 *  @param rtr is the forest address for a router in the comtree
 *  @param plnk is the link number of a link in the comtree
 *  @return true on success, false on failure
 */
inline bool ComtInfo::setPlink(int ctx, fAdr_t rtr, int plnk) {
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(rtr);
	rp->second.plnk = plnk;
	rp->second.lnkCnt++;
	if (plnk == 0) return true;

	fAdr_t parent = net->getNodeAdr(net->getPeer(net->getNodeNum(rtr),plnk));
	rp = comtree[ctx].rtrMap->find(parent);
	rp->second.lnkCnt++;
	return true;
}

/** Set the parent of a leaf in the comtree.
 *  @param ctx is a valid comtree index
 *  @param leaf is the forest address for a leaf in the comtree
 *  @param parent is the forest address of the parent node in the comtree
 *  @param llnk is the local link number that the parent uses to reach leaf
 *  @return true on success, false on failure
 */
inline bool ComtInfo::setParent(int ctx, fAdr_t leaf, fAdr_t parent, int llnk) {
	map<fAdr_t,ComtLeafInfo>::iterator lp;
	lp = comtree[ctx].leafMap->find(leaf);
	lp->second.parent = parent;
	lp->second.llnk = llnk;

	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(parent);
	rp->second.lnkCnt++;
	
	return true;
}

/** Freeze the rate of a comtree link.
 *  @param ctx is a valid comtree index
 *  @param rtr is a forest address for a router in the comtree; this method
 *  freezes the rate of the parent link at rtr
 */
inline void ComtInfo::freeze(int ctx, fAdr_t rtr) {
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(rtr);
	rp->second.frozen = true;
}

/** Thaw (unfreeze) the rate of a comtree link.
 *  @param ctx is a valid comtree index
 *  @param rtr is a forest address for a router in the comtree; this method
 *  unfreezes the rate of the parent link at rtr
 */
inline void ComtInfo::thaw(int ctx, fAdr_t rtr) {
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(rtr);
	rp->second.frozen = false;
}

/** Set the allocated RateSpec for a comtree link.
 *  @param ctx is a valid comtree index
 *  @param fa is the forest address for a node in the comtree; this method
 *  sets the rates for the parent link at fa
 *  @param newRates is the new allocated RateSpec for the link
 *  @return true on success, false on failure (if node has no parent link)
inline bool ComtInfo::setLinkRates(int ctx, fAdr_t fa, RateSpec& newRates) {
	map<fAdr_t,ComtRtrInfo>::iterator rp;
	rp = comtree[ctx].rtrMap->find(fa);
	if (rp != comtree[ctx].rtrMap->end()) {
		if (rp->second.plnk == 0) return false;
		rp->second.plnkRates = newRates;
		return true;
	}
	map<fAdr_t,ComtLeafInfo>::iterator lp;
	lp = comtree[ctx].leafMap->find(fa);
	lp->second.plnkRates = newRates;
	return true;
}
 */

#endif
