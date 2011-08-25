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

// Constructor for QuManager, allocates space and initializes everything.
QuManager::QuManager(int nL1, int nP1, int nQ1, int maxppl1,
		     PacketStore *ps1, StatsModule *sm1)
	   	    : nL(nL1), nP(nP1), nQ(nQ1),
		      maxppl(maxppl1), ps(ps1), sm(sm1) {
	queues = new UiListSet(nP,nQ);
	active = new Heap(nL);  
	vactive = new Heap(nL);
	quInfo = new QuInfo[nQ+1];
	lnkInfo = new LinkInfo[nL+1]; 
	hset = new HeapSet(nQ,nL);

	for (int lnk = 1; lnk <= nL; lnk++) {
		lnkInfo[lnk].vt = 0; setLinkRates(lnk, 1, 1);
	}

	// build free list of queues using lnk field
	for (int qid = 1; qid < nQ; qid++) {
		quInfo[qid].lnk = qid+1;
		quInfo[qid].pktLim = -1; // used to identify unassigned queues
	}
	quInfo[nQ].lnk = 0; free = 1;
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
	if (free == 0) return 0;
	int qid = free; free = quInfo[qid].lnk;
	quInfo[qid].lnk = lnk;
	quInfo[qid].pktLim = 0; // non-negative value for assigned queues
	return qid;
}

/** Free a queue.
 *  If the specified queue is empty, it is immediately returned to
 *  the free list. Otherwise, it is marked so that no new packets
 *  can be queued. When all existing packets have been sent, the
 *  queue will be returned to the free list.
 */
void QuManager::freeQ(int qid) {
	if (queues->empty(qid)) {
		quInfo[qid].lnk = free; free = qid;
	} 
	quInfo[qid].pktLim = -1; // negative value for free queues
}

/** Enqueue a packet.
 *  @param p is the packet number of the packet to be queued
 *  @param q is the the qid for the queue for the packet
 *  @param now is the current time
 *  @return true on success, false on failure; the operation can fail
 *  if the qid is invalid, if the queue is already full, or the link
 *  is at its limit for packets queued
 */
bool QuManager::enq(int p, int qid, uint64_t now) {
	int pleng = Forest::truPktLeng((ps->getHeader(p)).getLength());
	QuInfo& q = quInfo[qid]; int lnk = q.lnk;

	if (quInfo[qid].pktLim == -1) return false;

	// don't queue it if too many packets for link
	// or if queue is past its limits
	if (sm->getLinkQlen(lnk) >= maxppl ||
	    sm->getQlen(qid) >= q.pktLim ||
	    sm->getQbytes(qid) + pleng > q.byteLim) {
		return false;
	}

	if (queues->empty(qid)) {
		// make link active if need be
		if (!active->member(lnk)) {
			uint32_t d;
			if (vactive->member(lnk)) {
				d = vactive->key(lnk);
				if ((now - d) <= (1 << 31))  // now >= d
					d = now;
				vactive->remove(lnk);
			} else d = now;
			active->insert(lnk,d);
		}
		// set virtual finish time of queue
		uint64_t d = q.nsPerByte; d *= pleng;
		if (q.minDelta > d) d = q.minDelta;
		q.vft = max(q.vft, lnkInfo[lnk].vt) + d;

		// add queue to scheduling heap for link
		hset->insert(qid,q.vft,lnk);
	} 

	// add packet to queue
	queues->addLast(p,qid);
	sm->incQlen(qid,lnk,pleng);
	return true;
}

/** Dequeue the next packet that is ready to go out.
 *  @param now is the current time
 *  @param lnk is a reference argument; on a successful return,
 *  it is set to the number of the link on which the packet should be sent
 *  @return the packet number of the packet to be sent, or 0 if there
 *  are no links that are ready to send a packet
 */
int QuManager::deq(int& lnk, uint64_t now) {
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
	int p = queues->removeFirst(qid);
	int pleng = Forest::truPktLeng(ps->getHeader(p).getLength());
	sm->decQlen(qid,lnk,pleng);

	// and update scheduling heap and virtual time of lnk
	QuInfo& q = quInfo[qid];
	lnkInfo[lnk].vt = q.vft;
	if (queues->empty(qid)) {
		hset->deleteMin(lnk);
		if (q.pktLim < 0) {
			// move queue to the free list
			q.lnk = free; free = qid;
		}
	} else {
		int np = queues->first(qid);
		int npleng = Forest::truPktLeng(ps->getHeader(np).getLength());
		uint64_t d = q.nsPerByte; d *= npleng;
		if (q.minDelta > d) d = q.minDelta;
		q.vft += d;
		hset->changeKeyMin(q.vft, lnk);
	}

	// update the time when lnk can send its next packet
	uint64_t t = lnkInfo[lnk].nsPerByte; t *= pleng;
	if (lnkInfo[lnk].minDelta > t) t = lnkInfo[lnk].minDelta;
	t += active->key(lnk);

	if (hset->empty(lnk)) {
		active->remove(lnk);
		vactive->insert(lnk,t);
	} else {
		active->changekey(lnk,t);
	}

	return p;
}
