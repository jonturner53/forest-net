/** @file QuManager 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef QUMANAGER_H
#define QUMANAGER_H

#include "CommonDefs.h"
#include "UiListSet.h"
#include "Heap.h"
#include "HeapSet.h"
#include "PacketStore.h"
#include "StatsModule.h"

/** The QuManager class, manages a set of queues for each of the
 *  links in a router.
 *  
 *  This version provides a separate WDRR scheduler for each link.
 *  Queues for each link are numbered 1,2,... and each queue has
 *  quantum, which represents the number of "new" bytes an active
 *  queue may send each time it is visited by the packet scheduler.
 */
class QuManager {
public:
		QuManager(int,int,int,int,PacketStore*,StatsModule*);
		~QuManager();

	// predicates
	bool	validQ(int) const;
	
	// allocate/free queues
	int	allocQ(int);	
	void	freeQ(int);

	// set queue rates and length limits
	bool	setLinkRates(int,int,int);
	bool	setQRates(int,int,int);
	bool	setQLimits(int,int,int);

	// enq and deq packets
	bool	enq(int, int, uint64_t);
	int	deq(int&, uint64_t);	
	
private:
	int	nL;			///< number of links
	int	nP;			///< total # of packets in the system
	int	nQ;			///< number of queues per link
	int	maxppl;			///< max # of packets per link

	UiListSet *queues;		///< collection of lists of packets
	int	free;			///< first queue in the free list
	Heap	*active;		///< active queues, ordered by due time
	Heap	*vactive;		///< virtually active queues

	struct LinkInfo {		///< information on links
	uint32_t nsPerByte;		///< ns of delay per data byte
	uint32_t minDelta;		///< min # of ns between packets
	uint64_t vt;			///< virtual time for link
	};
	LinkInfo *lnkInfo;		///< lnkInfo[lnk] hold info on lnk

	struct QuInfo {
	int	lnk;			///< link that queue is assigned to
	uint32_t nsPerByte;		///< ns of delay per data byte
	uint32_t minDelta;		///< min # of ns between packets
	int	pktLim;			///< limit on # of packets in queue
	int	byteLim;		///< limit on # of bytes in queue
	uint64_t vft;			///< virtual finish time for queue
	};
	QuInfo	 *quInfo;		///< quInfo[q] is information for q

	HeapSet *hset;			///< set of heaps for pkt scheduler

	PacketStore *ps;		///< pointer to packet store object
	StatsModule *sm;		///< pointer to statistics module
};

inline bool QuManager::validQ(int qid) const {
	return 1 <= qid && qid <= nQ && quInfo[qid].pktLim >= 0;
}

inline bool QuManager::setLinkRates(int lnk, int br, int pr) {
	if (lnk < 1 || lnk > nL) return false;
	br = max(br,1); 	pr = max(pr,1);
	br = min(br,8000000); 	pr = min(pr,1000000000);
	lnkInfo[lnk].nsPerByte =   8000000/br;
	lnkInfo[lnk].minDelta = 1000000000/pr;
	return true;
}

inline bool QuManager::setQRates(int qid, int br, int pr) {
	if (!validQ(qid)) return false;
	br = min(br,8000000); pr = min(pr,1000000000);
	quInfo[qid].nsPerByte =   8000000/br;
	quInfo[qid].minDelta = 1000000000/pr;
	return true;
}

inline bool QuManager::setQLimits(int qid, int np, int nb) {
	if (!validQ(qid)) return false;
	np = max(0,np); nb = max(0,nb);
	quInfo[qid].pktLim = np;
	quInfo[qid].byteLim = nb;
	return true;
}

#endif
