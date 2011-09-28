/** @file NetMgr.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>
#include "CommonDefs.h"
#include "PacketHeader.h"
#include "PacketStoreTs.h"
#include "NetInfo.h"
#include "UiSetPair.h"
#include "IdMap.h"
#include "Queue.h"
#include <map>

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

int	intSock;		///< internal socket number
int	extSock;		///< external listening socket
int	connSock;		///< external connection socket

PacketStoreTs *ps;		///< pointer to packet store

NetInfo *net;			///< global view of net topology

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

IdMap	*tMap;			///< maps sequence numbers to thread #

bool	init();
void	cleanup();
bool	readPrefixInfo(char*);
void*	run(void*); 

void* 	handler(void *);
bool 	handleConsReq(int,CtlPkt&, Queue&, Queue&);
bool 	handleConDisc(int,CtlPkt&, Queue&, Queue&);
bool 	handleNewClient(int,CtlPkt&, Queue&, Queue&);
int	sendCtlPkt(CtlPkt&, comt_t, fAdr_t, Queue&, Queue&);
int 	sendAndWait(int,CtlPkt&,Queue&,Queue&);
void 	errReply(int,CtlPkt&,Queue&, const char*);

void	connect();		
void	disconnect();	

void	sendToCons(int);	
int	recvFromCons();

void	sendToForest(int);
int 	rcvFromForest();

bool	findCliRtr(ipa_t,fAdr_t&); ///< gives the rtrAdr of the prefix

#endif
