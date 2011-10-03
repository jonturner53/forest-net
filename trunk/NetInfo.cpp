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
	netTopo = new Graph(maxNode, maxLink);

	rtr = new RtrNodeInfo[maxRtr+1];
	routers = new UiSetPair(maxRtr);

	leaf = new LeafNodeInfo[maxLeaf+1];
	leaves = new UiSetPair(maxLeaf);
	nodeNumMap = new map<string,int>;
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
	delete nodeNumMap;
	delete [] link; delete locLnk2lnk;
	delete [] comtree; delete comtreeMap;
}

/** Get the interface associated with a given local link number.
 *  @param llnk is a local link number
 *  @param rtr is a router
 *  @return the number of the interface that hosts llnk
 */
int NetInfo::getIface(int llnk, int rtr) {
	for (int i = 1; i < getNumIf(rtr); i++) {
		if (validIf(i,rtr) && llnk >= getIfFirstLink(rtr)
				   && llnk <= getIfLastLink(rtr))
			return i;
	}
	return 0;
;

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
	if (nodeNumMap->find(name) != nodeNumMap->end()) return 0;
	routers->swap(r);

	rtr[r].name = name; (*nodeNumMap)[name] = r;
	rtr[r].nType = ROUTER;
	rtr[r].fAdr = -1;

	setNodeLat(r,UNDEF_LAT); setNodeLong(r,UNDEF_LONG);
	setFirstCliAdr(r,-1); setLastCliAdr(r,-1);
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
	if (nodeNumMap->find(name) != nodeNumMap->end()) return 0;
	leaves->swap(ln);

	int nodeNum = ln + maxRtr;
	leaf[ln].name = name;
	(*nodeNumMap)[name] = nodeNum;
	if (nTyp == CONTROLLER) controllers->insert(ln);
	leaf[ln].fAdr = -1;

	setLeafType(nodeNum,CLIENT); setLeafIpAdr(nodeNum,0); 
	setNodeLat(nodeNum,UNDEF_LAT); setNodeLong(nodeNum,UNDEF_LONG);
	return nodeNum;
}

/** Add a link to a forest network.
 *  @param u is a node number of some node in the network
 *  @param v is a leaf number of some node in the network
 *  @param uln if u is a ROUTER, then uln is a local link number used by
 *  u to identify the link - for leaf nodes, this argument is ignored
 *  @param vln if v is a ROUTER, then vln is a local link number used by
 *  v to identify the link - for leaf nodes,  this argument is ignored
 *  @return the link number for the new link or 0 if the operation fails
 *  (the operation fails if it is unable to associate a given local
 *  link number with a specified router)
 */
int NetInfo::addLink(int u, int v, int uln, int vln ) {
        int lnk = netTopo->join(u,v,0);
        if (lnk == 0) return 0;
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
        return lnk;
}

