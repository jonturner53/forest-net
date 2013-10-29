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
PacketLog::PacketLog(PacketStore *ps1) : ps(ps1) {
	evec = new EventStruct[MAX_EVENTS];
	eventCount = firstEvent = lastEvent = 0;
	fvec = new PacketFilter[MAX_FILTERS+1];
	filters = new UiSetPair(MAX_FILTERS);

	// default behavior just logs all packets to stdout
	logOn = true; logLocal = true;
	numOut = numDataOut = 0;
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
 *  If no filters are defined, we log every packet.
 */
void PacketLog::log(pktx px, int lnk, bool sendFlag, uint64_t now) {
	if (!logOn) return;
	if (firstFilter() != 0) {
		for (fltx f = firstFilter(); f != 0; f = nextFilter(f)) {
			if (match(f,px,lnk,sendFlag)) break;
			if (nextFilter(f) == 0) return; // no filters match
		}
	}
	// reach here if no filters or packet matched some filter
	// make a record in event vector
	int px1 = ps->fullCopy(px);
	if (px1 == 0 || eventCount == MAX_EVENTS) { // record gap
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
	} else {
		// common case - just add new record for packet
		if (eventCount > 0)
			if (++lastEvent >= MAX_EVENTS) lastEvent = 0;
		eventCount++;
		evec[lastEvent].px = px1;
		evec[lastEvent].sendFlag = sendFlag;
		evec[lastEvent].link = lnk;
		evec[lastEvent].time = now;
	}
	if (!logLocal) return;
	// optionally write log entries to cout once per second
	if (now < dumpTime + 1000000000) return;
	dumpTime = now;
	write(cout);
	// stop logging when we hit the output limit while logging locally
	// can stil re-enable logging
	if (numOut > OUT_LIMIT) logOn = logLocal = false;
}

/** Write all logged packets.
 *  @param out is an open output stream
 */
void PacketLog::write(ostream& out) {
	stringstream ss;
	while (eventCount > 0) {
		int px = evec[firstEvent].px;
		string s;
		out << Misc::nstime2string(evec[firstEvent].time,s);
		if (numOut <= OUT_LIMIT) {
			if (px == 0) {
				out << " missing " << evec[firstEvent].link
				    << endl;
				numOut++;
			} else {
				Packet& p = ps->getPacket(px);
				if (p.type != Forest::CLIENT_DATA ||
				    numDataOut <= DATA_OUT_LIMIT) {
					if (evec[firstEvent].sendFlag)
						out << " send ";
					else
						out << " recv ";
					out << "link " << setw(2)
					    << evec[firstEvent].link;
					out << " " << p.toString(s);
					numOut++;
					if (p.type == Forest::CLIENT_DATA)
						numDataOut++;
				}
			}
		}
		ps->free(px);
		eventCount--;
		if (++firstEvent >= MAX_EVENTS) firstEvent = 0;
	}
	out.flush();
}

/** Purge all logged packets.
 */
void PacketLog::purge() {
	while (eventCount > 0) {
		ps->free(evec[firstEvent].px);
		eventCount--;
		if (++firstEvent >= MAX_EVENTS) firstEvent = 0;
	}
}

/** Extract event records from the log for delivery to remote client.
 *  Remote logging requires at least one filter to be defined.
 *  It also disables local logging, as we cannot do both at the same time.
 *  @param maxLen is the maximum number of characters to return
 *  @param s is a reference to a string in which result is returned
 *  @return the number of log events in the returned string.
 *  Events that are copied to buf are removed from the log.
 */
int PacketLog::extract(int maxLen, string& s) {
	if (firstFilter() == 0) { return 0; }
	logLocal = false;
	stringstream ss;
	int count = 0; s = "";
	while (eventCount > 0) {
		ss.str("");
		int px = evec[firstEvent].px;
		string s1;
		ss << Misc::nstime2string(evec[firstEvent].time,s1);
		if (px == 0) {
			ss << " missing " << evec[firstEvent].link << endl;
		} else {
			if (evec[firstEvent].sendFlag)	ss << " send ";
			else				ss << " recv ";
			ss << "link " << setw(2) << evec[firstEvent].link;
			ss << " " << ps->getPacket(px).toString(s1);
		}
		if (s.length() + ss.str().length() > maxLen) break;
		s += ss.str();
		ps->free(px);
		count++; eventCount--;
		if (++firstEvent >= MAX_EVENTS) firstEvent = 0;
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

