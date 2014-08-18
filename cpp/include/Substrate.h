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
#include "CtlPkt.h"
#include "PacketStore.h"
#include "Quu.h"
#include "HashSet.h"
#include "ListPair.h"
#include "Logger.h"
#include "Controller.h"
#include "Repeater.h"
#include "RepeatHandler.h"
#include <thread> 
#include <mutex> 
#include <chrono> 
#include <string>

using std::thread;
using std::mutex;
using namespace chrono;

namespace forest {

/** This class provides a common substrate used by various controllers.
 *  It manages a pool of threads that respond to control packets,
 *  creating a new thread whenever a new request comes in and routing
 *  control packet replies back to the thread that sent the request.
 */
class Substrate {
public:		
		Substrate(int, Controller*, PacketStore*, Logger*);
		~Substrate();

	bool	init(fAdr_t, ipa_t, fAdr_t, ipa_t, ipp_t, uint64_t,
		     ipp_t, ipp_t, int);

	bool	run();	

	void	setRtrPort(ipp_t);
	void	setNonce(uint64_t);
	void	setRtrReady(bool);

private:
	fAdr_t	myAdr;		///< Forest address of self
	ipa_t	myIp;		///< address of interface to use
	fAdr_t	rtrAdr;		///< Forest address of router
	ipa_t	rtrIp;		///< IP address for forest router
	ipp_t	rtrPort;	///< port used by forest router
	uint64_t nonce;		///< nonce used when connecting
	bool	rtrReady;	///< true when router is ready to go
	
	uint64_t seqNum;	///< for outgoing requests

	high_resolution_clock::time_point tZero; ///< start time
	uint64_t now;		///< current time in ns since tZero
	int	finTime;	///< number of seconds to run
	
	int	threadCount;	///< number of threads in pool
	Controller *pool;	///< pointer to array of controller objects
				///< (one per thread)
	
	int	dgSock;		///< datagram socket to Forest router
	int	dgPort;		///< port number used for datagram socket
	int	listenSock;	///< listening stream socket
	int	listenPort;	///< port number for listening socket
	
	PacketStore *ps;	///< pointer to packet store
	Logger *logger;		///< error message logger
	ListPair *thredIdx;	///< idle/active thread indexes
	Repeater *rptr;		///< used to repeat ougoing requests as needed
	RepeatHandler *repH;	///< handle repeated incoming requests

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
