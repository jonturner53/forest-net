/** Simple program to test tunneling in ONL.
 *
 *  Opens a TCP port (30124) and after connection, echos back each
 *  line it receives.
 */

#include "stdinc.h"
#include "Misc.h"
#include "Np4d.h"

main() {
	ipa_t myIp = Np4d::myIpAddress();
	if (myIp == 0) fatal("can't determine my IP address");
	cout << "my adrress is ";
	Np4d::writeIpAdr(cout, myIp);
	cout << endl;

	// create stream socket
	int sock = Np4d::streamSocket();
	if (sock < 0) fatal("can't setup socket");

	// bind to default address and port 30124
	if (!Np4d::bind4d(sock, myIp, 30124))
		fatal("can't bind to default address and port");

	// wait for a connection and accept it
        if (!Np4d::listen4d(sock)) fatal("failed on listen");
	ipa_t farIp; ipp_t farPort;
        int newSock = Np4d::accept4d(sock,farIp,farPort);
	if (newSock < 0) fatal("failed on accept");

	cout << "accepted connection from ";
	Np4d::writeIpAdr(cout,farIp);
	cout << ":" << farPort << endl;

	if (!Np4d::nonblock(newSock))
		fatal("can't configure socket to be nonblocking");

	cout << "entering echo loop" << endl;
	while (true) {
		char buf[501];
        	int nbytes = read(newSock,(char *) buf, 500);
		buf[500] = 0;
		if (nbytes > 0) {
			write(newSock, (void *) buf, 1+Misc::strnlen(buf,500));
			cout << "echoing: " << buf << "\n";
		} else {
			useconds_t delay = 100000;
			usleep(delay);
		}
	}
	exit(0);
}
