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
		 int maxRtr1, int maxCtl1 )
		 : maxNode(maxNode1), maxLink(maxLink1),
		   maxRtr(maxRtr1), maxCtl(maxCtl1) {
	maxLeaf = maxNode-maxRtr;
	netTopo = new Graph(maxNode, maxLink);
	leaf = new LeafNodeInfo[maxLeaf+1];
	rtr = new RtrNodeInfo[maxRtr+1];
	link = new LinkInfo[maxLink+1];
	freeNodes = new UiDlist(maxNode);
	locLnk2lnk = new UiHashTbl(maxLink);
	nodeNumMap = new map<string,int>;
	for (int i = 1; i <= maxNode; i++) freeNodes->insert(i,0);
	freeLinks = new UiDlist(maxLink);
	for (int i = 1; i <= maxLink; i++) freeLinks->insert(i,0);
	routers = new list<int>();
	controllers = new list<int>();
}

NetInfo::~NetInfo() {
	delete netTopo; delete leaf; delete rtr; delete link;
	delete freeNodes; delete freeLinks;
	delete locLnk2lnk; delete nodeNumMap;
	delete routers; delete controllers;
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
 * 
 * LeafNodes # starts section defining other leaf nodes
 * 
 * # name nodeType ipAdr forestAdr x-coord y-coord
 * name=netMgr type=controller ipAdr=192.168.8.2 fAdr=2.900
 * location=(42,-50) bitRate=1000 pktRate=1000 ;
 * name=comtCtl type=controller ipAdr=192.168.7.5 fAdr=1.902
 * location=(42,-40) bitRate=1000 pktRate=1000 ;
 * 
 * Links # starts section that defines links
 * 
 * # name.link#:name.link# packetRate bitRate
 * link=(salt.1,kans.1) bitRate=20000 pktRate=40000 ;
 * link=(netMgr,kans.2) bitRate=3000 pktRate=5000 ;
 * link=(comtCtl,salt.2) bitRate=3000 pktRate=5000 ;
 */

bool NetInfo::read(istream& in) {
	RtrNodeInfo cRtr;	// holds data for router being parsed
	LeafNodeInfo cLeaf;	// holds data for leaf being parsed
	LinkInfo cLink;		// hods data for link being parsed
	IfInfo iface[Forest::MAXINTF+1]; // holds data for router interfaces

	int rtrNum = 1;		// node number of "current" router
	int ifNum = 1;		// interface number for current interface
	int maxIfNum = 0;	// largest interface number specified
	int leafNum = 1;	// number of current leaf (NOT its node number)
	int linkNum = 1;	// number of current link

	string leftName;	// name of a link's left end point
	string rightName;	// name of a link's right end point
	int linkLength = 0;	// length of a link

	enum ParseContexts {
		TOP, ROUTER_SEC, ROUTER_CTXT , IFACES, IFACES_ENTRY,
		LEAF_SEC, LEAF, LINK_SEC, LINK
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
				(*nodeNumMap)[cRtr.name] = rtrNum;

				rtr[rtrNum].name = cRtr.name;
				rtr[rtrNum].nType = cRtr.nType;
				rtr[rtrNum].fAdr = cRtr.fAdr;
				rtr[rtrNum].latitude = cRtr.latitude;
				rtr[rtrNum].longitude = cRtr.longitude;
				rtr[rtrNum].firstCliAdr = cRtr.firstCliAdr;
				rtr[rtrNum].lastCliAdr = cRtr.lastCliAdr;
				rtr[rtrNum].numIf = cRtr.numIf;
				rtr[rtrNum].iface = new IfInfo[cRtr.numIf+1];
				for (int i = 1; i <= cRtr.numIf; i++) {
					rtr[rtrNum].iface[i] = iface[i];
				}
				maxIfNum = 0;
				routers->push_back(rtrNum);
				freeNodes->remove(rtrNum);
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
			cLeaf.bitRate = cLeaf.pktRate = 0;
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
				if (cLeaf.bitRate == 0) {
					cerr << "NetInfo::read: no bit rate "
					     << "defined for leaf node "
					     << cLeaf.name << endl;
					return false;
				}
				if (cLeaf.pktRate == 0) {
					cerr << "NetInfo::read: no packet rate "
					     << "defined for leaf node "
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
				(*nodeNumMap)[cLeaf.name] = leafNum + maxRtr;
				leaf[leafNum].name = cLeaf.name;
				leaf[leafNum].nType = cLeaf.nType;
				leaf[leafNum].ipAdr = cLeaf.ipAdr;
				leaf[leafNum].fAdr = cLeaf.fAdr;
				leaf[leafNum].latitude = cLeaf.latitude;
				leaf[leafNum].longitude = cLeaf.longitude;
				leaf[leafNum].bitRate = cLeaf.bitRate;
				leaf[leafNum].pktRate = cLeaf.pktRate;
				if (cLeaf.nType == CONTROLLER)
					controllers->push_back(leafNum+maxRtr);
				freeNodes->remove(leafNum+maxRtr);
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
			} else if (s.compare("bitRate") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cLeaf.bitRate)) {
					cerr << "NetInfo::read: can't read "
					     << "bit rate for router"
					     << " number " << leafNum << endl;
					return false;
				}
			} else if (s.compare("pktRate") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Misc::readNum(in,cLeaf.pktRate)) {
					cerr << "NetInfo::read: can't read "
					     << "packet rate for router"
					     << " number " << leafNum << endl;
					return false;
				}
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
				link[lnk].leftLnum  = cLink.leftLnum;
				link[lnk].rightLnum = cLink.rightLnum;
				if (getNodeType(u) == ROUTER) {
					if (!locLnk2lnk->insert(
					     ll2l_key(u,cLink.leftLnum),lnk)) {
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
					     ll2l_key(u,cLink.rightLnum),lnk)) {
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
				freeLinks->remove(lnk);
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
		default: fatal("NetInfo::read: undefined context");
		}
	}
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
		    << "nodeType=" << ntString;
		out  << " fAdr="; Forest::writeForestAdr(out,getNodeAdr(r));
		out << "\n\tlocation=(" << getNodeLat(r) << ","
		    		     << getNodeLong(r) << ") "
		    << "CliAdrRange=(";
		Forest::writeForestAdr(out,getFirstCliAdr(r)); out << "-";
		Forest::writeForestAdr(out,getLastCliAdr(r)); out << ")\n";
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
		    << "nodeType=" << ntString << " "
		    << "ipAdr=";  Np4d::writeIpAdr(out,getLeafIpAdr(c));
		out  << " fAdr="; Forest::writeForestAdr(out,getNodeAdr(c));
		out << "\n\tlocation=(" << getNodeLat(c) << ","
		    		     << getNodeLong(c) << ") "
		    << "bitRate=" << getLeafBitRate(c) << " "
		    << "pktRate=" << getLeafPktRate(c) << ";\n";
	}
	// then any other leaf nodes
	for (int c = maxRtr+1; c <= netTopo->n(); c++) {
		if (!isLeaf(c) || getNodeType(c) == CONTROLLER) continue;
		ntyp_t nt = getNodeType(c);
		string ntString; Forest::addNodeType2string(ntString, nt);
		out << "name=" << getNodeName(c) << " "
		    << "nodeType=" << ntString << " "
		    << "ipAdr=";  Np4d::writeIpAdr(out,getLeafIpAdr(c));
		out  << " fAdr="; Forest::writeForestAdr(out,getNodeAdr(c));
		out << "\n\tlocation=(" << getNodeLat(c) << ","
		    		     << getNodeLong(c) << ") "
		    << "bitRate=" << getLeafBitRate(c) << " "
		    << "pktRate=" << getLeafPktRate(c) << ";\n";
	}
	out << ";\n\n";
	out << "Links\n\n";
	for (int lnk = 1; lnk <= netTopo->m(); lnk++) {
		if (!validLink(lnk)) continue;
		int ln = getLnkL(lnk); int rn = getLnkR(lnk);
		out << "link=(" << getNodeName(ln);
		if (isRouter(ln)) out << "." << getLocLnkL(ln);
		out << "," << getNodeName(rn);
		if (isRouter(rn)) out << "." << getLocLnkR(rn);
		out << ") "
		    << "bitRate=" << getLinkBitRate(lnk) << " "
		    << "pktRate=" << getLinkPktRate(lnk) << " "
		    << "length=" << getLinkLength(lnk) << ";\n";
	}
	out << ";\n";
}
