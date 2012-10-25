/** @file PacketLog.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "PacketLog.h"

/** Constructor for PacketLog, allocates space and initializes private data.
 */
PacketLog::PacketLog(int maxPkts1, int maxData1, PacketStore *ps1)
	  	     : maxPkts(maxPkts1), maxData(maxData1), ps(ps1) {
	events = new EventStruct[maxPkts];
	numPkts = numData = 0;
}

/** Destructor for PacketLog, deletes allocated space */
PacketLog::~PacketLog() { delete [] events; }

/** Log a packet.
 *  @param p is the number of the packet to be logged
 *  @param lnk is the packet is being sent on (or was received from)
 *  @param sendFlag is true if the packet is being sent; else it is false
 *  @param now is the time at which the packet is being sent/received
 */
void PacketLog::log(int p, int lnk, bool sendFlag, uint64_t now) {
        PacketHeader& h = ps->getHeader(p);
        if (numPkts < maxPkts &&
            (h.getPtype() != CLIENT_DATA || numData <= maxData)) {
                int p1 = (h.getPtype() == CLIENT_DATA ?
                          ps->clone(p) : ps->fullCopy(p));
                events[numPkts].pkt = p1;
                events[numPkts].sendFlag = sendFlag;
                events[numPkts].link = lnk;
                events[numPkts].time = now;
                numPkts++;
                if (h.getPtype() == CLIENT_DATA) numData++;
        }
}

/** Write all logged packets.
 *  @param out is an open output stream
 */
void PacketLog::write(ostream& out) const {
	for (int i = 0; i < numPkts; i++) {
		string s;
		if (events[i].sendFlag) cout << "send link ";
		else 			cout << "recv link ";
		cout << setw(2) << events[i].link << " at "
		     << Misc::nstime2string(events[i].time,s) << " ";
		int p = events[i].pkt;
		cout << (ps->getHeader(p)).toString(ps->getBuffer(p),s);
	}
}
