/** @file RouterOutProc.cpp 
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RouterOutProc.h"

using namespace forest;

namespace forest {

/** Constructor for RouterOutProc.
 *  @param rp1 is pointer to main router module, giving output processor
 *  access to all router variables and data structures
 */
RouterOutProc::RouterOutProc(Router *rtr1) : rtr(rtr1) {
	ift = rtr->ift; lt = rtr->lt;
	ps = rtr->ps; qm = rtr->qm; pktLog = rtr->pktLog;
}

RouterOutProc::~RouterOutProc() {
}

/** Start input processor.
 *  Start() is a static method used to initiate a separate thread.
 */
void RouterOutProc::start(RouterOutProc *self) { self->run(); }

/** Main output processing loop.
 *
 *  @param finishTime is the number of microseconds to run before stopping;
 *  if it is zero, the router runs without stopping (until killed)
 */
void RouterOutProc::run() {
        unique_lock<mutex>  ltLock( rtr->ltMtx,defer_lock);

	nanoseconds temp = high_resolution_clock::now() - rtr->tZero;
	now = temp.count(); // time since router started running
	int64_t runTime = nanoseconds(rtr->runLength).count();
	int64_t finishTime = now + runTime;
	while (runTime == 0 || now < finishTime) {
		// update time
		nanoseconds temp = high_resolution_clock::now() - rtr->tZero;
		now = temp.count();

		bool didNothing = true;

		pktx px;
		// process packet from transfer queue, if any
		if (!rtr->xferQ.empty()) {
			didNothing = false;
			px = rtr->xferQ.deq();
			Packet& p = ps->getPacket(px);
			uint32_t* buf = (uint32_t*) p.buffer;
			if (p.outQueue != 0) {
				qm->enq(px,p.outQueue,now);
			} else if (buf[1500] == 0) {
				ps->free(px);
			} else {
				int i = 1500;
				while (buf[i+1] != 0) {
					// not yet the last copy
					int cx = ps->clone(px);
					qm->enq(cx,buf[i],now);
					i++;
				}
				// and finally, enqueue p itself
				qm->enq(px,buf[i],now);
			}
		}

		// output processing
		int lnk;
		ltLock.lock();
		while ((px = qm->deq(lnk, now)) != 0) {
			didNothing = false;
			pktLog->log(px,lnk,true,now);
			send(px,lnk);
		}
		ltLock.unlock();

		// if did nothing on that pass, sleep for a millisecond.
		if (didNothing) 
			this_thread::sleep_for(chrono::milliseconds(1));
	}

	// write out recorded events
	pktLog->write(cout);
	ltLock.lock();
	StatCounts rtrStats, leafStats;
	lt->getStats(rtrStats, leafStats);
	cout << endl;
	cout << rtrStats.pktsIn + leafStats.pktsIn << " packets received, "
	     <<	rtrStats.pktsOut + leafStats.pktsOut << " packets sent\n";
	cout << rtrStats.pktsIn << " from routers,    "
	     << rtrStats.pktsOut << " to routers\n";
	cout << leafStats.pktsIn << " from clients,    "
	     << leafStats.pktsOut << " to clients\n";
}

/** Send packet on specified link and recycle storage.
 *  @param px is the packet index of the outgoing packet
 *  @param lnk is its link number
 */
void RouterOutProc::send(pktx px, int lnk) {
	Packet p = ps->getPacket(px);
	LinkTable::Entry& lte = lt->getEntry(lnk);
	if (lte.peerIp == 0 || lte.peerPort == 0) {
		ps->free(px); return;
	}
	int rv, lim = 0;
	unique_lock<mutex> iftLock(rtr->iftMtx);
	int sock = rtr->sock[lte.iface];
	iftLock.unlock();
	do {
		rv = Np4d::sendto4d(sock, (void *) p.buffer, p.length,
				    lte.peerIp, lte.peerPort);
	} while (rv == -1 && errno == EAGAIN && lim++ < 10);
	if (rv == -1) {
		perror("RouterOutProc::send: failure in sendto");
		exit(1);
	}
	lt->countOutgoing(lnk,Forest::truPktLeng(p.length));
	ps->free(px);
}

} // ends namespace
