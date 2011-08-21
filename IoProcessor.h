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
#include "IfaceTable.h"
#include "LinkTable.h"
#include "PacketStore.h"
#include "StatsModule.h"

class IoProcessor {
public:
		IoProcessor(int, IfaceTable*, LinkTable*,
			    PacketStore*, StatsModule*);
		~IoProcessor();

	int	receive();	
	void	send(int,int);

private:
	int	maxIface;		///< largest interface number
	int	maxSockNum;		///< largest socket num opened by ioProc
	fd_set	*sockets;		///< file descr set for open sockets
	int	cIf;			///< number of "current interface"
	int	nRdy;			///< number of ready sockets
	int	*sock;			///< sock[i] is socket for iface i

	IfaceTable *ift;		///< pointer to interface table
	LinkTable *lt;			///< pointer to link table
	PacketStore *ps;		///< pointer to packet store
	StatsModule *sm;		///< pointer to statistics module

	// helper methods
	bool	setup(int);
};


#endif