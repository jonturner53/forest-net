/** @file Repeater.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Repeater.h"

namespace forest {

/** Constructor for Repeater.
 */
Repeater::Repeater(int size) : n(size) {
	pmap = new HashMap<int64_t, Pair<int,int>, Hash::s64>(n);
	deadlines = new Dheap<int64_t>(n);
}

/** Destructor for Repeater. */
Repeater::~Repeater() { delete pmap; delete deadlines; } 

/** Save a copy of an outgoing request packet.
 *  @param cx is a copy of some outgoing request packet
 *  @param seqNum is the sequence number assigned to the packet
 *  @param now is the current time
 *  @param idx is an optional index that identifies the saved copy;
 *  if zero, an index is assigned automatically
 *  @return the index associated with the saved copy or 0 if a specified
 *  index is not available
 */
int Repeater::saveReq(int cx, int64_t seqNum, int64_t now, int idx) {
	int x = pmap->put(seqNum, Pair<int,int>(cx,3), idx);
	if (x == 0) return 0;
	deadlines->insert(x, now + 1000000000);
	return x;
}

/** Match a reply to a saved request and update.
 *  @param seqNum is the sequence number assigned to the reply
 *  @return a pair consisting of the packet index of the saved request packet
 *  and the internal index that was associated with the saved copy; if
 *  no matching request is found, the pair (0,0) is returned
 */
pair<int,int> Repeater::deleteMatch(int64_t seqNum) {
	int idx = pmap->find(seqNum);
	if (idx == 0) return pair<int,int>(0,0);
	Pair<int,int>& vp = pmap->getValue(idx);
	deadlines->remove(idx);
	pmap->remove(seqNum);
	return pair<int,int>(vp.first,idx);
}

/** Check for an overdue packet.
 *  @param now is the current time
 *  @return a pair (cx, idx) where cx is the packet index for an overdue packet
 *  that was saved earlier; idx is the index under which the packet
 *  was saved; if the packet has been overdue more than the allowed number
 *  of times (3), the value of cx is negated to signal this condition
 *  and the save copy is removed
 */
pair<int,int> Repeater::overdue(int64_t now) {
	int idx = deadlines->findmin();
	if (idx == 0 || now < deadlines->key(idx))
		return pair<int,int>(0,0);
	Pair<int, int>& vp = pmap->getValue(idx);
	if (vp.second == 0) {
		deadlines->remove(idx);
		pmap->remove(pmap->getKey(idx));
		return pair<int,int>(-vp.first,idx);
	}
	// push deadline back by one second and decrement repeat count
	deadlines->changekey(idx, now + 1000000000); vp.second--; 
	return pair<int,int>(vp.first,idx);
}
		
} // ends namespace
