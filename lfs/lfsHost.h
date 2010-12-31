// Header file for lfsHost class, which implements forest host

#ifndef LFSHOST_H
#define LFSHOST_H

#include "lfs.h"
#include "pktStore.h"

class lfsHost {
public:
		lfsHost(ipa_t, ipa_t, ipa_t);
		~lfsHost();

	bool	init();			// open and setup socket
	packet	receive();		// return next input packet
	void	send(packet);		// send packet
	void	run(bool,uint32_t,uint32_t); // run host
	bool 	getPacket(packet,int&,int&);    // get packet from stdin
private:
	int	nPkts;			// max number of input packets
	ipa_t	myIpAdr;		// IP address of host's interface
	uint16_t myPort;		// port number for all io
	ipa_t	rtrIpAdr;		// substrate IP address of router
	ipa_t	myLfsAdr;		// lfs address of host
	int	sock;			// socket number
	sockaddr_in sa, dsa;		// socket address structures

	pktStore *ps;			// pointer to packet store
};

#endif
