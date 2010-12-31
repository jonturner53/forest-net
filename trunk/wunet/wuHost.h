// Header file for wuHost class, which implements wunet host

#ifndef WUHOST_H
#define WUHOST_H

#include "wunet.h"
#include "lnkTbl.h"
#include "pktStore.h"

class wuHost {
public:
		wuHost(ipa_t, ipa_t, wuAdr_t);
		~wuHost();

	bool	init();			// open and setup socket
	int	receive();		// return next input packet
	void	send(int);		// send packet
	void	run(bool,uint32_t,uint32_t); // run host
	bool 	getPacket(int,int&,int&);    // get packet from stdin
private:
	int	nPkts;			// max number of input packets
	ipa_t	myIpAdr;		// IP address of interface
	uint16_t myPort;		// port number for all io
	ipa_t	rtrIpAdr;		// IP address of router
	wuAdr_t	myAdr;			// wunet address of host
	int	sock;			// socket number
	sockaddr_in sa, dsa;		// socket address structures

	pktStore *ps;			// pointer to packet store
};

#endif
