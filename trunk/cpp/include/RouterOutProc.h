/** @file RouterCore.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef ROUTEROUTPROC_H
#define ROUTEROUTPROC_H

#include "stdinc.h"
#include "Forest.h"
#include <thread>
#include <mutex>
#include <chrono>

#include "Quu.h"
#include "IfaceTable.h"
#include "LinkTable.h"
#include "ComtreeTable.h"
#include "RouteTable.h"
#include "PacketStore.h"
#include "CtlPkt.h"
#include "QuManager.h"
#include "StatsModule.h"
#include "PacketLog.h"
#include "Repeater.h"

#include "Router.h"

using namespace chrono;
using std::thread;
using std::mutex;
using std::cout;
using std::endl;

namespace forest {

class Router;

class RouterOutProc {
public:
		RouterOutProc(Router*);
		~RouterOutProc();

	bool	init();
	void	run();
	static void start(RouterOutProc*);
private:
	typedef high_resolution_clock::time_point timePoint;

	int64_t now;			///< current time

	Router	*rtr;			///< pointer to main router object

	IfaceTable *ift;		///< table defining interfaces
	LinkTable *lt;			///< table defining links
	ComtreeTable *ctt;		///< table of comtrees
	RouteTable  *rt;		///< table of routes
	PacketStore *ps;		///< packet buffers and headers
	StatsModule *sm;		///< class for recording statistics
	PacketLog *pktLog;		///< log for recording sample of packets
	QuManager *qm;			///< queues and link schedulers

	Repeater *rptr;			///< used for connect/subunsub packets

	bool	send();
	void	forward(pktx, int);
	void	multisend(pktx, int, int);
	void	sendRteReply(pktx, int);
	void	handleRteReply(pktx, int);
	void	subUnsub(pktx, int);
	void	handleConnDisc(pktx);
	void	returnAck(pktx int, bool);
};


} // ends namespace


#endif
