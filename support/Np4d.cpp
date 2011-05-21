/** \file Np4d.cpp */

#include "Np4d.h"

/** Return the IP address for the string pointed to by ips.
 *  Return 0 if string does not represent a valid IP address
 */
ipa_t Np4d::ipAddress(char *ips) {
	ipa_t ipa = inet_addr(ips);
	if (ipa == INADDR_NONE) return 0;
	return ntohl(ipa);
}

/** Return a pointer to a character buffer containing a
 *  dotted decimal string representing the given IP address.
 *  Note that the buffer returned is allocated on the heap
 *  and it is the caller's responsibility to delete it after
 *  use.
 */
char* Np4d::ipString(ipa_t ipa) {
	// ugly code thanks to inet_ntoa's dreadful inteface
	struct in_addr ipa_struct;
	ipa_struct.s_addr = htonl(ipa);
	char* p = inet_ntoa(ipa_struct);
	char *ips = new char[16];
	strncpy(ips,p,15);
	ips[15] = '\0';
	return ips;
}

/** If next thing on the current line is an ip address,
 *  return it in ipa and return true. Otherwise, return false.
 *  The expected format is dotted-decimal.
 *  The address is returned in host byte order.
 */
bool Np4d::readIpAdr(istream& in, ipa_t& ipa) {
	char adr[4];
	
	if (!Misc::readNum(in,adr[0]) || !Misc::verify(in,'.') ||
	    !Misc::readNum(in,adr[1]) || !Misc::verify(in,'.') ||
	    !Misc::readNum(in,adr[2]) || !Misc::verify(in,'.') || 
	    !Misc::readNum(in,adr[3]))
		return false;
	ipa = ntohl(*((ipa_t*) &adr[0]));
	return true;
}


void Np4d::writeIpAdr(ostream& out, ipa_t adr) {
        char *ips = ipString(adr); out << ips; delete ips;
}

/** Get the default IP address of a specified host
 *  @param hostName is the anme of the host
 *  @return an IP address in host byte order, or 0 on failure
 */
ipa_t Np4d::getIpAdr(char* hostName) {
        hostent* host = gethostbyname(hostName);
        if (host == NULL) return 0;
	return ntohl(*((ipa_t*) &host->h_addr_list[0][0]));
}

/** Get the default IP address of this host.
 *  @return the IP address in host byte order, or 0 on failure
 */
ipa_t Np4d::myIpAddress() {
 	char myName[1001];
        if (gethostname(myName, 1000) != 0) return 0;
	return getIpAdr(myName);
}

/** Initialize a socket address structure with a specified
 *  IP address and port number.
 *  @param ipa is an IP address
 *  @param ipp is a port number
 *  @param sap is pointer to the IP address structure
 */
void Np4d::initSockAdr(ipa_t ipa, ipp_t port, sockaddr_in *sap) {
        bzero(sap, sizeof(sockaddr_in));
        sap->sin_family = AF_INET;
        sap->sin_addr.s_addr = (ipa == 0 ? INADDR_ANY : htonl(ipa));
        sap->sin_port = htons(port);
}

/** Extract an IP address and port number from a socket address structure.
 *  @param sap is pointer to the IP address structure
 *  @param ipa is a reference to an IP address
 *  @param ipp is a reference to a port number
 */
void Np4d::extractSockAdr(sockaddr_in *sap, ipa_t& ipa, ipp_t& ipp) {
        ipa = ntohl(sap->sin_addr.s_addr);
        ipp = ntohs(sap->sin_port);
}

/** Configure a socket to be nonblocking.
 *  @param sock is the socket number
 *  @return true on success, false on failure
 */
bool Np4d::nonblock(int sock) {
        int flags;
        if ((flags = fcntl(sock, F_GETFL, 0)) < 0) return false;
        flags |= O_NONBLOCK;
        if ((flags = fcntl(sock, F_SETFL, flags)) < 0) return false;
        return true;
}

