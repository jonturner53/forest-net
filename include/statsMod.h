// Header file for statsMod class, which handles recording of
// of statistics to external files
//

#ifndef STATSMOD_H
#define STATSMOD_H

#include "forest.h"
#include "lnkTbl.h"
#include "qMgr.h"

class statsMod {
public:
		statsMod(int,lnkTbl*,qMgr*);
		~statsMod();

	void	record(uint32_t);	// record statistics to files

	// io routines for reading in description of stats to record
	friend	bool operator>>(istream&, statsMod&); // read entire stat set
	friend	ostream& operator<<(ostream&, const statsMod&); // print set
protected:
	int	maxStats;		// max number of recorded statistics
	int	n;			// number of statistics to record

	enum cntrTyp {
	inPkt, outPkt, inByt, outByt,  	// input/output counts
      	qPkt, qByt,		 	// packets/bytes in queues
	};		

	struct statItem {
	int	lnk;			// link number for stat
	int	qnum;			// for queue length stats
	cntrTyp	typ;			// type of counter for this stat
	} *stat;			// stat[i] is statistic number i
	ofstream fs;			// file stream for statistics

	lnkTbl	*lt;			// pointer to link table
	qMgr	*qm;			// pointer to queue manager

	// helper functions
	bool	getStat(istream&);	 	// read one new statistic
	void	putStat(ostream&, int) const; // write a single stat entry
};

#endif
