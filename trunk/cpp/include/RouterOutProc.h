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
#include "StatCounts.h"
#include "QuManager.h"
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
	int64_t now;			///< current time

	Router	*rtr;			///< pointer to main router object

	IfaceTable *ift;		///< table defining interfaces
	LinkTable *lt;			///< table defining links
	PacketStore *ps;		///< packet buffers and headers
	PacketLog *pktLog;		///< log for recording sample of packets
	QuManager *qm;			///< queues and link schedulers

	void	send(pktx,int);
};


} // ends namespace


#endif
