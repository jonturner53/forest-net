/** @file ComtCtl.h 
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
#include "Heap.h"
#include "Queue.h"
#include <map>

/** ComtCtl manages a set of comtrees in a forest network.
 *  It receives signalling requests from clients and configures
 *  comtrees in response to those requests.
 */

ipa_t	extIp;			///< IP address used by remote display program
ipa_t	intIp;			///< IP address for Forest net
ipa_t	rtrIp;			///< IP address of access router
fAdr_t	myAdr;			///< forest address of this host
fAdr_t	rtrAdr;			///< forest address of access router

int	intSock;		///< internal socket number
int	extSock;		///< external listening socket

PacketStoreTs *ps;		///< pointer to packet store

NetInfo *net;			///< global view of net topology

int	maxComtree;		///< max# of comtrees
int 	firstComt, lastComt;	///< defines range of comtrees controlled
UiSetPair *comtSet;		///< in-use and unused comtrees

// pair of queues used by a thread
struct QueuePair {
Queue	in;
Queue	out;
};

// display handler queue definitions
struct EventStruct {
	int comt;
	int rtr;
	int upDown;
};

static const int NORESPONSE = (1 << 31); ///< indicates non-response to ctl pkt

// defines thread pool
static const int TPSIZE = 500;	///< number of concurrent threads at ComtCtl
struct ThreadInfo {
	pthread_t thid;		///< thread id
	QueuePair qp;		///< associated queue pair
	uint64_t seqNum;	///< sequence number of pending request
	uint64_t ts;		///< timestamp of pending request
};
ThreadInfo *pool;		///< pool[i]: i-th thread in pool
UiSetPair *threads;		///< in-use and unused thread indices
IdMap *reqMap;                  ///< maps rcvd requests to handling thread

IdMap	*tMap;			///< maps sequence numbers to thread index

pthread_mutex_t allComtLock;	///< global comtree lock
				///< this lock must be held while executing
				///< any of the following operations on the
				///< NetInfo object:
				///< addComtree(), removeComtree(),
				///< validComtree(), validComtIndex(),
				///< firstComtIndex(), lastComtIndex(),
				///< lookupComtree()
pthread_mutex_t *comtLock;	///< per comtree locks indexed by comtree index
				///< must hold a comtree's lock to access or
				///< modify any of its data
pthread_mutex_t rateLock;	///< must hold this lock when accessing or
				///< modifying available link rates
				

bool	init(const char*);
void	cleanup();
void*	run(void*); 

// handlers for various operations
void* 	handler(void *);
bool 	handleComtreeDisplay(int, Queue&, Queue&);
bool 	handleAddComtReq(int,CtlPkt&, Queue&, Queue&);
bool 	handleDropComtReq(int,CtlPkt&, Queue&, Queue&);
bool 	handleJoinComtReq(int,CtlPkt&, Queue&, Queue&);
bool 	handleLeaveComtReq(int,CtlPkt&, Queue&, Queue&);
// helper funnctions for adding removing paths to comtrees
int	findPath(int, int, list<int>&);
bool	reservePath(int, int, list<int>&);
void	releasePath(int, int);
bool	addPath(int, int, list<int>&, Queue&, Queue&);
bool	dropPath(int,int,Queue&,Queue&);
void	updatePath(int,int,list<int>&,Queue&,Queue&);
// helper functions for sending control packets from threads
int	sendCtlPkt(CtlPkt&, fAdr_t, Queue&, Queue&);
int 	sendAndWait(int,CtlPkt&,Queue&,Queue&);
bool	handleReply(int,CtlPkt&,string&,string&);
void 	errReply(int,CtlPkt&,Queue&, const char*);

void	connect();		
void	disconnect();	

void	sendToRemote(int);	
int	recvFromRemote();

void	sendToForest(int);
int 	rcvFromForest();
#endif
