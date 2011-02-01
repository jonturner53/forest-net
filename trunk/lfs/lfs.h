// Common header file for wunet sources

#ifndef LFS_H
#define LFS_H

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

const uint8_t LFS_VERSION = 4; 	// version number of IPv4 protocol used with LFS
const uint8_t LFS_OPTION = 53; 	// LFS option code
const ipp_t LFS_PORT = 30125; 	// port number used by overlay LFS routers

// Lfs node types
enum ntyp_t {
	UNDEF_NODE=0,
	// untrusted node types
	ENDSYS=1,	// any untrusted end system
	// trusted node types
	TRUSTED=100,	// numeric separator
	ROUTER=101,	// router component
	CONTROLLER=102	// network control element
};

// LFS packet format - appears as IP option with code=53
//
// +-----------------------------------+
// |                                   |
// |       standard IPv4 header        |
// |                                   |
// +-------+-------+--+------+---------+
// |  53   |   8   |op|flags |  rrate  |
// +-------+-------+--+----------------+
// | arate |        trace              |
// +-------+---------------------------+
// |                                   |
// |              payload              |
// |                                   |
// +-----------------------------------+
//

// Lfs operation codes
enum lfsOp_t { Control=0, FirmReq=1, SoftReq=2, Release=3 };
enum lfsCtl_t { Connect=1, Disconnect=2 };

typedef uint8_t lfs_flags_t;
const lfs_flags_t REPORT = 0x01;		// request status report

// implementation parameters
const short int MAXLNK = 31;
const short int MAXLC = 31;
const int MINBITRATE = 50; 		// 50 Kb/s
const int MAXBITRATE = 1000000;		// 1 Gb/s
const int MINPKTRATE = 50; 		// 50 p/s
const int MAXPKTRATE = 800000;		// 800 Kp/s

const uint32_t BUF_SIZ = 1600;
const uint8_t MAXREFCNT = 255;
typedef uint32_t buffer_t[BUF_SIZ/sizeof(uint32_t)];

// Effective link packet length for a given LFS packet length
inline int truPktLeng(int x) { return 70+x; }

// Return true if given address is unicast, else false.
inline bool ucastAdr(ipa_t adr) { return ((adr >> 28) >= 0xe) ? false : true; }

// Return true if given address is multicast, else false.
inline bool mcastAdr(ipa_t adr) { return ((adr >> 28) >= 0xe) ? true : false; }

#endif
