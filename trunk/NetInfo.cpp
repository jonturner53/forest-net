/** @file NetInfo.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "NetInfo.h"

// Constructor for NetInfo, allocates space and initializes private data
NetInfo::NetInfo(int maxNode1, int maxLink1,
		 int maxRtr1, int maxCtl1 , int maxComtree1)
		 : maxNode(maxNode1), maxLink(maxLink1),
		   maxRtr(maxRtr1), maxCtl(maxCtl1), maxComtree(maxComtree1) {
	maxLeaf = maxNode-maxRtr;
	netTopo = new Graph(maxNode, maxLink);

	rtr = new RtrNodeInfo[maxRtr+1];
	freeRouters = new UiList(maxRtr);
	for (int i = 1; i <= maxRtr; i++) freeRouters->addLast(i);
	routers = new list<int>();

	leaf = new LeafNodeInfo[maxLeaf+1];
	freeLeaves = new UiList(maxLeaf);
	for (int i = 1; i <= maxLeaf; i++) freeLeaves->addLast(i);
	nodeNumMap = new map<string,int>;
	controllers = new list<int>();

	link = new LinkInfo[maxLink+1];
	locLnk2lnk = new UiHashTbl(2*min(maxLink,maxRtr*(maxRtr-1)/2)+1);

	comtree = new ComtreeInfo[maxComtree+1];
	comtreeMap = new UiHashTbl(maxComtree);
	freeComtIndices = new UiList(maxComtree);
	for (int i = 1; i <= maxComtree; i++) freeComtIndices->addLast(i);
}

NetInfo::~NetInfo() {
	delete netTopo;
	delete [] rtr; delete freeRouters; delete routers;
	delete [] leaf; delete freeLeaves; delete nodeNumMap;
	delete controllers;
	delete [] link; delete locLnk2lnk;
	delete [] comtree; delete comtreeMap; delete freeComtIndices;
}

/* Exampe of NetInfo input file format
 * 
 * Routers # starts section that defines routers
 * 
 * # define salt router
 * name=salt type=router ipAdr=2.3.4.5 fAdr=1.1000
 * location=(40,-50) clientAdrRange=(1.1-1.200)
 * 
 * interfaces # router interfaces
 * # ifaceNum ifaceIp      ifaceLinks  bitRate  pktRate
 *      1     193.168.3.4  1           50000    25000 ;
 *      2     193.168.3.5  2-30        40000    20000 ;
 * end
 * ;
 * 
 * # define kans router
 * name=kans type=router ipAdr=6.3.4.5 fAdr=2.1000
 * location=(40,-40) clientAdrRange=(2.1-2.200)
 * 
 * interfaces
 * # ifaceNum ifaceIp      ifaceLinks  bitRate  pktRate
 *      1     193.168.5.6  1           50000    25000 ;
 *      2     193.168.5.7  2-30        40000    20000 ;
 * end
 * ;
 * ;
 * 
 * LeafNodes # starts section defining other leaf nodes
 * 
 * name=netMgr type=controller ipAdr=192.168.8.2 fAdr=2.900
 * location=(42,-50);
 * name=comtCtl type=controller ipAdr=192.168.7.5 fAdr=1.902
 * location=(42,-40);
 * ;
 * 
 * Links # starts section that defines links
 * 
 * # name.link#:name.link# packetRate bitRate
 * link=(salt.1,kans.1) bitRate=20000 pktRate=40000 ;
 * link=(netMgr,kans.2) bitRate=3000 pktRate=5000 ;
 * link=(comtCtl,salt.2) bitRate=3000 pktRate=5000 ;
 * ;
 * 
 * Comtrees # starts section that defines comtrees
 * 
 * comtree=1001 root=salt core=kans # may have multiple core=x assignments
 * bitRateUp=5000 bitRateDown=10000 pktRateUp=3000 pktRateDown=6000
 * leafBitRateUp=100 leafBitRateDown=200 leafPktRateUp=50 leafPktRateDown=200
 * link=(salt.1,kans.1) link=(netMgr,kans.2) 	
 * ;
 * 
 * comtree=1002 root=kans core=salt 
 * bitRateUp=5000 bitRateDown=10000 pktRateUp=3000 pktRateDown=6000
 * leafBitRateUp=100 leafBitRateDown=200 leafPktRateUp=50 leafPktRateDown=200
 * link=(salt.1,kans.1) link=(comtCtl,salt.2) 	
 * ;
 * ; # end of comtrees section
 */

