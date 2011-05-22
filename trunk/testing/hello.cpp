/** Simple program to test tunneling in ONL.
 *
 *  Connects to a remote TCP server, sends some text
 *  and prints any text it gets back.
 */

#include "stdinc.h"
#include "Np4d.h"

main(int argc, char* argv[]) {
	if (argc != 3)
		fatal("usage: hello hostname port");

	ipa_t farIp = Np4d::getIpAdr(argv[1]);
	if (farIp == 0)
		fatal("can't get remote host's address");

	int farPort;
	sscanf(argv[2],"%d",&farPort);
	cout << "connecting to "; Np4d::writeIpAdr(cout, farIp);
	cout << ":" << farPort << endl;

        // create stream socket
        int sock = Np4d::streamSocket();
        if (sock < 0) fatal("can't open socket");

	if (!Np4d::connect4d(sock, farIp, farPort))
		fatal("can't establish connection");

	char buf[501];
	write(sock, (void *) "hello ", 7);
        int n = read(sock,(char *) buf, 500);
	cout << "read " << n << " bytes: " << buf << endl;

	char buf2[501];
	write(sock, (void *) "world ", 7);
        n = read(sock,(char *) buf2, 500);
	cout << "read " << n << " bytes: " << buf2 << endl;

	exit(0);
}
