// Header file for qMgr class, which manages a set of queues.
// links in the system.
// 
// This version provides a separate WDRR scheduler for each link.
// Queues for each link are numbered 1,2,... and each queue has
// quantum, which represents the number of "new" bytes an active
// queue may send each time it is visited by the packet scheduler.

#ifndef QMGR_H
#define QMGR_H

#include "forest.h"
#include "UiDlist.h"
#include "UiListSet.h"
#include "ModHeap.h"
#include "lnkTbl.h"
#include "pktStore.h"

class qMgr {
public:
		qMgr(int,int,int,int,pktStore*,lnkTbl*);
		~qMgr();

	bool	enq(int,int,int,uint32_t); // add packet on link queue
	int	deq(int);		// deq packet from given link
	int	nextReady(uint32_t);	// index of next ready link
	int	qlenPkts(int) const;	// return # of packets queued for link
	int	qlenBytes(int) const;	// return # of bytes queued for link
	int	qlenPkts(int,int) const;// return # of packets in queue
	int	qlenBytes(int,int) const;// return # of bytes in queue
	int&	quantum(int,int);	// return quantum for (link,queue)
	void	print(int,int) const;	// print contents of a (link,queue)
	void	print() const;		// print (almost) everything
private:
	int	nL;			// number of links
	int	nP;			// total number of packets in the system
	int	nQ;			// number of queues per link
	int	qL;			// maximum queue length (in packets)

	UiListSet *queues;		// collection of lists of packets
	ModHeap	*active;		// active links, ordered by due time
	ModHeap	*vactive;		// virtually active links
	int	*npq;			// npq[L] = number of packets for link L
	int	*nbq;			// nbq[L] = number of bytes for link L

	UiDlist	**pSched;		// pSched[L] non-empty queues for link L
	int	*cq;			// cq[L] is current queue for link L
	struct qStatStruct {
		int quantum;		// quantum used in scheduling
		int credits;		// unused credits
		int np, nb;		// number of packets, bytes in queue
		int pktLim;		// limit on # of packets in queue
		int byteLim;		// limit on # of bytes in queue
	 } *qStatus;			// qStatus[L][q] (link L, queue q)

	pktStore *ps;			// pointer to packet store object
	lnkTbl *lt;			// pointer to link table object
};

inline int qMgr::qlenPkts(int L) const { return npq[L]; }
inline int qMgr::qlenBytes(int L) const { return nbq[L]; }
inline int qMgr::qlenPkts(int L, int q) const {
	return (q == 0 ? qlenPkts(L) : qStatus[(L-1)*nQ+q].np);
}
inline int qMgr::qlenBytes(int L, int q) const {
	return (q == 0 ? qlenBytes(L) : qStatus[(L-1)*nQ+q].nb);
}

inline int& qMgr::quantum(int L, int q) {
	return qStatus[(L-1)*nQ+q].quantum;
}

#endif
