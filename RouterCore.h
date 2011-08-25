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

struct ArgInfo {
        string  mode;

        fAdr_t  myAdr;
        ipa_t   myIp;
        fAdr_t  nmAdr;
        ipa_t   nmIp;
        fAdr_t  ccAdr;
        fAdr_t  firstLeafAdr;
        fAdr_t  lastLeafAdr;

        string  ifTbl;
        string  lnkTbl;
        string  comtTbl;
        string  rteTbl;
        string  statSpec;

        int     finTime;
};

class RouterCore {
public:
		RouterCore(const ArgInfo&);
		~RouterCore();

	bool	init(const ArgInfo&);
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
	void 	handleRteReply(int, int);
	void	sendRteReply(int,int);	
	void	subUnsub(int,int);

	// signalling packets
	void	handleCtlPkt(int);
	void	errReply(packet,CtlPkt&,const char*);
	void	returnToSender(packet,int);
};

inline fAdr_t RouterCore::allocLeafAdr() {
	int offset = leafAdr->firstOut();
	if (offset == 0) return 0;
	leafAdr->swap(offset);
	return firstLeafAdr + offset - 1;
}

inline bool RouterCore::allocLeafAdr(fAdr_t adr) {
	int offset = (adr - firstLeafAdr) + 1;
	if (!leafAdr->isOut(offset)) return false;
	leafAdr->swap(offset);
	return true;
}

inline void RouterCore::freeLeafAdr(fAdr_t adr) {
	int offset = (adr - firstLeafAdr) + 1;
	if (!leafAdr->isIn(offset)) return;
	leafAdr->swap(offset);
	return;
}


inline bool RouterCore::validLeafAdr(fAdr_t adr) const {
	int offset = (adr - firstLeafAdr) + 1;
	bool foo = leafAdr->isIn(offset);
	return foo;
}

inline bool RouterCore::isFreeLeafAdr(fAdr_t adr) const {
	int offset = (adr - firstLeafAdr) + 1;
	return leafAdr->isOut(offset);
}

#endif
