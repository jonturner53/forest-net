/** \file StatsModule.h */

#ifndef STATSMODULE_H
#define STATSMODULE_H

#include "Forest.h"
#include "ComtreeTable.h"

namespace forest {


class StatsModule {
public:
		StatsModule(int, int, int, ComtreeTable*);
		~StatsModule();

	// access statistics
	int	iPktCnt(int) const;
	int	oPktCnt(int) const;
	int	discCnt(int) const;
	int	qDiscCnt(int) const;
	int	iByteCnt(int) const;
	int	oByteCnt(int) const;
	int	getQlen(int) const;
	int	getQbytes(int) const;
	int	getLinkQlen(int) const;

	// update statistics counts
	void	clearLnkStats(int);
	void	clearQuStats(int);
	void	cntInLink(int, int, bool);
	void	cntOutLink(int, int, bool);
	void	cntDiscards(int, int, bool);
	void	incQlen(int, int, int);
	void	decQlen(int, int, int);

	// record stats to a file
	void	record(uint64_t);

	// input/output 
	bool read(istream&);
	string toString() const;
private:
	int	maxStats;		// max number of recorded statistics
	int	maxLnk;			// largest link number
	int	maxQ;			// largest queue number
	int	n;			// number of statistics to record

	enum cntrTyp {
	inPkt, outPkt, inByt, outByt,  	// input/output counts
      	qPkt, qByt,		 	// packets/bytes in queues
	disc				// discards from queues
	};		

	struct StatItem {
	int	lnk;			// link number for stat
	int	comt;			// for queue length stats
	cntrTyp	typ;			// type of counter for this stat
	};
	StatItem  *stat;		// stat[i] is statistic number i
	ofstream fs;			// file stream for statistics

	struct LinkCounts {
	int	inByte;
	int	outByte;
	int	inPkt;
	int	outPkt;
	int	discards;
	int	numPkt;
	};
	LinkCounts *lnkCnts;

	struct QueueCounts {
        int     bytLen;
        int     pktLen;
	int	discards;
        };
        QueueCounts *qCnts;

	// system-wide counts
	int	totInByte;
	int	totInPkt;
	int	totDiscards;
	int	rtrInByte;
	int	rtrInPkt;
	int	leafInByte;
	int	leafInPkt;
	int	totOutByte;
	int	totOutPkt;
	int	rtrOutByte;
	int	rtrOutPkt;
	int	rtrDiscards;
	int	leafOutByte;
	int	leafOutPkt;
	int	leafDiscards;

	ComtreeTable *ctt;

	// helper functions
	bool	readStat(istream&);	 
	string	stat2string(int) const;
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

inline int StatsModule::discCnt(int lnk) const {
	return lnkCnts[lnk].discards;
}

inline int StatsModule::qDiscCnt(int qid) const {
	return qCnts[qid].discards;
}

inline int StatsModule::getQlen(int qid) const {
	return qCnts[qid].pktLen;
}

inline int StatsModule::getQbytes(int qid) const {
	return qCnts[qid].bytLen;
}

inline int StatsModule::getLinkQlen(int lnk) const {
	return lnkCnts[lnk].numPkt;
}

inline void StatsModule::clearLnkStats(int lnk) {
	lnkCnts[lnk].inByte = lnkCnts[lnk].outByte = 0;
	lnkCnts[lnk].inPkt = lnkCnts[lnk].outPkt = 0;
	lnkCnts[lnk].numPkt = lnkCnts[lnk].discards = 0;
}
	
inline void StatsModule::clearQuStats(int qid) {
	qCnts[qid].bytLen = qCnts[qid].pktLen = qCnts[qid].discards = 0;
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
		totOutByte += len; totOutPkt++;
		if (isRtr) { rtrOutByte += len; rtrOutPkt++; }
		else	   { leafOutByte += len; leafOutPkt++; }
	}
}

inline void StatsModule::cntDiscards(int qid, int lnk, bool isRtr) {
	if (1 <= lnk && lnk <= maxLnk) {
		totDiscards++;
		lnkCnts[lnk].discards++;
		if (isRtr) rtrDiscards++;
		else	   leafDiscards++;
		if (1 <= qid && qid <= maxQ) 
			qCnts[qid].discards++;
	}
}

inline void StatsModule::incQlen(int qid, int lnk, int len) {
	if (1 <= lnk && lnk <= maxLnk) lnkCnts[lnk].numPkt++;
	if (1 <= qid && qid <= maxQ) {
		qCnts[qid].bytLen += len; qCnts[qid].pktLen++;
	}
}

inline void StatsModule::decQlen(int qid, int lnk, int len) {
	if (1 <= lnk && lnk <= maxLnk) lnkCnts[lnk].numPkt--;
	if (1 <= qid && qid <= maxQ) {
		qCnts[qid].bytLen -= len; qCnts[qid].pktLen--;
	}
}

} // ends namespace


#endif
