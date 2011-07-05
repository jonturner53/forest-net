/** @file IoProcessor.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

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
	int	getDefaultIface() const;

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

	struct ifTbl {
	ipa_t	ipa;			// IP address of interface
	int	sock;			// socket number of interface
	int	maxbitrate;		// max bit rate for interface (Kb/s)
	int	maxpktrate;		// max packet rate for interface
	} ift[Forest::MAXINTF+1];	// ift[i]=data for interface i

	LinkTable *lt;			// pointer to link table
	PacketStore *ps;		// pointer to packet store

	// helper methods
	bool	setup(int);
	int	readEntry(istream&);		
	void	writeEntry(ostream&, int) const;
};

/** Check an interface number for validity.
 *  @param i is the interface to be checked
 *  @return true if i is a valid interface number, else false
 */
inline bool IoProcessor::valid(int i) const {
	return 1 <= i && i <= Forest::MAXINTF && ift[i].ipa != 0;
}

/** Get the IP address associated with a given interface.
 *  @param i is the interface number
 *  @return the associated IP address
 */
inline ipa_t IoProcessor::getIpAdr(int i) const { return ift[i].ipa; }	

/** Get the maximum bit rate allowed for this interface.
 *  @param i is the interface number
 *  @return the bit rate in Kb/s
 */
inline int IoProcessor::getMaxBitRate(int i) const { return ift[i].maxbitrate; }

/** Get the maximum packet rate allowed for this interface.
 *  @param i is the interface number
 *  @return the packet rate in p/s
 */
inline int IoProcessor::getMaxPktRate(int i) const { return ift[i].maxpktrate; }

/** Set the maximum bit rate for this interface.
 *  @param i is the interface number
 *  @param r is the max bit rate in Kb/s
 */
inline void IoProcessor::setMaxBitRate(int i, int r) { ift[i].maxbitrate = r; }

/** Set the maximum packet rate for this interface.
 *  @param i is the interface number
 *  @param r is the max packet rate in Kb/s
 */
inline void IoProcessor::setMaxPktRate(int i, int r) { ift[i].maxpktrate = r; }

#endif
