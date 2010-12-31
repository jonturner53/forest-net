// Header file for wuRouter class.

#ifndef WUROUTER_H
#define WUROUTER_H

#include "stdinc.h"
#include "wunet.h"

#include "lnkTbl.h"
#include "vnetTbl.h"
#include "rteTbl.h"
#include "pktStore.h"
#include "qMgr.h"
#include "ioProc.h"
#include "statsMod.h"

class wuRouter {
public:
		wuRouter(ipa_t,wuAdr_t);
		~wuRouter();
	bool	init(char*,char*,char*,char*);// initialize tables from files
	void	run(uint32_t);		// main processing loop
	void	dump(ostream& os); 	// print the tables
private:
	ipa_t	myIpAdr;		// IP address of this router
	wuAdr_t	myAdr;			// wunet address of this router
		
	int	nLnks;			// max number of links
	int	nVnets;			// max number of vnets
	int	nRts;			// max number of routes
	int	nPkts;			// number of packets
	int	nBufs;			// number of packet buffers
	int	nQus;			// number of queues

	uint32_t now;			// current time in 32 bit form

	lnkTbl 	*lt;			// table defining links
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
	int 	forward(int);		// do lookup and enqueue packet(s)
};

#endif
