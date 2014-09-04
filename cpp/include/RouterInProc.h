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
#include "Router.h"
#include "RouterControl.h"
#include "Repeater.h"
#include "RepeatHandler.h"
#include "CtlPkt.h"
#include "QuManager.h"
#include "Quu.h"
#include "StatCounts.h"
#include "PacketLog.h"

using namespace std::chrono;
using std::thread;
using std::mutex;
using std::unique_lock;

namespace forest {

class Router;
class RouterControl;

class RouterInProc {
public:
		RouterInProc(Router*);
		~RouterInProc();

	static void start(RouterInProc*);
private:
	const static int numThreads = 100; ///< max number in thread pool
	const static int maxReplies = 10000; ///< max # of remembered replies
	const static int MAXFANOUT = 512; ///< limit on packet fanout
	typedef high_resolution_clock::time_point timePoint;

	uint64_t now;			///< relative to router start time
	bool overload;			///< set when xferQ fills

	Router	*rtr;			///< pointer to main router object

	IfaceTable *ift;		///< table defining interfaces
	LinkTable *lt;			///< table defining links
	ComtreeTable *ctt;		///< table of comtrees
	RouteTable  *rt;		///< table of routes
	PacketStore *ps;		///< packet buffers and headers
	PacketLog *pktLog;		///< log for recording sample of packets
	QuManager *qm;			///< queues and link schedulers

	fd_set	*sockets;		///< file descr set for open sockets
	int	cIf;			///< number of "current interface"
	int	nRdy;			///< number of ready sockets

	/// info for worker thread used to process an incoming control packet
	struct ThreadInfo {
                thread	thred;		///< thread object
		RouterControl rc;	///< per thread RouterControl object
                Quu<pktx> q;		///< used to send packets to thread
		int64_t rcvSeqNum;	///< rcvSeqNum of last packet to thred
		ThreadInfo() {};
        };
	ThreadInfo *tpool;		///< pointer to thread pool
	List	freeThreads;		///< list of unassigned thread indexs
	Quu<pair<int,int>> retQ;	///< Quu coming from worker threads
	int64_t rcvSeqNum;		///< used to coordinate thread dealloc

	HashSet<comt_t, Hash::u32> *comtSet; ///< maps comt->thx
	Repeater *rptr;			///< for repeating control packets
	RepeatHandler *repH;		///< for handling received repeats

	void	run();
	bool	mainline();

	// booting
	int	bootSock;		///< socket used while booting
	bool	bootRouter();
	bool	bootStart();
	pktx	bootReceive();
	void	bootSend(pktx);

	// forwarding 
	pktx	receive();
	bool	pktCheck(pktx,int);
	void	forward(pktx, int);
	void	multiForward(pktx, int, int);

	// control packets
	void 	handleControl(pktx, int);
	void 	handleConnDisc(pktx);
	void 	handleRteReply(pktx, int);
	void	sendRteReply(pktx,int);	
	void	returnAck(pktx,int,bool);	
	void	subUnsub(pktx,int);
};

} // ends namespace

#endif
