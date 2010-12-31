#include "statsMod.h"

// Constructor for statsMod, allocates space.
statsMod::statsMod(int maxStats1, lnkTbl *lt1, qMgr *qm1,
		   lcTbl *lct1, int myLcn1)
		   : maxStats(maxStats1), lt(lt1), qm(qm1),
		     lct(lct1), myLcn(myLcn1) {
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
			case inBklg: val = lct->inBklg(s->lnk); break;
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
//   xPkt I O	number of packets sent from input linecard I to O
//   xByt I O	number of bytes sent from input linecard I to O
// inBklg L 	number of bytes at all inputs for output link L
//
// The three types xPkt, xByt and inBklg only apply to configurations
// with multiple linecards. In the queue length statistics, if the
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
        else if (typStr ==   "xPkt") typ = xPkt;
        else if (typStr ==   "xByt") typ = xByt;
        else if (typStr == "inBklg") typ = inBklg;
        else return false;

	if (myLcn == 0 && (typ == xPkt || typ == xByt || typ == inBklg))
		return false;

	switch (typ) {
	case inPkt: case outPkt: case inByt: case outByt: case inBklg:
		if (!misc::getNum(is,lnk)) return false;
		if (myLcn != 0 && lnk != myLcn) return true;
			// ignore stats for other linecards
		break;
	case qPkt: case qByt:
		if (!misc::getNum(is,lnk) || !misc::getNum(is,qnum))
			return false;
		if (myLcn != 0 && lnk != myLcn) return true;
		break;
	case xPkt: case xByt:
		if (!misc::getNum(is,lcIn) || !misc::getNum(is,lcOut)) 
			return false;
		if (lcIn == myLcn) {
			lnk = lcOut;
			typ = (typ == xPkt) ? outPkt : outByt;
		} else if (lcOut == myLcn) {
			lnk = lcIn; 
			typ = (typ == xPkt) ? inPkt : inByt;
		} else return true; // stat for another linecard
		break;
	}
	misc::cflush(is,'\n');

	if (n >= maxStats) return false;
	n++;

	statItem* s = &stat[n];
	s->typ = typ;
	s->lnk = lnk;
	s->qnum = qnum;

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
	if (sm.myLcn != 0) {
		char buf[16];
		sprintf(buf,"%d",sm.myLcn); fname += buf;
	}
	sm.fs.open(fname.c_str(),fstream::app);
	if (sm.fs.fail()) return false;
	return true;
}

void statsMod::putStat(ostream& os, int i) const {
// Print i-th entry
	statItem *s = &stat[i];
	switch(s->typ) {
	case  inPkt:
		if (myLcn == 0 || s->lnk == myLcn)
			os << " inPkt " << setw(2) << s->lnk << endl;
		else
			os << "  xPkt " << setw(2) << s->lnk 
			   << " to " << setw(2) << myLcn << endl;
		break;
	case  outPkt:
		if (myLcn == 0 || s->lnk == myLcn)
			os << "outPkt " << setw(2) << s->lnk << endl;
		else
			os << "  xPkt " << setw(2) << myLcn
			   << " to " << setw(2) << s->lnk << endl;
		break;
	case  inByt:
		if (myLcn == 0 || s->lnk == myLcn)
			os << " inByt " << setw(2) << s->lnk << endl;
		else
			os << "  xByt " << setw(2) << s->lnk 
			   << " to " << setw(2) << myLcn << endl;
		break;
	case outByt:
		if (myLcn == 0 || s->lnk == myLcn)
			os << "outByt " << setw(2) << s->lnk << endl;
		else
			os << "  xByt " << setw(2) << s->lnk 
			   << " to " << setw(2) << myLcn << endl;
		break;
	case   qPkt:
		if (myLcn == 0 || s->lnk == myLcn)
			os << "  qPkt " << setw(2) << s->lnk
			   << " " << setw(2) << s->qnum << endl;
		break;
	case   qByt:
		if (myLcn == 0 || s->lnk == myLcn)
			os << "  qByt " << setw(2) << s->lnk
			   << " " << setw(2) << s->qnum << endl;
		break;
	case   inBklg:
		if (myLcn != 0 || s->lnk == myLcn)
			os << "inBklg " << setw(2) << s->lnk << endl;
		break;
	}
}

ostream& operator<<(ostream& os, const statsMod& sm) {
// Output human readable representation of link table.
	for (int i = 1; i <= sm.n; i++) sm.putStat(os,i);
	return os;
}
