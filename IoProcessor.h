// Header file for ioProc class, which supports io processing.

#ifndef IOPROC_H
#define IOPROC_H

#include "forest.h"
#include "lnkTbl.h"
#include "pktStore.h"

class ioProc {
public:
		ioProc(lnkTbl*, pktStore*);
		~ioProc();

	int	receive();		// return next input packet
	void	send(int,int);		// send packet
	bool 	valid(int) const;	// check for valid interface
	bool	addEntry(int,ipa_t,int,int); // add interface table entry
	void	removeEntry(int); 	// remove interface table entry
	bool	checkEntry(int); 	// do consistency check
	// routines for accessing interface table fields
	ipa_t	ipAdr(int) const;	
	int	maxBitRate(int) const;
	int	maxPktRate(int) const;
	// routines for setting interface table fields
	void	setMaxBitRate(int, int);
	void	setMaxPktRate(int, int);
	// io routines
	friend	bool operator>>(istream&, ioProc&);
	friend	ostream& operator<<(ostream&, const ioProc&);
private:
	sockaddr_in dsa;		// destination socket address structure

	int	maxSockNum;		// largest socket num opened by ioProc
	fd_set	*sockets;		// file descriptor set for open sockets
	int	cIf;			// number of "current interface"
	int	nRdy;			// number of ready sockets

	static int const MAXINT = 20;	// max number of distinct interfaces
	struct ifTbl {
	ipa_t	ipa;			// IP address of interface
	int	sock;			// socket number of interface
	int	maxbitrate;		// max bit rate for interface (Kb/s)
	int	maxpktrate;		// max packet rate for interface
	} ift[MAXINT+1];		// ift[i]=data for interface i

	lnkTbl *lt;			// pointer to link table
	pktStore *ps;			// pointer to packet store

	// helper methods
	int	getEntry(istream&);		// read ift entry
	void	putEntry(ostream&, int) const;	// write ift entry
	bool	setup(int);			// open and setup socket
};

// Return true if there is a valid entry for interface i
inline bool ioProc::valid(int i) const {
	return 1 <= i && i <= MAXINT && ift[i].ipa != 0;
}

// routines to access and set various fields in interface table
inline ipa_t ioProc::ipAdr(int i) const		{ return ift[i].ipa; }	
inline int ioProc::maxBitRate(int i) const	{ return ift[i].maxbitrate; }
inline int ioProc::maxPktRate(int i) const	{ return ift[i].maxpktrate; }
inline void ioProc::setMaxBitRate(int i, int r) { ift[i].maxbitrate = r; }
inline void ioProc::setMaxPktRate(int i, int r) { ift[i].maxpktrate = r; }

#endif
