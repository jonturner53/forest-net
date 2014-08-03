/** @file RouterInProc.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef ROUTERINPROC_H
#define ROUTERINPROC_H

#include <queue>
#include <map>

#include "stdinc.h"
#include "Forest.h"

#include "IfaceTable.h"
#include "LinkTable.h"
#include "ComtreeTable.h"
#include "RouteTable.h"
#include "PacketStore.h"
#include "CtlPkt.h"
#include "QuManager.h"
#include "Quu.h"
#include "IoProcessor.h"
#include "StatsModule.h"
#include "PacketLog.h"

using namespace std::chrono;
using std::thread;
using std::mutex;
using std::unique_lock;

namespace forest {

class Router;

class RouterInProc {
public:
		RouterInProc(Router*);
		~RouterInProc();

	static void start(RouterInProc*);
private:
	const static int numThreads = 100; ///< max number in thread pool
	const static int maxReplies = 10000; ///< max # of remembered replies
	typedef high_resolution_clock::time_point timePoint;

	uint64_t now;			///< relative to router start time

	Router	*rtr;			///< pointer to main router object

	int	maxSockNum;		///< largest socket num opened by ioProc
	fd_set	*sockets;		///< file descr set for open sockets
	int	cIf;			///< number of "current interface"
	int	nRdy;			///< number of ready sockets

	QuManager *qm;			///< queues and link schedulers

	/// info for worker thread used to process an incoming control packet
	struct ThreadInfo {
                thread	thred;		///< thread object
		RouterControl *rc;	///< per thread RouterControl object
                Quu<pktx> q;		///< used to send packets to thread
		pktx	px;		///< outgoing request or sent reply
		int	reps;		///< # of times request has been sent
		int64_t rcvSeqNum;	///< rcvSeqNum of last packet to thred
        };
	ThreadInfo *tpool;		///< pointer to thread pool
	List	freeThreads;		///< list of unassigned thread indexs
	Quu<pair<int,int>> retQ;	///< Quu coming from worker threads
	int64_t rcvSeqNum;		///< used to coordinate thread dealloc

	HashSet<comt_t, Hash::u32> *comtSet; ///< maps comt->thx
	Repeater *rptr;			///< for repeating control packets
	RepeatHandler *repH;		///< for handling received repeats

	void	run();
	bool	inBound();
	bool	outBound();

	// booting
	int	bootSock;		///< socket used while booting
	bool	bootStart();
	pktx	bootReceive();
	void	bootSend(pktx);

	// basic forwarding 
	pktx	receive();
	void 	forward(pktx);
	bool	pktCheck(pktx,int);

	// inband control
	void 	handleConnDisc(pktx);
	void 	handleRteReply(pktx, int);
	void	sendRteReply(pktx,int);	
	void	subUnsub(pktx,int);
};

inline void RouterInProc::forward(pktx px) {
	if (rtr->booting) bootSend(px);
	else rtr->xferQ.enq(px);
}

} // ends namespace

#endif
