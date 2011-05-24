/** \file QuManager */

/*
Implementation Notes

We represent time using 32 bit values and use 1 us resolution.
This allows for 4000 seconds of time, so more than an hour.
We assume that each link will send at least one packet every
30 minutes, a safe assumption if link speeds are at least 1 Kb/s,
AND if neighbors' links are constrained so they can't send us
packets faster than the max rate at which we can forward them.
This allows us to use circular time values that wrap around.
If a, b are distinct time values, we interpret a to be greater
than b if (a-b) < 2^31.

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
*/

#include "QuManager.h"

// Constructor for QuManager, allocates space and initializes everything.
QuManager::QuManager(int nL1, int nP1, int nQ1, int qL1,
		     PacketStore *ps1, LinkTable *lt1)
	   	    : nL(nL1), nP(nP1), nQ(nQ1), qL(qL1), ps(ps1), lt(lt1) {
	int lnk, q, qid;

	queues = new UiListSet(nP,nL*nQ);
	active = new ModHeap(nL,4,true);  // active is also a min-heap
	vactive = new ModHeap(nL,4,true);  // vactive is a min-heap 
	npq = new int[nL+1]; nbq = new int[nL+1];

	for (lnk = 1; lnk <= nL; lnk++) {
		npq[lnk] = 0; nbq[lnk] = 0;
	}

	pSched = new UiDlist*[nL+1];
	cq = new int[nL+1];
	qStatus = new qStatStruct[nL*nQ+1];
	for (lnk = 1; lnk <= nL; lnk++) {
		pSched[lnk] = new UiDlist(nQ);
		cq[lnk] = 0; 
		for (q = 1; q <= nQ; q++) {
			qid = (lnk-1)*nQ+q;
			qStatus[qid].quantum = 100; // default value
			qStatus[qid].credits = 0;
			qStatus[qid].np = qStatus[qid].nb = 0;
			qStatus[qid].pktLim = qL;	// default packet limit
			qStatus[qid].byteLim = qL*1600;	// default byte limit
		}
	}
}
		
QuManager::~QuManager() {
	delete queues; delete active; delete [] npq; delete [] nbq;
	delete [] pSched; delete [] cq; delete [] qStatus;
}

bool QuManager::enq(int p, int lnk, int q, uint32_t now) {
// Enqueue packet on specified queue for lnk. Return true if
// packet was successfully queued, else false. Note, it
// is up to the caller to discard packets that are not queued.
	int pleng = Forest::truPktLeng((ps->getHeader(p)).getLength());
	int qid = (lnk-1)*nQ + q; // used for accessing queues, qStatus
	qStatStruct *qs = &qStatus[qid];

	// don't queue it if too many packets for link
	// or if queue is past its limits
	if (npq[lnk] >= qL || qs->np >= qs->pktLim ||
			      qs->nb + pleng > qs->byteLim) {
		return false;
	}

	// update queue and packet scheduler
	if (queues->empty(qid)) {
		pSched[lnk]->addLast(q);
		if (q == pSched[lnk]->get(1)) {
			cq[lnk] = q;
			qs->credits = qs->quantum;
			// update heap of active links
			uint32_t d = now;
			if (vactive->member(lnk)) {
				d = vactive->key(lnk);
				if ((now - d) <= (1 << 31))  // now >= d
					d = now;
				vactive->remove(lnk);
			}
			active->insert(lnk,d);
		} else {
			qs->credits = 0;
		}
	}
	queues->addLast(p,qid);
	qs->np++; qs->nb += pleng;
	npq[lnk]++; nbq[lnk] += pleng;
	return true;
}

int QuManager::deq(int lnk) {
// Find the next queue for the given link that has sufficient credits
// to send its next packet. Add quantum credits for each queue that
// is considered but does not have enough credits now.
// Once a queue with sufficient credits is found, remove and return
// its first packet. Update all data structures appropriately.
	int q = cq[lnk];
	int qid = (lnk-1)*nQ + q; // used for accessing queues, qstatus
	qStatStruct *qs = &qStatus[qid];
	int p = queues->first(qid); PacketHeader h = ps->getHeader(p);

	// if current queue has too few credits, advance to next queue
	while (qs->credits < h.getLength()) { 
		cq[lnk] = pSched[lnk]->next(q) != 0 ?
				pSched[lnk]->next(q) : pSched[lnk]->get(1);
		q = cq[lnk]; qid = (lnk-1)*nQ + q; qs = &qStatus[qid];
		qs->credits += qs->quantum;
		p = queues->first(qid); h = ps->getHeader(p);
	}
	// Now, current queue has enough credit, so can deque is first
	// packet, update credits, pSched and heaps
	p = queues->removeFirst(qid); h = ps->getHeader(p);
	int pleng = Forest::truPktLeng(h.getLength());
	qs->credits -= pleng;
	qs->np--; qs->nb -= pleng;
	npq[lnk]--; nbq[lnk] -= pleng;

	if (queues->empty(qid)) {
		cq[lnk] = pSched[lnk]->next(q) != 0 ?
				pSched[lnk]->next(q) : pSched[lnk]->get(1);
		pSched[lnk]->remove(q);
		qs = &qStatus[(lnk-1)*nQ+cq[lnk]];
		qs->credits += qs->quantum;
	}

	// update heaps for lnk
	uint32_t d;
	d = (pleng*8000)/lt->getBitRate(lnk);
	d = max(d,lt->getMinDelta(lnk));
	d += active->key(lnk);
	if (pSched[lnk]->empty()) {
		vactive->insert(lnk,d);
		active->remove(lnk);
		cq[lnk] = 0;
	} else {
		active->changekey(lnk,d);
	}

	return p;
}


int QuManager::nextReady(uint32_t now) {
// Return the index of the next link that is ready to send, or 0 if
// none is ready to send. This routine also checks to see if any of
// the links in the vactive heap can be removed.
	// remove vactive links whose times are now past
	int lnk = vactive->findmin(); uint32_t d = vactive->key(lnk);
	while (lnk != 0 && (now - d) <= (1 << 31)) {
		vactive->remove(lnk);
		lnk = vactive->findmin(); d = vactive->key(lnk);
	}
	// determine next active link that is ready to send
	if (active->empty()) return 0;
	lnk = active->findmin(); d = active->key(lnk);
	if ((now - d) <= (1 << 31)) return lnk;
	return 0;
}

void QuManager::write(int lnk, int q) const {
// Print the packets in the specified queue
	cout << "[" << lnk << "," << q << "] ";
	queues->write(cout, (lnk-1)*nQ+q);
}

void QuManager::write() const {
// Print the active heap and status of all active links
	int lnk, q, qid; qStatStruct *qs;

	active->write(cout);
	for (lnk = 1; lnk <= nL; lnk++) {
		if (pSched[lnk]->empty()) continue;
		cout << "link " << lnk << ": ";
		pSched[lnk]->write(cout);
		cout << " | " << cq[lnk] << endl;
		for (q = pSched[lnk]->get(1); q != 0; q = pSched[lnk]->next(q)){
			qid = (lnk-1)*nQ + q; qs = &qStatus[qid];
			cout << "queue " << q << "(" << qs->quantum << ","
			     << qs->credits << ") ";
			write(lnk,q);
		}
	}
	cout << endl;
}
