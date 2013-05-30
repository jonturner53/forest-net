/** @file BuildRtables.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include <string>
#include <queue>
#include "stdinc.h"
#include "Forest.h"
#include "NetInfo.h"
#include "ComtInfo.h"
#include "IfaceTable.h"
#include "QuManager.h"
#include "LinkTable.h"
#include "ComtreeTable.h"

using namespace forest;

/** usage:
 *       BuildRtables <netConfigFile 
 * 
 *  Build router configuration tables from a network
 *  configuration file. For each router named in the file
 *  build its interface table, its link table and its
 *  comtree table. For a router named foo, these tables
 *  will be written in files foo/ift, foo/lt and foo/ctt.
 */

bool buildIfaceTable(int, const NetInfo&, IfaceTable&);
bool buildLinkTable(int, const NetInfo&, LinkTable&);
bool buildComtTable(int, const NetInfo&, ComtInfo&, ComtreeTable&);

int main() {
	int maxNode = 100000; int maxLink = 10000;
	int maxRtr = 5000; int maxCtl = 200;
	int maxComtree = 10000;

        NetInfo net(maxNode, maxLink, maxRtr, maxCtl);
        ComtInfo comtrees(maxComtree,net);

	if (!net.read(cin) || !comtrees.read(cin)) {
		cerr << "buildTables: cannot read input\n";
		exit(1);
	}

	for (int r = net.firstRouter(); r != 0; r = net.nextRouter(r)) {
		string rName; net.getNodeName(r,rName);

		// build and write interface table for r
		IfaceTable *ifTbl = new IfaceTable(Forest::MAXINTF);
		if (!buildIfaceTable(r, net, *ifTbl)) {
			cerr << "buildTables: could not build interface table "
			     << "for router " << r << endl;
			exit(1);
		}
		string iftName = rName + "/ift";
		ofstream ifts; ifts.open(iftName.c_str());
		if (ifts.fail()) {
			cerr << "buildTables:: can't open interface table\n";
			exit(1);
		}
		string s;
		ifts << ifTbl->toString(s); ifts.close();

		// build and write link table for r
		LinkTable *lnkTbl = new LinkTable(Forest::MAXLNK);
		if (!buildLinkTable(r, net, *lnkTbl)) {
			cerr << "buildTables: could not build link table "
			     << "for router " << r << endl;
			exit(1);
		}
		string ltName = rName + "/lt";
		ofstream lts; lts.open(ltName.c_str());
		if (lts.fail()) {
			cerr << "buildTables:: can't open link table\n";
			exit(1);
		}
		lts << lnkTbl->toString(s); lts.close();

		// build and write comtree table for r
		ComtreeTable *comtTbl = new ComtreeTable(10*Forest::MAXLNK,
					20*Forest::MAXLNK,lnkTbl);
		if (!buildComtTable(r, net, comtrees, *comtTbl)) {
			cerr << "buildTables: could not build comtree table "
			     << "for router " << r << endl;
			exit(1);
		}
		string cttName = rName + "/ctt";
		ofstream ctts; ctts.open(cttName.c_str());
		if (ctts.fail()) {
			cerr << "buildTables:: can't open comtree table\n";
			exit(1);
		}
		ctts << comtTbl->toString(s); ctts.close();

		delete lnkTbl; delete comtTbl;
	}
}

bool buildIfaceTable(int r, const NetInfo& net, IfaceTable& ift) {
	for (int i = 1; i <= net.getNumIf(r); i++) {
		if (!net.validIf(r,i)) continue;
		RateSpec rs = net.getIfRates(r,i);
		ift.addEntry(i,	net.getIfIpAdr(r,i), net.getIfPort(r,i), rs);
	}
	return true;
}

