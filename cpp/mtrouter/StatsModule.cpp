/** @file StatsModule.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "StatsModule.h"

namespace forest {


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
	totInByte = 0; totInPkt = 0; totDiscards = 0;
	rtrInByte = 0; rtrInPkt = 0; 
	leafInByte = 0; leafInPkt = 0; 
	totOutByte = 0; totOutPkt = 0;
	rtrOutByte = 0; rtrOutPkt = 0; rtrDiscards = 0;
	leafOutByte = 0; leafOutPkt = 0; leafDiscards = 0;
};
	
StatsModule::~StatsModule() { delete [] stat; }

// Record statistics at time now.
void StatsModule::record(uint64_t now) {
	unique_lock<mutex> lck(mtx);
	int i, val, ctx, cLnk, qid;

	val = -1;
	if (n == 0) return;

	// check for the statsSwitch file
	string fname = "statsSwitch";
	ifstream sfs; sfs.open(fname.c_str());
	string statsSwitch;
	if (sfs.fail()) return;
	if (!Util::readWord(sfs,statsSwitch) || statsSwitch != "on") {
				sfs.close(); return;
	}
		sfs.close();
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
				if (s.comt == 0) {
					val = getLinkQlen(s.lnk);
					break;
				}
				ctx = ctt->getComtIndex(s.comt);
				cLnk = ctt->getClnkNum(s.comt,s.lnk);
				qid = ctt->getLinkQ(ctx,cLnk);
				val = getQlen(qid);
				break;
			case qByt:
				if (s.comt == 0) {
					val = discCnt(s.lnk);
					break;
				}
				ctx = ctt->getComtIndex(s.comt);
				cLnk = ctt->getClnkNum(s.comt,s.lnk);
				qid = ctt->getLinkQ(ctx,cLnk);
				val = qDiscCnt(qid);
				break;
			case disc:
				cLnk = ctt->getClnkNum(s.comt,s.lnk);
				qid = ctt->getLinkQ(ctx,cLnk);
				val = getQlen(qid);
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
 *	qPkt L C	number of packets for comtree C output link L
 *	qByt L C	number of bytes for comtree C on output link L
 *	disc L C 	number of packets discarded for comtree C on link L
 * 
 *  If inPkt, outPkt, inByt, outByt are given with a link # of 0,
 *  the statistics for the router as a whole are reported. If the
 *  link # is -1, the statistics for packets to/from other routers
 *  is reported. If the link # is -2, the statistics for packets
 *  to/from leaf nodes is reported. In the queue packet length statistics,
 *  if the given comtree number is zero, then the total number of packets (bytes)
 *  queued for the given link (over all queues) is reported. Similarly,
 *  for the discard statistic, a comtree of 0 reports all discards for
 *  the link.
 */ 
bool StatsModule::readStat(istream& in) {
	// no lock here, caller must acquire lock
	int lnk, comt;
	cntrTyp typ; string typStr, fname;

	Util::skipBlank(in);
	if (!Util::readWord(in,typStr)) return false;

			 if (typStr ==  "inPkt") typ = inPkt;
		else if (typStr == "outPkt") typ = outPkt;
		else if (typStr ==  "inByt") typ = inByt;
		else if (typStr == "outByt") typ = outByt;
		else if (typStr ==   "qPkt") typ = qPkt;
		else if (typStr ==   "qByt") typ = qByt;
		else if (typStr ==   "disc") typ = disc;
		else return false;

	switch (typ) {
	case inPkt: case outPkt: case inByt: case outByt:
		if (!Util::readInt(in,lnk)) return false;
		break;
	case qPkt: case qByt: case disc:
		if (!Util::readInt(in,lnk) || !Util::readInt(in,comt))
			return false;
		break;
	}
	Util::nextLine(in);

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
	unique_lock<mutex> lck(mtx);
	int num;
 	Util::skipBlank(in);
	if (!Util::readInt(in,num)) return false;
	Util::nextLine(in);
	while (num--) {
		if (!readStat(in)) return false;
	}
	string fname = "stats";
	fs.open(fname.c_str(),fstream::app);
	if (fs.fail()) return false;
	return true;
}

/** Create a string representing an entry in the table.
 *  @param i is the index of an entry
 *  @return the string
 */
string StatsModule::stat2string(int i) const {
	// no lock here - caller must acquire lock
	stringstream ss;
	StatItem& si = stat[i];
	switch(si.typ) {
	case  inPkt:
		ss << " inPkt " << setw(2) << si.lnk << endl;
		break;
	case  outPkt:
		ss << "outPkt " << setw(2) << si.lnk << endl;
		break;
	case  inByt:
		ss << " inByt " << setw(2) << si.lnk << endl;
		break;
	case outByt:
		ss << "outByt " << setw(2) << si.lnk << endl;
		break;
	case   qPkt:
		ss << "  qPkt " << setw(2) << si.lnk
		   << " " << setw(2) << si.comt << endl;
		break;
	case   qByt:
		ss << "  qByt " << setw(2) << si.lnk
		   << " " << setw(2) << si.comt << endl;
		break;
	case disc:
		ss << "  disc " << setw(2) << si.lnk
		   << " " << setw(2) << si.comt << endl;
		break;
	}
	return ss.str();
}

/** Create a string representing the stats module.
 *  @param s is a reference to a string in which the result is returned
 *  @return a reference to s
 */
string StatsModule::toString() {
	unique_lock<mutex> lck(mtx);
	string s;
	for (int i = 1; i <= n; i++) s += stat2string(i);
	return s;
}

} // ends namespace

