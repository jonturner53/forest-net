/** \file Host.h */

#ifndef HOST_H
#define HOST_H

#include "CommonDefs.h"
#include "PacketHeader.h"
#include "PacketStore.h"

/** Host implements a forest host.
 *
 *  More specifically, this program generates and sends a set of
 *  forest packets based on a script. The script format supports
 *  a simple repetition mechanism, allowing for non-trivial
 *  demonstrations.
 */
class Host {
public:
		Host(ipa_t, ipa_t);
		~Host();

	bool	init();			// open and setup socket
	int	receive();		// return next input packet
	void	send(int);		// send packet
	void	run(bool,uint32_t,uint32_t); // run host
	bool 	readPacket(int,int&,int&);    // get packet from stdin
private:
	int	nPkts;			// max number of input packets
	ipa_t	myIpAdr;		// IP address of interface
	uint16_t myPort;		// port number for all io
	ipa_t	rtrIpAdr;		// IP address of router
	fAdr_t	myAdr;			// forest address of host
	int	sock;			// socket number
	sockaddr_in sa, dsa;		// socket address structures

	PacketStore *ps;		// pointer to packet store
};

#endif
