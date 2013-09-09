/** @file PacketLog.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "PacketLog.h"

namespace forest {

/** Constructor for PacketLog, allocates space and initializes private data.
 */
PacketLog::PacketLog(int maxEvents1, int maxFilters1, PacketStore *ps1)
	  	     : maxEvents(maxEvents1), maxFilters(maxFilters1), ps(ps1) {
	evec = new EventStruct[maxEvents];
	eventCount = firstEvent = lastEvent = 0;
	fvec = new PacketFilter[maxFilters+1];
	filters = new UiSetPair(maxFilters);
}

/** Destructor for PacketLog, deletes allocated space */
PacketLog::~PacketLog() { delete [] evec; delete [] fvec; delete filters; }

/** Log a packet if it matches a stored filter.
 *  @param p is the number of the packet to be logged
 *  @param lnk is the link the packet is being sent on (or was received from)
 *  @param sendFlag is true if the packet is being sent; else it is false
 *  @param now is the time at which the packet is being sent/received
 *  Received packet is compared to all enabled filters. If it matches
 *  any filter, a copy is made and saved in the log. If the packet
 *  can't be saved, we record a "gap" in the log. If several packets
 *  in a row cannot be logged, the number of missing packets is
 *  recorded as part of the gap record.
 */
void PacketLog::log(pktx px, int lnk, bool sendFlag, uint64_t now) {
	if (firstFilter() == 0) return;
	for (fltx f = firstFilter(); f != 0; f = nextFilter(f)) {
		if (match(f,px,lnk,sendFlag)) break;
		if (nextFilter(f) == 0) return; // no filters match
	}
	// reach here if packet matched some filter
	// make a record in event vector
	int px1 = ps->fullCopy(px);
	if (px1 == 0 || eventCount == maxEvents) { // record gap
		if (px1 != 0) ps->free(px1);
		// use px == 0 to indicate gap in sequence
		// for these records, link field is used to record
		// number of packets that were missed (size of gap)
		if (eventCount > 0 && evec[lastEvent].px == 0) {
			// existing gap record
			evec[lastEvent].link++; return;
		}
		// need to convert last record to a gap record
		evec[lastEvent].px = 0;
		evec[lastEvent].link = 1;
		evec[lastEvent].time = now;
		if (eventCount == 0) eventCount++;
		return;
	}
	// common case - just add new record for packet
	if (eventCount > 0)
		if (++lastEvent >= maxEvents) lastEvent = 0;
	eventCount++;
	evec[lastEvent].px = px1;
	evec[lastEvent].sendFlag = sendFlag;
	evec[lastEvent].link = lnk;
	evec[lastEvent].time = now;
	return;
}

/** Extract event records from the log.
 *  @param maxLen is the maximum number of characters to return
 *  @param s is a reference to a string in which result is returned
 *  @return the number of log events in the returned string.
 *  Events that are copied to buf are removed from the log.
 */
int PacketLog::log2string(int maxLen, string& s) {
	stringstream ss;
	int count = 0; s = "";
	while (eventCount > 0) {
		ss.str("");
		int px = evec[firstEvent].px;
		ss << Misc::nstime2string(evec[firstEvent].time,s);
		if (px == 0) {
			ss << " missing " << evec[firstEvent].link << endl;
		} else {
			if (evec[firstEvent].sendFlag)	ss << "send ";
			else				ss << "recv ";
			ss << "link " << setw(2) << evec[firstEvent].link;
			ss << " " << ps->getPacket(px).toString(s);
		}
		if (s.length() + ss.str().length() > maxLen) break;
		s += ss.str();
		ps->free(px);
		count++; eventCount--;
		if (++firstEvent >= maxEvents) firstEvent = 0;
	}
	return count;
}

/** Add a new filter to the table of filters.
 *  The filter is disabled initially
 *  @return the filter number of the new filter or 0, if could not add
 */
int PacketLog::addFilter() {
	fltx f = filters->firstOut();
	if (f == 0) return 0;
	filters->swap(f); disable(f);
	return f;
}

/** Remves a filter from the table.  */
void PacketLog::dropFilter(fltx f) {
	if (!filters->isIn(f)) return;
	disable(f); filters->swap(f);
}

} // ends namespace

