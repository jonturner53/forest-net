// Header file for lcTbl class, which stores information
// about all the linecards that implement a given router.
//

#ifndef LCTBL_H
#define LCTBL_H

#include "forest.h"

class lcTbl {
public:
		lcTbl(int=1024);
		~lcTbl();
	int	lookup(ipa_t); 			// retrieve matching entry
	int	addEntry(ipa_t,int);		// create new entry
	bool	removeEntry(int);		// remove entry from table
	bool	valid(int) const;		// return true for valid entries
	// routines for returning value of various fields
	ipa_t	ipAdr(int) const;	
	int	nlc() const;
	int	maxBitRate(int) const;	
	int	maxPktRate(int) const;	
	int	bitRate(int) const;	
	int	pktRate(int) const;	
	int	voqLen(int) const;
	int	inBklg(int) const;
	int	outBklg(int) const;
	// routines for setting various fields
	void	setIpAdr(int,ipa_t); 
	void	setBitRate(int,int);
	void	setPktRate(int,int);
	void	setVoqLen(int,int);
	void	setInBklg(int,int);
	void	setOutBklg(int,int);
	// statistics routines
	uint32_t iPktCnt(int) const;
	uint32_t oPktCnt(int) const;
	uint32_t iBytCnt(int) const;
	uint32_t oBytCnt(int) const;
	void	postIcnt(int, int);
	void	postOcnt(int, int);
	// io routines
	int	getEntry(istream&);	 	// read one new table entry
	friend	bool operator>>(istream&, lcTbl&); // read entire table
	void	putEntry(ostream&, int) const; // write a single entry
	friend	ostream& operator<<(ostream&, const lcTbl&); // print table
private:
	int	maxlc;			// max number of linecards in table
	int	numlc;			// actual number of linecards
	struct lctEntry {
	ipa_t	ipa;			// IP address of line card
	// maximum data rates for inter-VOQ traffic
	int	maxbitrate;		// maximum bit rate of link (MAC level)
	int	maxpktrate;		// maximum packet rate of link
	// data for coarse-grained scheduler
	int	voqlen;			// length of VOQ to myLcn
	int	inbklg;			// backlog from all inputs
	int	outbklg;		// # of outgoing bytes
	// statistics
	uint32_t iPkt;			// input packet counter
	uint32_t oPkt;			// output packet counter
	uint32_t iByt;			// input byte counter
	uint32_t oByt;			// output byte counter
	} *lct; 			// lct[i] is data for linecard i
};

inline bool lcTbl::valid(int i) const { return (lct[i].ipa != 0); }

// Routines to access various fields in lcTbl entries
inline ipa_t lcTbl::ipAdr(int i) const 	{ return lct[i].ipa; }
inline int lcTbl::nlc() const 		{ return numlc; }
inline int lcTbl::maxBitRate(int i) const { return lct[i].maxbitrate; }
inline int lcTbl::maxPktRate(int i) const { return lct[i].maxpktrate; }
inline int lcTbl::voqLen(int i) const   { return lct[i].voqlen; }
inline int lcTbl::inBklg(int i) const   { return lct[i].inbklg; }
inline int lcTbl::outBklg(int i) const  { return lct[i].outbklg; }

// Routines for setting various fields.
// Note, cannot set IP addresses of endpoints as these define the
// the link table entries.
inline void lcTbl::setIpAdr(int i, ipa_t ipa) 	{ lct[i].ipa = ipa; }
inline void lcTbl::setVoqLen(int i, int len) 	{ lct[i].voqlen = len; }
inline void lcTbl::setInBklg(int i, int b) 	{ lct[i].inbklg = b; }
inline void lcTbl::setOutBklg(int i, int b) 	{ lct[i].outbklg = b; }

// statistics routines
inline uint32_t lcTbl::iPktCnt(int i) const { return lct[i].iPkt; }
inline uint32_t lcTbl::oPktCnt(int i) const { return lct[i].oPkt; }
inline uint32_t lcTbl::iBytCnt(int i) const { return lct[i].iByt; }
inline uint32_t lcTbl::oBytCnt(int i) const { return lct[i].oByt; }
inline void lcTbl::postIcnt(int i, int leng) {
	lct[i].iPkt++; lct[i].iByt += truPktLeng(leng);
}
inline void lcTbl::postOcnt(int i, int leng) {
	lct[i].oPkt++; lct[i].oByt += truPktLeng(leng);
}

#endif
