/** @file Substrate.h 
 *
 *  @author Jon Turner
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef SUBSTRATE_H
#define SUBSTRATE_H

#include "Forest.h"
#include "Packet.h"
#include "PacketStoreTs.h"
#include "Queue.h"
#include "IdMap.h"
#include "UiSetPair.h"
#include "Logger.h"
#include <pthread.h> 
#include <string>

namespace forest {

/** This class provides a common substrate used by various controllers.
 *  It manages a pool of threads that respond to control packets,
 *  creating a new thread whenever a new request comes in and routing
 *  control packet replies back to the thread that sent the request.
 */
class Substrate {
public:		
		Substrate(fAdr_t, ipa_t, fAdr_t, ipa_t, ipp_t, uint64_t, int,
			  void* (*)(void*), int, int, PacketStoreTs*, Logger*);
		~Substrate();

	bool	init();
	bool	run(int);	

	void	setRtrPort(ipp_t);
	void	setNonce(uint64_t);
	void	setRtrReady(bool);

	struct QueuePair {
		Queue in; Queue out;
	};
private:
	fAdr_t	myAdr;		///< Forest address of self
	ipa_t	myIp;		///< address of interface to use
	fAdr_t	rtrAdr;		///< Forest address of router
	ipa_t	rtrIp;		///< IP address for forest router
	ipp_t	rtrPort;	///< port used by forest router
	uint64_t nonce;		///< nonce used when connecting
	bool	rtrReady;	///< true when router is ready to go
	
	uint64_t seqNum;	///< for outgoing requests
	uint64_t now;		///< current time
	
	int	threadCount;	///< number of threads in pool
	struct ThreadInfo {
		pthread_t thid;
		QueuePair qp;
		uint64_t seqNum;
		uint64_t ts;
	};
	
	int	dgSock;		///< datagram socket to Forest router
	int	dgPort;		///< port number used for datagram socket
	int	listenSock;	///< listening stream socket
	int	listenPort;	///< port number for listening socket
	
	PacketStoreTs *ps;	///< pointer to packet store
	Logger *logger;		///< error message logger
	ThreadInfo *pool;	///< pool of "worker threads"
	UiSetPair *threads;	///< idle/active thread indexes
	void*	(*handler)(void*); ///< pointer to handler for worker threads
	IdMap *inReqMap;	///< maps srcAdr and sequence # of an inbound
				///< request to index of assigned thread
	IdMap *outReqMap;	///< maps sequence number of an outbound
				///< request to index of thread that sent it

	// internal helper methods
	void	inbound(pktx);
	void	outbound(pktx,int);
	void	sendToForest(int);		
	int	recvFromForest();	
	bool	connect();		
	bool	disconnect();
};

inline void Substrate::setRtrPort(ipp_t port) { rtrPort = port; }
inline void Substrate::setNonce(uint64_t nonce1) { nonce = nonce1; }
inline void Substrate::setRtrReady(bool ready) { rtrReady = ready; }

} // ends namespace

#endif
