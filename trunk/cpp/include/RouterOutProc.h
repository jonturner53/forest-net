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

	// resendMap used to retransmit subscription packets and
	// connection/disconnection requests, as needed
	HashMap<uint64_t,pktx,Hash::u64>
		*resendMap;		///< maps seqNum->pktx
	Dheap<uint64_t> *resendTimes;	///< resendMap indexes, by resend time

	bool	send();
};


} // ends namespace


#endif
