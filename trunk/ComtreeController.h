/** @file ComtreeController.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef MONITOR_H
#define MONITOR_H

#include "CommonDefs.h"
#include "PacketHeader.h"
#include "PacketStore.h"
#include "UiHashTbl.h"

/** ComtreeController processes comtree control messages from Forest clients.
 *  The controller starts by reading a topology file that defines
 *  a the backbone topology of a Forest network and the backbone
 *  topology for one or more comtrees.
 * 
 *  The controller then waits for messages from Forest clients to
 *  join or leave comtrees. It keeps track of which clients are in
 *  each comtree, but in this version does not generate router control
 *  messages to actually configure comtrees at routers.
 *
 *  The controller also opens a stream socket for use by a remote
 *  display program.  As the controller gets join/leave messages,
 *  it forwards this information to the remote display as well.
 *  
 *  The remote display shows the network topology allows the user
 *  to request display of any comtree. The requested comtree is shown
 *  on the network display by drawing the comtree links with a heavier
 *  weight. In addition, each router is labeled with the number of
 *  clients that are connected to the comtree at that router. This
 *  information is inferred from join/leave information passed to it
 *  by the controller.
 */
class ComtreeController {
public:
		ComtreeController();
		~ComtreeController();

	bool	init();
	void	run(int); 
private:
	const static short CC_PORT = xxx; ///< port# for used by remote display
	
	ipa_t	extIp;			///< IP address for remote UI
	ipa_t	intIp;			///< IP address for Forest net
	ipa_t	rtrIp;			///< IP address of router
	fAdr_t	myAdr;			///< forest address of this host
	fAdr_t	rtrAdr;			///< forest address of router

	int	intSock;		///< internal socket number
	int	extSock;		///< external listening socket
	int	connSock;		///< external connection socket

	PacketStore *ps;		///< pointer to packet store

	// private helper methods
	void	connect();		
	void	disconnect();	

	void	writeToDisplay(int);	
	int	readFromDisplay();

	void	sendToForest(int);
	int 	rcvFromForest();

};

#endif