/* Example of NetInfo input file format.
 *  
 *  Routers # starts section that defines routers
 *  
 *  # define salt router
 *  name=salt type=router ipAdr=2.3.4.5 fAdr=1.1000
 *  location=(40,-50) clientAdrRange=(1.1-1.200)
 *  
 *  interfaces # router interfaces
 *  # ifaceNum ifaceIp      ifaceLinks  bitRate  pktRate
 *       1     193.168.3.4  1           50000    25000 ;
 *       2     193.168.3.5  2-30        40000    20000 ;
 *  end
 *  ;
 *  
 *  # define kans router
 *  name=kans type=router ipAdr=6.3.4.5 fAdr=2.1000
 *  location=(40,-40) clientAdrRange=(2.1-2.200)
 *  
 *  interfaces
 *  # ifaceNum ifaceIp      ifaceLinks  bitRate  pktRate
 *       1     193.168.5.6  1           50000    25000 ;
 *       2     193.168.5.7  2-30        40000    20000 ;
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
	cComt.coreSet = 0; cComt.linkMap = 0;

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
		if (!Misc::skipBlank(in)) { 
		break; }
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
			cRtr.firstCliAdr = cRtr.lastCliAdr = 0;
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
				if (!Forest::validUcastAdr(cRtr.firstCliAdr) ||
				    !Forest::validUcastAdr(cRtr.lastCliAdr) ||
				     Forest::zipCode(cRtr.fAdr)
				      != Forest::zipCode(cRtr.firstCliAdr) ||
				     Forest::zipCode(cRtr.fAdr)
				      != Forest::zipCode(cRtr.lastCliAdr) ||
				    cRtr.firstCliAdr > cRtr.lastCliAdr) {
					cerr << "NetInfo::read: no valid client"
					     << " address range for router "
					     << cRtr.name << endl;
					return false;
				}
				if (cRtr.numIf == 0) {
					cerr << "NetInfo::read: interfaces "
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
				setFirstCliAdr(r,cRtr.firstCliAdr);
				setLastCliAdr(r,cRtr.lastCliAdr);
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
			} else if (s.compare("clientAdrRange") == 0 &&
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
				cRtr.firstCliAdr = first;
				cRtr.lastCliAdr  = last;
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
				int u = (*nodeNumMap)[leftName];
				int v = (*nodeNumMap)[rightName];
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
				if (nodeNumMap->find(leftName)  
				     == nodeNumMap->end() ||
				    nodeNumMap->find(rightName)
				     == nodeNumMap->end()) {
					cerr << "NetInfo::read: link number "
					     << linkNum 
					     << " refers to unknown node name "
					     << endl;
					return false;
				}
				if (cLink.leftLnum == 0 &&
				    getNodeType((*nodeNumMap)[leftName])
				     ==ROUTER) {
					cerr << "NetInfo::read: missing local "
					     << "link number for router in "
					     << "link " << linkNum 
					     << endl;
					return false;
				}
				if (cLink.rightLnum == 0 &&
				    getNodeType((*nodeNumMap)[rightName])
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
			cComt.bitRateDown = 0; cComt.bitRateUp = 0;
			cComt.pktRateDown = 0; cComt.pktRateUp = 0;
			cComt.leafBitRateDown = 0; cComt.leafBitRateUp = 0;
			cComt.leafPktRateDown = cComt.leafPktRateUp = 0;
			cComt.coreSet = new set<int>;
			cComt.linkMap = new map<int,RateSpec>;
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
				int i = comtreeMap->addPair(cComt.comtreeNum);
				if (i == 0) {
					cerr << "NetInfo::read: too many "
						"comtrees" << endl;
					return false;
				}
				comtree[i].comtreeNum = cComt.comtreeNum;
				comtree[i].root = cComt.root;
				comtree[i].bitRateDown = cComt.bitRateDown;
				comtree[i].bitRateUp = cComt.bitRateUp;
				comtree[i].pktRateDown = cComt.pktRateDown;
				comtree[i].pktRateUp = cComt.pktRateUp;
				comtree[i].leafBitRateDown =
							cComt.leafBitRateDown;
				comtree[i].leafBitRateUp =
							cComt.leafBitRateUp;
				comtree[i].leafPktRateDown =
							cComt.leafPktRateDown;
				comtree[i].leafPktRateUp =
							cComt.leafPktRateUp;
				comtree[i].coreSet = cComt.coreSet;
				comtree[i].linkMap = cComt.linkMap;
				cComt.coreSet = 0; cComt.linkMap = 0;
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
			} else if (s.compare("root") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readWord(in,s)) {
					cerr << "NetInfo::read: can't read "
					     << "root node for "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				if (nodeNumMap->find(s) == nodeNumMap->end()) {
					cerr << "NetInfo::read: root in "
					     << comtNum  << "-th comtree "
					     << " is an unknown node name"
					     << endl;
					return false;
				}
				cComt.root = (*nodeNumMap)[s];
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
				if (nodeNumMap->find(s) == nodeNumMap->end()) { 
					cerr << "NetInfo::read: invalid router "
					     << "name for core in "
					     << comtNum << "-th comtree"
					     << endl;
					return false;
				}
				int r = (*nodeNumMap)[s];
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
				if (nodeNumMap->find(leftName)  
				     == nodeNumMap->end() ||
				    nodeNumMap->find(rightName)
				     == nodeNumMap->end()) {
					cerr << "NetInfo::read: " << comtNum
					     << "-th comtree refers to "
					     << "unknown node name "
					     << endl;
					return false;
				}
				int left = (*nodeNumMap)[leftName];
				if (leftNum == 0 && isRouter(left)) {
					cerr << "NetInfo::read: missing local "
					     << "link number for router in "
					     << comtNum << "-th comtree" 
					     << endl;
					return false;
				}
				int right = (*nodeNumMap)[rightName];
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
				if (ll != lr ) {
					cerr << "NetInfo::read: reference to "
					     << "a non-existent link in "
					     << comtNum << "-th comtree" 
					     << endl;
					return false;
				}
				pair<int,RateSpec> mv;
        			mv.first = ll;
        			mv.second.bitRateLeft = 0;
				mv.second.bitRateRight = 0;
        			mv.second.pktRateLeft = 0;
				mv.second.pktRateRight = 0;
				cComt.linkMap->insert(mv);
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
	if (cComt.linkMap != 0) delete cComt.linkMap;
	return in.eof() && context == TOP;
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
void NetInfo::write(ostream& out) {
	string s0, s1, s2, s3; // temp strings used with xx2string methods below

	// First write the "Routers" section
	out << "Routers\n\n";
	for (int r = firstRouter(); r != 0; r = nextRouter(r)) {
		out << "name=" << getNodeName(r,s1) << " "
		    << "type=" << Forest::nodeType2string(getNodeType(r),s2)
		    << " fAdr=" << Forest::fAdr2string(getNodeAdr(r),s3);
		out << " clientAdrRange=("
		    << Forest::fAdr2string(getFirstCliAdr(r),s1) << "-"
		    << Forest::fAdr2string(getLastCliAdr(r),s2) << ")";
		out << "\n\tlocation=(" << getNodeLat(r) << ","
		    		     << getNodeLong(r) << ")\n";
		out << "interfaces\n"
		    << "# iface#   ipAdr  linkRange  bitRate  pktRate\n";
		for (int i = 1; i <= getNumIf(r); i++) {
			if (!validIf(r,i)) continue;
			out << "   " << i << "  "
			    << Np4d::ip2string(getIfIpAdr(r,i),s1); 
			if (getIfFirstLink(r,i) == getIfLastLink(r,i)) 
				out << " " << getIfFirstLink(r,i) << " ";
			else
				out << " " << getIfFirstLink(r,i) << "-"
				    << getIfLastLink(r,i) << "  ";
			out << getIfBitRate(r,i) << "  "
			    << getIfPktRate(r,i) << ";\n";
		}
		out << "end\n;\n";
	}
	out << ";\n\n";

	// Next write the LeafNodes, starting with the controllers
	out << "LeafNodes\n\n";
	for (int c = firstController(); c != 0; c = nextController(c)) {
		out << "name=" << getNodeName(c,s0) << " "
		    << "type=" << Forest::nodeType2string(getNodeType(c),s1)
		    << " ipAdr=" << Np4d::ip2string(getLeafIpAdr(c),s2) 
		    << " fAdr=" << Forest::fAdr2string(getNodeAdr(c),s3);
		out << "\n\tlocation=(" << getNodeLat(c) << ","
		    		     << getNodeLong(c) << ");\n";
	}
	for (int c = firstLeaf(); c != 0; c = nextLeaf(c)) {
		if (getNodeType(c) == CONTROLLER) continue;
		out << "name=" << getNodeName(c,s0) << " "
		    << "type=" << Forest::nodeType2string(getNodeType(c),s1)
		    << " ipAdr=" << Np4d::ip2string(getLeafIpAdr(c),s2) 
		    << " fAdr=" << Forest::fAdr2string(getNodeAdr(c),s3);
		out << "\n\tlocation=(" << getNodeLat(c) << ","
		    		     << getNodeLong(c) << ");\n";
	}
	out << ";\n\n";

	// Now, write the Links
	out << "Links\n\n";
	for (int lnk = firstLink(); lnk != 0; lnk = nextLink(lnk)) {
		int ln = getLinkL(lnk); int rn = getLinkR(lnk);
		out << "link=(" << getNodeName(ln,s0);
		if (isRouter(ln)) out << "." << getLocLinkL(lnk);
		out << "," << getNodeName(rn,s0);
		if (isRouter(rn)) out << "." << getLocLinkR(lnk);
		out << ") "
		    << "bitRate=" << getLinkBitRate(lnk) << " "
		    << "pktRate=" << getLinkPktRate(lnk) << " "
		    << "length=" << getLinkLength(lnk) << ";\n";
	}
	out << ";\n\n";

	// And finally, the Comtrees
	out << "Comtrees\n\n";
	for (int ctx = firstComtIndex(); ctx != 0; ctx = nextComtIndex(ctx)) {
		if (!validComtIndex(ctx)) continue;

		out << "comtree=" << getComtree(ctx)
		    << " root=" << getNodeName(getComtRoot(ctx),s0)
		    << "\nbitRateDown=" << getComtBrDown(ctx)
		    << " bitRateUp=" << getComtBrUp(ctx)
		    << " pktRateDown=" << getComtPrDown(ctx)
		    << " pktRateUp=" << getComtPrUp(ctx)
		    << "\nleafBitRateDown=" << getComtLeafBrDown(ctx)
		    << " leafBitRateUp=" << getComtLeafBrUp(ctx)
		    << " leafPktRateDown=" << getComtLeafPrDown(ctx)
		    << " leafPktRateUp=" << getComtLeafPrUp(ctx);

		// iterate through core nodes and print
		out << "\n";
		for (int c = firstCore(ctx); c != 0; c = nextCore(c,ctx)) {
			if (c != getComtRoot(ctx)) 
				out << "core=" << getNodeName(c,s0) << " ";
		}
		out << "\n";
		// iterate through links and print
		for (int lnk = firstComtLink(ctx); lnk != 0;
		         lnk = nextComtLink(lnk,ctx)) {

			int left = getLinkL(lnk); int right = getLinkR(lnk);
			out << "link=(" << getNodeName(left,s0);
			if (isRouter(left)) out << "." << getLocLinkL(lnk);
			out << "," << getNodeName(right,s0);
			if (isRouter(right)) out << "." << getLocLinkR(lnk);
			out << ") ";
		}
		out << "\n;\n";
	}
	out << ";\n";
}
