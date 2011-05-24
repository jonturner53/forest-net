/** \file StatsModule.h */

#ifndef STATSMODULE_H
#define STATSMODULE_H

#include "CommonDefs.h"
#include "LinkTable.h"
#include "QuManager.h"

class StatsModule {
public:
		StatsModule(int,LinkTable*,QuManager*);
		~StatsModule();

	/** record stats to a file */
	void	record(uint32_t);

	/** input/output */
	bool read(istream&);
	void write(ostream&) const;
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

	LinkTable *lt;			// pointer to link table
	QuManager *qm;			// pointer to queue manager

	// helper functions
	bool	readStat(istream&);	 	// read one new statistic
	void	writeStat(ostream&, int) const; // write a single stat entry
};

#endif
