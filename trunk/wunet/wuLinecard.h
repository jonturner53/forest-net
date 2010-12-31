// Header file for wuLinecard class.

#ifndef WULINECARD_H
#define WULINECARD_H

#include "stdinc.h"
#include "wunet.h"

#include "lnkTbl.h"
#include "lcTbl.h"
#include "vnetTbl.h"
#include "rteTbl.h"
#include "pktStore.h"
#include "qMgr.h"
#include "ioProc.h"
#include "statsMod.h"

class wuLinecard {
public:
		wuLinecard(int,wuAdr_t);
		~wuLinecard();
	bool	init(char*,char*,char*,char*,char*);
					// initialize tables from files
	void	run(uint32_t);		// main processing loop
	void	dump(ostream& os); 	// print the tables
private:
	ipa_t	myIpAdr;		// IP address of the linecard
	wuAdr_t	myAdr;			// wunet address of the router
	int	myLcn;			// linecard number of this linecard
		
	int	nLnks;			// max number of links
	int	nVnets;			// max number of vnets
	int	nRts;			// max number of routes
	int	nPkts;			// number of packets
	int	nBufs;			// number of packet buffers
	int	nQus;			// number of queues

	int	schedInterval;		// time between scheduling packets
	int	schedPeriod;		// time between updates for each linecard
	int	minBitRate;		// min bit rate between linecards
	int	minPktRate;		// min packet rate between linecards

	uint32_t now;			// current time in 32 bit form

	lnkTbl 	*lt;			// table defining links
	lcTbl 	*lct;			// table defining linecards
	vnetTbl *vnt;			// table of vnets
	rteTbl  *rt;			// table of routes
	pktStore *ps;			// packet buffers and headers
	qMgr 	*qm;			// collection of queues for all links
	ioProc 	*iop;			// class for handling io
	statsMod *sm;			// class for recording statistics

	bool	checkTables();		// perform consistency checks
	void	addLocalRoutes();	// add routes for directly connected hosts
	bool	pktCheck(int); 		// perform basic checks on packet
	void	addRevRte(int); 	// check for & add route back to source
	int 	ingress(int);		// do ingress processing for packet
	int 	egress(int);		// do egress processing for packet
	void	sendVOQstatus();	// send status update for scheduling
	void	voqUpdate(int);		// update VOQ rates
};

#endif
