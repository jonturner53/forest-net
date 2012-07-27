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
 */
NetInfo::NetInfo(int maxNode1, int maxLink1, int maxRtr1, int maxCtl1)
		 : maxRtr(maxRtr1), maxNode(maxNode1), maxLink(maxLink1),
		   maxCtl(maxCtl1) {
	maxLeaf = maxNode-maxRtr;
	netTopo = new Wgraph(maxNode, maxLink);

	rtr = new RtrNodeInfo[maxRtr+1];
	routers = new UiSetPair(maxRtr);

	leaf = new LeafNodeInfo[maxLeaf+1];
	leaves = new UiSetPair(maxLeaf);
	nameNodeMap = new map<string,int>;
	adrNodeMap = new map<fAdr_t,int>;
	controllers = new set<int>();
	defaultLinkRates.set(50,500,25,250); // default for access link rates

	link = new LinkInfo[maxLink+1];
	locLnk2lnk = new HashMap(2*min(maxLink,maxRtr*(maxRtr-1)/2)+1);
}

/** Destructor for NetInfo class.
 *  Deallocates all dynamic storage
 */
NetInfo::~NetInfo() {
// add code to delete other comtree info
	delete netTopo;
	delete [] rtr; delete routers;
	delete [] leaf; delete leaves; delete controllers;
	delete nameNodeMap; delete adrNodeMap;
	delete [] link; delete locLnk2lnk;
}


/** Get the interface associated with a given local link number.
 *  @param llnk is a local link number
 *  @param rtr is a router
 *  @return the number of the interface that hosts llnk
 */
