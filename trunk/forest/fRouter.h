// Header file for fRouter class.

#ifndef FROUTER_H
#define FROUTER_H

#include <queue>

#include "stdinc.h"
#include "forest.h"

#include "lnkTbl.h"
#include "comtTbl.h"
#include "rteTbl.h"
#include "pktStore.h"
#include "CtlPkt.h"
#include "qMgr.h"
#include "ioProc.h"
#include "statsMod.h"

class fRouter {
public:
		fRouter(fAdr_t);
		~fRouter();
	bool	init(char*,char*,char*,char*,char*);
					// initialize tables from files
	void	run(uint32_t,int);	// main processing loop
	void	dump(ostream& os); 	// print the tables
private:
	fAdr_t	myAdr;			// forest address of this router
		
	int	nLnks;			// max number of links
	int	nComts;			// max number of comtrees
	int	nRts;			// max number of routes
	int	nPkts;			// number of packets
	int	nBufs;			// number of packet buffers
	int	nQus;			// number of queues

	uint32_t now;			// current time in 32 bit form

	lnkTbl 	*lt;			// table defining links
	comtTbl *ctt;			// table of comtrees
	rteTbl  *rt;			// table of routes
	pktStore *ps;			// packet buffers and headers
	qMgr 	*qm;			// collection of queues for all links
	ioProc 	*iop;			// class for handling io
	statsMod *sm;			// class for recording statistics

	void	addLocalRoutes();	// add routes for adjacent hosts
	bool	pktCheck(int,int);	// perform basic checks on packet
	void	subUnsub(int,int);	// subscription processing
	void	handleCtlPkt(int);	// process control packets
	void	errReply(packet,CtlPkt&,char*); // return error message
	void	returnToSender(packet,int); // return control packet
	void 	handleRteReply(int, int); // handle route reply packet
	void	sendRteReply(int,int);	// send route reply packet
	void	multiSend(int,int,int);	// handle packets with >1 destination
	void 	forward(int,int);	// do lookup and enqueue packet(s)
};

#endif
