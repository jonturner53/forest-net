// Header file for lfsRouter class.

#ifndef LFSROUTER_H
#define LFSROUTER_H

#include "stdinc.h"
#include "lfs.h"

#include "lnkTbl.h"
#include "fltrTbl.h"
#include "rteTbl.h"
#include "pktStore.h"
#include "header.h"
#include "qMgr.h"
#include "ioProc.h"
#include "statsMod.h"

class lfsRouter {
public:
		lfsRouter(ipa_t);
		~lfsRouter();
	bool	init(char*,char*,char*,char*,char*);
					// initialize tables from files
	void	run(uint32_t,int);	// main processing loop
	void	dump(ostream& os); 	// print the tables
private:
	ipa_t	myAdr;			// forest address of this router
		
	int	nLnks;			// max number of links
	int	nFltrs;			// max number of filters
	int	nRts;			// max number of routes
	int	nPkts;			// number of packets
	int	nBufs;			// number of packet buffers
	int	nQus;			// number of queues

	uint32_t now;			// current time in 32 bit form

	lnkTbl 	*lt;			// table defining links
	fltrTbl *ft;			// table of filters
	rteTbl  *rt;			// table of routes
	pktStore *ps;			// packet buffers and headers
	qMgr 	*qm;			// collection of queues for all links
	ioProc 	*iop;			// class for handling io
	statsMod *sm;			// class for recording statistics
	int	*avail;			// avail[i]=available bw for link i

	void	addLocalRoutes();	// add routes for adjacent hosts
	bool	pktCheck(packet);	// perform basic checks on packet
	void 	rateCalc(int,int,int&,int&); // do lfs rate calculation
	int 	forward(packet);	// do lookup and enqueue packet(s)
	int 	options(packet);	// handle packets with LFS option
};

#endif
