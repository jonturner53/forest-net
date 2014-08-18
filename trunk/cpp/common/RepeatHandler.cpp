/** @file RepeatHandler.cpp
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RepeatHandler.h"

namespace forest {

/** Constructor for RepeatHandler.  */
RepeatHandler::RepeatHandler(int size) : n(size) {
	pmap = new HashMap<Pair<fAdr_t,int64_t>, int, Hash::s32s64>(n);
	deadlines = new Dheap<int64_t>(n);
}

/** Destructor for RepeatHandler. */
RepeatHandler::~RepeatHandler() { delete pmap; delete deadlines; } 

/** Look for a saved packet with a given peer address and sequence number.
 *  @param peerAdr is the peer address for the packet
 *  @param seqNum is the packet's sequence number
 *  @return the packet index for the packet, or 0 if no match;
 */
int RepeatHandler::find(fAdr_t peerAdr, int64_t seqNum) {
	int x = pmap->find(Pair<fAdr_t,int64_t>(peerAdr, seqNum));
	if (x == 0) return 0;
	return pmap->getValue(x);
}

/** Save a copy of a received request packet.
 *  @param cx is a copy of a received request packet
 *  @param peerAdr is the Forest address of the sender
 *  @param seqNum is the sequence number assigned to the packet
 *  @param now is the current time
 *  @return true on success, false on failure
 */
bool RepeatHandler::saveReq(int cx, fAdr_t peerAdr, int64_t seqNum,
			    int64_t now) {
	if (pmap->size() == n) {
		// when full, remove the oldest to make room
		int x = deadlines->deletemin();
		pktx ox = pmap->getValue(x);
		pmap->remove(pmap->getKey(x));
		return ox;
	}
	int x = pmap->put(Pair<fAdr_t,int64_t>(peerAdr,seqNum),cx);
	if (x != 0) 
		deadlines->insert(x, now + 20000000000); // keep for 20 seconds
	return 0;
}

/** Match a reply to a saved request and update.
 *  The request packet's index is removed from the saved entry and replaced
 *  with the reply packet's index.
 *  @param cx is the packet index for the reply packet corresponding to
 *  a saved request packet
 *  @param peerAdr is the Forest address of the sender of the original request
 *  @param seqNum is the sequence number assigned to the reply
 *  @return the index associated with the saved copy of the original request,
 *  or 0 if no matching request was found 
 */
int RepeatHandler::saveRep(int cx, fAdr_t peerAdr, int64_t seqNum) {
	Pair<fAdr_t,int64_t> kee(peerAdr,seqNum);
	int x = pmap->find(kee);
	if (x == 0) return 0;
	int px = pmap->getValue(x);
	pmap->getValue(x) = cx;
	deadlines->remove(x);
	pmap->remove(kee);
	return px;
}

/** Check for an expired packet and delete it.
 *  @param now is the current time
 *  @return the packet index of the oldest expired packet, or 0 if there
 *  is no expired packet
 */
int RepeatHandler::expired(int64_t now) {
	int x = deadlines->findmin();
	if (x == 0 || now < deadlines->key(x)) return 0;
	int px = pmap->getValue(x);
	deadlines->remove(x);
	pmap->remove(pmap->getKey(x));
	return px;
}
		
} // ends namespace