/** Open a datagram socket.
 *  @return socket number or 0 on failure
 */
int Np4d::datagramSocket() {
	return socket(AF_INET,SOCK_DGRAM,0);
}

/** Open a stream socket.
 *  @return socket number or 0 on failure
 */
int Np4d::streamSocket() {
	return socket(AF_INET,SOCK_STREAM,0);
}

/** Bind a socket to a specified address and port.
 *  Uses bind system call but hides the ugliness.
 *  @param sock is socket number
 *  @param ipa is IP address to bind to
 *  @param ipp is IP port to bind to
 *  @return true on success, false on failure
 */
bool Np4d::bind4d(int sock, ipa_t ipa, ipp_t ipp) {
	sockaddr_in sa;
	initSockAdr(ipa,ipp,&sa);
	return (bind(sock,(struct sockaddr *) &sa, sizeof(sa)) == 0);
}

/** Listen for a stream connection. Uses the listen
 *  system call but hides details.
 *  @param sock is socket number
 *  @return true on success, false on failure
 */
bool Np4d::listen4d(int sock) {
	return (listen(sock, 100) == 0);
}

/** Accept the next waiting connection request.
 *  Uses accept system call but hides the ugliness.
 *  @param sock is socket number
 *  @param ipa is a reference to an IP address;
 *  on return its value is the address of the remote host
 *  @param ipp is a reference to an IP port number;
 *  on return its value is the port number of the remote host
 *  @return the socket number of the new connection or -1 on failure
 */
int Np4d::accept4d(int sock, ipa_t& ipa, ipp_t& ipp) {
	sockaddr_in sa; socklen_t len = sizeof(sa);
	sock = accept(sock,(struct sockaddr *) &sa, &len);
	extractSockAdr(&sa,ipa,ipp);
	return sock;
}

/** Connect to a remote host.
 *  Uses connect system call but hides the ugliness.
 *  @param sock is socket number
 *  @param ipa is IP address of remote host
 *  @param ipp is IP port or the remote host
 *  @return true on success, false on failure
 */
bool Np4d::connect4d(int sock, ipa_t ipa, ipp_t ipp) {
	sockaddr_in sa; initSockAdr(ipa,ipp,&sa);
	return (connect(sock,(struct sockaddr *) &sa,sizeof(sa)) == 0);
}

/** Send a datagram to a remote host.
 *  Uses sendto system call but hides the ugliness.
 *  @param sock is socket number
 *  @param buf is pointer to the buffer containing the packet payload
 *  @param leng is the number of bytes to be sent
 *  @param ipa is IP address of remote host
 *  @param ipp is IP port or the remote host
 *  @return number of bytes sent, or -1 on failure
 */
int Np4d::sendto4d(int sock, void* buf, int leng, ipa_t ipa, ipp_t ipp) {
	sockaddr_in sa; initSockAdr(ipa,ipp,&sa);
	return sendto(sock,buf,leng,0,(struct sockaddr *) &sa, sizeof(sa));
}

/** Receive a datagram from a remote host.
 *  Uses receivefrom system call but hides the ugliness.
 *  @param sock is socket number
 *  @param buf is pointer to the buffer containing the packet payload
 *  @param leng is the maximum number of bytes to be stored in buf
 *  @param ipa is a reference to an IP address; on return,
 *  it contains the address of the remote host
 *  @param ipp is a reference to an IP port; on return,
  * it contains the port number of the remote host
 *  @return the number of bytes received, or -1 on failure
 */
int Np4d::recvfrom4d(int sock, void* buf, int leng, ipa_t& ipa, ipp_t& ipp) {
	sockaddr_in sa; initSockAdr(ipa,ipp,&sa);
	socklen_t socklen = sizeof(sa);
	int nbytes = recvfrom(sock,buf,leng,0,(struct sockaddr *) &sa, &socklen);
	extractSockAdr(&sa,ipa,ipp);
	return nbytes;
}
