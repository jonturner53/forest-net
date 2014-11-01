/** @file Host.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef HOST_H
#define HOST_H

#include <thread>
#include <chrono>

using namespace std::chrono;

#include "Forest.h"
#include "Packet.h"
#include "PacketStore.h"

namespace forest {


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

	bool	init();			
	void	run(bool,int64_t,int);
	bool 	readPacket(int,int64_t&,int&); 
	void	send(int);
	int	receive();	
private:
	int	nPkts;			// max number of packets in script
	ipa_t	myIpAdr;		// IP address of interface
	ipa_t	rtrIpAdr;		// IP address of router
	int	sock;			// socket number

	PacketStore *ps;		// pointer to packet store
};


} // ends namespace

#endif
