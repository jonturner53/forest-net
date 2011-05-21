/** \file Np4d.h
 *  Network programming for dummies.
 *
 *  This class defines a library of routines for IPv4 network
 *  programming, that hides much of the ugliness of the standard
 *  system calls. The big advantage is that you can avoid socket
 *  address structures completely.
 * 
 *  Many of the routines are just wrappers on the standard system
 *  calls with more intuitive interfaces.
 */

#ifndef NP4D_H
#define NP4D_H

#include "stdinc.h"
#include "Misc.h"

// shorthands for internet address/port types
typedef in_addr_t ipa_t;
typedef in_port_t ipp_t;

class Np4d {
public:
	// ip address utilities
	static ipa_t ipAddress(char*); 		
	static char* ipString(ipa_t);		
	static bool readIpAdr(istream&, ipa_t&); 
	static void writeIpAdr(ostream&, ipa_t);
	static ipa_t getIpAdr(char*);
	static ipa_t myIpAddress();

	// socket address structure utilities
	static void initSockAdr(ipa_t, ipp_t, sockaddr_in*);
	static void extractSockAdr(sockaddr_in*, ipa_t&, ipp_t&); 

	// setting up sockets
	static int datagramSocket();
	static int streamSocket();
	static bool bind4d(int, ipa_t, ipp_t);
	static bool listen4d(int);
	static int accept4d(int, ipa_t&, ipp_t&);
	static bool connect4d(int, ipa_t, ipp_t);
	static bool nonblock(int);

	// sending and receiving deatagrams
	static int sendto4d(int, void*, int, ipa_t, ipp_t);
	static int recvfrom4d(int, void*, int, ipa_t&, ipp_t&);
};

#endif