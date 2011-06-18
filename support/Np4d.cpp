/** @file Np4d.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Np4d.h"

/** Return the IP address for the string pointed to by ips.
 *  Return 0 if string does not represent a valid IP address
 */
ipa_t Np4d::ipAddress(const char *ips) {
	ipa_t ipa = inet_addr(ips);
	if (ipa == INADDR_NONE) return 0;
	return ntohl(ipa);
}

/** Add the string representation of an IP address to a given string.
 *  @param s is the string to be extended
 *  @param ipa is the IP address in host byte order
 */
void Np4d::addIp2string(string& s, ipa_t ipa) {
	// ugly code thanks to inet_ntoa's dreadful interface
	struct in_addr ipa_struct;
	ipa_struct.s_addr = htonl(ipa);
	s += inet_ntoa(ipa_struct);
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
	string s; addIp2string(s,adr); out << s;
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

	int x = bind(sock,(struct sockaddr *) &sa, sizeof(sa));

	return (x == 0);
}

/** Listen for a stream connection. Uses the listen
 *  system call but hides details.
 *  @param sock is socket number
 *  @return true on success, false on failure
 */
bool Np4d::listen4d(int sock) { return (listen(sock, 5) == 0); }

/** Accept the next waiting connection request.
 *  Uses accept system call but hides the ugliness.
 *  @param sock is socket number
 *  @return the socket number of the new connection or -1 on failure
 */
int Np4d::accept4d(int sock) { return accept(sock,NULL,NULL); }

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
	if (sock < 0) return -1;
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
 *  Uses recv system call.
 *  @param sock is socket number
 *  @param buf is pointer to the buffer containing the packet payload
 *  @param leng is the maximum number of bytes to be stored in buf
 *  @return the number of bytes received, or -1 on failure
 */
int Np4d::recv4d(int sock, void* buf, int leng) {
	return recv(sock,buf,leng,0);
}

/** Receive a datagram from a remote host.
 *  Uses recvfrom system call but hides the ugliness.
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
	int nbytes = recvfrom(sock,buf,leng,0,(struct sockaddr *) &sa,&socklen);
	extractSockAdr(&sa,ipa,ipp);
	return nbytes;
}

/** Test a socket to see if it has data to be read.
 *  Uses the poll system call.
 *  @param sock is the socket to be tested
 *  @return true if the socket has data to be read, else false
 */
bool Np4d::hasData(int sock) {
	struct pollfd ps;
	ps.fd = sock; ps.events = POLLIN;
	return poll(&ps, 1, 0) == 1;
}

/** Determine the amount of data available for reading from socket.
 *  @param is the socket number
 *  @return the number of bytes available for reading or -1 on an error
 */
int Np4d::dataAvail(int sock) {
	int dAvail; socklen_t daSize = sizeof(dAvail);
#ifdef SO_NREAD
        if (getsockopt(sock, SOL_SOCKET, SO_NREAD, &dAvail, &daSize) == -1)
#else
        if (ioctl(sock, SIOCINQ, &dAvail ) == -1)
#endif
                return -1;
	return dAvail;
}

/** Determine the space available for writing on a socket.
 *  @param is the socket number
 *  @return the number of bytes that can be written before the socket
 *  buffer becomes full, or -1 on error
 */
int Np4d::spaceAvail(int sock) {
	int sbSize; socklen_t sbSizeSize = sizeof(sbSize);
cout << "x\n";
	if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sbSize, &sbSizeSize) != 0)
		return -1;
cout << "y\n";
	int dQueued; socklen_t dqSize = sizeof(dQueued);
#ifdef SO_NWRITE
	if (getsockopt(sock, SOL_SOCKET, SO_NWRITE, &dQueued, &dqSize) != 0)
#else
        if (ioctl(sock, SIOCOUTQ, &dQueued ) == -1)
#endif
		return -1;
cout << "z\n";
	return sbSize - dQueued;
}

