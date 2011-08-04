/** @file CommonDefs
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef COMMONDEFS_H
#define COMMONDEFS_H

#include "stdinc.h"
#include "Misc.h"
#include "Np4d.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>

/** Forest node types.
 *
 *  Nodes in a Forest network are assigned specific roles.
 *  Nodes with node type codes smaller than 100, are considered
 *  untrusted. All packets received from such hosts are subjected
 *  to extra checks. For example, they may only send packets with
 *  a source address equal to their assigned address.
 */
enum ntyp_t {
	UNDEF_NODE=0,
	// untrusted node types
	CLIENT=1,	///< client component
	SERVER=2,	///< server component
	// trusted node types
	TRUSTED=100,	///< numeric separator
	ROUTER=101,	///< router component
	CONTROLLER=102	///< network control element
};

/** Forest packet types.
 *  This enumeration lists the distinct packet types that are
 *  currently defined. These are the types that go in the type
 *  field of the first word of each Forest packet.
 */
enum ptyp_t {
	UNDEF_PKT=0,
	// client packet types
	CLIENT_DATA=1,		///< normal data packet from a host
	SUB_UNSUB=2,		///< subscribe to multicast groups

	CLIENT_SIG=10,		///< client signalling packet

	CONNECT=11,		///< establish connection for an access link
	DISCONNECT=12,		///< disconnect anaccess link

	// internal control packet types
	NET_SIG=100,		///< network signalling packet
	RTE_REPLY=101,		///< route reply for multicast route learning

	// router internal types
	RTR_CTL=200,
	VOQSTATUS=201
};

typedef int32_t fAdr_t;			///< denotes a forest address
typedef uint32_t comt_t;		///< denotes a comtree
typedef uint8_t flgs_t;			///< flags field from packet header

/** Miscellaneous utility functions.
 *  This class defines various constants and common functions useful
 *  within a Forest router and Forest hosts.
 */
class Forest {
public:
	/** constants related to packet formats */
	static const uint8_t FOREST_VERSION = 1;///< version of forest protocol
	static const int HDR_LENG = 20;		///< header length in bytes
	static const int OVERHEAD = 24;		///< total overhead
	static const flgs_t RTE_REQ = 0x01;	///< route request flag
	static const ipp_t ROUTER_PORT = 30123; ///< port # used by routers

	/** router implementation parameters */
	static const short int MAXLNK = 31;	///< max # of links per router
	static const short int MAXINTF= 20;	///< max # of interfaces
	static const int MINBITRATE = 500; 	///< min link bit rate in Kb/s
	static const int MAXBITRATE = 1000000;	///< max link bit rate in Kb/s
	static const int MINPKTRATE = 500; 	///< min packet rate in p/s
	static const int MAXPKTRATE = 800000;	///< max packet rate in p/s
	static const uint32_t BUF_SIZ = 1600;	///< size of a packet buffer

	/** methods for manipulating addresses */
	static bool validUcastAdr(fAdr_t);
	static bool mcastAdr(fAdr_t);
	static int zipCode(fAdr_t);
	static int localAdr(fAdr_t);
	static fAdr_t forestAdr(int,int);
	static fAdr_t forestAdr(const char*);
	static void addFadr2string(string&, fAdr_t);
	static string& fAdr2string(fAdr_t, string&);
	static bool readForestAdr(istream&, fAdr_t&);
	static void writeForestAdr(ostream&, fAdr_t);

	/** miscellaneous */
	static int truPktLeng(int);
	static void addNodeType2string(string&, ntyp_t);
	static string& nodeType2string(ntyp_t, string&);
	static ntyp_t getNodeType(string&);
};

typedef uint32_t buffer_t[Forest::BUF_SIZ/sizeof(uint32_t)];

/** Determine if given Forest address is a valid unicast address.
 *  @param adr is a Forest address
 *  @return true if it is a valid unicast address (is greater than
 *  zero and both the zip code and local part of the address are >0)
 */
inline bool Forest::validUcastAdr(fAdr_t adr) {
	return adr > 0 && zipCode(adr) != 0 && localAdr(adr) != 0;
}

/** Determine if given Forest address is a valid multicast address.
 *  @param adr is a Forest address
 *  @return true if it is a valid multicast address (is <0)
 */
inline bool Forest::mcastAdr(fAdr_t adr) { return adr < 0; }

/** Get the zip code of a unicast address.
 *  Assumes that the address is valid.
 *  @param adr is a Forest address
 *  @return the zip code part of the address
 */
inline int Forest::zipCode(fAdr_t adr) { return (adr >> 16) & 0x7fff; }

/** Get the local address part of a unicast address.
 *  Assumes that the address is valid.
 *  @param adr is a Forest address
 *  @return the local address part of the address
 */
inline int Forest::localAdr(fAdr_t adr) { return adr & 0xffff; }

/** Construct a forest address from a zip code and local address.
 *  Assumes that both arguments are >0.
 *  @param zip is the zip code part of the address
 *  @param local is the local address part
 *  @return the corresponding unicast address
 */
inline fAdr_t Forest::forestAdr(int zip, int local ) {
	return ((zip & 0xffff) << 16) | (local & 0xffff);
}

/** Construct a forest address for the string pointed to by fas.
 *  
 *  A string representing a negative number is interpreted as a
 *  multicast address. Otherwise, we expect a unicast address
 *  with the form zip_code.local_addr.
 *
 *  @param fas is the forest address string to be converted
 *  @return the corresponding forest address, or 0 if the input
 *  is not a valid address
 */
inline fAdr_t Forest::forestAdr(const char *fas) {
	int zip, local, mcAdr;
	if (sscanf(fas,"%d.%d", &zip, &local) == 2 && zip > 0 && local > 0)
		return forestAdr(zip,local);
	else if (sscanf(fas,"%d", &mcAdr) == 1 && mcAdr < 0)
		return mcAdr;
	else
		return 0;
}

/** Append the string representation of a Forest address to a given string.
 *  
 *  @param s is the string to be extended
 *  @param fAdr is the forest address which is to be appended to the end of s
 */
inline void Forest::addFadr2string(string& s, fAdr_t fAdr) {
	char fas[16];
	if (mcastAdr(fAdr)) sprintf(fas, "%d", fAdr);
	else sprintf(fas, "%d.%d", zipCode(fAdr), localAdr(fAdr));
	s += fas;
}

/** Create a string representation of a forest address.
 *  
 *  @param fAdr is the forest address which is to be appended to the end of s
 *  @param s is the string to be extended
 *  @return a reference to the modified string
 */
inline string& Forest::fAdr2string(fAdr_t fAdr, string& s) {
	char fas[16];
	if (mcastAdr(fAdr)) sprintf(fas, "%d", fAdr);
	else sprintf(fas, "%d.%d", zipCode(fAdr), localAdr(fAdr));
	s = fas;
	return s;
}

/** Compute link packet length for a given forest packet length.
 *
 *  @param x is the number of bytes in the Forest packet
 *  @return the number of bytes sent on the link, including the
 *  IP/UDP header and a presumed Ethernet header plus inter-frame gap.
 */
inline int Forest::truPktLeng(int x) { return 70+x; }

#endif