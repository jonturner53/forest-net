/** @file PacketFilter.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef PACKETFILTER
#define PACKETFILTER

#include "Forest.h"
#include "Misc.h"
#include "NetBuffer.h"
#include "Packet.h"
#include "CtlPkt.h"

namespace forest {

/** Support class used by PacketLog to control packet logging */
class PacketFilter {
public:
	bool	on;			///< true if filter is turned enabled
	int	lnk;			///< input or output link for packet
	bool	in;			///< match incoming packets
	bool	out;			///< match out-going packets
	int	comt;			///< comtree field for packet
	fAdr_t	srcAdr;			///< source address
	fAdr_t	dstAdr;			///< destination address
	Forest::ptyp_t type;		///< packet type
	CtlPkt::CpType cpType;		///< control packet type

		PacketFilter();
	string& toString(string& s) const;
	bool fromString(string& s);
};

} // ends namespace

#endif


