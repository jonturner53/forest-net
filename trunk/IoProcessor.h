/** \file IoProcessor.h */

#ifndef IOPROCESSOR_H
#define IOPROCESSOR_H

#include "CommonDefs.h"
#include "LinkTable.h"
#include "PacketStore.h"

class IoProcessor {
public:
		IoProcessor(LinkTable*, PacketStore*);
		~IoProcessor();

	/** access methods */
	ipa_t	getIpAdr(int) const;	
	int	getMaxBitRate(int) const;
	int	getMaxPktRate(int) const;

	/** predicates */
	bool 	valid(int) const;	
	bool	checkEntry(int); 

	/** modifiers */
	int	receive();	
	void	send(int,int);
	bool	addEntry(int,ipa_t,int,int);
	void	removeEntry(int);
	void	setMaxBitRate(int, int);
	void	setMaxPktRate(int, int);

	// io routines
	bool	read(istream&);
	void	write(ostream&) const;
private:
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

	LinkTable *lt;			// pointer to link table
	PacketStore *ps;		// pointer to packet store

	// helper methods
	bool	setup(int);
	int	readEntry(istream&);		
	void	writeEntry(ostream&, int) const;
};

// Return true if there is a valid entry for interface i
inline bool IoProcessor::valid(int i) const {
	return 1 <= i && i <= MAXINT && ift[i].ipa != 0;
}

// routines to access and set various fields in interface table
inline ipa_t IoProcessor::getIpAdr(int i) const { return ift[i].ipa; }	
inline int IoProcessor::getMaxBitRate(int i) const { return ift[i].maxbitrate; }
inline int IoProcessor::getMaxPktRate(int i) const { return ift[i].maxpktrate; }
inline void IoProcessor::setMaxBitRate(int i, int r) { ift[i].maxbitrate = r; }
inline void IoProcessor::setMaxPktRate(int i, int r) { ift[i].maxpktrate = r; }

#endif
