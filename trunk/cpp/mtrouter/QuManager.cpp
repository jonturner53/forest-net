/** @file QuManager 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

/*
Implementation Notes
We maintain two heaps defined on the links. The active heap
contains links for which packets are currently queued. The
vactive heap contains links that are "virtually active", meaning
that they recently sent their last packet, but they are not
yet allowed to send their next, given the limit on their bit
rate and packet rate. If a packet arrives for a virtually active
link, it is moved to the active heap with a deadline inherited
from the vactive heap.

It's important that links be moved off the vactive heap when
they become eligible to send new packets. This is done in the
nextReady routine, which is expected to be called frequently
(e.g. once on each iteration of the router's main loop).

In addition to the active and vactive heaps, there is a HeapSet
data structure that contains a heap for each link. These heaps
contain queues and the key of a queue in its heap, is the
virtual finish time of the queue in a Self-Clocked Fair Queueing
packet scheduler.
*/

#include "QuManager.h"

namespace forest {


// Constructor for QuManager, allocates space and initializes everything.
QuManager::QuManager(int nL1, int nP1, int nQ1, int maxppl1, PacketStore *ps1)
	   	    : nL(nL1), nP(nP1), nQ(nQ1), maxppl(maxppl1), ps(ps1) {
	queues = new ListSet(nP,nQ);
	active = new Dheap<uint64_t>(nL);  
	vactive = new Dheap<uint64_t>(nL);
	quInfo = new QuInfo[nQ+1];
	lnkInfo = new LinkInfo[nL+1]; 
	hset = new DheapSet<uint64_t>(nQ,nL);

	RateSpec rs(Forest::MINBITRATE,Forest::MINBITRATE,
		    Forest::MINPKTRATE,Forest::MINPKTRATE);
	for (int lnk = 1; lnk <= nL; lnk++) {
		lnkInfo[lnk].vt = 0; setLinkRates(lnk, rs);
	}

	// build free list of queues using lnk field
	for (int qid = 1; qid < nQ; qid++) {
		quInfo[qid].lnk = qid+1;
		quInfo[qid].pktLim = -1; // used to identify unassigned queues
		quInfo[qid].vft = 0;
	}
	quInfo[nQ].lnk = 0; free = 1;
	qCnt = 0;
}
		
QuManager::~QuManager() {
	delete queues; delete active; delete vactive;
	delete [] quInfo; delete [] lnkInfo; delete hset;
}

/** Allocate a queue and assign it to a link.
 *  @param lnk is the link to be assigned a new queue.
 *  @return the qid of the assigned queue, or 0 no queues are available
 */
int QuManager::allocQ(int lnk) {

	unique_lock<mutex> lck(mtx);
	if (free == 0) return 0;
	int qid = free; free = quInfo[qid].lnk;
	quInfo[qid].lnk = lnk;
	quInfo[qid].pktLim = 0; // non-negative value for assigned queues
	quInfo[qid].pktCount = 0; quInfo[qid].byteCount = 0;
	qCnt++;
	return qid;
}

/** Free a queue.
 *  If the specified queue is empty, it is immediately returned to
 *  the free list. Otherwise, it is marked so that no new packets
 *  can be queued. When all existing packets have been sent, the
 *  queue will be returned to the free list.
 */
void QuManager::freeQ(int qid) {
	unique_lock<mutex> lck(mtx);
	if (qid == 0) return;
	if (queues->empty(qid)) {
		quInfo[qid].lnk = free; free = qid; qCnt--;
	} 
	quInfo[qid].pktLim = -1; // negative value for free queues
}

/** Enqueue a packet.
 *  If the queue is full or the link has reached the maximum allowed,
 *  the packet is silently discarded.
 *  @param p is the packet number of the packet to be queued
 *  @param q is the the qid for the queue for the packet
 *  @param now is the current time
 */
void QuManager::enq(int px, int qid, uint64_t now) {
	//unique_lock<mutex> lck(mtx);
	if (px == 0 || qid < 0 || qid > nQ || quInfo[qid].pktLim < 0)
		return;
	QuInfo& q = quInfo[qid]; int lnk = q.lnk;
	int pleng = Forest::truPktLeng((ps->getPacket(px)).length);

	// don't queue it if too many packets for link
	// or if queue is past its limits
	if (lnkInfo[lnk].pktCount >= maxppl ||
	    q.pktCount >= q.pktLim || q.byteCount + pleng > q.byteLim) {
		return;
	}

	if (queues->empty(qid)) {
		// make link active if need be
		if (!active->member(lnk)) {
			uint64_t d;
			if (vactive->member(lnk)) {
				d = vactive->key(lnk);
				if (now >= d) d = now;
				vactive->remove(lnk);
			} else {
				d = now;
				lnkInfo[lnk].avgPktTime = lnkInfo[lnk].minDelta;
			}
			active->insert(lnk,d);
		}
		// set virtual finish time of queue
		uint64_t d = q.nsPerByte; d *= pleng;
		if (q.minDelta > d) d = q.minDelta;
		q.vft = max(q.vft, lnkInfo[lnk].vt) + d;

		// add queue to scheduling heap for link
		if (!hset->insert(qid,q.vft,lnk)) {
			cerr << "enq attempt to insert in hset failed qid="
			     << qid << " lnk=" << lnk << " hset="
			     << hset->toString(lnk);
		}
	} 

	// add packet to queue
	queues->addLast(px,qid);
	lnkInfo[lnk].pktCount++; q.pktCount++; q.byteCount += pleng;
	return;
}

/** Dequeue the next packet that is ready to go out.
 *  @param now is the current time
 *  @param lnk is a reference argument; on a successful return,
 *  it is set to the number of the link on which the packet should be sent
 *  @return the packet number of the packet to be sent, or 0 if there
 *  are no links that are ready to send a packet
 */
int QuManager::deq(int& lnk, uint64_t now) {
	//unique_lock<mutex> lck(mtx);
	// first process virtually active links that should now be idle
	int vl = vactive->findmin(); uint64_t d = vactive->key(vl);
	while (vl != 0 && now >= d) {
		vactive->remove(vl);
		vl = vactive->findmin(); d = vactive->key(vl);
	}
	// determine next active link that is ready to send
	if (active->empty()) return 0;
	lnk = active->findmin(); d = active->key(lnk);
	if (now < d) return 0;

	// dequeue packet and update statistics
	int qid = hset->findMin(lnk);
	pktx px = queues->removeFirst(qid);
	int pleng = Forest::truPktLeng(ps->getPacket(px).length);

	// and update scheduling heap and virtual time of lnk
	QuInfo& q = quInfo[qid];
	lnkInfo[lnk].pktCount--; q.pktCount--; q.byteCount -= pleng;
	lnkInfo[lnk].vt = q.vft;
	if (queues->empty(qid)) {
		hset->deleteMin(lnk);
		if (q.pktLim < 0) {
			// move queue to the free list
			q.lnk = free; free = qid; qCnt--;
		}
	} else {
		int npx = queues->first(qid);
		Packet& np = ps->getPacket(npx);
		int npleng = Forest::truPktLeng(np.length);
		uint64_t d = q.nsPerByte; d *= npleng;
		if (q.minDelta > d) d = q.minDelta;
		q.vft += d;
		hset->changeKeyMin(q.vft, lnk);
	}

	// update the time when lnk can send its next packet
	uint64_t t = lnkInfo[lnk].nsPerByte; t *= pleng;
	lnkInfo[lnk].avgPktTime = (t/16) + (15*(lnkInfo[lnk].avgPktTime/16));
	if (lnkInfo[lnk].avgPktTime < lnkInfo[lnk].minDelta &&
	    t < lnkInfo[lnk].minDelta)
		t = lnkInfo[lnk].minDelta;
	t += active->key(lnk);

	if (hset->empty(lnk)) {
		active->remove(lnk);
		vactive->insert(lnk,t);
	} else {
		active->changekey(lnk,t);
	}
	
	return px;
}

} // ends namespace

