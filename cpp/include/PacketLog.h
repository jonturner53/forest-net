/** @file PacketLog.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef PACKETLOG_H
#define PACKETLOG_H

#include "Forest.h"
#include "Misc.h"
#include "PacketStore.h"
#include "Packet.h"

namespace forest {


class PacketLog {
public:
		PacketLog(int,int,PacketStore*);
		~PacketLog();

	void 	log(int,int,bool,uint64_t);	
	void	write(ostream&) const;
private:
	int	maxPkts;		///< max # of packets to record
	int	maxData;		///< max # of data packets to record

	int	numPkts;		///< # of packets logged so far
	int	numData;		///< # of data packets logged so far

	uint64_t dumpTime;		///< # time of last dump

	struct EventStruct {
	pktx px;
        int sendFlag;
	int link;
	uint64_t time;
        };
	EventStruct *events;

	PacketStore *ps;
};

} // ends namespace


#endif
