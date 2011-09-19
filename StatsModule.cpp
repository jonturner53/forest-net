/** @file StatsModule.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "StatsModule.h"

StatsModule::StatsModule(int maxStats1, int maxLnk1, int maxQ1,
			 ComtreeTable* ctt1)
		         : maxStats(maxStats1), maxLnk(maxLnk1), maxQ(maxQ1),
			   ctt(ctt1) {
	stat = new StatItem[maxStats+1];
	lnkCnts = new LinkCounts[maxLnk+1];
	qCnts = new QueueCounts[maxQ+1];
	for (int i = 1; i <= maxLnk; i++) {
		lnkCnts[i].inByte = lnkCnts[i].outByte = 0;
		lnkCnts[i].inPkt = lnkCnts[i].outPkt = 0;
		lnkCnts[i].numPkt = 0;
	}
	for (int i = 1; i <= maxQ; i++) {
		qCnts[i].bytLen = qCnts[i].pktLen = 0;
	}
	n = 0;
};
	
StatsModule::~StatsModule() { delete [] stat; }

void StatsModule::record(uint64_t now) {
// Record statistics at time now.
	int i, val, cLnk, qid;

	if (n == 0) return;
	for (i = 1; i <= n; i++) {
		StatItem& s = stat[i];
		switch(s.typ) {
			case inPkt:  
				if (s.lnk == 0) val = totInPkt;
				else if (s.lnk == -1) val = rtrInPkt;
				else if (s.lnk == -2) val = leafInPkt;
				else val = lnkCnts[s.lnk].inPkt;
				break;
			case outPkt:
				if (s.lnk == 0) val = totOutPkt;
				else if (s.lnk == -1) val = rtrOutPkt;
				else if (s.lnk == -2) val = leafOutPkt;
				else val = lnkCnts[s.lnk].outPkt;
				break;
			case inByt:  
				if (s.lnk == 0) val = totInByte;
				else if (s.lnk == -1) val = rtrInByte;
				else if (s.lnk == -2) val = leafInByte;
				else val = lnkCnts[s.lnk].inByte;
				break;
			case outByt:
				if (s.lnk == 0) val = totOutByte;
				else if (s.lnk == -1) val = rtrOutByte;
				else if (s.lnk == -2) val = leafOutByte;
				else val = lnkCnts[s.lnk].outByte;
				break;
			case qPkt:   
				cLnk = ctt->getComtLink(s.comt,s.lnk);
				qid = ctt->getLinkQ(cLnk);
				val = getQlen(qid);
				break;
			case qByt:
				cLnk = ctt->getComtLink(s.comt,s.lnk);
				qid = ctt->getLinkQ(cLnk);
				val = getQbytes(qid);
				break;
			default: break;
		}
		fs << val << " ";
	}
	fs << double(now)/1000000000 << endl;
	fs.flush();
}

/** Read an entry from in and store it in the stats table.
 *  Return true on success false on failure.
 *  A line is a pure comment line if it starts with a # sign.
 *  A trailing comment is also allowed at the end of a line by
 *  including a # sign. All lines that are not pure comment lines
 *  or blank are assumed to contain a complete statistics request.
 *  Each entry consists of a type string and one or more additional
 *  arguments that depend on the type. The full list is given below.
 * 
 *   inPkt L	number of packets received on input link L
 *  outPkt L	number of packets sent on output link L
 *   inByt L	number of bytes received on input link L
 *  outByt L	number of bytes sent on output link L
 *    qPkt L C	number of packets for comtree C output link L
 *    qByt L C	number of bytes for comtree C on output link L
 * 
 *  If inPkt, outPkt, inByt, outByt are given with a link # of 0,
 *  the statistics for the router as a whole are reported. If the
 *  link # is -1, the statistics for packets to/from other routers
 *  is reported. If the link # is -2, the statistics for packets
 *  to/from leaf nodes is reported. Note, the current implementation
 *  does not support byte counts for the statistics to/from other
 *  routers or to/from other clients. In the queue length statistics, if the
 *  given queue number is zero, then the total number of packets (bytes)
 *  queued for the given link (over all queues) is reported.
 */ 
bool StatsModule::readStat(istream& in) {
	int lnk, comt, lcIn, lcOut;
	cntrTyp typ; string typStr, fname;
	char buf[32];

	Misc::skipBlank(in);
	if (!Misc::readWord(in,typStr)) return false;

             if (typStr ==  "inPkt") typ = inPkt;
        else if (typStr == "outPkt") typ = outPkt;
        else if (typStr ==  "inByt") typ = inByt;
        else if (typStr == "outByt") typ = outByt;
        else if (typStr ==   "qPkt") typ = qPkt;
        else if (typStr ==   "qByt") typ = qByt;
        else return false;

	switch (typ) {
	case inPkt: case outPkt: case inByt: case outByt:
		if (!Misc::readNum(in,lnk)) return false;
		break;
	case qPkt: case qByt:
		if (!Misc::readNum(in,lnk) || !Misc::readNum(in,comt))
			return false;
		break;
	}
	Misc::cflush(in,'\n');

	if (n >= maxStats) return false;
	n++;

	StatItem& s = stat[n];
	s.typ = typ;
	s.lnk = lnk;
	s.comt = comt;

	return true;
}

bool StatsModule::read(istream& in) {
// Read statistics items from the input. The first line must contain an
// integer, giving the number of items to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
	int num;
 	Misc::skipBlank(in);
        if (!Misc::readNum(in,num)) return false;
        Misc::cflush(in,'\n');
	while (num--) {
		if (readStat(in) == Null) return false;
	}
	string fname = "stats";
	fs.open(fname.c_str(),fstream::app);
	if (fs.fail()) return false;
	return true;
}

void StatsModule::writeStat(ostream& out, int i) const {
// Print i-th entry
	StatItem& s = stat[i];
	switch(s.typ) {
	case  inPkt:
		out << " inPkt " << setw(2) << s.lnk << endl;
		break;
	case  outPkt:
		out << "outPkt " << setw(2) << s.lnk << endl;
		break;
	case  inByt:
		out << " inByt " << setw(2) << s.lnk << endl;
		break;
	case outByt:
		out << "outByt " << setw(2) << s.lnk << endl;
		break;
	case   qPkt:
		out << "  qPkt " << setw(2) << s.lnk
		   << " " << setw(2) << s.comt << endl;
		break;
	case   qByt:
		out << "  qByt " << setw(2) << s.lnk
		   << " " << setw(2) << s.comt << endl;
		break;
	}
}

// Output human readable representation of stats module.
void StatsModule::write(ostream& out) const {
	for (int i = 1; i <= n; i++) writeStat(out,i);
}
