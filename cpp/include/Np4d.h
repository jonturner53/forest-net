/** \file Np4d.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef NP4D_H
#define NP4D_H

#include "stdinc.h"
#include "Util.h"

using namespace grafalgo;

namespace forest {


// shorthands for internet address/port types
typedef in_addr_t ipa_t;
typedef in_port_t ipp_t;

/** Network programming for dummies.
 *
 *  This class defines a library of routines for IPv4 network
 *  programming, that hides much of the ugliness of the standard
 *  system calls. The big advantage is that you can avoid socket
 *  address structures completely.
 * 
 *  Many of the routines are just wrappers on the standard system
 *  calls with more intuitive interfaces.
 */
class Np4d {
public:
	// ip address utilities
	static ipa_t ipAddress(const char*); 		
	static string ip2string(ipa_t);		
	static bool readIpAdr(istream&, ipa_t&); 
	static ipa_t getIpAdr(const char*);
	static ipa_t myIpAddress();

	// socket address structure utilities
	static void initSockAdr(ipa_t, ipp_t, sockaddr_in&);
	static void extractSockAdr(sockaddr_in&, ipa_t&, ipp_t&); 
	static ipp_t getSockPort(int);
	static ipa_t getSockIp(int);
	static ipa_t getPeerIp(int);

	// setting up sockets
	static int  datagramSocket();
	static int  streamSocket();
	static bool bind4d(int, ipa_t, ipp_t);
	static bool listen4d(int);
	static int  accept4d(int);
	static int  accept4d(int, ipa_t&, ipp_t&);
	static bool connect4d(int, ipa_t, ipp_t);
	static bool nonblock(int);

	// sending and receiving datagrams
	static int  sendto4d(int, void*, int, ipa_t, ipp_t);
	static int  sendto4d(int, void*, int, sockaddr_in&);
	static int  recv4d(int, void*, int);
	static int  recvfrom4d(int, void*, int, ipa_t&, ipp_t&);

	// sending and receiving data on stream sockets
	static bool hasData(int);
	static int  dataAvail(int);
	static int  spaceAvail(int);
	static bool recvInt(int, uint32_t&);
	static bool sendInt(int, uint32_t);
	static bool recvIntBlock(int, uint32_t&);
	static bool sendIntBlock(int, uint32_t);
	static bool recvIntVec(int, uint32_t*, int);
	static bool sendIntVec(int, uint32_t*, int);
	static int  recvBuf(int, char*, int);
	static int  sendBuf(int, char*, int);
	static int  recvBufBlock(int, char*, int);
	static int  sendBufBlock(int, char*, int);
	static int  sendString(int, const string&);

	static void pack64(uint64_t, uint32_t*);
	static uint64_t unpack64(uint32_t*);
};

inline void Np4d::pack64(uint64_t x, uint32_t* p) {
	*p++ = htonl((uint32_t) ((x >> 32) & 0xffffffff));
	*p   = htonl((uint32_t) (x & 0xffffffff));
}

inline uint64_t Np4d::unpack64(uint32_t* p) {
	uint64_t x;
	x  = ((uint64_t) ntohl(*p++)) << 32;
	x |= (uint64_t) ntohl(*p);
	return x;
}

} // ends namespace


#endif
