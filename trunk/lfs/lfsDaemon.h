// Header file for lfsDaemon class. This is used in the version of
// LFS that uses the SPP fastpath

#ifndef LFSDAEMON_H
#define LFSDAEMON_H

#include "stdinc.h"
#include "lfs.h"

#include "lnkTbl.h"
#include "fltrTbl.h"
#include "rteTbl.h"
#include "pktStore.h"
#include "packet.h"
#include "qMgr.h"
#include "ioProc.h"
#include "statsMod.h"

class lfsDaemon {
public:
		lfsDaemon(ipa_t);
		~lfsDaemon();
	bool	init(char*,char*,char*,char*,char*);
					// initialize tables from files
	void	dump(ostream& os); 	// print the tables
private:
	ipa_t	myAdr;			// LFS address of this router
		
	int	nLnks;			// max number of links
	int	nFltrs;			// max number of filters
	int	nRts;			// max number of routes
	int	nPkts;			// number of packets
	int	nBufs;			// number of packet buffers
	int	nQus;			// number of queues
	int	qSiz;			// number of packets per queue

	uint32_t now;			// current time in 32 bit form

	lnkTbl 	*lt;			// table defining links
	fltrTbl *ft;			// table of filters
	rteTbl  *rt;			// table of routes
	pktStore *ps;			// packet buffers and headers
	qMgr 	*qm;			// collection of queues for all links
	ioProc 	*iop;			// class for handling io
	statsMod *sm;			// class for recording statistics
	int	*avail;			// avail[i]=available bw for link i

	void	addInterfacesBypass();	// add fastpath interfaces and configure bypass filters
	void	addQueues();		// add datagram queues
	void	addRoutesFilters();	// add routes and filters
	bool	pktCheck(packet*);	// perform basic checks on packet
	void 	rateCalc(int,int,int&,int&); // do lfs rate calculation
	int 	options(uint32_t*);	// handle packets with LFS option
};

#endif
