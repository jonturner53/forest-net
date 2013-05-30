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
PacketLog::PacketLog(int maxPkts1, int maxData1, PacketStore *ps1)
	  	     : maxPkts(maxPkts1), maxData(maxData1), ps(ps1) {
	events = new EventStruct[maxPkts];
	numPkts = numData = 0;
	dumpTime = 0;
}

/** Destructor for PacketLog, deletes allocated space */
PacketLog::~PacketLog() { delete [] events; }

/** Log a packet.
 *  @param p is the number of the packet to be logged
 *  @param lnk is the link the packet is being sent on (or was received from)
 *  @param sendFlag is true if the packet is being sent; else it is false
 *  @param now is the time at which the packet is being sent/received
 */
void PacketLog::log(pktx px, int lnk, bool sendFlag, uint64_t now) {
        Packet& p = ps->getPacket(px);
        if (numPkts < maxPkts &&
            (p.type != Forest::CLIENT_DATA || numData <= maxData)) {
                int px1 = (p.type == Forest::CLIENT_DATA ?
                          ps->clone(px) : ps->fullCopy(px));
                events[numPkts].px = px1;
                events[numPkts].sendFlag = sendFlag;
                events[numPkts].link = lnk;
                events[numPkts].time = now;
                numPkts++;
                if (p.type == Forest::CLIENT_DATA) numData++;
        }
	if (now < dumpTime + 1000000000) return;
	dumpTime = now;
	string fname = "packetLogSwitch";
	ifstream fs; fs.open(fname.c_str());
	string plSwitch;
	if (fs.fail()) return;
        if (Misc::readWord(fs,plSwitch) && plSwitch == "on")
		write(cout);
	fs.close();
	numPkts = 0;
}

/** Write all logged packets.
 *  @param out is an open output stream
 */
void PacketLog::write(ostream& out) const {
	for (int i = 0; i < numPkts; i++) {
		string s;
		if (events[i].sendFlag) out << "send link ";
		else 			out << "recv link ";
		out << setw(2) << events[i].link << " at "
		     << Misc::nstime2string(events[i].time,s) << " ";
		pktx px = events[i].px;
		out << ps->getPacket(px).toString(s);
	}
	out.flush();
}

} // ends namespace

