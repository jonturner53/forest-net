/** \file RouterCore.h */

#ifndef ROUTERCORE_H
#define ROUTERCORE_H

#include <queue>

#include "stdinc.h"
#include "CommonDefs.h"

#include "LinkTable.h"
#include "ComtreeTable.h"
#include "RouteTable.h"
#include "PacketStore.h"
#include "CtlPkt.h"
#include "QuManager.h"
#include "IoProcessor.h"
#include "StatsModule.h"

class RouterCore {
public:
		RouterCore(fAdr_t);
		~RouterCore();

	bool	init(char*,char*,char*,char*,char*);
	void	run(uint32_t,int);
	void	dump(ostream& os);
private:
	fAdr_t	myAdr;			// forest address of this router
		
	int	nLnks;			// max number of links
	int	nComts;			// max number of comtrees
	int	nRts;			// max number of routes
	int	nPkts;			// number of packets
	int	nBufs;			// number of packet buffers
	int	nQus;			// number of queues

	uint32_t now;			// current time in 32 bit form

	LinkTable *lt;			// table defining links
	ComtreeTable *ctt;		// table of comtrees
	RouteTable  *rt;		// table of routes
	PacketStore *ps;		// packet buffers and headers
	QuManager *qm;			// collection of queues for all links
	IoProcessor *iop;		// class for handling io
	StatsModule *sm;		// class for recording statistics

	/** setup */
	void	addLocalRoutes();

	/** basic forwarding */
	void 	forward(int,int);
	bool	pktCheck(int,int);
	void	multiSend(int,int,int);

	/** inband control */
	void 	handleRteReply(int, int);
	void	sendRteReply(int,int);	
	void	subUnsub(int,int);

	/*** signalling packets */
	void	handleCtlPkt(int);
	void	errReply(packet,CtlPkt&,const char*);
	void	returnToSender(packet,int);
};

#endif