bool buildLinkTable(int r, const NetInfo& net, LinkTable& lt) {

	for (int lnk = net.firstLinkAt(r); lnk != 0;
		 lnk = net.nextLinkAt(r,lnk)) {

		// find the interface that "owns" this link
		int iface = 0;
		int llnk = net.getLLnum(lnk,r);
		for (int i = 1; i <= net.getNumIf(r); i++) {
			if (!net.validIf(r,i)) continue;
			pair<int,int> leafRange; net.getIfLinks(r,i,leafRange);
			if (llnk >= leafRange.first &&
			    llnk <= leafRange.second) {
				iface = i; break;
			}
		}
		// find peer and interface for link at peer
		int peer = net.getPeer(r,lnk);
		int peerIface = 0;
		int plnk = net.getLLnum(lnk,peer);
		for (int i = 1; i <= net.getNumIf(peer); i++) {
			if (!net.validIf(peer,i)) continue;
			pair<int,int> leafRange;
			net.getIfLinks(peer,i,leafRange);
			if (plnk >= leafRange.first &&
			    plnk <= leafRange.second) {
				peerIface = i; break;
			}
		}
		ipa_t peerIp = (net.getNodeType(peer) == Forest::ROUTER ?
				net.getIfIpAdr(peer,peerIface) :
				net.getLeafIpAdr(peer));
		ipp_t peerPort = (net.getNodeType(peer) == Forest::ROUTER ?
				  Forest::ROUTER_PORT : 0);
		lt.addEntry(llnk, peerIp, peerPort, 0);
		lt.setIface(llnk,iface);
		lt.setPeerType(llnk,net.getNodeType(peer));
		lt.setPeerAdr(llnk,net.getNodeAdr(peer));
		lt.getRates(llnk) = net.getLinkRates(lnk);
	}

	return true;
}

/** Find the link to the parent of a node in a comtree.
 *  Link to parent is found by doing a tree-traversal from the comtree root.
 *  @param r is the node whose parent link we're looking for
 *  @param ctx is the index of the comtree of interest in the NetInfo object
 *  @param net is the NetInfo object
 *  @return the link number of the link connecting r to its parent,
 *  or 0 if r is the root, or -1 on an error
 */
int findParentLink(int r, int ctx, const NetInfo& net,
				   const ComtInfo& comtrees) {
	int ctRoot = comtrees.getRoot(ctx);
	if (r == ctRoot) return 0;

	queue<int> pending; pending.push(ctRoot);

	int plink[net.getMaxNode()];
	for (int i = 1; i <= net.getMaxNode(); i++) plink[i] = 0;

	while (!pending.empty()) {
		vertex u = pending.front(); pending.pop();
		for (edge e = net.firstLinkAt(u); e != 0;
		 	  e = net.nextLinkAt(u,e)) {
			if (!comtrees.isComtLink(ctx,e) || e == plink[u]) continue;
			vertex v = net.getPeer(u,e);
			if (plink[v] != 0) {
				cerr << "findParentLink: found cycle in "
				     << "comtree " << ctx << endl;
				return -1;
			}
			if (v == r) return e;
			pending.push(v); plink[v] = e;
		}
	}
	cerr << "findParentLink: could not find target node " << r 
	     << " in comtree "  << comtrees.getComtree(ctx) << endl;
	return -1;
}

bool buildComtTable(int r, const NetInfo& net, ComtInfo& comtrees,
			   ComtreeTable& comtTbl) {

	// find the subset of comtrees involving this router
	set<int> comtreeSet;
	for (int ctx = comtrees.firstComtree(); ctx != 0;
	         ctx = comtrees.nextComtree(ctx)) {
		fAdr_t radr = net.getNodeAdr(r);
		if (comtrees.isComtRtr(ctx,radr))
			comtreeSet.insert(ctx);
	}

	set<int>::iterator p;
	for (p = comtreeSet.begin(); p != comtreeSet.end(); p++) {
		int ctx = *p;
		int ctte = comtTbl.addEntry(comtrees.getComtree(ctx));
		if (ctte == 0) {
			cerr << "buildComtTable: detected inconsistency "
			     << "while building comtree table for router "
			     << r << " comtree " << comtrees.getComtree(ctx)
			     << endl;
			return false;
		}

		comtTbl.setCoreFlag(ctte,comtrees.isCoreNode(ctx,r));

		// add all comtree links incident to r and to the table entry
		for (int lnk = net.firstLinkAt(r); lnk != 0;
			 lnk = net.nextLinkAt(r,lnk)) {
			if (!comtrees.isComtLink(ctx,lnk)) continue;
			int llnk = net.getLLnum(lnk,r);
			int peer = net.getPeer(r,lnk);
			comtTbl.addLink(ctte,llnk,net.isRouter(peer),
					comtrees.isCoreNode(ctx,peer));
		}

		// find parent link by doing a tree-traversal from root
		//int plink = findParentLink(r,ctx,net,comtrees);
		//if (plink < 0) return false;
		int plink = comtrees.getPlink(ctx,r);
		comtTbl.setPlink(ctte,net.getLLnum(plink,r));
	} 
	return true;
}

