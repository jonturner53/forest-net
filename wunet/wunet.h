// Common header file for wunet sources

#ifndef WUNET_H
#define WUNET_H

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

typedef uint32_t wuAdr_t;
typedef uint32_t vnet_t;

const uint8_t WUNET_VERSION = 1; 	// version number of wunet protocol
const ipp_t WUNET_PORT = 30123; 	// port number used by wunet routers

const short int MAXLNK = 4000;
const short int MAXLC = 31;
const uint32_t BUF_SIZ = 1600;
const uint16_t MAXREFCNT = 65535;

typedef uint32_t buffer_t[BUF_SIZ/sizeof(uint32_t)];

enum ntyp_t {UNDEF_NODE=0, ROUTER=1, HOST=2};
enum ptyp_t {DATA=0, SUBSCRIBE=1, UNSUBSCRIBE=2, VOQSTATUS=3};

// Effective link packet length for a given wunet packet length
inline int truPktLeng(int x) { return 70+max(x,18); }

// Return true if given address is unicast, else false.
inline bool ucastAdr(wuAdr_t adr) { return (adr >> 31) ? false : true; }

// Return true if given address is multicast, else false.
inline bool mcastAdr(wuAdr_t adr) { return (adr >> 31) ? true : false; }

#endif
