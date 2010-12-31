// Header file for ioProc class, which supports io processing.
// This version generates packets from a script rather than
// doing any actual IO. See the .c file for details.

#ifndef IOPROC_H
#define IOPROC_H

#include "wunet.h"
#include "lnkTbl.h"
#include "pktStore.h"

class ioProc {
public:
		ioProc(ipa_t, ipp_t, lnkTbl*, pktStore*);
		~ioProc();

	bool	init();			// open and setup socket
	int	receive();		// return next input packet
	void	send(int,int);		// send packet
private:
	ipa_t	myIpAdr;		// IP address of interface
	uint16_t myPort;		// port number for all io
	int	sock;			// socket number
	sockaddr_in sa, dsa;		// socket address structures

	lnkTbl *lt;			// pointer to link table
	pktStore *ps;			// pointer to packet store

	static int const maxPkts = 1000; // max # of packets in script
	struct { int delay; ipa_t srcIp; ipp_t srcPort; int p; }
 	pktScript[maxPkts];		// vector of packets forming script
	int nPkts;			// number of packets in script
	int cPkt;			// current packet in script

	bool getPacket(int,int&,ipa_t&,ipp_t&); // read in packet for script
};

#endif