/** Read a 32 bit integer from a stream socket.
 *  Uses recv() system call but hides the ugliness.
 *  Intended for use with nonblocking socket.
 *  Will fail if the socket buffer has less than 4 bytes available to read.
 *  @param sock is socket number
 *  @param val is a reference argument; on return it will have the value read
 *  @return true on success, false on failure
 */
bool Np4d::recvInt(int sock, uint32_t& val) {
	if (dataAvail(sock) < sizeof(uint32_t)) return false;
	uint32_t temp;
	int nbytes = recv(sock, (void *) &temp, sizeof(uint32_t), 0);
	if (nbytes != sizeof(uint32_t)) 
		fatal("Np4d::recvInt: can't receive integer");
	val = ntohl(temp);
	return true;
}

/** Send a 32 bit integer on a stream socket.
 *  Uses send() system call but hides the ugliness.
 *  @param sock is socket number
 *  @param val is value to be sent
 *  @return true on success, false on failure
 */
bool Np4d::sendInt(int sock, uint32_t val) {
	if (spaceAvail(sock) < sizeof(uint32_t)) return false;
	val = htonl(val);
	int nbytes = send(sock, (void *) &val, sizeof(uint32_t), 0);
	if (nbytes != sizeof(uint32_t))
		fatal("Np4d::sendInt: can't send integer");
	return true;
}

/** Receive a vector of 32 bit integers on a stream socket.
 *  @param sock is socket number
 *  @param vec is an array in which values are to be stored
 *  @param length is the number of elements in vec
 *  @return true if all expected values are received, else false
 *
 *  After a successful return, vec will contain the received
 *  integers on host byte order. After an unsuccessful return,
 *  the value of vec is undefined.
 */
bool Np4d::recvIntVec(int sock, uint32_t vec[], int length) {
	int vecSiz = length * sizeof(uint32_t);
	if (dataAvail(sock) < vecSiz) return false;
	uint32_t buf[length];
	int nbytes = recv(sock,(void *) buf, vecSiz, 0);
	if (nbytes != vecSiz)
		fatal("Np4d::recvIntVec: can't receive vector");
	for (int i = 0; i < length; i++) vec[i] = ntohl(buf[i]);
	return true;
}

/** Send a vector of 32 bit integers on a stream socket.
 *  Uses send() system call but hides the ugliness.
 *  @param sock is socket number
 *  @param vec is array of values to be sent
 *  @param length is number of elements in vec
 *  @return true on success, false on failure
 */
bool Np4d::sendIntVec(int sock, uint32_t vec[], int length) {
	socklen_t vecSiz = length * sizeof(uint32_t);
	if (spaceAvail(sock) < vecSiz) return false;
	uint32_t buf[length];
	for (int i = 0; i < length; i++) buf[i] = htonl(vec[i]);
	int nbytes = send(sock, (void *) buf, vecSiz, 0);
	if (nbytes != vecSiz) 
		fatal("Np4d::sendIntVec: can't send vector");
	return true;
}

int Np4d::recvBuf(int sock, char* buf, int buflen) {
	uint32_t length;
	int nbytes = recv(sock,(void *) &length, sizeof(uint32_t), MSG_PEEK);
	if (nbytes != sizeof(uint32_t)) return -1;
	if (dataAvail(sock) < length) return -1;
	length = min(length, buflen);
	nbytes = recv(sock,(void *) &length, sizeof(uint32_t), 0);
	nbytes = recv(sock,(void *) buf, length - sizeof(uint32_t), 0);
	return nbytes;
}

int Np4d::sendBuf(int sock, char* buf, int buflen) {
cout << "p\n";
	if (spaceAvail(sock) < buflen + sizeof(uint32_t))
		return -1;
cout << "q\n";
	int nbytes = send(sock, (void *) &buflen, sizeof(uint32_t), 0);
cout << "r\n";
	if (nbytes != sizeof(uint32_t))
		fatal("Np4d::sendBuf: can't send buffer");
cout << "s\n";
	nbytes = send(sock, (void *) buf, buflen, 0);
cout << "t\n";
	if (nbytes != buflen)
		fatal("Np4d::sendBuf: can't send buffer");
cout << "w\n";
	return buflen;
}
