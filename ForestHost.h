// Header file for fHost class, which implements forest host

#ifndef FHOST_H
#define FHOST_H

#include "forest.h"
#include "header.h"
#include "pktStore.h"

class fHost {
public:
		fHost(ipa_t, ipa_t);
		~fHost();

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
	fAdr_t	myAdr;			// forest address of host
	int	sock;			// socket number
	sockaddr_in sa, dsa;		// socket address structures

	pktStore *ps;			// pointer to packet store
};

#endif
