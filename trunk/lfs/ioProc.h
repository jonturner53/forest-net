// Header file for ioProc class, which supports io processing.

#ifndef IOPROC_H
#define IOPROC_H

#include "lfs.h"
#include "lnkTbl.h"
#include "pktStore.h"

class ioProc {
public:
		ioProc(lnkTbl*, pktStore*);
		~ioProc();

	packet	receive();		// return next input packet
	void	send(packet,int);	// send packet
	bool 	valid(int) const;	// check for valid interface
	bool	addEntry(int,ipa_t,int,int); // add interface table entry
	void	removeEntry(int); 	// remove interface table entry
	int	lookup(ipa_t);		// return interfae # for given IP addr
	bool	checkEntry(int); 	// do consistency check
	// routines for accessing interface table fields
	ipa_t	ipAdr(int) const;	
	int&	fpi(int);		// return ref to fastpath interface #
	int&	maxBitRate(int);
	int&	maxPktRate(int);
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
	int	fpi;			// fastpath interface number
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
inline int& ioProc::fpi(int i) 			{ return ift[i].fpi; }	
inline int& ioProc::maxBitRate(int i) 		{ return ift[i].maxbitrate; }
inline int& ioProc::maxPktRate(int i) 		{ return ift[i].maxpktrate; }

#endif
