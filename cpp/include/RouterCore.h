/** @file RouterCore.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef ROUTERCORE_H
#define ROUTERCORE_H

#include <queue>
#include <map>

#include "stdinc.h"
#include "Forest.h"

#include "IfaceTable.h"
#include "LinkTable.h"
#include "ComtreeTable.h"
#include "RouteTable.h"
#include "PacketStore.h"
#include "CtlPkt.h"
#include "QuManager.h"
#include "IoProcessor.h"
#include "StatsModule.h"
#include "PacketLog.h"

namespace forest {

/** Structure used to carry information about a router. */
struct RouterInfo {
        string  mode; 		///< router operation mode (local or remote)

        fAdr_t  myAdr; 		///< forest address of the router
        ipa_t   bootIp; 	///< IP address used for booting
        ipp_t   portNum; 	///< port # for all interfaces (default=0)
        fAdr_t  nmAdr; 		///< forest address of the network manager
        ipa_t   nmIp; 		///< IP address of the network manager
        fAdr_t  ccAdr; 		///< forest address of the comtree controller
        fAdr_t  firstLeafAdr; 	///< first assignable leaf address
        fAdr_t  lastLeafAdr; 	///< last assignable leaf address

        string  ifTbl; 		///< name of interface table file
        string  lnkTbl; 	///< name of link table file
        string  comtTbl; 	///< name of comtree table file
        string  rteTbl; 	///< name of route table file
        string  statSpec; 	///< name of statistics specification file

        int     finTime; 	///< number of seconds for router to run
};

class RouterCore {
public:
		RouterCore(bool, const RouterInfo&);
		~RouterCore();

	bool	readTables(const RouterInfo&);
	bool	setup();
	bool	setBooting(bool state) { return booting = state; }
	void	run(uint64_t);
	void	dump(ostream& os);
private:
	ipa_t	bootIp;			///< IP address used during booting
	fAdr_t	myAdr;			///< forest address of this router
	ipa_t	nmIp;			///< IP address of the netMgr
	fAdr_t	nmAdr;			///< forest address of the netMgr
	fAdr_t	ccAdr;			///< address of comtree controller

	bool 	booting;		///< true when booting remotely

	uint64_t now;			///< current time in 64 bit form

	uint64_t seqNum;		///< seq # for outgoing control packets

	fAdr_t	firstLeafAdr;		///< first leaf address
	UiSetPair *leafAdr;		///< offsets for in-use and free leaf
					///< addresses

	struct ControlInfo {		///< info on outgoing control packets
	pktx	px;			///< packet number of retained copy
	int	nSent;			///< number of times we've sent packet
	int	lnk;			///< out link (if 0, use forward())
	uint64_t timestamp;		///< time when we last sent a request
	};
	map<uint64_t,ControlInfo> *pending; ///< map of pending requests

	int	nIfaces;		///< max number of interfaces
	int	nLnks;			///< max number of links
	int	nComts;			///< max number of comtrees
	int	nRts;			///< max number of routes
	int	nPkts;			///< number of packets
	int	nBufs;			///< number of packet buffers
	int	nQus;			///< number of queues

	IfaceTable *ift;		///< table defining interfaces
	LinkTable *lt;			///< table defining links
	ComtreeTable *ctt;		///< table of comtrees
	RouteTable  *rt;		///< table of routes
	PacketStore *ps;		///< packet buffers and headers
	QuManager *qm;			///< collection of queues for all links
	IoProcessor *iop;		///< class for handling io
	StatsModule *sm;		///< class for recording statistics
	PacketLog *pktLog;		///< log for recording sample of packets

	/** structure for tracking pending clients */
	struct nuClient {
	uint64_t nonce;			///< secret nonce to identify client
	time_t	timestamp;		///< in seconds, discard after 30
	int	iface;			///< iface where client connects
	int	link;			///< pre-assigned link number
	};
	list<nuClient> *pendingClients; ///< list of clients that have not
					///< yet connected

	// setup 
	bool	setupIfaces();
	bool	setupLeafAddresses();
	bool	setupQueues();
	bool	checkTables();
	bool	setAvailRates();
	void	addLocalRoutes();

	fAdr_t	allocLeafAdr();
	bool	allocLeafAdr(fAdr_t);
	void	freeLeafAdr(fAdr_t);
	bool	validLeafAdr(fAdr_t) const;
	bool	isFreeLeafAdr(fAdr_t) const;

	// basic forwarding 
	void 	forward(pktx,int);
	bool	pktCheck(pktx,int);
	void	multiSend(pktx,int,int);