bool NetInfo::read(istream& in) {
	RtrNodeInfo cRtr;	// holds data for router being parsed
	LeafNodeInfo cLeaf;	// holds data for leaf being parsed
	LinkInfo cLink;		// hods data for link being parsed
	IfInfo iface[Forest::MAXINTF+1]; // holds data for router interfaces
	ComtreeInfo cComt;	// holds data comtree being parsed
	cComt.coreSet = cComt.linkSet = 0;

	int rtrNum = 1;		// rtrNum = i for i-th router in file
	int ifNum = 1;		// interface number for current interface
	int maxIfNum = 0;	// largest interface number specified
	int leafNum = 1;	// leafNum = i for i-th leaf in file 
	int linkNum = 1;	// linkNum = i for i-th link in file
	int comtNum = 1;	// comtNum = i for i-th comtree in file

	string leftName;	// name of a link's left end point
	string rightName;	// name of a link's right end point
	int linkLength = 0;	// length of a link

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
				if (nodeNumMap->find(cRtr.name)
				    != nodeNumMap->end()) {
					cerr << "NetInfo::read: attempting "
					     << "to re-define router "
					     << cRtr.name << endl;
					return false;
				}
				// get an unused router number
				int r = freeRouters->first();
				if (r == 0) {
					cerr << "NetInfo::read: too many "
						"routers" << endl;
					return false;
				}
				freeRouters->removeFirst();
				(*nodeNumMap)[cRtr.name] = r;

				rtr[r].name = cRtr.name;
				rtr[r].nType = cRtr.nType;
				rtr[r].fAdr = cRtr.fAdr;
				rtr[r].latitude = cRtr.latitude;
				rtr[r].longitude = cRtr.longitude;
				rtr[r].firstCliAdr = cRtr.firstCliAdr;
				rtr[r].lastCliAdr = cRtr.lastCliAdr;
				rtr[r].numIf = cRtr.numIf;
				rtr[r].iface = new IfInfo[cRtr.numIf+1];
				for (int i = 1; i <= cRtr.numIf; i++) {
					rtr[r].iface[i] = iface[i];
				}
				maxIfNum = 0;
				routers->push_back(r);
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
			for (int i = 1; i < Forest::MAXINTF; i++) {
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
				if (nodeNumMap->find(cLeaf.name)
				    != nodeNumMap->end()) {
					cerr << "NetInfo::read: attempting "
					     << "to re-define leaf "
					     << cLeaf.name << endl;
					return false;
				}
				// get unused leaf number
				int ln = freeLeaves->first();
				if (ln == 0) {
					cerr << "NetInfo::read: too many "
						"leaves" << endl;
					return false;
				}
				freeLeaves->removeFirst();
				(*nodeNumMap)[cLeaf.name] = ln + maxRtr;
				leaf[ln].name = cLeaf.name;
				leaf[ln].nType = cLeaf.nType;
				leaf[ln].ipAdr = cLeaf.ipAdr;
				leaf[ln].fAdr = cLeaf.fAdr;
				leaf[ln].latitude = cLeaf.latitude;
				leaf[ln].longitude = cLeaf.longitude;
				if (cLeaf.nType == CONTROLLER)
					controllers->push_back(leafNum+maxRtr);
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
				
				int u = (*nodeNumMap)[leftName];
				int v = (*nodeNumMap)[rightName];
				int lnk = netTopo->join(u,v,linkLength);
				if (lnk == 0) {
					cerr << "NetInfo::read: too many "
						"links" << endl;
					return false;
				}
				link[lnk].leftLnum  = cLink.leftLnum;
				link[lnk].rightLnum = cLink.rightLnum;
				if (getNodeType(u) == ROUTER) {
					if (!locLnk2lnk->insert(
					   ll2l_key(u,cLink.leftLnum),2*lnk)) {
						cerr << "NetInfo::read: attempt"
						     << " to re-use local link "
						     << " number for router "
						     << leftName << " in link "
						     << linkNum << endl;
						return false;
					}
				}
				if (getNodeType(v) == ROUTER) {
					if (!locLnk2lnk->insert(
					     ll2l_key(v,cLink.rightLnum),
						    2*lnk+1)) {
						cerr << "NetInfo::read: attempt"
						     << " to re-use local link "
						     << " number for router "
						     << rightName << " in link "
						     << linkNum << endl;
						return false;
					}
				}
				link[lnk].bitRate = cLink.bitRate;
				link[lnk].pktRate = cLink.pktRate;
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
			cComt.linkSet = new set<int>;
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
				int comt = freeComtIndices->first();
				if (comt == 0) {
					cerr << "NetInfo::read: too many "
						"comtrees" << endl;
					return false;
				}
				freeComtIndices->removeFirst();
				comtree[comt].comtreeNum = cComt.comtreeNum;
				comtree[comt].root = cComt.root;
				comtree[comt].bitRateDown = cComt.bitRateDown;
				comtree[comt].bitRateUp = cComt.bitRateUp;
				comtree[comt].pktRateDown = cComt.pktRateDown;
				comtree[comt].pktRateUp = cComt.pktRateUp;
				comtree[comt].leafBitRateDown =
							cComt.leafBitRateDown;
				comtree[comt].leafBitRateUp =
							cComt.leafBitRateUp;
				comtree[comt].leafPktRateDown =
							cComt.leafPktRateDown;
				comtree[comt].leafPktRateUp =
							cComt.leafPktRateUp;
				comtree[comt].coreSet = cComt.coreSet;
				comtree[comt].linkSet = cComt.linkSet;
				cComt.coreSet = cComt.linkSet = 0;
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
				cComt.linkSet->insert(ll);
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
	if (cComt.linkSet != 0) delete cComt.linkSet;
	return in.eof() && context == TOP;
}

void NetInfo::write(ostream& out) {
	out << "Routers\n\n";
	list<int>::iterator rp = routers->begin();
	while (rp != routers->end()) {
		int r = *rp; rp++;
		ntyp_t nt = getNodeType(r);
		string ntString; Forest::addNodeType2string(ntString, nt);
		out << "name=" << getNodeName(r) << " "
		    << "type=" << ntString;
		out  << " fAdr="; Forest::writeForestAdr(out,getNodeAdr(r));
		out << " clientAdrRange=(";
		Forest::writeForestAdr(out,getFirstCliAdr(r)); out << "-";
		Forest::writeForestAdr(out,getLastCliAdr(r)); out << ")";
		out << "\n\tlocation=(" << getNodeLat(r) << ","
		    		     << getNodeLong(r) << ")\n";
		out << "interfaces\n"
		    << "# iface#   ipAdr  linkRange  bitRate  pktRate\n";
		for (int i = 1; i <= getNumIf(r); i++) {
			if (!validIf(r,i)) continue;
			out << "   " << i << "  ";
			Np4d::writeIpAdr(out, getIfIpAdr(r,i)); 
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
	out << "LeafNodes\n\n";
	list<int>::iterator cp = controllers->begin();
	// print controllers first
	while (cp != controllers->end()) {
		int c = *cp; cp++;
		ntyp_t nt = getNodeType(c);
		string ntString; Forest::addNodeType2string(ntString, nt);
		out << "name=" << getNodeName(c) << " "
		    << "type=" << ntString << " "
		    << "ipAdr=";  Np4d::writeIpAdr(out,getLeafIpAdr(c));
		out  << " fAdr="; Forest::writeForestAdr(out,getNodeAdr(c));
		out << "\n\tlocation=(" << getNodeLat(c) << ","
		    		     << getNodeLong(c) << ");\n";
	}
	// then any other leaf nodes
	for (int c = maxRtr+1; c <= netTopo->n(); c++) {
		if (!isLeaf(c) || getNodeType(c) == CONTROLLER) continue;
		ntyp_t nt = getNodeType(c);
		string ntString; Forest::addNodeType2string(ntString, nt);
		out << "name=" << getNodeName(c) << " "
		    << "type=" << ntString << " "
		    << "ipAdr=";  Np4d::writeIpAdr(out,getLeafIpAdr(c));
		out  << " fAdr="; Forest::writeForestAdr(out,getNodeAdr(c));
		out << "\n\tlocation=(" << getNodeLat(c) << ","
		    		     << getNodeLong(c) << ");\n";
	}
	out << ";\n\n";
	out << "Links\n\n";
	for (int lnk = 1; lnk <= netTopo->m(); lnk++) {
		if (!validLink(lnk)) continue;
		int ln = getLinkL(lnk); int rn = getLinkR(lnk);
		out << "link=(" << getNodeName(ln);
		if (isRouter(ln)) out << "." << getLocLinkL(lnk);
		out << "," << getNodeName(rn);
		if (isRouter(rn)) out << "." << getLocLinkR(lnk);
		out << ") "
		    << "bitRate=" << getLinkBitRate(lnk) << " "
		    << "pktRate=" << getLinkPktRate(lnk) << " "
		    << "length=" << getLinkLength(lnk) << ";\n";
	}
	out << ";\n\n";
	out << "Comtrees\n\n";
	for (int comt = 1; comt <= maxComtree; comt++) {
		if (!validComtIndex(comt)) continue;

		out << "comtree=" << getComtree(comt)
		    << " root=" << getNodeName(getComtRoot(comt))
		    << "\nbitRateDown=" << getComtBrDown(comt)
		    << " bitRateUp=" << getComtBrUp(comt)
		    << " pktRateDown=" << getComtPrDown(comt)
		    << " pktRateUp=" << getComtPrUp(comt)
		    << "\nleafBitRateDown=" << getComtLeafBrDown(comt)
		    << " leafBitRateUp=" << getComtLeafBrUp(comt)
		    << " leafPktRateDown=" << getComtLeafPrDown(comt)
		    << " leafPktRateUp=" << getComtLeafPrUp(comt);

		// iterate through core nodes and print
		out << "\n";
		set<int>::iterator cp;
		for (cp = beginCore(comt); cp != endCore(comt); cp++) {
			if (*cp != getComtRoot(comt)) 
				out << "core=" << getNodeName(*cp) << " ";
		}
		out << "\n";
		// iterate through links and print
		set<int>::iterator lp;
		for (lp = beginComtLink(comt); lp != endComtLink(comt); lp++) {
			int left = getLinkL(*lp); int right = getLinkR(*lp);
			out << "link=(" << getNodeName(left);
			if (isRouter(left)) out << "." << getLocLinkL(*lp);
			out << "," << getNodeName(right);
			if (isRouter(right)) out << "." << getLocLinkR(*lp);
			out << ") ";
		}
		out << "\n;\n";
	}
	out << ";\n";
}
