// Header file for ioProc class, which supports io processing.

#ifndef IOPROC_H
#define IOPROC_H

#include "wunet.h"
#include "lnkTbl.h"
#include "lcTbl.h"
#include "pktStore.h"

class ioProc {
public:
		ioProc(ipa_t, ipp_t, lnkTbl*, pktStore*,lcTbl* =Null,int=0);
		~ioProc();

	bool	init();			// open and setup socket
	int	receive();		// return next input packet
	void	send(int,int);		// send packet
private:
	ipa_t	myIpAdr;		// IP address of interface
	uint16_t myPort;		// port number for all io
	int	sock;			// socket number
	sockaddr_in sa, dsa;		// socket address structures

	int myLcn;			// linecard # (0 for single node config)

	lnkTbl *lt;			// pointer to link table
	lcTbl  *lct;			// pointer to linecard table
	pktStore *ps;			// pointer to packet store
};

#endif
