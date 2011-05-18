// Common header file for forest.
// Includes miscellaneous definitions and common utility routines

#ifndef FOREST_H
#define FOREST_H

#include "stdinc.h"
#include "misc.h"

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

const uint8_t FOREST_VERSION = 1; 	// version number of forest protocol
const ipp_t FOREST_PORT = 30123; 	// port number used by forest routers

// Forest node types
enum ntyp_t {
	UNDEF_NODE=0,
	// untrusted node types
	CLIENT=1,	// client component
	SERVER=2,	// server component
	// trusted node types
	TRUSTED=100,	// numeric separator
	ROUTER=101,	// router component
	CONTROLLER=102	// network control element
};

// Forest packet format
//
// +----+------------+--------+--------+
// |ver |   length   |  type  |  flags |
// +-----------------------------------+
// |              comtree              |
// +-----------------------------------+
// |              src adr              |
// +-----------------------------------+
// |              dst adr              |
// +-----------------------------------+
// |              hdr chk              |
// +-----------------------------------+
// |                                   |
// |              payload              |
// |                                   |
// +-----------------------------------+
// |            payload chk            |
// +-----------------------------------+
//
// Subscribe/unsubscribe packets have additional fields that
// appear in the payload field.
// Specifically, the first word is an addcnt that specifies
// the number of multicast groups that the sender wants to
// be added to. The next addcnt words are the multicast addresses
// of those groups. The next word following the set of added
// multicast addresses is a drop count. This is followed
// by a list of multicast addresses for groups that the
// sender wants to be removed from.
//
// 

enum ptyp_t {
	UNDEF_PKT=0,
	// user packet types
	CLIENT_DATA=1,
	SUB_UNSUB=2,

	CLIENT_SIG=10,

	CONNECT=11,
	DISCONNECT=12,

	// internal control packet types
	NET_SIG=100,
	RTE_REPLY=101,

	// router internal types
	RTR_CTL=200,
	VOQSTATUS=201
};

const int HDR_LENG = 20;

const uint32_t BUF_SIZ = 1600;
const uint8_t MAXREFCNT = 255;
typedef uint32_t buffer_t[BUF_SIZ/sizeof(uint32_t)];

typedef int32_t fAdr_t;			// negative values are multicast
typedef uint32_t comt_t;
typedef uint8_t flgs_t;
const flgs_t RTE_REQ = 0x01;		// route request flag

// implementation parameters
const short int MAXLNK = 31;
const short int MAXINTF= 31;
const int MINBITRATE = 50; 		// 50 Kb/s
const int MAXBITRATE = 1000000;		// 1 Gb/s
const int MINPKTRATE = 50; 		// 50 p/s
const int MAXPKTRATE = 800000;		// 800 Kp/s

class forest {
public:
	// io helper functions
	static bool getForestAdr(istream&, fAdr_t&);// read forest address
	static void putForestAdr(ostream&, fAdr_t);// write forest address

	static int truPktLeng(int);
	static bool ucastAdr(fAdr_t);
	static bool mcastAdr(fAdr_t);
	static int zipCode(fAdr_t);
	static int localAdr(fAdr_t);
	static fAdr_t forestAdr(int,int);
	static fAdr_t forestAdr(char*);
	static char* forestStr(fAdr_t);
};

// Effective link packet length for a given forest packet length
inline int forest::truPktLeng(int x) { return 70+x; }

// Return true if given address is a valid unicast address, else false.
inline bool forest::ucastAdr(fAdr_t adr) {
	return adr > 0 && zipCode(adr) != 0 && localAdr(adr) != 0;
}

// Return true if given address is a valid multicast address, else false.
inline bool forest::mcastAdr(fAdr_t adr) { return adr < 0; }

// Return the "zip code" part of a unicast address
inline int forest::zipCode(fAdr_t adr) { return (adr >> 16) & 0x7fff; }

// Return the "local address" part of a unicast address
inline int forest::localAdr(fAdr_t adr) { return adr & 0xffff; }

// Return a forest address with a given zip code and local address
inline fAdr_t forest::forestAdr(int zip, int local ) {
	return ((zip & 0xffff) << 16) | (local & 0xffff);
}


inline fAdr_t forest::forestAdr(char *fas) {
// Return the forest address for the string pointed to by fas.
// A string representing a negative number is interpreted as a
// multicast address. Otherwise, we expect a unicast address
// with the form zip_code.local_addr.
	int zip, local, mcAdr;
	if (sscanf(fas,"%d.%d", &zip, &local) == 2 && zip >= 0)
		return forestAdr(zip,local);
	else if (sscanf(fas,"%d", &mcAdr) == 1 && mcAdr < 0)
		return mcAdr;
	else
		return 0;
}

inline char* forest::forestStr(fAdr_t fAdr) {
// Return a pointer to a character buffer containing a string
// representing the given forest address.
// Note that the buffer returned is allocated on the heap
// and it is the caller's responsbility to delete it after use.
	char* fas = new char[12];
	if (mcastAdr(fAdr)) sprintf(fas, "%d", fAdr);
	else sprintf(fas, "%d.%d", zipCode(fAdr), localAdr(fAdr));
	return fas;
}

#endif
