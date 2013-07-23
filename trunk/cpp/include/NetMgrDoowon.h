/** @file NetMgr.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef NETMGR_H
#define NETMGR_H

#include <pthread.h>
#include "Forest.h"
#include "Packet.h"
#include "PacketStoreTs.h"
#include "NetInfo.h"
#include "ComtInfo.h"
#include "UiSetPair.h"
#include "IdMap.h"
#include "Queue.h"
#include "CpHandler.h"
#include "Substrate.h"
#include "NetBuffer.h"
#include "Logger.h"
#include <map>

namespace forest {


/** NetMgr manages a Forest network.
 *  It relays control packets from a remote Console and
 *  provides some autonomous control capabilities. 
 */


ipa_t	myIp;			///< IP address of this host
fAdr_t	myAdr;			///< forest address of this host
fAdr_t	rtrAdr;			///< forest address of router
fAdr_t  cliMgrAdr;		///< forest address of the client manager
fAdr_t  comtCtlAdr;		///< forest address of the comtree controller
int	netMgr;			///< node number of net manager in NetInfo
int	nmRtr;			///< node number of net manager's router

Logger *logger;			///< error message logger

PacketStoreTs *ps;		///< pointer to packet store

NetInfo *net;			///< global view of net topology
ComtInfo *comtrees;		///< pre-configured comtrees

Substrate *sub;			///< substrate for routing control packets

// Information relating client addresses and router addresses
// This is a temporary expedient and will be replaced later
int	numPrefixes;
struct prefixInfo {
	string prefix;
	fAdr_t rtrAdr;
};
struct clientInfo {
	ipa_t cliIp;
	fAdr_t rtrAdr;
	fAdr_t cliAdr;
	ipa_t rtrIp;
};
prefixInfo prefixes[1000];

bool	init(const char*);
bool	readPrefixInfo(char*);

void* 	handler(void *);
bool 	handleConsole(int,CpHandler&);
bool 	handleConDisc(int,CtlPkt&,CpHandler&);
bool 	handleNewSession(int,CtlPkt&,CpHandler&);
bool 	handleCancelSession(int,CtlPkt&,CpHandler&);
bool 	handleBootLeaf(int,CtlPkt&,CpHandler&);
bool 	handleBootRouter(int,CtlPkt&,CpHandler&);

void	getNet(NetBuffer&, string&);
//feng
void	getLinkSet(NetBuffer&, string&, CpHandler&);

uint64_t generateNonce();
fAdr_t	setupLeaf(int, pktx, CtlPkt&, int, int, uint64_t,CpHandler&,bool=false);
bool	setupEndpoint(int, int, pktx, CtlPkt&, CpHandler&, bool=false);
bool	setupComtree(int, int, pktx, CtlPkt&, CpHandler&, bool=false);
bool	processReply(pktx, CtlPkt&, pktx, CtlPkt&, CpHandler&, const string&);

void	sendToCons(int);	
int	recvFromCons();

bool	findCliRtr(ipa_t,fAdr_t&); ///< gives the rtrAdr of the prefix

} // ends namespace


#endif
