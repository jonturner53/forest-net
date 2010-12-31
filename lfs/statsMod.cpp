#include "statsMod.h"

// Constructor for statsMod, allocates space.
statsMod::statsMod(int maxStats1, lnkTbl *lt1, qMgr *qm1, int* avail1)
		   : maxStats(maxStats1), lt(lt1), qm(qm1), avail(avail1) {
	stat = new statItem[maxStats+1];
	n = 0;
};
	
statsMod::~statsMod() { delete [] stat; }

void statsMod::record(uint32_t now) {
// Record statistics at time now.
	int i, val; statItem *s;

	if (n == 0) return;
	for (i = 1; i <= n; i++) {
		s = &stat[i];
		switch(s->typ) {
			case inPkt:  val = lt->iPktCnt(s->lnk); break;
			case outPkt: val = lt->oPktCnt(s->lnk); break;
			case qPkt:   val = qm->qlenPkts(s->lnk,s->qnum); break;
			case inByt:  val = lt->iBytCnt(s->lnk); break;
			case outByt: val = lt->oBytCnt(s->lnk); break;
			case qByt:   val = qm->qlenBytes(s->lnk,s->qnum); break;
			case abw:    val = avail[s->lnk]; break;
			default: break;
		}
		fs << val << " ";
	}
	fs << double(now)/1000000 << endl;
	fs.flush();
}

bool statsMod::getStat(istream& is) {
// Read an entry from is and store it in the stats table.
// Return true on success false on failure.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete statistics request.
// Each entry consists of a type string and one or more additional
// arguments that depend on the type. The full list is given below.
//
//  inPkt L	number of packets received on input link L
// outPkt L	number of packets sent on output link L
//  inByt L	number of bytes received on input link L
// outByt L	number of bytes sent on output link L
//   qPkt L Q	number of packets in queue Q on output link L
//   qByt L Q	number of bytes in queue Q on output link L
//    abw L  	available bandwidth on link L for lfs (in Kb/s)
//
// In the queue length statistics, if the
// given queue number is zero, then the total number of packets (bytes)
// queued for the given link (over all queues) is reported.
//
	int lnk, qnum, lcIn, lcOut;
	cntrTyp typ; string typStr, fname;
	char buf[32];

	misc::skipBlank(is);
	if (!misc::getWord(is,typStr)) return false;

             if (typStr ==  "inPkt") typ = inPkt;
        else if (typStr == "outPkt") typ = outPkt;
        else if (typStr ==  "inByt") typ = inByt;
        else if (typStr == "outByt") typ = outByt;
        else if (typStr ==   "qPkt") typ = qPkt;
        else if (typStr ==   "qByt") typ = qByt;
        else if (typStr ==    "abw") typ = abw;
        else return false;

	
	if (!misc::getNum(is,lnk)) return false;
	if ((typ == qPkt || typ == qByt) && !misc::getNum(is,qnum))
		return false;
	misc::cflush(is,'\n');

	if (n >= maxStats) return false;
	n++;

	statItem* s = &stat[n];
	s->typ = typ; s->lnk = lnk; s->qnum = qnum;

	return true;
}

bool operator>>(istream& is, statsMod& sm) {
// Read statistics items from the input. The first line must contain an
// integer, giving the number of items to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
	int num;
 	misc::skipBlank(is);
        if (!misc::getNum(is,num)) return false;
        misc::cflush(is,'\n');
	while (num--) {
		if (sm.getStat(is) == Null) return false;
	}
	string fname = "stats";
	sm.fs.open(fname.c_str(),fstream::app);
	if (sm.fs.fail()) return false;
	return true;
}

void statsMod::putStat(ostream& os, int i) const {
// Print i-th entry
	statItem *s = &stat[i];
	switch(s->typ) {
	case  inPkt:
		os << " inPkt " << setw(2) << s->lnk << endl;
		break;
	case  outPkt:
		os << "outPkt " << setw(2) << s->lnk << endl;
		break;
	case  inByt:
		os << " inByt " << setw(2) << s->lnk << endl;
		break;
	case outByt:
		os << "outByt " << setw(2) << s->lnk << endl;
		break;
	case   qPkt:
		os << "  qPkt " << setw(2) << s->lnk
		   << " " << setw(2) << s->qnum << endl;
		break;
	case   qByt:
		os << "  qByt " << setw(2) << s->lnk
		   << " " << setw(2) << s->qnum << endl;
		break;
	case abw:
		os << "   abw " << setw(2) << s->lnk << endl;
		break;
	}
}

ostream& operator<<(ostream& os, const statsMod& sm) {
// Output human readable representation of link table.
	for (int i = 1; i <= sm.n; i++) sm.putStat(os,i);
	return os;
}
