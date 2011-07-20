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
		 int maxRtr1, int maxCtl1, int maxComt1)
		 : maxNode(maxNode1), maxLink(maxLink1),
		   maxRtr(maxRtr1), maxCtl(maxCtl1), maxComt(maxComt1) {
	netTopo = new Wgraph(maxNode, maxLink);
	node = new nodeInfo[maxNode+1];
	link = new linkInfo[maxLink+1];
	freeNodes = new UiDlist(maxNode);
	freeLinks = new UiDlist(maxLink);
	locLnk2lnk = new UiHashTbl(maxLink);
	routers = new list<int>(maxRtr);
	controllers = new list<int>(maxCtl);
}

NetInfo::~NetInfo() {
	delete netTopo; delete node; delete link;
	delete freeNodes; delete freeLinks;
	delete locLnk2lnk;
	delete routers; delete controllers;
}

bool NetInfo::read(istream& in) {
	RtrNodeInfo cRtr;	// holds data for router being parsed
	LeafNodeInfo cLeaf;	// holds data for leaf being parsed
	LinkInfo cLink;		// hods data for link being parsed
	IfInfo iface[Forest::MAXINTF+1]; // holds data for router interfaces

	int rtrNum = 1;		// node number of next router
	int ifNum = 1;		// interface number for next interface
	int leafNum = maxRtr+1;	// node number of next leaf
	int lnkNum = 1;		// edge number of next link

	enum ParseContexts {
		TOP, ROUTER_SEC, ROUTER, IFACES, IFACES_ENTRY,
		LEAF_SEC, LEAF, LINK_SEC, LINK
	} contexts;
	
	string s;
	context = TOP;
	while (!in.eof()) {
		skipBlank(in);
		s.clear();
		switch (context) {
		case TOP:
			if (!readWord(in, s)) { return false; }
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
			// clear the current router structure
			cRtr.name = ""; cRtr.nType = UNDEF_NODE; cRtr.fAdr = 0;
			cRtr.latitude = 91000000; cRtr.longitude = 361000000; 
			cRtr.firstFadr = cRtr.lastFadr = 0;
			cRtr.numIf = 0; cRtr.iface = 0;
			context = ROUTER;
			// falling through to ROUTER context
		case ROUTER:
			if (Misc::verify(in,';')) {
				// means we've read in a complete router
				// definition
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
				if (!Forest::validUcastAdr(cRtr.fadr)) {
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
				    !Forest::validUcastAdr(cRtr.lastCliAdr)) {
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
				rtr[rtrNum].name = cRtr.name;
				rtr[rtrNum].nType = cRtr.nType;
				rtr[rtrNum].fAdr = cRtr.fAdr;
				rtr[rtrNum].latitude = cRtr.latitude;
				rtr[rtrNum].longitude = cRtr.longitude;
				rtr[rtrNum].firstCliAdr = cRtr.firstCliAdr;
				rtr[rtrNum].lastCliAdr = cRtr.lastCliAdr;
				rtr[rtrNum].numIf = cRtr.numIf;
				rtr[rtrNum].iface = new IfInfo[ctr.numIf+1];
				for (int i = 1; i <= numIf; i++) {
					rtr[rtrNum].iface[i] = iface[i];
				}
				context = ROUTER_SEC; break;
			}
			if (!readWord(in, s)) {
				cerr << "NetInfo::read: syntax error when "
				     << "reading router number " << rtrNum
				     << endl;
				return false;
			}
			if (s.compare("name") == 0 && Misc::verify(in,'=')) {
				if (!readWord(in,s)) {
					cerr << "NetInfo::read: can't read name"
					     << " for router number " << rtrNum
					     << endl;
					return false;
				}
				cRtr.name = s;
			} else if (s.compare("type") == 0 &&
				   Misc::verify(in,'=')) {
				if (!readWord(in,s)) {
					cerr << "NetInfo::read: can't read type"
					     << " for router number " << rtrNum
					     << endl;
					return false;
				}
				cRtr.nType = Forest:getNodeType(in,s));
			} else if (s.compare("ipAdr") == 0 &&
				   Misc::verify(in,'=')) {
				if (!Np4d::readIpAdr(in,cRtr.ipAdr)) {
					cerr << "NetInfo::read: can't read ip"
					     << " address for router number " 
					     << rtrNum << endl;
					return false;
				}
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
			if next thing is a semicolon, check the current iface
			   and check for an "end"
			break;
		case LEAF_SEC:
			break;
		case LEAF:
			break;
		case LINK_SEC:
			break;
		case LINK:
			break;
		default: fatal("NetInfo::read: undefined context");
		}
	}
	return !in.fail();
}

bool NetInfo::readRtrSec(instream& in) {
	while (not yet done) {
		readRtr();
	}
}

bool NetInfo::readRtr(instream& in) {
	// get name, IP address, Forest address, coordinates, address range
	// while there is an interface left to read
		// read iface#, IP addr, bitRate, pktRate, link range
}

bool NetInfo::readTok(instream& in, enum tokenType, string& str, int val) {
}

/* Format of NetInfo file.

General structure:
series of labeled sections that start with a keyword
entity descriptions include attribute/value pairs
may also include tables - start with table, end with eltab
; used to terminate table entries and entity descriptions
Lines may have comments that start with # and continue to end of line

Routers: # starts section that defines nodes

# name nodeType forestAdr x-coord y-coord
name=rtr1 type=router ipAdr=2.3.4.5 fAdr=1.1000
location=(50,22) fAdrRange=(1.1-1.200)

table # router interfaces:
# ifaceNum ifaceIp      ifaceLinks  bitRate  pktRate
     1     193.168.3.4  1-5	    50000    25000
     2     193.168.3.4  6-10        40000    20000
elbat
;

LeafNodes

# name nodeType ipAdr forestAdr x-coord y-coord
name=netMgr type=controller ipAdr=192.168.3.2 fAdr=2.900
location=(30,-20) bitRate=1000 pktRat=1000 ;
name=comtMgr type=controller ipAdr=192.168.3.5 fAdr=2.902
location=(30,-25) bitRate=1000 pktRat=1000 ;

Links # starts section that defines links

# name.link#:name.link# packetRate bitRate
link=(rtr1.1,rtr3.3) bitRate=20000 pktRate=40000 ;
link=(rtr1.2,rtr2.1) bitRate=30000 pktRate=50000 ;

*/

void NetInfo::write(ostream& out) const {
	out << "Routers\n\n";
	list<int>::iterator rp = routers.begin();
	while (rp != routers.end()) {
		int r = *rp; rp++;
		ntype_t nt = getNodeType(r);
		string ntString; addNodeType2string(ntString, nt);
		out << "name=" << getNodeName(r) << " "
		    << "nodeType=" << ntString << " "
		    << "ipAdr=";  Np4d::writeIpAdr(out,getNodeIp(r));
		out  << " fAdr="; Forest::writeForestAdr(out,getNodeAdr(r));
		out << " location=(" << getNodeLat(r) << ","
		    << "location=(" << getNodeLat(r) << ","
		    		    << getNodeLong(r) << ") "
		    << "fAdrRange=(" << getFirstCliAdr(r) << "-"
		    		     << getLastCliAdr(r) << ")\n";
		out << "# iface table\n"
		    << "# iface#   ipAdr  linkRange  bitRate  pktRate\n";
		for (int i = 1; i <= getNumIf(r); i++) {
			if (!validIf(r,i)) continue;
			out << "   " << i << "  ";
			Np4d::writeIpAdr(out, getIfIpAdr(r,i)); 
			if (getIfFirstLink(r,i) == getIfLastLink(r,i)) 
				out << " " << getIfFirstLink(r,i) << " ";
			else
				out << " " << getIfFirstLink(r,i) << "-" <<
				    << getIfLastLink(r,i) << "  ";
			out << getIfBitRate(r,i) << "  " <<
			    << getIfPktRate(r,i) << ";\n";
		}
		out << "elbat\n;";
	}
	out << "\n\n";
	out << "LeafNodes\n\n";
	list<int>::iterator cp = controllers.begin();
	// print controllers first
	while (cp != controllers.end()) {
		int c = *cp; cp++;
		ntype_t nt = getNodeType(c);
		string ntString; addNodeType2string(ntString, nt);
		out << "name=" << getNodeName(c) << " "
		    << "nodeType=" << ntString << " "
		    << "ipAdr=";  Np4d::writeIpAdr(out,getNodeIp(c));
		out  << " fAdr="; Forest::writeForestAdr(out,getNodeAdr(r));
		out << " location=(" << getNodeLat(c) << ","
		    		     << getNodeLong(c) << ") "
		    << "bitRate=" << getLeafBitRate(c) << " "
		    << "pktRate=" << getLeafPktRate(c) << ";\n"
	}
	// then any other leaf nodes
	for (int c = maxRtr+1; c <= netTopo.n(); c++) {
		if (!isLeaf(c) || getNodeType(c) == CONTROLLER) continue;
		ntype_t nt = getNodeType(c);
		string ntString; addNodeType2string(ntString, nt);
		out << "name=" << getNodeName(c) << " "
		    << "nodeType=" << ntString << " "
		    << "ipAdr=";  Np4d::writeIpAdr(out,getNodeIp(c));
		out  << " fAdr="; Forest::writeForestAdr(out,getNodeAdr(c));
		out << " location=(" << getNodeLat(c) << ","
		    		     << getNodeLong(c) << ") "
		    << "bitRate=" << getLeafBitRate(c) << " "
		    << "pktRate=" << getLeafPktRate(c) << ";\n"
	}
	out << "\n\n";
	out << "Links\n\n"
	for (int lnk = 1; lnk <= netTopo.m(); lnk++) {
		if (!validLink(lnk)) continue;
		int ln = getLnkL(lnk); int rn = getLnkR(lnk);
		out << "link=(" << getNodeName(ln) << "." << getLocLnkL(ln)
		    << "," << getNodeName(rn) << "." << getLocLnkR(ln) << ") "
		    << "bitRate=" << getLnkBitRate(ln) << " "
		    << "pktRate=" << getLnkPktRate(ln) << ";"
	}
	out << "\n\n";
}
