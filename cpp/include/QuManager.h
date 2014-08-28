/** @file QuManager 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef QUMANAGER_H
#define QUMANAGER_H

#include <chrono>
#include <thread>
#include <mutex>

#include "Forest.h"
/** Output operator for high resolution time point.
 *  Needed by Dheap to print heap contents.
 */
inline ostream& operator<<(ostream& out,
	const chrono::high_resolution_clock::time_point& t) {
        return out << t.time_since_epoch().count();
}

#include "ListSet.h"
#include "Dheap.h"
#include "DheapSet.h"
#include "PacketStore.h"
#include "RateSpec.h"

using namespace chrono;
using std::thread;
using std::mutex;
using std::unique_lock;

namespace forest {


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
		QuManager(int,int,int,int,PacketStore*);
		~QuManager();

	// predicates
	bool	validQ(int);
	
	// allocate/free queues
	int	allocQ(int);	
	void	freeQ(int);

	int	getLink(int) const;

	// set queue rates and length limits
	bool	setLinkRates(int,RateSpec&);
	bool	setQRates(int,RateSpec&);
	bool	setQLimits(int,int,int);
	void 	getStats(int, int, int&, int&, int&);

	// enq and deq packets
	void	enq(int, int, uint64_t);
	int	deq(int&, uint64_t);
	
private:
	int	nL;			///< number of links
	int	nP;			///< total # of packets in the system
	int	nQ;			///< number of queues per link
	int	maxppl;			///< max # of packets per link
	int	qCnt;			///< number of allocated queues

	ListSet *queues;		///< collection of lists of packets
	int	free;			///< first queue in the free list
	Dheap<uint64_t> *active;	///< active queues, ordered by due time
	Dheap<uint64_t> *vactive;	///< virtually active queues

	mutex	mtx;			///< to make all operations atomic

	struct LinkInfo {		///< information on links
	uint32_t nsPerByte;		///< ns of delay per data byte
	uint32_t minDelta;		///< min # of ns between packets
	uint64_t avgPktTime;		///< average time to send recent packets
	uint64_t vt;			///< virtual time for link
	int	pktCount;		///< number of packets queued for link
	};
	LinkInfo *lnkInfo;		///< lnkInfo[lnk] hold info on lnk

	struct QuInfo {
	int	lnk;			///< link that queue is assigned to
	uint32_t nsPerByte;		///< ns of delay per data byte
	uint32_t minDelta;		///< min # of ns between packets
	int	pktCount;		///< number of packets in queue
	int	byteCount;		///< number of bytes in queue
	int	pktLim;			///< limit on # of packets in queue
	int	byteLim;		///< limit on # of bytes in queue
	uint64_t vft;			///< virtual finish time for queue
	};
	QuInfo	 *quInfo;		///< quInfo[q] is information for q

	DheapSet<uint64_t> *hset;	///< set of heaps for pkt scheduler

	PacketStore *ps;		///< pointer to packet store object
};


inline bool QuManager::validQ(int qid) {
	unique_lock<mutex> lck(mtx);
	return 1 <= qid && qid <= nQ && quInfo[qid].pktLim >= 0;
}

inline int QuManager::getLink(int qid) const { return quInfo[qid].lnk; }

inline bool QuManager::setLinkRates(int lnk, RateSpec& rs) {
	unique_lock<mutex> lck(mtx);
	if (lnk < 1 || lnk > nL) return false;
	int br = max(rs.bitRateDown,1); 
	int pr = max(rs.pktRateDown,1); 
	br = min(br,8000000); 	pr = min(pr,1000000000);
	lnkInfo[lnk].nsPerByte =   8000000/br;
	lnkInfo[lnk].minDelta = 1000000000/pr;
	return true;
}

inline bool QuManager::setQRates(int qid, RateSpec& rs) {
	if (!validQ(qid)) return false;
	unique_lock<mutex> lck(mtx);
	int br = min(rs.bitRateDown,8000000); 
	int pr = min(rs.pktRateDown,1000000000); 
	quInfo[qid].nsPerByte =   8000000/br;
	quInfo[qid].minDelta = 1000000000/pr;
	return true;
}

inline bool QuManager::setQLimits(int qid, int np, int nb) {
	if (!validQ(qid)) return false;
	unique_lock<mutex> lck(mtx);
	np = max(0,np); nb = max(0,nb);
	quInfo[qid].pktLim = np;
	quInfo[qid].byteLim = nb;
	return true;
}

/** Sample the statistics counters.
 *  @param lnk is a link number
 *  @param qid is a queue identifier
 *  @param lnkPktCount is a reference used to return the value of the packet
 *  count for the specified link; if lnk == 0, the packet count for the
 *  link associated with qid is returned
 *  @param qPktCount is a reference used to return the value of the packet
 *  count for the specified queue
 *  @param qByteCount is a reference used to return the value of the byte
 *  count for the specified queue
 */
inline void QuManager::getStats(int lnk, int qid, int& lnkPktCount,
				int& qPktCount, int& qByteCount) {
	unique_lock<mutex> lck(mtx);
	lnkPktCount = (lnk == 0 ? lnkInfo[quInfo[qid].lnk].pktCount :
				  lnkInfo[lnk].pktCount);
	qPktCount = quInfo[qid].pktCount;
	qByteCount = quInfo[qid].byteCount;
}

} // ends namespace


#endif
