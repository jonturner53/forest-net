/** @file RouterControl.h 
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef RTRCTL_H
#define RTRCTL_H

#include <thread>
#include <mutex>
#include <chrono>
#include "BlockingQ.h"
#include "Forest.h"
#include "Router.h"
#include "Packet.h"
#include "CtlPkt.h"

using namespace chrono;
using std::thread;
using std::unique_lock;
using std::defer_lock;

namespace forest {

class Router;

/** This class handles incoming and outgoing control packets on
 *  behalf of a router core.
 *
 *  It is a friend of the RouterCore, giving it access to private data.
 */
class RouterControl {
public:
		RouterControl(Router*, int,
			      BlockingQ<int>*, BlockingQ<pair<int,int>>*);
		RouterControl() {};
		~RouterControl();
	static void start(RouterControl*);

private:
	Router	*rtr;

	IfaceTable *ift;		///< table defining interfaces
	LinkTable *lt;			///< table defining links
	ComtreeTable *ctt;		///< table of comtrees
	RouteTable  *rt;		///< table of routes
	PacketStore *ps;		///< packet buffers and headers
	PacketLog *pktLog;		///< log for recording sample of packets
	QuManager *qm;			///< queues and link schedulers

	int	myThx;			///< my thread index
	BlockingQ<int> *inQ;		///< input queue for this thread
	BlockingQ<pair<int,int>> *outQ;	///< output queue, shared among threads

	void	run();

	// methods for handling incoming signalling requests
	void	handleRequest(pktx, CtlPkt&);
	void	returnToSender(pktx, CtlPkt&);

	// interface table packets
	void	addIface(CtlPkt&);
	void	dropIface(CtlPkt&);
	void	getIface(CtlPkt&);
	void	modIface(CtlPkt&);
	void 	getIfaceSet(CtlPkt&);

	// link table packets
	void	addLink(CtlPkt&);
	void	dropLink(CtlPkt&);
	void	dropLink(int,fAdr_t=0);
	void	getLink(CtlPkt&);
	void	modLink(CtlPkt&);
	void	getLinkSet(CtlPkt&);

	// comtree table packets
	void	addComtree(CtlPkt&);
	void	dropComtree(CtlPkt&);
	void	getComtree(CtlPkt&);
	void	modComtree(CtlPkt&);
	void 	getComtreeSet(CtlPkt&);

	void	addComtreeLink(CtlPkt&);
	void	dropComtreeLink(CtlPkt&);
	void	dropComtreeLink(int, int, int);
	void	modComtreeLink(CtlPkt&);
	void	getComtreeLink(CtlPkt&);

	// route table packets
	void	addRoute(CtlPkt&);
	void	dropRoute(CtlPkt&);
	void	getRoute(CtlPkt&);
	void	modRoute(CtlPkt&);
	void 	getRouteSet(CtlPkt&);

	// filter table packets
	void	addFilter(CtlPkt&);
	void	dropFilter(CtlPkt&);
	void	getFilter(CtlPkt&);
	void	modFilter(CtlPkt&);
	void	getFilterSet(CtlPkt&);
	void	getLoggedPackets(CtlPkt&);
	void	enablePacketLog(CtlPkt&);

	// configuration
	void	setLeafRange(CtlPkt&);

	// comtree setup packets
	void	joinComtree(CtlPkt&);
	void	leaveComtree(CtlPkt&);
	void	addBranch(CtlPkt&);
	void	prune(CtlPkt&);
	void	confirm(CtlPkt&);
	void	abort(CtlPkt&);

	// comtree setup packets
/*
	bool	sendAddBranch(..);
	bool	sendPrune(..);
	bool	sendConfirm(..);
	bool	sendAbort(..);
	bool	sendAddNode(..);
	bool	sendDropNode(..);
*/
};

} // ends namespace

#endif
