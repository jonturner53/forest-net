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

int i1=0, i2=0, i3=0, i4=0;
high_resolution_clock::time_point t1, t2, t3, t4;
nanoseconds d1(0), d2(0), d3(0), d4(0);
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

t1 = high_resolution_clock::now();
		pktx px = rtr->xferQ.deq();
		// process packet from transfer queue, if any
		if (px != 0) {
d1 += high_resolution_clock::now() - t1; i1++;
			didNothing = false;

			Packet& p = ps->getPacket(px);
			uint32_t* buf = (uint32_t*) p.buffer;
			if (p.outQueue != 0) {
t2 = high_resolution_clock::now();
				qm->enq(px,p.outQueue,now);
d2 += high_resolution_clock::now() - t2; i2++;
			} else if (buf[1500] == 0) {
				ps->free(px);
			} else {
t2 = high_resolution_clock::now();
				int i = 1500;
				while (buf[i+1] != 0) {
					// not yet the last copy
					int cx = ps->clone(px);
					qm->enq(cx,buf[i],now);
					i++;
				}
				// and finally, enqueue p itself
				qm->enq(px,buf[i],now);
d2 += high_resolution_clock::now() - t2; i2++;
			}
		}

		// output processing
		int lnk;
		//ltLock.lock();
t3 = high_resolution_clock::now();
		if ((px = qm->deq(lnk, now)) != 0) {
d3 += high_resolution_clock::now() - t3; i3++;
			didNothing = false;
			//pktLog->log(px,lnk,true,now);
t4 = high_resolution_clock::now();
			send(px,lnk);
d4 += high_resolution_clock::now() - t4; i4++;
		}
		//ltLock.unlock();

		// if did nothing on that pass, sleep for a millisecond.
		//if (didNothing) 
		//	this_thread::sleep_for(chrono::milliseconds(1));
	}

this_thread::sleep_for(chrono::seconds(2));
cerr << "from xferQ: " << i1 << " " << (d1.count()/i1) << endl;
cerr << "       enq: " << i2 << " " << (d2.count()/i2) << endl;
cerr << "       deq: " << i3 << " " << (d3.count()/i3) << endl;
cerr << "      send: " << i4 << " " << (d4.count()/i4) << endl;

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
	Packet& p = ps->getPacket(px);
	LinkTable::Entry& lte = lt->getEntry(lnk);
	if (lte.peerIp == 0 || lte.peerPort == 0) {
		ps->free(px); return;
	}
	int rv, lim = 0;
	//unique_lock<mutex> iftLock(rtr->iftMtx);
	int sock = rtr->sock[lte.iface];
	//iftLock.unlock();
	do {
		rv = Np4d::sendto4d(sock, (void *) p.buffer, p.length, lte.sa);
	} while (rv == -1 && errno == EAGAIN && lim++ < 10);
if (lim > 0) { cerr << "lim=" << lim << endl; exit(1); }
	if (rv == -1) {
		perror("RouterOutProc::send: failure in sendto");
		exit(1);
	}
	//lt->countOutgoing(lnk,Forest::truPktLeng(p.length));
	ps->free(px);
}

} // ends namespace
