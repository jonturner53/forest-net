/** @file NetMgr.h 
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

/** NetMgr manages a Forest network.
 *  It relays control packets from a remote NetMgrUserInterface and
 *  provides some autonomous control capabilities. 
 */
class NetMgr {
public:
		NetMgr(ipa_t, ipa_t, ipa_t, fAdr_t, fAdr_t);
		~NetMgr();

	bool	init();
	void	run(int); 

	bool readClientInfo(char*);
private:
	const static short NM_PORT = 30122; ///< port# for used by remote UI
	
	ipa_t	extIp;			///< IP address for remote UI
	ipa_t	intIp;			///< IP address for Forest net
	ipa_t	rtrIp;			///< IP address of router
	fAdr_t	myAdr;			///< forest address of this host
	fAdr_t	rtrAdr;			///< forest address of router

	int	intSock;		///< internal socket number
	int	extSock;		///< external listening socket
	int	connSock;		///< external connection socket

	PacketStore *ps;		///< pointer to packet store

	// Information relating client addresses and router addresses
	// This is a temporary expedient and will be replaced later
	static const int MAX_CLIENTS = 1000;
	int	numClients;
	struct clientAdrInfo {
		ipa_t	cliIp;
		fAdr_t	cliAdr;
		ipa_t	rtrIp;
		fAdr_t	rtrAdr;
		bool	inUse;
	};
	clientAdrInfo clientInfo[MAX_CLIENTS];

	// private helper methods
	void	connect();		
	void	disconnect();	

	void	sendToUi(int);	
	int	recvFromUi();

	void	sendToForest(int);
	int 	rcvFromForest();

	bool setupClient(ipa_t , fAdr_t&, ipa_t&, fAdr_t&);
};


#endif