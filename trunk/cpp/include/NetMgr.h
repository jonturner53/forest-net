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
#include "Logger.h"
#include <map>

namespace forest {


/** NetMgr manages a Forest network.
 *  It relays control packets from a remote Console and
 *  provides some autonomous control capabilities. 
 */


ipa_t	extIp;			///< IP address for remote console
ipa_t	intIp;			///< IP address for Forest net
ipa_t	rtrIp;			///< IP address of router
fAdr_t	myAdr;			///< forest address of this host
fAdr_t	rtrAdr;			///< forest address of router
fAdr_t  cliMgrAdr;		///< forest address of the cliMgr

bool	booting;		///< true until all routers are booted

int	intSock;		///< internal socket number
int	extSock;		///< external listening socket
int	connSock;		///< external connection socket

Logger logger;			///< error message logger

PacketStoreTs *ps;		///< pointer to packet store

NetInfo *net;			///< global view of net topology
ComtInfo *comtrees;		///< pre-configured comtrees

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
map<uint32_t,clientInfo> *packetsSent;

// pair of queues used by a thread
struct QueuePair {
Queue	in;
Queue	out;
};

static const int NORESPONSE = (1 << 31); ///< to indicate non-reponse to ctl pkt

// defines thread pool
static const int TPSIZE = 500;	///< number of concurrent threads at NetMgr
struct ThreadInfo {
	pthread_t thid;		///< thread id
	QueuePair qp;		///< associated queue pair
	uint64_t seqNum;	///< sequence number of pending request
	uint64_t ts;		///< timestamp of pending request
};
ThreadInfo *pool;		///< pool[i]: i-th thread in pool
UiSetPair *threads;		///< in-use and unused thread indices
IdMap *reqMap;			///< maps rcvd requests to handling thread

IdMap	*tMap;			///< maps sequence numbers to thread #

bool	init(const char*);
void	cleanup();
bool	readPrefixInfo(char*);
void*	run(void*); 

void* 	handler(void *);
bool 	handleConsReq(int,CtlPkt&,CpHandler&);
bool 	handleConDisc(int,CtlPkt&,CpHandler&);
bool 	handleNewClient(int,CtlPkt&,CpHandler&);
bool 	handleBootRequest(int,CtlPkt&,CpHandler&);

void	connect();		
void	disconnect();	

void	sendToCons(int);	
int	recvFromCons();

void	sendToForest(int);
int 	rcvFromForest();

bool	findCliRtr(ipa_t,fAdr_t&); ///< gives the rtrAdr of the prefix

} // ends namespace


#endif
