/** \file StatsModule.h */

#ifndef STATSMODULE_H
#define STATSMODULE_H

#include "CommonDefs.h"

class StatsModule {
public:
		StatsModule(int, int, int);
		~StatsModule();

	// access statistics
	int	iPktCnt(int) const;
	int	oPktCnt(int) const;
	int	iByteCnt(int) const;
	int	oByteCnt(int) const;
	int	getQlen(int) const;
	int	getQbytes(int) const;
	int	getLinkQlen(int) const;

	// update statistics counts
	void	cntInLink(int, int, bool);
	void	cntOutLink(int, int, bool);
	void	incQlen(int, int, int);
	void	decQlen(int, int, int);

	// record stats to a file
	void	record(uint32_t);

	// input/output 
	bool read(istream&);
	void write(ostream&) const;
private:
	int	maxStats;		// max number of recorded statistics
	int	maxLnk;			// largest link number
	int	maxQ;			// largest queue number
	int	n;			// number of statistics to record

	enum cntrTyp {
	inPkt, outPkt, inByt, outByt,  	// input/output counts
      	qPkt, qByt,		 	// packets/bytes in queues
	};		

	struct StatItem {
	int	lnk;			// link number for stat
	int	qnum;			// for queue length stats
	cntrTyp	typ;			// type of counter for this stat
	};
	StatItem  *stat;		// stat[i] is statistic number i
	ofstream fs;			// file stream for statistics

	struct LinkCounts {
	int	inByte;
	int	outByte;
	int	inPkt;
	int	outPkt;
	int	numPkt;
	};
	LinkCounts *lnkCnts;

	struct QueueCounts {
	int	bytLen;
	int	pktLen;
	};
	QueueCounts *qCnts;

	// system-wide counts
	int	totInByte;
	int	totInPkt;
	int	rtrInByte;
	int	rtrInPkt;
	int	leafInByte;
	int	leafInPkt;
	int	totOutByte;
	int	totOutPkt;
	int	rtrOutByte;
	int	rtrOutPkt;
	int	leafOutByte;
	int	leafOutPkt;

	// helper functions
	bool	readStat(istream&);	 	// read one new statistic
	void	writeStat(ostream&, int) const; // write a single stat entry
};

inline int StatsModule::iPktCnt(int lnk) const {
	return (lnk == 0 ? totInPkt :
		(lnk == -1 ? rtrInPkt :
		 (lnk == -2 ? leafInPkt : lnkCnts[lnk].inPkt)));
}

inline int StatsModule::oPktCnt(int lnk) const {
	return (lnk == 0 ? totOutPkt :
		(lnk == -1 ? rtrOutPkt :
		 (lnk == -2 ? leafOutPkt : lnkCnts[lnk].outPkt)));
}

inline int StatsModule::iByteCnt(int lnk) const {
	return (lnk == 0 ? totInByte :
		(lnk == -1 ? rtrInByte :
		 (lnk == -2 ? leafInByte : lnkCnts[lnk].inByte)));
}

inline int StatsModule::oByteCnt(int lnk) const {
	return (lnk == 0 ? totOutByte :
		(lnk == -1 ? rtrOutByte :
		 (lnk == -2 ? leafOutByte : lnkCnts[lnk].outByte)));
}
inline int StatsModule::getQlen(int q) const {
	return qCnts[q].pktLen;
}

inline int StatsModule::getQbytes(int q) const {
	return qCnts[q].bytLen;
}

inline int StatsModule::getLinkQlen(int lnk) const {
	return lnkCnts[lnk].numPkt;
}

inline void StatsModule::cntInLink(int lnk, int len, bool isRtr) {
	if (1 <= lnk && lnk <= maxLnk) {
		lnkCnts[lnk].inByte += len; lnkCnts[lnk].inPkt++;
		totInByte += len; totInPkt++;
		if (isRtr) { rtrInByte += len; rtrInPkt++; }
		else	   { leafInByte += len; leafInPkt++; }
	}
}

inline void StatsModule::cntOutLink(int lnk, int len, bool isRtr) {
	if (1 <= lnk && lnk <= maxLnk) {
		lnkCnts[lnk].outByte += len; lnkCnts[lnk].outPkt++;
	}
}

inline void StatsModule::incQlen(int qn, int lnk, int len) {
	if (1 <= lnk && lnk <= maxLnk) lnkCnts[lnk].numPkt++;
	if (1 <= qn && qn <= maxQ) {
		qCnts[qn].bytLen += len; qCnts[qn].pktLen++;
	}
}

inline void StatsModule::decQlen(int qn, int lnk, int len) {
	if (1 <= lnk && lnk <= maxLnk) lnkCnts[lnk].numPkt--;
	if (1 <= qn && qn <= maxQ) {
		qCnts[qn].bytLen -= len; qCnts[qn].pktLen--;
	}
}

#endif