int NetInfo::getIface(int llnk, int rtr) const {
	for (int i = 1; i <= getNumIf(rtr); i++) {
		if (!validIf(rtr,i)) continue;
		pair<int,int> links; getIfLinks(rtr,i,links);
		if (llnk >= links.first && llnk <= links.second) return i;
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

	pair<double,double> loc(UNDEF_LAT,UNDEF_LONG);
	setNodeLocation(r,loc);
	pair<fAdr_t,fAdr_t> range(-1,-1);
	setLeafRange(r,range);
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

	setLeafType(nodeNum,nTyp); setLeafIpAdr(nodeNum,0); 
	pair<double,double> loc(UNDEF_LAT,UNDEF_LONG);
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
int NetInfo::addLink(int u, int v, int uln, int vln) {
	int lnk = (isLeaf(v) ? netTopo->join(v,u) : netTopo->join(u,v));
	if (lnk == 0) return 0;
	netTopo->setWeight(lnk,0);
	if (isRouter(u)) {
		if (!locLnk2lnk->put(ll2l_key(u,uln),2*lnk)) {
			netTopo->remove(lnk); return 0;
		}
		if (isLeaf(v))	setRightLLnum(lnk,uln);
		else 		setLeftLLnum(lnk,uln);
	}
	if (isRouter(v)) {
		if (!locLnk2lnk->put(ll2l_key(v,vln),2*lnk+1)) {
			locLnk2lnk->remove(ll2l_key(u,uln));
			netTopo->remove(lnk); return 0;
		}
		setRightLLnum(lnk,vln);
	}
	RateSpec rs(Forest::MINBITRATE,Forest::MINBITRATE,
		    Forest::MINPKTRATE,Forest::MINPKTRATE);
	setLinkRates(lnk,rs);
	return lnk;
}


/** Reads a network topology file and initializes the NetInfo data structure.
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
 *  (failures are typically accompanied by error messages on cerr)
 */
bool NetInfo::read(istream& in) {
	RtrNodeInfo cRtr;	// holds data for router being parsed
	LeafNodeInfo cLeaf;	// holds data for leaf being parsed
	LinkDesc cLink;		// hods data for link being parsed
	IfInfo iface[Forest::MAXINTF+1]; // holds data for router interfaces

	int rtrNum = 1;		// rtrNum = i for i-th router in file
	int leafNum = 1;	// leafNum = i for i-th leaf in file 
	int linkNum = 1;	// linkNum = i for i-th link in file

	string s, errMsg;
	while (!in.eof()) {
		if (!Util::skipBlank(in) || Util::verify(in,';')) break;
		if (!Util::readWord(in, s)) {
			cerr << "NetInfo:: read: syntax error: expected "
				"(;) or keyword (router,leaf,link)\n";
			return false;
		}
		if (s == "router") {
			if (!readRouter(in, cRtr, iface, errMsg)) {
				cerr << "NetInfo::read: error when attempting "
					" to read " << rtrNum << "-th router ("
					<< errMsg << ")\n";
				return false;
			}
			int r = addRouter(cRtr.name);
			if (r == 0) {
				cerr << "NetInfo::read: cannot add router "
				     << cRtr.name << endl;
				return false;
			}
			setNodeAdr(r,cRtr.fAdr);
			pair<double,double> loc(cRtr.latitude/1000000.0,
						cRtr.longitude/1000000.0);
			setNodeLocation(r,loc);
			pair<fAdr_t,fAdr_t> range(cRtr.firstLeafAdr,
						  cRtr.lastLeafAdr);
			setLeafRange(r,range);
			addInterfaces(r,cRtr.numIf);
			for (int i = 1; i <= getNumIf(r); i++)
				rtr[r].iface[i] = iface[i];
			rtrNum++;
		} else if (s == "leaf") {
			if (!readLeaf(in, cLeaf, errMsg)) {
				cerr << "NetInfo::read: error when attempting "
					" to read " << leafNum << "-th leaf ("
					<< errMsg << ")\n";
				return false;
			}
			int nodeNum = addLeaf(cLeaf.name,cLeaf.nType);
			if (nodeNum == 0) {
				cerr << "NetInfo::read: cannot add leaf "
				     << cLeaf.name << endl;
				return false;
			}
			setLeafType(nodeNum,cLeaf.nType);
			setLeafIpAdr(nodeNum,cLeaf.ipAdr);
			setNodeAdr(nodeNum,cLeaf.fAdr);
			pair<double,double> loc(cLeaf.latitude/1000000.0,
						cLeaf.longitude/1000000.0);
			setNodeLocation(nodeNum,loc);
			leafNum++;
		} else if (s == "link") {
			if (!readLink(in, cLink, errMsg)) {
				cerr << "NetInfo::read: error when attempting "
					" to read " << linkNum << "-th link ("
					<< errMsg << ")\n";
				return false;
			}
			// add new link and set attributes
			map<string,int>::iterator pp;
			pp = nameNodeMap->find(cLink.nameL);
			if (pp == nameNodeMap->end()) {
				cerr << "NetInfo::read: error when attempting "
					" to read " << linkNum << "-th link ("
				     << cLink.nameL << " invalid node name)\n";
			}
			int u = pp->second;
			pp = nameNodeMap->find(cLink.nameR);
			if (pp == nameNodeMap->end()) {
				cerr << "NetInfo::read: error when attempting "
					" to read " << linkNum << "-th link ("
				     << cLink.nameR << " invalid node name)\n";
			}
			int v = pp->second;
			int lnk = addLink(u,v,cLink.numL,cLink.numR);
			if (lnk == 0) {
				cerr << "NetInfo::read: can't add link ("
				     << cLink.nameL << "." << cLink.numL << ","
				     << cLink.nameR << "." << cLink.numR << ")"
				     << endl;
				return false;
			}
			setLinkRates(lnk, cLink.rates);
			cLink.rates.scale(.9);
			setAvailRates(lnk, cLink.rates);
			setLinkLength(lnk, cLink.length);
			linkNum++;
		} else if (s == "defaultLinkRates") {
			if (!readRateSpec(in,defaultLinkRates)) {
				cerr << "NetInfo::read: can't read default "
				    	"rates for links\n";
				return false;
			}
		} else {
			cerr << "NetInfo::read: unrecognized keyword (" << s
			     << ")\n";
			return false;
		}
	}
	return check();
}

/** Read a router description from an input stream.
 *  Expects the next nonblank input character to be the opening parentheis
 *  of the router description.
 *  @param in is an open input stream
 *  @param rtr is a reference to a RtrNodeInfo struct used to return the result
 *  @param ifaces points to an array of IfInfo structs used to return the
 *  interface definitions
 *  @param errMsg is a reference to a string used for returning an error
 *  message
 *  @return true on success, false on failure
 */
bool NetInfo::readRouter(istream& in, RtrNodeInfo& rtr, IfInfo *ifaces,
			 string& errMsg) {
	string name; fAdr_t fadr; pair<double,double> loc;
	pair<fAdr_t,fAdr_t> adrRange;
	
	errMsg = "";
	if (!Util::verify(in,'(',50) ||
	    !Util::readWord(in,name) || !Util::verify(in,',',20)) {
		errMsg = "could not read router name";
		return false;
	}
	if (!Forest::readForestAdr(in,fadr) || !Util::verify(in,',',20)) {
		errMsg = "could not read Forest address for router " + name;
		return false;
	}
	if (!readLocation(in,loc) || !Util::verify(in,',',20)) {
		errMsg = "could not read location for router " + name;
		return false;
	}
	if (!readAdrRange(in,adrRange) || !Util::verify(in,',',20)) {
		errMsg = "could not read address range for router " + name;
		return false;
	}
	Util::skipBlank(in);
	int rv = readIface(in,ifaces,errMsg);
	if (rv == 0) return false;
	int maxif = rv;
	while (Util::verify(in,',',20)) {
		Util::skipBlank(in);
		rv = readIface(in,ifaces,errMsg);
		if (rv == 0) return false;
		maxif = max(rv,maxif);
	}
	if (!Util::verify(in,')',50)) return false;

	rtr.name = name; rtr.fAdr = fadr;
	rtr.latitude = (int) 1000000*loc.first;
	rtr.longitude = (int) 1000000*loc.second;
	rtr.firstLeafAdr = adrRange.first; rtr.lastLeafAdr = adrRange.second;
	rtr.numIf = maxif;
	return true;
}

/** Read a (latitude,longitude) pair.
 *  @param in is an open input stream
 *  @param loc is a reference to a pair of ints in which result is returned
 *  @return true on success, false on failure
 */
bool NetInfo::readLocation(istream& in, pair<double,double>& loc) {
	if (!Util::verify(in,'(',50)) return false;
	in >> loc.first; if (in.fail()) return false;
	if (!Util::verify(in,',',20)) return false;
	in >> loc.second; if (in.fail()) return false;
	if (!Util::verify(in,')',20)) return false;
	return true;
}

/** Read an address range pair. 
 *  @param in is an open input stream
 *  @param range is a reference to a pair of Forest addresses in which
 *  result is returned
 *  @return true on success, false on failure
 */
bool NetInfo::readAdrRange(istream& in, pair<fAdr_t,fAdr_t>& range) {
	return 	Util::verify(in,'(',50) &&
	    	Forest::readForestAdr(in,range.first) &&
	    	Util::verify(in,'-',20) &&
	    	Forest::readForestAdr(in,range.second) &&
	    	Util::verify(in,')',20);
}

/** Read a rate specification.
 *  @param in is an open input stream
 *  @param rs is a reference to a RateSpec in which result is returned
 *  @return true on success, false on failure
 */
bool NetInfo::readRateSpec(istream& in, RateSpec& rs) {
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

/** Read a router interface description from an input stream.
 *  @param in is an open input stream
 *  @param ifaces points to an array of IfInfo structs used to return the
 *  interface definition; this interface is written at the position
 *  specified by its interface number
 *  @param errMsg is a reference to a string used for returning an error
 *  message
 *  @return the number of the interface on success, else 0
 */
int NetInfo::readIface(istream& in, IfInfo *ifaces, string& errMsg) {
	int ifn; ipa_t ip; int firstLink, lastLink; RateSpec rs;

	if (!Util::verify(in,'[',50)) {
		errMsg = "syntax error: expected left bracket";
		return 0;
	}
	in >> ifn;
	if (in.fail()) {
		errMsg = "could not read interface number";
		return 0;
	}
	stringstream ss;
	if (ifn < 1 || ifn > Forest::MAXINTF) {
		ss << "interface number " << ifn << " exceeds allowed range";
		errMsg = ss.str();
		return 0;
	}
	if (!Util::verify(in,',',20) ||
	    !Np4d::readIpAdr(in,ip)) {
		ss << "could not read ip address for interface " << ifn;
		errMsg = ss.str();
		return 0;
	}
	if (!Util::verify(in,',',20)) {
		ss << "syntax error in iface " << ifn << ", expected comma";
		errMsg = ss.str();
		return 0;
	}
	in >> firstLink;
	if (in.fail()) {
		ss << "could not read link range for iface " << ifn;
		errMsg = ss.str();
		return 0;
	}
	lastLink = firstLink;
	if (Util::verify(in,'-',20)) {
		in >> lastLink;
		if (in.fail()) {
			ss << "could not read link range for iface " << ifn;
			errMsg = ss.str();
			return 0;
		}
	}
	if (!Util::verify(in,',',20)) {
		ss << "syntax error in iface " << ifn << ", expected comma";
		errMsg = ss.str();
		return 0;
	}
	if (!readRateSpec(in,rs)) {
		ss << "could not read ip address for interface " << ifn;
		errMsg = ss.str();
		return 0;
	}
	if (!Util::verify(in,']',20)) {
		ss << "syntax error in iface " << ifn
		   << ", expected right bracket";
		errMsg = ss.str();
		return 0;
	}
	ifaces[ifn].ipAdr = ip;
	ifaces[ifn].firstLink = firstLink;
	ifaces[ifn].lastLink = lastLink;
	ifaces[ifn].rates = rs;
	return ifn;
}

/** Read a leaf node description from an input stream.
 *  Expects the next nonblank input character to be the opening parentheis
 *  of the leaf node description.
 *  @param in is an open input stream
 *  @param leaf is a reference to a LeafNodeInfo struct used to return result
 *  @param errMsg is a reference to a string used for returning an error
 *  message
 *  @return true on success, false on failure
 */
bool NetInfo::readLeaf(istream& in, LeafNodeInfo& leaf, string& errMsg) {
	string name; ntyp_t ntyp = UNDEF_NODE; ipa_t ip; fAdr_t fadr;
	pair<double,double> loc; 
	
	errMsg = "";
	if (!Util::verify(in,'(',50) ||
	    !Util::readWord(in,name) || !Util::verify(in,',',20)) {
		errMsg = "could not read leaf node name";
		return false;
	}
	string typstr;
	if (!Util::readWord(in,typstr) ||
	    (ntyp = Forest::getNodeType(typstr)) == UNDEF_NODE ||
	    !Util::verify(in,',',20)) {
		errMsg = "could not read leaf node name";
		return false;
	}
	if (!Np4d::readIpAdr(in,ip) || !Util::verify(in,',',20)) {
		errMsg = "could not read IP address for leaf node " + name;
		return false;
	}
	if (!Forest::readForestAdr(in,fadr) || !Util::verify(in,',',20)) {
		errMsg = "could not read Forest address for leaf node " + name;
		return false;
	}
	if (!readLocation(in,loc) || !Util::verify(in,')',20)) {
		errMsg = "could not read location for leaf node " + name;
		return false;
	}
	leaf.name = name; leaf.nType = ntyp; leaf.ipAdr = ip;
	leaf.fAdr = fadr;
	leaf.latitude = (int) 1000000*loc.first;
	leaf.longitude = (int) 1000000*loc.second;
	return true;
}

/** Read a link description from an input stream.
 *  Expects the next nonblank input character to be the opening parentheis
 *  of the link description.
 *  @param in is an open input stream
 *  @param link is a reference to a LinkDesc struct used to return the result
 *  @param errMsg is a reference to a string used for returning an error
 *  message
 *  @return true on success, false on failure
 */
bool NetInfo::readLink(istream& in, LinkDesc& link, string& errMsg) {
	string nameL, nameR; int numL, numR, length; RateSpec rs;
	
	errMsg = "";
	if (!Util::verify(in,'(',50) ||
	    !readLinkEndpoint(in,nameL,numL) || !Util::verify(in,',',20)) {
		errMsg = "could not first link endpoint";
		return false;
	}
	if (!readLinkEndpoint(in,nameR,numR) || !Util::verify(in,',',20)) {
		errMsg = "could not read second link endpoint";
		return false;
	}
	in >> length;
	if (in.fail()) {
		errMsg = "could not read link length";
		return false;
	}
	if (Util::verify(in,')',20)) { // omitted rate spec
		rs = defaultLinkRates;
	} else {
		if (!Util::verify(in,',',20) || !readRateSpec(in,rs)) {
			errMsg = "could not read rate specification";
			return false;
		}
		if (!Util::verify(in,')',20)) {
			errMsg = "syntax error, expected right paren";
			return false;
		}
	}
	link.nameL = nameL; link.numL = numL;
	link.nameR = nameR; link.numR = numR;
	link.length = length; link.rates = rs;
	return true;
}

/** Read a link endpoint.
 *  @param in is an open input stream
 *  @param name is a reference to a string used to return the endpoint name
 *  @param num is a reference to an int used to return the local link number
 *  @return true on success, false on failure
 */
bool NetInfo::readLinkEndpoint(istream& in, string& name, int& num) {
	if (!Util::readWord(in,name)) return false;
	num = 0;
	if (Util::verify(in,'.')) {
		in >> num;
		if (in.fail() || num < 1) return false;
	}
	return true;	
}

/** Perform a series of consistency checks.
 *  Print error message for each detected problem.
 *  @return true if all checks passed, else false
 */
bool NetInfo::check() const {
	bool status = true;

	// make sure there is at least one router
	if (getNumRouters() == 0 || firstRouter() == 0) {
		cerr << "NetInfo::check: no routers in network, terminating\n";
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
bool NetInfo::checkLocalLinks() const {
	bool status = true;
	for (int rtr = firstRouter(); rtr != 0; rtr = nextRouter(rtr)) {
		for (int l1 = firstLinkAt(rtr); l1 != 0;
			 l1 = nextLinkAt(rtr,l1)) {
			for (int l2 = nextLinkAt(rtr,l1); l2 != 0;
				 l2 = nextLinkAt(rtr,l2)) {
				if (getLLnum(l1,rtr) == getLLnum(l2,rtr)) {
					string s1,s2;
					cerr << "NetInfo::checkLocalLinks: "
						"detected two links at router "
					     << rtr << " with same local link "
						"number: "
					     << link2string(l1,s1) << " and "
					     << link2string(l2,s2) << "\n";
					status = false;
				}
			}
			// check that local link numbers fall within the
			// range of some valid interface
			int llnk = getLLnum(l1,rtr);
			if (getIface(llnk,rtr) == 0) {
				string s;
				cerr << "NetInfo::checkLocalLinks: link "
				     << llnk << " at " << getNodeName(rtr,s)
				     << " is not in the range assigned "
					"to any valid interface\n";
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
bool NetInfo::checkBackBone() const {
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
	if ((int) seen.size() == getNumRouters()) return true;
	cerr << "NetInfo::checkBackbone: network is not connected";
	return false;
}

/** Check that no two nodes have the same Forest address.
 *  Write message to stderr for every error detected.
 *  @return true if all addresses are unique, else false
 */
bool NetInfo::checkAddresses() const {
	bool status = true;
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
	return status;
}

/** Check leaf address ranges of all routers.
 *  Write message to stderr for every error detected.
 *  @return true if each router's leaf address range is consistent with
 *  its address (must all lie in its zip code) and no two routers have
 *  overlapping leaf address ranges, otherwise return false
 */
bool NetInfo::checkLeafRange() const {
	bool status = true;
	// check that the leaf address range for a router
	// is compatible with the router's address
	for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
		int rzip = Forest::zipCode(getNodeAdr(r));
		pair<fAdr_t,fAdr_t> range; getLeafRange(r,range);
		int flzip = Forest::zipCode(range.first);
		int llzip = Forest::zipCode(range.second);
		if (rzip != flzip || rzip != llzip) {
			cerr << "netInfo::checkLeafRange: detected router " << r
			     << "with incompatible address and leaf address "
			     << "range\n";
			status = false;
		}
		if (range.first > range.second) {
			cerr << "netInfo::check: detected router " << r
			     << "with empty leaf address range\n";
			status = false;
		}
	}

	// make sure that no two routers have overlapping leaf
	// address ranges
	for (int r1 = firstRouter(); r1 != 0; r1 = nextRouter(r1)) {
		pair<fAdr_t,fAdr_t> range1; getLeafRange(r1,range1);
		for (int r2 = nextRouter(r1); r2 != 0; r2 = nextRouter(r2)) {
			pair<fAdr_t,fAdr_t> range2; getLeafRange(r2,range2);
			if (range1.first > range2.first) continue;
			if (range2.first <= range1.second) {
				cerr << "netInfo::checkLeafRange: detected two "
					"routers " << r1 << " and " << r2
			     	     << " with overlapping address ranges\n";
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
bool NetInfo::checkLeafNodes() const {
	bool status = true;
	for (int u = firstLeaf(); u != 0; u = nextLeaf(u)) {
		int lnk = firstLinkAt(u);
		if (lnk == 0) {
			string s;
			cerr << "NetInfo::checkLeafNodes: detected a leaf node "
			     << getNodeName(u,s) << " with no links\n";
			status = false; continue;
		}
		if (nextLinkAt(u,lnk) != 0) {
			string s;
			cerr << "NetInfo::checkLeafNodes: detected a leaf node "
			     << getNodeName(u,s)<< " with more than one link\n";
			status = false; continue;
		}
		if (getNodeType(getPeer(u,lnk)) != ROUTER) {
			string s;
			cerr << "NetInfo::checkLeafNodes: detected a leaf node "
			     << getNodeName(u,s) << " with link to non-router"
				"\n";
			status = false; continue;
		}
		int rtr = getPeer(u,lnk);
		int adr = getNodeAdr(u);
		pair<fAdr_t,fAdr_t> range; getLeafRange(rtr,range);
		if (adr < range.first || adr > range.second) {
			string s;
			cerr << "NetInfo::checkLeafNodes: detected a leaf node "
			     << getNodeName(u,s) << " with an address outside "
				" the leaf address range of its router\n";
			status = false; continue;
		}
	}
	return status;
}

/** Check that link rates are consistent with Forest requirements.
 *  @return true if all link rates are consistent, else false
 */
bool NetInfo::checkLinkRates() const {
	bool status = true;
	for (int lnk = firstLink(); lnk != 0; lnk = nextLink(lnk)) {
		RateSpec rs; getLinkRates(lnk,rs);
		if (rs.bitRateUp < Forest::MINBITRATE ||
		    rs.bitRateUp > Forest::MAXBITRATE ||
		    rs.bitRateDown < Forest::MINBITRATE ||
		    rs.bitRateDown > Forest::MAXBITRATE) {
			string s;
			cerr << "NetInfo::check: detected a link "
			     << link2string(lnk,s) << " with bit rate "
				"outside the allowed range\n";
			status = false;
		}
		if (rs.pktRateUp < Forest::MINPKTRATE ||
		    rs.pktRateUp > Forest::MAXPKTRATE ||
		    rs.pktRateDown < Forest::MINPKTRATE ||
		    rs.pktRateDown > Forest::MAXPKTRATE) {
			string s;
			cerr << "NetInfo::check: detected a link "
			     << link2string(lnk,s) << " with packet rate "
				"outside the allowed range\n";
			status = false;
		}
	}
	return status;
}

/** Check interface and link rates at all routers for consistency.
 *  Write an error message to stderr for each error detected
 *  @return true if all check pass, else false
 */
bool NetInfo::checkRtrRates() const {
	bool status =true;
	for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
		// check all interfaces at r
		for (int i = 1; i <= getNumIf(r); i++) {
			if (!validIf(r,i)) continue;
			RateSpec rs; getIfRates(r,i,rs);
			if (rs.bitRateUp < Forest::MINBITRATE ||
			    rs.bitRateUp >Forest::MAXBITRATE ||
			    rs.bitRateDown < Forest::MINBITRATE ||
			    rs.bitRateDown >Forest::MAXBITRATE) {
				cerr << "NetInfo::checkRtrRates: interface "
				     << i << "at router " << r << " has bit "
					"rate outside the allowed range\n";
				status = false;
			}
			if (rs.pktRateUp < Forest::MINPKTRATE ||
			    rs.pktRateUp >Forest::MAXPKTRATE ||
			    rs.pktRateDown < Forest::MINPKTRATE ||
			    rs.pktRateDown >Forest::MAXPKTRATE) {
				cerr << "NetInfo::checkRtrRates: interface "
				     << i << "at router " << r << " has packet "
					"rate outside the allowed range\n";
				status = false;
			}
		}
		// check that the link rates at each interface do not
		// exceed interface rate
		RateSpec* ifrates = new RateSpec[getNumIf(r)+1];
		for (int i = 0; i <= getNumIf(r); i++) ifrates[i].set(0);
		for (int lnk = firstLinkAt(r); lnk != 0;
			 lnk = nextLinkAt(r,lnk)) {
			int llnk = getLLnum(lnk,r);
			int iface = getIface(llnk,r);
			RateSpec rs; getLinkRates(lnk,rs);
			if (r == getLeft(lnk)) rs.flip();
			ifrates[iface].add(rs);
		}
		for (int i = 0; i <= getNumIf(r); i++) {
			if (!validIf(r,i)) continue;
			RateSpec ifrs; getIfRates(r,i,ifrs);
			if (!ifrates[i].leq(ifrs)) {
				string s;
				cerr << "NetInfo::check: links at interface "
				     << i << " of router " << getNodeName(r,s)
				     <<	" exceed its capacity\n";
			}
		}
		delete [] ifrates;
	}
	return status;
}

/** Create a string representation of a link.
 *  @param lnk is a link number
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& NetInfo::link2string(int lnk, string& s) const {
	stringstream ss; string s1;
	if (lnk == 0) { s = "-"; return s; }
	int left = getLeft(lnk); int right = getRight(lnk);
	ss << "(" << getNodeName(left,s1);
	if (getNodeType(left) == ROUTER)  ss << "." << getLeftLLnum(lnk);
	ss << "," << getNodeName(right,s1);
	if (getNodeType(right) == ROUTER) ss << "." << getRightLLnum(lnk);
	ss << ")";
	s = ss.str();
	return s;
}

/** Create a string representation of a link and its properties.
 *  @param link is a link number
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& NetInfo::linkProps2string(int lnk, string& s) const {
	if (lnk == 0) { s = "-"; return s; }
	stringstream ss; string s1;
	int left = getLeft(lnk); int right = getRight(lnk);
	ss << "(" << getNodeName(left,s1);
	if (getNodeType(left) == ROUTER) ss << "." << getLeftLLnum(lnk);
	ss << "," << getNodeName(right,s1);
	if (getNodeType(right) == ROUTER) ss << "." << getRightLLnum(lnk);
	ss << "," << getLinkLength(lnk) << ",";
	RateSpec rs; getLinkRates(lnk,rs); ss << rs.toString(s1) << ")";
	s = ss.str();
	return s;
}

/** Create a string representation of a link and its current state.
 *  @param link is a link number
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& NetInfo::linkState2string(int lnk, string& s) const {
	if (lnk == 0) { s = "-"; return s; }
	stringstream ss; string s1;
	int left = getLeft(lnk); int right = getRight(lnk);
	ss << "(" << getNodeName(left,s1);
	if (getNodeType(left) == ROUTER)  ss << "." << getLeftLLnum(lnk);
	ss << "," << getNodeName(right,s1);
	if (getNodeType(right) == ROUTER)  ss << "." << getRightLLnum(lnk);
	ss << "," << getLinkLength(lnk) << ",";
	RateSpec rs; getLinkRates(lnk,rs); ss << rs.toString(s1);
	getAvailRates(lnk,rs); ss << "," << rs.toString(s1) << ")";
	s = ss.str();
	return s;
}

/** Create a string representation of the NetInfo object.
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& NetInfo::toString(string& s) const {
	// First write the routers section
	s = ""; string s1;
	for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
		s += rtr2string(r,s1);
	}

	// Next write the leaf nodes, starting with the controllers
	for (int c = firstController(); c != 0; c = nextController(c)) {
		s += leaf2string(c,s1);
	}
	for (int n = firstLeaf(); n != 0; n = nextLeaf(n)) {
		if (getNodeType(n) != CONTROLLER) s += leaf2string(n,s1);
	}
	s += '\n';

	// Now, write the links
	for (int lnk = firstLink(); lnk != 0; lnk = nextLink(lnk)) {
		s += "link" + linkProps2string(lnk,s1) + "\n";
	}

	// and finally, the default link rates
	s += "defaultLinkRates" + (defaultLinkRates.toString(s1)) + "\n";

	s += ";\n"; return s;
}

/** Create a string representation of a router.
 *  @param rtr is a node number for a router
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& NetInfo::rtr2string(int rtr, string& s) const {
	stringstream ss; string s1;
	ss << "router(" + getNodeName(rtr,s1) << ", ";
	ss << Forest::fAdr2string(getNodeAdr(rtr),s1) << ", ";
	pair<double,double>loc; getNodeLocation(rtr,loc);
	ss << "(" << loc.first << "," << loc.second << "), ";
	pair<fAdr_t,fAdr_t>range; getLeafRange(rtr,range);
	ss << "(" << Forest::fAdr2string(range.first,s1);
	ss << "-" << Forest::fAdr2string(range.second,s1) << "),\n";
	for (int i = 1; i <= getNumIf(rtr); i++) {
		if (!validIf(rtr,i)) continue;
		ss << "\t[ " << i << ",  "
		   << Np4d::ip2string(getIfIpAdr(rtr,i),s1) << ", "; 
		pair<int,int> links; getIfLinks(rtr,i,links);
		if (links.first == links.second) {
			ss << links.first << ", ";
		} else {
			ss << links.first << "-" << links.second << ", ";
		}
		RateSpec rs; getIfRates(rtr,i,rs);
		ss << rs.toString(s1) << " ]\n";
	}
	ss << ")\n";
	s = ss.str();
	return s;
}

/** Create a string representation of a leaf node.
 *  @param leaf is a node number for a leaf node
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& NetInfo::leaf2string(int leaf, string& s) const {
	stringstream ss; string s1;
	ss << "leaf(" << getNodeName(leaf,s1) << ", ";
	ss << Forest::nodeType2string(getNodeType(leaf),s1) << ", ";
	ss << Np4d::ip2string(getLeafIpAdr(leaf),s1) << ", "; 
	ss << Forest::fAdr2string(getNodeAdr(leaf),s1) << ", ";
	pair<double,double>loc; getNodeLocation(leaf,loc);
	ss << "(" << loc.first << "," << loc.second << ")";
	ss << ")\n";
	s = ss.str();
	return s;
}
