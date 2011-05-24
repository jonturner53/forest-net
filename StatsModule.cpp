/** \file StatsModule.cpp */

#include "StatsModule.h"

StatsModule::StatsModule(int maxStats1, LinkTable *lt1, QuManager *qm1)
		         : maxStats(maxStats1), lt(lt1), qm(qm1) {
	stat = new statItem[maxStats+1];
	n = 0;
};
	
StatsModule::~StatsModule() { delete [] stat; }

void StatsModule::record(uint32_t now) {
// Record statistics at time now.
	int i, val; statItem *s;

	if (n == 0) return;
	for (i = 1; i <= n; i++) {
		s = &stat[i];
		switch(s->typ) {
			case inPkt:  val = lt->iPktCnt(s->lnk); break;
			case outPkt: val = lt->oPktCnt(s->lnk); break;
			case qPkt:   val = qm->getLengthPkts(s->lnk,s->qnum);
				     break;
			case inByt:  val = lt->iBytCnt(s->lnk); break;
			case outByt: val = lt->oBytCnt(s->lnk); break;
			case qByt:   val = qm->getLengthBytes(s->lnk,s->qnum);
				     break;
			default: break;
		}
		fs << val << " ";
	}
	fs << double(now)/1000000 << endl;
	fs.flush();
}

/** Read an entry from is and store it in the stats table.
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
 *    qPkt L Q	number of packets in queue Q on output link L
 *    qByt L Q	number of bytes in queue Q on output link L
 * 
 *  If inPkt, outPkt, inByt, outByt are given with a link # of 0,
 *  the statistics for the router as a whole are reported. If the
 *  link # is -1, the statistics for packets to/from other routers
 *  is reported. If the link # is -2, the statistics for packets
 *  to/from clients is reported. Note, the current implementation
 *  does not support byte counts for the statistics to/from other
 *  routers or to/from other clients. In the queue length statistics, if the
 *  given queue number is zero, then the total number of packets (bytes)
 *  queued for the given link (over all queues) is reported.
 */ 
bool StatsModule::readStat(istream& in) {
	int lnk, qnum, lcIn, lcOut;
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
		if (!Misc::readNum(in,lnk) || !Misc::readNum(in,qnum))
			return false;
		break;
	}
	Misc::cflush(in,'\n');

	if (n >= maxStats) return false;
	n++;

	statItem* s = &stat[n];
	s->typ = typ;
	s->lnk = lnk;
	s->qnum = qnum;

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
	statItem *s = &stat[i];
	switch(s->typ) {
	case  inPkt:
		out << " inPkt " << setw(2) << s->lnk << endl;
		break;
	case  outPkt:
		out << "outPkt " << setw(2) << s->lnk << endl;
		break;
	case  inByt:
		out << " inByt " << setw(2) << s->lnk << endl;
		break;
	case outByt:
		out << "outByt " << setw(2) << s->lnk << endl;
		break;
	case   qPkt:
		out << "  qPkt " << setw(2) << s->lnk
		   << " " << setw(2) << s->qnum << endl;
		break;
	case   qByt:
		out << "  qByt " << setw(2) << s->lnk
		   << " " << setw(2) << s->qnum << endl;
		break;
	}
}

// Output human readable representation of stats module.
void StatsModule::write(ostream& out) const {
	for (int i = 1; i <= n; i++) writeStat(out,i);
}
