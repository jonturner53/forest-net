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

#include "stdinc.h"
#include "CommonDefs.h"

#include "IfaceTable.h"
#include "LinkTable.h"
#include "ComtreeTable.h"
#include "RouteTable.h"
#include "PacketStore.h"
#include "CtlPkt.h"
#include "QuManager.h"
#include "IoProcessor.h"
#include "StatsModule.h"

/** Structure used to carry information about a router. */
struct RouterInfo {
        string  mode; 		///< router operation mode (local or remote)

        fAdr_t  myAdr; 		///< forest address of the router
        ipa_t   myIp; 		///< IP address of the router 
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
		RouterCore(const RouterInfo&);
		~RouterCore();

	bool	init(const RouterInfo&);
	void	run(uint64_t);
	void	dump(ostream& os);
private:
	ipa_t	myIp;			///< IP address of this router
	fAdr_t	myAdr;			///< forest address of this router
	ipa_t	nmIp;			///< IP address of the netMgr
	fAdr_t	nmAdr;			///< forest address of the netMgr
	fAdr_t	ccAdr;			///< address of comtree controller

	uint64_t now;			///< current time in 64 bit form

	fAdr_t	firstLeafAdr;		///< first leaf address
	UiSetPair *leafAdr;		///< offsets for in-use and free leaf
					///< addresses

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
	void 	forward(int,int);
	bool	pktCheck(int,int);
	void	multiSend(int,int,int);

	// inband control
	void 	handleConnDisc(int);
	void 	handleRteReply(int, int);
	void	sendRteReply(int,int);	
	void	subUnsub(int,int);

	// signalling packets
	void	handleCtlPkt(int);
	void	errReply(packet,CtlPkt&,const char*);
	void	returnToSender(packet,int);
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

#endif
