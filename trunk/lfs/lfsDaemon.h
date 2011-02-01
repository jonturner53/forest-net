// Header file for lfsDaemon class. This is used in the version of
// LFS that uses the SPP fastpath

#ifndef LFSDAEMON_H
#define LFSDAEMON_H

#include <inttypes.h>
#include <string>
#include <iostream>
#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h> // includes stdint.h
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <wulib/wulog.h>
#include <wulib/wunet.h>
#include <wulib/keywords.h>
#include <wulib/wusock.h>
#include <wulib/ipnet.h>
#include <wulib/bits.h>
#include <wuPP/sockInet.h>
#include <wuPP/error.h>
#include <wuPP/wusock.h>
#include <wuPP/net.h>
#include <subLib/rmpOps.h>
#include <subLib/rmpClient.h>
#include <subLib/rmpSliceClient.h>
#include <subLib/srmClient.h>
#include <subLib/substrateMsg.h>
#include <subLib/jstWrapper.h>
#include <mnet/ipv4/mnet.h>

#include "stdinc.h"
#include "lfs.h"

#include "lnkTbl.h"
#include "fltrTbl.h"
#include "rteTbl.h"
#include "header.h"
#include "ioProc.h"
#include "statsMod.h"

class lfsDaemon {
public:
		lfsDaemon(ipa_t);
		~lfsDaemon();
	bool	init(const char*,const char*,const char*,const char*,
		     const char*, const int, const int, const int,
		     const int, const int);
					// initialize tables from files
	bool	setup();
	bool 	handleConnectDisconnect(header&);
	void 	handleOptions(wunet::sockInet&, mnpkt_t&);
	void	dump(ostream& os); 	// print the tables
private:
	ipa_t	myAdr;			// LFS address of this router
		
	int	fpBw;			// configured fastpath bandwidth
	int	fpFltrs;		// configured # fastpath filters
	int	fpQus;			// configured # fastpath queues
	int	fpBufs;			// configured # fastpath buffers
	int	fpStats;		// configured # fastpath stats indices

	// LFS-centric parameters
	int	nIntf;			// number of interfaces configured
	int	nLnks;			// number of links configured
	int	nFltrs;			// number of flow filters
	int	nRts;			// number of routes
	int	nPkts;			// number of packets
	int	nBufs;			// number of packet buffers
	int	nQus;			// number of queues per link
	int	qSiz;			// number of packets per queue

	uint32_t now;			// current time in 32 bit form

	lnkTbl 	*lt;			// table defining links
	fltrTbl *ft;			// table of filters
	rteTbl  *rt;			// table of routes
	ioProc 	*iop;			// class for handling io
	statsMod *sm;			// class for recording statistics
	int	*avail;			// avail[i]=available bw for link i

	void	addInterfaces();	// add fastpath interfaces
	void	addQueues();		// add datagram queues
	void	addBypass();		// add bypass filters
	void	addRoutesFilters();	// add routes and filters
	bool	pktCheck(header&);	// perform basic checks on packet
	void 	rateCalc(int,int,int&); // do lfs rate calculation
	int 	options(header&);	// handle packets with LFS option
};

#endif