	// inband control
	void 	handleConnDisc(pktx);
	void 	handleRteReply(pktx, int);
	void	sendRteReply(pktx,int);	
	void	subUnsub(pktx,int);

	// signalling packets
	void	handleCtlPkt(pktx);

	bool	addIface(CtlPkt&, CtlPkt&);
	bool	dropIface(CtlPkt&, CtlPkt&);
	bool	getIface(CtlPkt&, CtlPkt&);
	bool	modIface(CtlPkt&, CtlPkt&);
	bool 	getIfaceSet(CtlPkt&, CtlPkt&);

	bool	addLink(CtlPkt&, CtlPkt&);
	bool	dropLink(CtlPkt&, CtlPkt&);
	void	dropLink(int,fAdr_t=0);
	bool	getLink(CtlPkt&, CtlPkt&);
	bool	modLink(CtlPkt&, CtlPkt&);
	bool	getLinkSet(CtlPkt&, CtlPkt&);

	bool	addComtree(CtlPkt&, CtlPkt&);
	bool	dropComtree(CtlPkt&, CtlPkt&);
	bool	getComtree(CtlPkt&, CtlPkt&);
	bool	modComtree(CtlPkt&, CtlPkt&);
	bool 	getComtreeSet(CtlPkt&, CtlPkt&);

	bool	addComtreeLink(CtlPkt&, CtlPkt&);
	bool	dropComtreeLink(CtlPkt&, CtlPkt&);
	void	dropComtreeLink(int, int, int);
	bool	modComtreeLink(CtlPkt&, CtlPkt&);
	bool	getComtreeLink(CtlPkt&, CtlPkt&);

	bool	addRoute(CtlPkt&, CtlPkt&);
	bool	dropRoute(CtlPkt&, CtlPkt&);
	bool	getRoute(CtlPkt&, CtlPkt&);
	bool	modRoute(CtlPkt&, CtlPkt&);
	bool 	getRouteSet(CtlPkt&, CtlPkt&);

	bool	addFilter(CtlPkt&, CtlPkt&);
	bool	dropFilter(CtlPkt&, CtlPkt&);
	bool	getFilter(CtlPkt&, CtlPkt&);
	bool	modFilter(CtlPkt&, CtlPkt&);
	bool	getFilterSet(CtlPkt&, CtlPkt&);
	bool	getLoggedPackets(CtlPkt&, CtlPkt&);
	bool	enablePacketLog(CtlPkt&, CtlPkt&);

	bool	setLeafRange(CtlPkt&, CtlPkt&);

	void	sendConnDisc(int,Forest::ptyp_t);
	bool	sendCpReq(CtlPkt&, fAdr_t);
	bool	sendControl(pktx,uint64_t,int);
	void	resendControl();
	void	handleControlReply(pktx);

	void	returnToSender(pktx,CtlPkt&);
};

/** Allocate a new leaf address.
 *  @return a previously unused address in the range of assignable
 *  leaf addresses for this router, or 0 if all addresses are in use
 */
inline fAdr_t RouterCore::allocLeafAdr() {
	int offset = leafAdr->firstOut();
	if (offset == 0) return 0;
	leafAdr->swap(offset);
	return firstLeafAdr + offset - 1;
}

/** Allocate a specified leaf address.
 *  @param adr is an address in the range of assignable addresses
 *  @return true of the adr is available for use and was successfully
 *  allocated, else false
 */
inline bool RouterCore::allocLeafAdr(fAdr_t adr) {
	int offset = (adr - firstLeafAdr) + 1;
	if (!leafAdr->isOut(offset)) return false;
	leafAdr->swap(offset);
	return true;
}

/** De-allocate a leaf address.
 *  @param adr is a previously allocated address.
 */
inline void RouterCore::freeLeafAdr(fAdr_t adr) {
	int offset = (adr - firstLeafAdr) + 1;
	if (!leafAdr->isIn(offset)) return;
	leafAdr->swap(offset);
	return;
}

/** Determine if a given address is currently assigned.
 *  @param adr is a forest unicast address
 *  @return true if adr is one that had been previously assigned
 */
inline bool RouterCore::validLeafAdr(fAdr_t adr) const {
	int offset = (adr - firstLeafAdr) + 1;
	return leafAdr->isIn(offset);
}

/** Determine if a given address is currently unassigned.
 *  @param adr is a forest unicast address
 *  @return true if adr is not currently assigned.
 */
inline bool RouterCore::isFreeLeafAdr(fAdr_t adr) const {
	int offset = (adr - firstLeafAdr) + 1;
	return leafAdr->isOut(offset);
}

} // ends namespace

#endif
