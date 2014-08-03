/** @file Router.h 
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef ROUTER_H
#define ROUTER_H

#include <queue>
#include <map>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

#include "stdinc.h"
#include "Forest.h"
#include "CtlPkt.h"

#include "Quu.h"
#include "IfaceTable.h"
#include "LinkTable.h"
#include "ComtreeTable.h"
#include "RouteTable.h"
#include "PacketStore.h"
#include "StatsModule.h"
#include "PacketLog.h"
#include "QuManager.h"

#include "RouterInProc.h"
#include "RouterOutProc.h"

using namespace std::chrono;
using std::thread;
using std::mutex;
using std::unique_lock;

namespace forest {

class RouterInProc;
class RouterOutProc;

/** Structure used to carry information about a router.
 *  Used during initialization process.
 */
struct RouterInfo {
        string  mode; 		///< router operation mode (local or remote)

        fAdr_t  myAdr; 		///< forest address of the router
        ipa_t   bootIp; 	///< IP address used for booting
        ipp_t   portNum; 	///< port # for all interfaces (default=0)
        fAdr_t  nmAdr; 		///< forest address of the network manager
        ipa_t   nmIp; 		///< IP address of the network manager
        fAdr_t  ccAdr; 		///< forest address of the comtree controller
        fAdr_t  firstLeafAdr; 	///< first assignable leaf address
        fAdr_t  lastLeafAdr; 	///< last assignable leaf address

        string  ifTbl; 		///< name of interface table file
        string  lnkTbl; 	///< name of link table file
        string  comtTbl; 	///< name of comtree table file
        string  rteTbl; 	///< name of route table file
        string  statSpec; 	///< name of statistics specification file

        seconds runLength; 	///< number of seconds for router to run
};

class Router {
public:
		Router(const RouterInfo&);
		~Router();

	bool	readTables(const RouterInfo&);
	bool	setup();
	bool	setupIface(int);
	bool	setupAllIfaces();
	void	run();
	void	dump(ostream& os);

private:
	ipa_t	bootIp;			///< IP address used during booting
	bool	booting;		///< true while booting

	fAdr_t	myAdr;			///< forest address of this router
	ipa_t	nmIp;			///< IP address of the netMgr
	fAdr_t	nmAdr;			///< forest address of the netMgr
	fAdr_t	ccAdr;			///< address of comtree controller

        seconds runLength; 		///< # of seconds for router to run
	high_resolution_clock::time_point tZero; ///< router start time

	uint64_t seqNum;		///< sequence number for ctl packets
	mutex	snLock;			///< lock for sequence number

	fAdr_t	firstLeafAdr;		///< first leaf address
	fAdr_t	lastLeafAdr;		///< last leaf address
	ListPair *leafAdr;		///< offsets for leaf addresses

	/// XferQ used to transfer packets from input thread to output thread.
	Quu<int> xferQ;

	IfaceTable *ift;		///< table defining interfaces
	LinkTable *lt;			///< table defining links
	ComtreeTable *ctt;		///< table of comtrees
	RouteTable  *rt;		///< table of routes
	PacketStore *ps;		///< packet buffers and headers
	StatsModule *sm;		///< class for recording statistics
	PacketLog *pktLog;		///< log for recording sample of packets
	QuManager *qm;			///< queues and link schedulers

	mutex	iftMtx;			///< lock for iface table
	mutex	ltMtx;			///< lock for link table
	mutex	cttMtx;			///< lock for comtree table
	mutex	rtMtx;			///< lock for routing table

	int	*sock;			///< vector of socket numbers
	int	maxSockNum;		///< largest socket number used

	// sub-components of the router - run as separate threads
	friend class RouterInProc;
	friend class RouterOutProc;
	friend class RouterControl;

	RouterInProc *rip;
	RouterOutProc *rop;

	// setup 
	bool	setupIfaces();
	bool	setupLeafAddresses();
	bool	setupQueues();
	bool	checkTables();
	bool	setAvailRates();
	void	addLocalRoutes();

	// manage leaf addresses
	fAdr_t	allocLeafAdr();
	bool	allocLeafAdr(fAdr_t);
	void	freeLeafAdr(fAdr_t);
	bool	validLeafAdr(fAdr_t) const;
	bool	isFreeLeafAdr(fAdr_t) const;

	uint64_t nextSeqNum();
};

/** Allocate a new leaf address.
 *  Caller is assumed to hold the LinkTable lock.
 *  @return a previously unused address in the range of assignable
 *  leaf addresses for this router, or 0 if all addresses are in use
 */
inline fAdr_t Router::allocLeafAdr() {
	int offset = leafAdr->firstOut();
	if (offset == 0) return 0;
	leafAdr->swap(offset);
	return firstLeafAdr + offset - 1;
}

/** Allocate a specified leaf address.
 *  Caller is assumed to hold the LinkTable lock.
 *  @param adr is an address in the range of assignable addresses
 *  @return true of the adr is available for use and was successfully
 *  allocated, else false
 */
inline bool Router::allocLeafAdr(fAdr_t adr) {
	int offset = (adr - firstLeafAdr) + 1;
	if (!leafAdr->isOut(offset)) return false;
	leafAdr->swap(offset);
	return true;
}

/** De-allocate a leaf address.
 *  Caller is assumed to hold the LinkTable lock.
 *  @param adr is a previously allocated address.
 */
inline void Router::freeLeafAdr(fAdr_t adr) {
	int offset = (adr - firstLeafAdr) + 1;
	if (!leafAdr->isIn(offset)) return;
	leafAdr->swap(offset);
	return;
}

/** Determine if a given address is currently assigned.
 *  Caller is assumed to hold the LinkTable lock.
 *  @param adr is a forest unicast address
 *  @return true if adr is one that had been previously assigned
 */
inline bool Router::validLeafAdr(fAdr_t adr) const {
	int offset = (adr - firstLeafAdr) + 1;
	return leafAdr->isIn(offset);
}

/** Determine if a given address is currently unassigned.
 *  Caller is assumed to hold the LinkTable lock.
 *  @param adr is a forest unicast address
 *  @return true if adr is not currently assigned.
 */
inline bool Router::isFreeLeafAdr(fAdr_t adr) const {
	int offset = (adr - firstLeafAdr) + 1;
	return leafAdr->isOut(offset);
}

} // ends namespace

#endif
