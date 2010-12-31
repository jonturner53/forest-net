// Header file for lnkTbl class, which stores information
// about all the links incident to a given router.
//

#ifndef LNKTBL_H
#define LNKTBL_H

#include "wunet.h"

struct lnkdata {
	ipa_t	ipa;			// IP address of local endpoint
	ipa_t	pipa;			// IP address of peer endpoint
	ipp_t	pipp;			// IP address of peer port number
	ntyp_t	ptyp;			// node type of peer
	wuAdr_t	padr;			// peer's wunet address
	int	bitrate;		// maximum bit rate of link (MAC level)
	int	pktrate;		// maximum packet rate of link
	int	mindelta;		// minimum time between packets (us)
	uint32_t iPkt;			// input packet counter
	uint32_t oPkt;			// output packet counter
	uint32_t iByt;			// input byte counter
	uint32_t oByt;			// output byte counter
};

class lnkTbl {
public:
		lnkTbl(int=1024);
		~lnkTbl();
	int	lookup(ipa_t,ipp_t); 		// retrieve matching entry
	int	addEntry(ipa_t,ipa_t,ipp_t,int);// create new entry
	bool	removeEntry(int);		// remove entry from table
	bool	valid(int) const;		// return true for valid entries
	// routines for returning value of various fields
	ipa_t	ipAdr(int) const;	
	ipa_t	peerIpAdr(int) const;	
	ipp_t 	peerPort(int) const;	
	ntyp_t 	peerTyp(int) const;	
	wuAdr_t	peerAdr(int) const;	
	int	bitRate(int) const;	
	int	pktRate(int) const;	
	int	minDelta(int) const;	
	// routines for setting various fields
	void	setIpAdr(int,ipa_t); 
	void	setPeerIpAdr(int,ipa_t); 
	void	setPeerPort(int,ipp_t); 
	void	setPeerTyp(int,ntyp_t); 
	void	setPeerAdr(int, wuAdr_t);
	void	setBitRate(int,int);
	void	setPktRate(int,int);
	// statistics routines
	uint32_t iPktCnt(int) const;
	uint32_t oPktCnt(int) const;
	uint32_t iBytCnt(int) const;
	uint32_t oBytCnt(int) const;
	void	postIcnt(int, int);
	void	postOcnt(int, int);
	// io routines
	int	getEntry(istream&);	 	// read one new table entry
	friend	bool operator>>(istream&, lnkTbl&); // read entire table
	void	putEntry(ostream&, int) const; // write a single entry
	friend	ostream& operator<<(ostream&, const lnkTbl&); // print table
private:
	int	nlnk;			// max number of links in table
	lnkdata	*ld;			// ld[i] is link data for link i
};

inline bool lnkTbl::valid(int i) const { return (ld[i].padr != 0); }

// Routines to access various fields in lnkTbl entries
inline ipa_t lnkTbl::ipAdr(int i) const 	{ return ld[i].ipa; }
inline ipa_t lnkTbl::peerIpAdr(int i) const 	{ return ld[i].pipa; }
inline ipp_t lnkTbl::peerPort(int i) const 	{ return ld[i].pipp; }
inline ntyp_t lnkTbl::peerTyp(int i) const 	{ return ld[i].ptyp; }
inline wuAdr_t lnkTbl::peerAdr(int i) const 	{ return ld[i].padr; }
inline int lnkTbl::bitRate(int i) const 	{ return ld[i].bitrate; }
inline int lnkTbl::pktRate(int i) const 	{ return ld[i].pktrate; }
inline int lnkTbl::minDelta(int i) const 	{ return ld[i].mindelta; }

// Routines for setting various fields.
// Note, cannot set IP addresses of endpoints as these define the
// the link table entries.
inline void lnkTbl::setIpAdr(int i, ipa_t ipa) 	{ ld[i].ipa = ipa; }
inline void lnkTbl::setPeerIpAdr(int i, ipa_t pipa) 	{ ld[i].pipa = pipa; }
inline void lnkTbl::setPeerPort(int i, ipp_t pipp) 	{ ld[i].pipp = pipp; }
inline void lnkTbl::setPeerTyp(int i, ntyp_t pt) 	{ ld[i].ptyp = pt; }
inline void lnkTbl::setPeerAdr(int i, wuAdr_t pa) 	{ ld[i].padr = pa; }
inline void lnkTbl::setBitRate(int i, int br) 	{ ld[i].bitrate = max(br,10); }
inline void lnkTbl::setPktRate(int i, int pr) {
	ld[i].pktrate = max(pr,5);
	ld[i].mindelta = int(1000000.0/pr);
}

// statistics routines
inline uint32_t lnkTbl::iPktCnt(int i) const { return ld[i].iPkt; }
inline uint32_t lnkTbl::oPktCnt(int i) const { return ld[i].oPkt; }
inline uint32_t lnkTbl::iBytCnt(int i) const { return ld[i].iByt; }
inline uint32_t lnkTbl::oBytCnt(int i) const { return ld[i].oByt; }
inline void lnkTbl::postIcnt(int i, int leng) {
	ld[i].iPkt++; ld[i].iByt += truPktLeng(leng);
}
inline void lnkTbl::postOcnt(int i, int leng) {
	ld[i].oPkt++; ld[i].oByt += truPktLeng(leng);
}

#endif
