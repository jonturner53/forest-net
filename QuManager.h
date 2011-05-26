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
#include "UiDlist.h"
#include "UiListSet.h"
#include "ModHeap.h"
#include "LinkTable.h"
#include "PacketStore.h"

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
		QuManager(int,int,int,int,PacketStore*,LinkTable*);
		~QuManager();

	/** getters */
	int	getLengthPkts(int) const;
	int	getLengthBytes(int) const;
	int	getLengthPkts(int,int) const;
	int	getLengthBytes(int,int) const;
	int	getQuantum(int,int) const;

	/** setters */
	void	setQuantum(int,int,int);

	/** modifiers */
	bool	enq(int,int,int,uint32_t);
	int	deq(int);	
	int	nextReady(uint32_t);	

	/** input/output */
	void	write(int,int) const;
	void	write() const;	
private:
	int	nL;			///< number of links
	int	nP;			///< total # of packets in the system
	int	nQ;			///< number of queues per link
	int	qL;			///< maximum queue length (in packets)

	UiListSet *queues;		///< collection of lists of packets
	ModHeap	*active;		///< active links, ordered by due time
	ModHeap	*vactive;		///< virtually active links
	int	*npq;			///< npq[L] = # of packets for link L
	int	*nbq;			///< nbq[L] = # of bytes for link L

	UiDlist	**pSched;		///< pSched[L] non-empty queues for L
	int	*cq;			///< cq[L] is current queue for L
	struct qStatStruct {
		int quantum;		///< quantum used in scheduling
		int credits;		///< unused credits
		int np, nb;		///< number of packets, bytes in queue
		int pktLim;		///< limit on # of packets in queue
		int byteLim;		///< limit on # of bytes in queue
	 } *qStatus;			///< qStatus[L][q] (link L, queue q)

	PacketStore *ps;		///< pointer to packet store object
	LinkTable *lt;			///< pointer to link table object
};

inline int QuManager::getLengthPkts(int L) const { return npq[L]; }
inline int QuManager::getLengthBytes(int L) const { return nbq[L]; }
inline int QuManager::getLengthPkts(int L, int q) const {
	return (q == 0 ? getLengthPkts(L) : qStatus[(L-1)*nQ+q].np);
}
inline int QuManager::getLengthBytes(int L, int q) const {
	return (q == 0 ? getLengthBytes(L) : qStatus[(L-1)*nQ+q].nb);
}

inline int QuManager::getQuantum(int L, int q) const {
	return qStatus[(L-1)*nQ+q].quantum;
}

inline void QuManager::setQuantum(int L, int q, int quant) {
	qStatus[(L-1)*nQ+q].quantum = quant;
}

#endif
