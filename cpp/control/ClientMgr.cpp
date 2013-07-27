/** @file ClientMgr.cpp 
 *
 *  @author Jonathan Turner (based on earlier version by Logan Stafman)
 *  @author Andrew Kelley and Sam Jonesi added extensions to display
 *  and cancel sessions
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ClientMgr.h"
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace forest;

/** usage:
 *  ClientMgr nmIp myIp finTime
 *
 *  Command line arguments include the following:
 *  nmIp - IP address of network manager
 *  myIp - IP address of the preferred interface for client manager
 *  finTime - how many seconds this program should run before terminating
 */
int main(int argc, char *argv[]) {
	ipa_t nmIp, myIp; uint32_t finTime = 0;

	if(argc != 4 ||
	    (nmIp = Np4d::ipAddress(argv[1])) == 0 ||
	    (myIp = Np4d::ipAddress(argv[2])) == 0 ||
	    (sscanf(argv[3],"%d", &finTime)) != 1) 
		fatal("usage: ClientMgr nmIp myIp fintime");
	
	if (!init(nmIp, myIp))
		fatal("init: Failed to initialize ClientMgr");
	
	sub->run(finTime);

	return 0;
}

namespace forest {

/** Initializes sockets, and acts as a psuedo-constructor
 *  @param nmIp1 is the IP address of the network manager
 *  @param myIp1 is the IP address to bind sockets to
 *  @return true on success, false on failure
 */
bool init(ipa_t nmIp1, ipa_t myIp1) {
	nmIp = nmIp1; myIp = myIp1;

	ps = new PacketStoreTs(10000);
	cliTbl = new ClientTable(10000,30000);
	logger = new Logger();

	if (!cliTbl->init()) {
		logger->log("ClientMgr::init: could not initialize "
			    "client data",2);
		return false;
	}

	uint64_t nonce;
	if (!bootMe(nmIp, myIp, nmAdr, myAdr, rtrAdr, rtrIp, rtrPort, nonce)) {
		return false;
	}

	sub = new Substrate(myAdr, myIp, rtrAdr, rtrIp, rtrPort, nonce,
			    500,handler,0,Forest::CM_PORT,ps,logger);
	if (!sub->init()) return false;
	sub->setRtrReady(true);

	dummyRecord = 0; maxRecord = 0;

	// read clientData file
	clientFile.open("clientData");
	if (!clientFile.good() || !cliTbl->read(clientFile)) {
		logger->log("ClientMgr::init: could not read clientData "
			    "file",2);
		return false;
	}
	clientFile.clear();

	// if size of file is not equal to count*RECORD_SIZE
	// re-write the file using padded records
	int n = cliTbl->getMaxClx();
	clientFile.seekp(0,ios_base::end);
	long len = clientFile.tellp();
	if (len != (n+1)*RECORD_SIZE) {
		for (int clx = 0; clx <= n; clx++) writeRecord(clx);
	}
	if (pthread_mutex_init(&clientFileLock,NULL) != 0) {
		logger->log("ClientMgr::init: could not initialize lock "
			    "on client data file",2);
		return false;
	}

	acctFile.open("acctRecords",fstream::app);
	if (!acctFile.good()) {
		logger->log("ClientMgr::init: could not open acctRecords "
			   "file",2);
		return false;
	}
	return true;
}

/** Send a boot request to NetMgr and process configuraton packets.
 *  @param nmIp is the ip address of the network manager
 *  @param myIp is the ip address to bind to the local socket
 *  @param nmAdr is a reference to a forest address in which the network
 *  manager's address is returned
 *  @param myAdr is a reference to a forest address in which this client
 *  manager's address is returned
 *  @param rtrAdr is a reference to a forest address in which this client
 *  manager's access router's address is returned
 *  @param rtrIp is a reference to an ip address in which this client
 *  manager's access router's ip address is returned
 *  @param rtrPort is a reference to an ip port number in which this client
 *  manager's access router's port number is returned
 *  @param nonce is a reference to a 64 bit nonce in which the nonce used
 *  by this client manager to connect to the forest network is returned
 */
bool bootMe(ipa_t nmIp, ipa_t myIp, fAdr_t& nmAdr, fAdr_t& myAdr,
	    fAdr_t& rtrAdr, ipa_t& rtrIp, ipp_t& rtrPort, uint64_t& nonce) {

	// open boot socket 
	int bootSock = Np4d::datagramSocket();
	if (bootSock < 0) return false;
	if (!Np4d::bind4d(bootSock,myIp,0) || !Np4d::nonblock(bootSock)) {
		close(bootSock);
		return false;
	}

	// setup boot leaf packet to net manager
	Packet p; buffer_t buf1; p.buffer = &buf1;
	CtlPkt cp(CtlPkt::BOOT_LEAF,CtlPkt::REQUEST,1,p.payload());
	int plen = cp.pack();
	if (plen == 0) { close(bootSock); return false; }
	p.length = Forest::OVERHEAD + plen; p.type = Forest::NET_SIG;
	p.flags = 0; p.srcAdr = p.dstAdr = 0; p.comtree = Forest::NET_SIG_COMT;
	p.pack();

	// setup reply packet
	Packet reply; buffer_t buf2; reply.buffer = &buf2;
	CtlPkt repCp;

	int resendTime = Misc::getTime();
	ipa_t srcIp; ipp_t srcPort;
	while (true) {
		int now = Misc::getTime();
		if (now > resendTime) {
			if (Np4d::sendto4d(bootSock,(void *) p.buffer, p.length,
					   nmIp,Forest::NM_PORT) == -1) {
				close(bootSock); return false;
			}
			resendTime += 1000000; // retry after 1 second
		}
		int nbytes = Np4d::recvfrom4d(bootSock,reply.buffer,1500,
					      srcIp, srcPort);
		if (nbytes < 0) { usleep(100000); continue; }
		reply.unpack();

		// do some checking
		if (srcIp != nmIp || reply.type != Forest::NET_SIG) {
			logger->log("unexpected response to boot request",
				     2,reply);
			close(bootSock); return false;
		}
		repCp.reset(reply);
		if (repCp.type != CtlPkt::CONFIG_LEAF ||
		    repCp.mode != CtlPkt::REQUEST) {
			logger->log("unexpected response from NetMgr",2,reply);
			close(bootSock); return false;
		}

		// unpack data from packet
		myAdr = repCp.adr1; rtrAdr = repCp.adr2;
		rtrIp = repCp.ip1; rtrPort = repCp.port1;
		nonce = repCp.nonce;
		nmAdr = reply.srcAdr;

		// send positive reply
		repCp.reset(CtlPkt::CONFIG_LEAF,CtlPkt::POS_REPLY,repCp.seqNum,
			    repCp.payload);
		plen = repCp.pack();
		if (plen == 0) { close(bootSock); return false; }
		reply.length = Forest::OVERHEAD + plen; 
		reply.srcAdr = myAdr; reply.dstAdr = nmAdr;
		reply.pack();
		if (Np4d::sendto4d(bootSock,(void *) reply.buffer, reply.length,
				   nmIp,Forest::NM_PORT) == -1) {
			close(bootSock); return false;
		}
		break; // proceed to next step
	}
	// we have the configuration information, now just wait for
	// final ack
	while (true) {
		int now = Misc::getTime();
		if (now > resendTime) {
			if (Np4d::sendto4d(bootSock,(void *) p.buffer, p.length,
					   nmIp,Forest::NM_PORT) == -1) {
				close(bootSock); return false;
			}
			resendTime += 1000000; // retry after 1 second
		}
		int nbytes = Np4d::recvfrom4d(bootSock,reply.buffer,1500,
					      srcIp, srcPort);
		if (nbytes < 0) { usleep(100000); continue; }
		reply.unpack();
		if (srcIp != nmIp || reply.type != Forest::NET_SIG) {
			logger->log("unexpected response to boot request",
				     2,reply);
			close(bootSock); return false;
		}
		// do some checking
		repCp.reset(reply);
		if (repCp.type == CtlPkt::CONFIG_LEAF &&
		    repCp.mode == CtlPkt::REQUEST) {
			// our reply must have been lost, send it again
			repCp.reset(CtlPkt::CONFIG_LEAF,CtlPkt::POS_REPLY,
				    repCp.seqNum, repCp.payload);
			plen = repCp.pack();
			if (plen == 0) { close(bootSock); return false; }
			reply.length = Forest::OVERHEAD + plen; 
			reply.srcAdr = myAdr; reply.dstAdr = nmAdr;
			reply.pack();
			if (Np4d::sendto4d(bootSock,(void *) reply.buffer,
					   reply.length,nmIp,Forest::NM_PORT)
					   == -1) {
				close(bootSock); return false;
			}
		} else if (repCp.type != CtlPkt::BOOT_LEAF ||
			   repCp.mode != CtlPkt::POS_REPLY) {
			logger->log("unexpected response from NetMgr",
				     2,reply);
			close(bootSock); return false;
		}
		break; // success
	}
	close(bootSock); return true;
}

/** Control packet handler for client manager.
 *  On receiving a control packet in its input queue, it calls the
 *  appropriate individual handler.
 *  If a negative number is received through the queue,
 *  it is interpreted as a negated socket number, for an accepted
 *  stream socket.
 *  @param qp is a pair of queues; one is the input queue to the
 *  handler, the other is its output queue
 */
void* handler(void *qp) {
	Queue& inq  = ((Substrate::QueuePair *) qp)->in;
	Queue& outq = ((Substrate::QueuePair *) qp)->out;
	CpHandler cph(&inq, &outq, myAdr, logger, ps);

	while (true) {
		pktx px = inq.deq();
		bool success = false;
		if (px < 0) {
			// in this case, p is really a negated socket number
			// for a remote client
			int sock = -px;
			success = handleClient(sock, cph);
			close(sock);
		} else {
			Packet& p = ps->getPacket(px);
string s;
cerr << "handler, got packet " << p.toString(s) << endl;
			CtlPkt cp(p);
			switch (cp.type) {
			case CtlPkt::CLIENT_CONNECT: 
			case CtlPkt::CLIENT_DISCONNECT: 
				success = handleConnDisc(px, cp, cph);
				break;
			default:
				cph.errReply(px,cp,"invalid control packet "
						   "type for ClientMgr");
				break;
			}
			if (!success) {
				string s;
				cerr << "handler: operation failed\n"
				     << p.toString(s);
			}
		}
		ps->free(px); // release px now that we're done
		outq.enq(0); // signal completion to main thread
	}
}

/** Handle a connection from a client.
 *  Interprets variety of requests from a remote client,
 *  including requests to login, get profile information,
 *  change a password, and so forth.
 *  @param sock is a socket number for an open stream socket
 *  @return true if the interaction proceeds normally, followed
 *  by a normal close; return false if an error occurs
 */
bool handleClient(int sock,CpHandler& cph) {
	NetBuffer buf(sock,1024);
	string cmd, reply, clientName;
	bool loggedIn;

	// verify initial "greeting"
	if (!buf.readLine(cmd) || cmd != "Forest-login-v1") {
		Np4d::sendString(sock,"misformatted user dialog\n"
				      "overAndOut\n");
		return false;
	}
	// main processing loop for requests from client
	loggedIn = false;
	while (buf.readAlphas(cmd)) {
cerr << "cmd=" << cmd << endl;
		reply = "success";
		if (cmd == "over") {
			// shouldn't happen, but ignore it, if it does
			buf.nextLine(); continue;
		} else if (cmd == "overAndOut") {
			buf.nextLine(); return true;
		} else if (cmd == "login") {
			loggedIn = login(buf,clientName,reply);
		} else if (cmd == "newAccount") {
			loggedIn = newAccount(buf,clientName,reply);
		} else if (!loggedIn) {
			continue; // must login before anything else
		} else if (cmd == "newSession") {
			newSession(sock,cph,buf,clientName,reply);
		} else if (cmd == "getProfile") {
			getProfile(buf,clientName,reply);
		} else if (cmd == "updateProfile") {
			updateProfile(buf,clientName,reply);
		} else if (cmd == "changePassword") {
			changePassword(buf,clientName,reply);
		} else if (cmd == "uploadPhoto") {
			uploadPhoto(sock,buf,clientName,reply);
		} else if (cmd == "getSessions") {
			getSessions(buf,clientName,reply);
		} else if (cmd == "cancelSession") {
			cancelSession(buf,clientName,cph,reply);
		} else if (cmd == "cancelAllSessions") {
			cancelAllSessions(buf,clientName,cph,reply);
		} else if (cmd == "addComtree" && buf.nextLine()) {
			addComtree(buf,clientName,reply);
		} else {
			reply = "unrecognized input";
		}
cerr << "sending reply: " << reply << endl;
		reply += "\nover\n";
		Np4d::sendString(sock,reply);
	}
	return true;
cerr << "terminating" << endl;
}

/** Handle a login request.
 *  Reads from the socket to identify client and obtain password
 *  @param buf is a NetBuffer associated with the stream socket to a client
 *  @param clientName is a reference to a string in which the name of the
 *  client is returned
 *  @param reply is a reference to a string in which an error message may be
 *  returned if the operation does not succeed.
 *  @return true on success, else false
 */
bool login(NetBuffer& buf, string& clientName, string& reply) {
	string pwd, s1, s2;
	if (buf.verify(':') && buf.readName(clientName) && buf.nextLine() &&
	    buf.readAlphas(s1) && s1 == "password" &&
	      buf.verify(':') && buf.readWord(pwd) && buf.nextLine() &&
	    buf.readLine(s2) && s2 == "over") {
		int clx =cliTbl->getClient(clientName);
		// locks clx
		if (clx == 0) {
			reply = "login failed: try again";
			return false;
		} else if (cliTbl->checkPassword(clx,pwd)) {
cerr << "login succeeded " << clientName << endl;
			cliTbl->releaseClient(clx);
			return true;
		} else {
			cliTbl->releaseClient(clx);
			reply = "login failed: try again";
			return false;
		}
	} else {
		reply = "misformatted login request";
		return false;
	}
}

/** Handle a new account request.
 *  Reads from the socket to identify client and obtain password
 *  @param buf is a NetBuffer associated with the stream socket to a client
 *  @param clientName is a reference to a string in which the name of the
 *  client is returned
 *  @param reply is a reference to a string in which an error message may be
 *  returned if the operation does not succeed.
 *  @return true on success, else false
 */
bool newAccount(NetBuffer& buf, string& clientName, string& reply) {
	string pwd, s1, s2;
	if (buf.verify(':') && buf.readName(clientName) && buf.nextLine() &&
	    buf.readAlphas(s1) && s1 == "password" &&
	      buf.verify(':') && buf.readWord(pwd) && buf.nextLine() &&
	    buf.readLine(s2) && s2 == "over") {
		int clx =cliTbl->getClient(clientName);
		// locks clx
		if (clx != 0) {
			cliTbl->releaseClient(clx);
			reply = "name not available, select another";
			return false;
		}
		clx = cliTbl->addClient(clientName,pwd,ClientTable::STANDARD);
		if (clx != 0) {
			writeRecord(clx);
			cliTbl->releaseClient(clx);
			return true;
		} else {
			reply = "unable to add client";
			return false;
		}
	} else {
		reply = "misformatted new account request";
		return false;
	}
}

/** Handle a request for a new forest session.
 *  Reads from the socket to get an optional rate spec.
 *  @param sock is the socket number for the socket to the client
 *  @param cph is a reference to the control packet handler, used for
 *  sending control packets to the NetMgr
 *  @param buf is a NetBuffer associated with the stream socket to a client
 *  @param clientName is the name of the currently logged-in client
 *  @param reply is a reference to a string in which an error message may be
 *  returned if the operation does not succeed.
 *  @return true on success, else false
 */
void newSession(int sock, CpHandler& cph, NetBuffer& buf,
		string& clientName, string& reply) {
	string s1;
	RateSpec rs;
	if (buf.verify(':')) {
		if (!buf.readRspec(rs) || !buf.nextLine() ||
		    !buf.readLine(s1)  || s1 != "over") {
			reply = "misformatted new session request";
			return;
		}
	} else {
		if (!buf.nextLine() || !buf.readLine(s1)  || s1 != "over") {
			reply = "misformatted new session request";
			return;
		}
	}
	int clx = cliTbl->getClient(clientName);
	if (clx == 0) {
		reply = "cannot access client data(" + clientName + ")";
		return;
	}
	if (!rs.isSet()) rs = cliTbl->getDefRates(clx);
	// make sure we have sufficient capacity
	if (!rs.leq(cliTbl->getAvailRates(clx))) {
		cliTbl->releaseClient(clx);
		reply = "session rate exceeds available capacity";
		return;
	}
	cliTbl->getAvailRates(clx).subtract(rs);
	cliTbl->releaseClient(clx);
	// proceed with new session setup
	CtlPkt repCp;
	ipa_t clientIp = Np4d::getSockIp(sock);
	pktx rpx = cph.newSession(nmAdr, clientIp, rs, repCp);
	clx = cliTbl->getClient(clientName);
	if (clx == 0) {
		reply = "cannot update client data(" + clientName + ")";
		logger->log("ClientMgr::newSession: cannot update session data "
			    "session state may be inconsistent",2);
		return;
	}
	if (rpx == 0) {
		cliTbl->getAvailRates(clx).add(rs);
		cliTbl->releaseClient(clx);
		reply = "cannot setup session: NetMgr never responded";
		return;
	}
	if (repCp.mode != CtlPkt::POS_REPLY) {
		cliTbl->getAvailRates(clx).add(rs);
		cliTbl->releaseClient(clx);
		reply = "cannot setup new session: NetMgr failed (" +
			 repCp.errMsg + ")";
		ps->free(rpx); return;
	}
	int sess = cliTbl->addSession(repCp.adr1, repCp.adr2, clx);
	if (sess == 0) {
		cliTbl->getAvailRates(clx).add(rs);
		cliTbl->releaseClient(clx);
		reply = "cannot setup new session: could not create "
			"session record";
		return;
	}

	// set session information
	cliTbl->setClientIp(sess,clientIp);
	cliTbl->setRouterAdr(sess,repCp.adr2);
	cliTbl->setState(sess,ClientTable::PENDING);
	cliTbl->setStartTime(sess,Misc::currentTime());
	cliTbl->getSessRates(sess) = rs;
	cliTbl->releaseClient(clx);

	// output accounting record
	writeAcctRecord(clientName,repCp.adr1,clientIp,repCp.adr2,NEWSESSION);

	// send information back to client
	stringstream ss; string s;
	ss << "yourAddress: " << Forest::fAdr2string(repCp.adr1,s) << endl;
	ss << "yourRouter: (" << Np4d::ip2string(repCp.ip1,s)
	   << "," << repCp.port1 << ",";
	ss << Forest::fAdr2string(repCp.adr2,s) << ")\n";
	ss << "comtCtlAddress: " << Forest::fAdr2string(repCp.adr3,s) << endl;
	ss << "connectNonce: " << repCp.nonce << "\noverAndOut\n";
	reply = ss.str();

	ps->free(rpx); return;
}

/** Verify that a client may perform privileged operations on a target.
 *  @param clientName is the name of the client who is currently logged in
 *  @param targetName is the name of the client on which some privileged
 *  operation is to be performed
 *  @return 0 if targetName != clientName and clientName lacks the
 *  privileges needed to perform a privileged operation, or if either
 *  of the names does not match a client in the table; otherwise
 *  return the client table index (clx) for targetName and lock the
 *  entry (the calling program must release the entry when done)
 */
int checkPrivileges(string clientName, string targetName) {
	// if names match, get and return client index (or 0 if no match)
	if (targetName == clientName) return cliTbl->getClient(clientName);

	// get client's privileges and check if privileged
	int clx = cliTbl->getClient(clientName);
	if (clx == 0) return 0;
	ClientTable::privileges priv = cliTbl->getPrivileges(clx);
	cliTbl->releaseClient(clx);
	if (priv != ClientTable::ADMIN && priv != ClientTable::ROOT) return 0;

	// get target's privileges
	int tclx = cliTbl->getClient(targetName);
	if (tclx == 0) return 0;
	ClientTable::privileges tpriv = cliTbl->getPrivileges(tclx);
	if (priv == ClientTable::ADMIN && 
	    (tpriv == ClientTable::ADMIN || tpriv == ClientTable::ROOT)) {
		cliTbl->releaseClient(tclx); return 0;
	}
	return tclx;
}

/** Handle a get profile request.
 *  Reads from the socket to identify the target client. This operation
 *  requires either that the target client is the currently logged-in
 *  client, or that the logged-in client has administrative privileges.
 *  @param buf is a NetBuffer associated with the stream socket to a client
 *  @param clientName is the name of the currently logged-in client
 *  @param reply is a reference to a string in which an error message may be
 *  returned if the operation does not succeed.
 */
void getProfile(NetBuffer& buf, string& clientName, string& reply) {
	string s, targetName;
	if (!buf.verify(':') || !buf.readName(targetName) ||
	    !buf.nextLine() || !buf.readLine(s) || s != "over") {
		reply = "misformatted get profile request"; return;
	}
	int tclx = checkPrivileges(clientName, targetName);
	if (tclx == 0) {
		reply = "insufficient privileges for requested operation";
		return;
	}
	reply  = "realName: \"" + cliTbl->getRealName(tclx) + "\"\n";
	reply += "email: " + cliTbl->getEmail(tclx) + "\n";
	reply += "defRates: " + cliTbl->getDefRates(tclx).toString(s) + "\n";
	reply += "totalRates: " + cliTbl->getTotalRates(tclx).toString(s) +"\n";
	cliTbl->releaseClient(tclx);
}

/** Handle an update profile request.
 *  Reads from the socket to identify the target client. This operation
 *  requires either that the target client is the currently logged-in
 *  client, or that the logged-in client has administrative privileges.
 *  @param buf is a NetBuffer associated with the stream socket to a client
 *  @param clientName is the name of the currently logged-in client
 *  @param reply is a reference to a string in which an error message may be
 *  returned if the operation does not succeed.
 */
void updateProfile(NetBuffer& buf, string& clientName, string& reply) {
	string s, targetName;
	if (!buf.verify(':') || !buf.readName(targetName) ||
	    !buf.nextLine()) {
		reply = "misformatted updateProfile request"; return;
	}
	// read profile information from client
	string item; RateSpec rates;
	string realName, email; RateSpec defRates, totRates;
	while (buf.readAlphas(item)) {
		if (item == "realName" && buf.verify(':') &&
		    buf.readString(realName) && buf.nextLine()) {
			// that's all for now
		} else if (item == "email" && buf.verify(':') &&
		    buf.readWord(email) && buf.nextLine()) {
			// that's all for now
		} else if (item == "defRates" && buf.verify(':') &&
			   buf.readRspec(defRates) && buf.nextLine()) {
			// that's all for now
		} else if (item == "totalRates" && buf.verify(':') &&
			   buf.readRspec(totRates) && buf.nextLine()) {
			// that's all for now
		} else if (item == "over" && buf.nextLine()) {
			break;
		} else {
			reply = "misformatted update profile request (" +
				item + ")";
			return;
		}
	}
	int tclx = checkPrivileges(clientName, targetName);
	if (tclx == 0) {
		reply = "insufficient privileges for requested operation";
		return;
	}
	if (realName.length() > 0) cliTbl->setRealName(tclx,realName);
	if (email.length() > 0) cliTbl->setEmail(tclx,email);
	ClientTable::privileges tpriv = cliTbl->getPrivileges(tclx);
	if (tpriv != ClientTable::LIMITED) {
		if (defRates.isSet() && 
		    defRates.leq(cliTbl->getTotalRates(tclx)))
			cliTbl->getDefRates(tclx) = defRates;
		if (totRates.isSet() && 
	    	    defRates.leq(cliTbl->getTotalRates(tclx)))
			cliTbl->getTotalRates(tclx) = totRates;
	}
	writeRecord(tclx);
	cliTbl->releaseClient(tclx);
	return;
}

/** Handle a change password request.
 *  Reads from the socket to identify the target client. This operation
 *  requires either that the target client is the currently logged-in
 *  client, or that the logged-in client has administrative privileges.
 *  @param buf is a NetBuffer associated with the stream socket to a client
 *  @param clientName is the name of the currently logged-in client
 *  @param reply is a reference to a string in which an error message may be
 *  returned if the operation does not succeed.
 */
void changePassword(NetBuffer& buf, string& clientName, string& reply) {
	string targetName, pwd;
	if (!buf.verify(':') || !buf.readName(targetName) || !buf.nextLine() ||
	    !buf.readWord(pwd) && buf.nextLine()) {
		reply = "misformatted change password request"; return;
	}
	int tclx = checkPrivileges(clientName, targetName);
	if (tclx == 0) {
		reply = "insufficient privileges for requested operation";
		return;
	}
	cliTbl->setPassword(tclx,pwd);
	writeRecord(tclx);
	cliTbl->releaseClient(tclx);
	return;
}

/** Handle an upload photo request.
 *  Reads from the socket to identify the target client. This operation
 *  requires either that the target client is the currently logged-in
 *  client, or that the logged-in client has administrative privileges.
 *  @param sock is the socket number for the stream socket to the client
 *  @param buf is a NetBuffer associated with the stream socket to a client
 *  @param clientName is the name of the currently logged-in client
 *  @param reply is a reference to a string in which an error message may be
 *  returned if the operation does not succeed.
 */
void uploadPhoto(int sock, NetBuffer& buf, string& clientName, string& reply) {
	int length;
	if (!buf.verify(':') || !buf.readInt(length) || !buf.nextLine()) {
		reply = "misformatted upload photo request"; return;
	}
	if (length > 50000) {
		reply = "photo file exceeds 50000 byte limit"; return;
	}
	int clx = cliTbl->getClient(clientName);
	ofstream photoFile;
	string photoName = "clientPhotos/"; photoName += clientName + ".jpg";
	photoFile.open(photoName.c_str(), ofstream::binary);
	if (!photoFile.good()) {
		reply = "cannot open photo file"; return;
	}
	cliTbl->releaseClient(clx);
	Np4d::sendString(sock,"proceed\n");
	char xbuf[1001];
	int numRcvd = 0;
	while (true) {
		int n = buf.readBlock(xbuf,min(1000,length-numRcvd));
		if (n <= 0) break;
		photoFile.write(xbuf,n);
		numRcvd += n;
		if (numRcvd == length) break;
	}
	if (numRcvd != length) {
		reply = "could not transfer complete file"; return;
	}
	string s1, s2;
	if (!buf.readLine(s1) || s1 != "photo finished" ||
	    !buf.readLine(s2) || s2 != "over") {
		reply = "misformatted photo request"; return;
	}
	photoFile.close();
	return;
}

/** Respond to getSessions request from a client.
 *  @param buf is a NetBuffer for the socket to the client.
 *  @param clientName is the client's name
 *  @param reply is a string in which a reply to the client can be
 *  returned
 */
void getSessions(NetBuffer& buf, string& clientName, string& reply) {
	string s, targetName;
 	if (!buf.verify(':') || !buf.readName(targetName) ||
	    !buf.nextLine() || !buf.readLine(s) || s != "over") {
		reply = "misformatted get sessions request"; return;
	}
	int tclx = checkPrivileges(clientName, targetName);
	if (tclx == 0) {
		reply = "insufficient privileges for requested operation";
		return;
	}
	int sess = cliTbl->firstSession(tclx);
	reply += "\n";
	while (sess != 0) {
		string sessString;
		cliTbl->session2string(sess, sessString);
		reply += sessString;
		sess = cliTbl->nextSession(sess,tclx);
	}
	reply.erase(reply.end()-1);
	cliTbl->releaseClient(tclx);
	return;
}

/** Respond to cancelSessions request from a client.
 *  @param buf is a NetBuffer for the socket to the client.
 *  @param clientName is the client's name
 *  @param cph is the control packet handler for this thread
 *  @param reply is a string in which a reply to the client can be
 *  returned
 */
void cancelSession(NetBuffer& buf, string& clientName, CpHandler& cph,
		   string& reply) {
	string s, targetName, cancelAdrStr;
 	if (!buf.verify(':') || !buf.readName(targetName) ||
	    !buf.readForestAddress(cancelAdrStr) || !buf.nextLine() ||
	    !buf.readLine(s) || s != "over") {
		reply = "misformatted cancel session request"; return;
	}
	fAdr_t cancelAdr = Forest::forestAdr(cancelAdrStr.c_str());
	if (cancelAdr == 0) {
		reply = "misformatted address"; return;
	}
	int tclx = checkPrivileges(clientName, targetName);
	if (tclx == 0) {
		reply = "insufficient privileges for requested operation";
		return;
	}
	cliTbl->releaseClient(tclx); // release before calling getSession()
	int sess = cliTbl->getSession(cancelAdr); // locked again
	if (sess == 0) {
		reply = "invalid session address"; return;
	}
	if (cliTbl->getClientIndex(sess) != tclx) {
		reply = "session address belongs to another client"; return;
	}
	ipa_t  cliIp  = cliTbl->getClientIp(sess);
	fAdr_t rtrAdr = cliTbl->getRouterAdr(sess);

	// release lock on entry while waiting for reply from NetMgr
	cliTbl->releaseClient(tclx);

	CtlPkt repCp;
	pktx rpx = cph.cancelSession(nmAdr, cancelAdr, rtrAdr, repCp);
	if (rpx == 0) {
		reply = "cannot cancel session: NetMgr never responded";
		return;
	}
	if (repCp.mode != CtlPkt::POS_REPLY) {
		reply = "cannot complete cancelSession: NetMgr failed (" +
			 repCp.errMsg + ")";
		ps->free(rpx); return;
	}
	tclx = cliTbl->getClient(targetName); // re-acquire lock on entry
	if (tclx == 0) {
		reply = "cannot update session data for " + targetName;
		logger->log("ClientMgr::cancelSession: cannot update session "
			    "data session state may be inconsistent",2);
		return;
	}
	cliTbl->getAvailRates(tclx).add(cliTbl->getSessRates(sess));
	writeAcctRecord(clientName,cancelAdr,cliIp,rtrAdr,CANCELSESSION);
	cliTbl->removeSession(sess);
	cliTbl->releaseClient(tclx); 
	return;
}

/** Respond to cancelAllSessions request from a client.
 *  @param buf is a NetBuffer for the socket to the client.
 *  @param clientName is the client's name
 *  @param reply is a string in which a reply to the client can be
 *  returned
 */
void cancelAllSessions(NetBuffer& buf, string& clientName, CpHandler& cph,
			string& reply) {
	string s, targetName;
	if (!buf.verify(':') || !buf.readName(targetName) || !buf.nextLine() ||
	    !buf.readLine(s) || s != "over") {
                reply = "misformatted cancel all sessions request"; return;
        }
	int tclx = checkPrivileges(clientName, targetName);
	if (tclx == 0) {
		reply = "insufficient privileges for requested operation";
		return;
	}
        int sess = cliTbl->firstSession(tclx);
        while (sess != 0) {
		fAdr_t cliAdr = cliTbl->getClientAdr(sess);
		ipa_t  cliIp  = cliTbl->getClientIp(sess);
		fAdr_t rtrAdr = cliTbl->getRouterAdr(sess);

		cliTbl->releaseClient(tclx); // release while waiting for reply
		CtlPkt repCp;
		pktx rpx = cph.cancelSession(nmAdr, cliAdr, rtrAdr, repCp);
		if (rpx == 0) {
			reply = "cannot cancel session: NetMgr never responded";
			return;
		}
		if (repCp.mode != CtlPkt::POS_REPLY) {
			reply = "cannot complete cancelSession: NetMgr failed "
				"(" + repCp.errMsg + ")";
			ps->free(rpx); return;
		}
		tclx = cliTbl->getClient(targetName); // re-acquire lock 

		cliTbl->getAvailRates(tclx).add(cliTbl->getSessRates(sess));
		writeAcctRecord(clientName,cliAdr,cliIp,rtrAdr,CANCELSESSION);
		cliTbl->removeSession(sess);
        	sess = cliTbl->firstSession(tclx);
        }
        cliTbl->releaseClient(tclx);
        return;
}

void addComtree(NetBuffer& buf, string& clientName, string& reply) {
}
	
/** Handle a connect or disconnect message from the NetBgr.
 *  @param px is the index of the packet
 *  @param cp is the control packet contained in px
 *  @param cph is a reference to the control packet handler for this
 *  thread
 */
bool handleConnDisc(pktx px, CtlPkt& cp, CpHandler& cph) {
	Packet& p = ps->getPacket(px);
	if (p.srcAdr != nmAdr || cp.mode != CtlPkt::REQUEST) {
		return false;
	}
	fAdr_t cliAdr = cp.adr1;
	int sess = cliTbl->getSession(cliAdr); // locks client
	if (sess == 0) {
		cph.errReply(px,cp,"no record of session for "
			           "specified client address");
		return false;
	}
	int clx = cliTbl->getClientIndex(sess);
	const string& cliName = cliTbl->getClientName(clx);
	ipa_t cliIp = cliTbl->getClientIp(sess);
	fAdr_t rtrAdr = cliTbl->getRouterAdr(sess);

	if (cp.type == CtlPkt::CLIENT_CONNECT) {
		cliTbl->setState(sess,ClientTable::CONNECTED);
	} else {
		cliTbl->removeSession(sess);
	}

	cliTbl->releaseClient(clx);
	acctRecType typ = (cp.type == CtlPkt::CLIENT_CONNECT ?
			   CONNECT_REC : DISCONNECT_REC);	
	writeAcctRecord(cliName, cliAdr, cliIp, rtrAdr, typ);
	
	// send reply to original request
	CtlPkt repCp(cp.type,CtlPkt::POS_REPLY,cp.seqNum);
	cph.sendReply(repCp, p.srcAdr);
	return true;
}

/** Writes a record to the accounting file.
 *  @param cname is the name of the client
 *  @param cliAdr is the Forest address assigned to the client
 *  @param cliIp is the IP address used by client to login
 *  @param rtrAdr is the Forest address for the router assigned to the client
 *  @param recType is the type of accounting record
 */
void writeAcctRecord(const string& cname, fAdr_t cliAdr, ipa_t cliIp,
		     fAdr_t rtrAdr, acctRecType recType) {
	// should really lock while writing
	if (!acctFile.good()) {
		logger->log("ClientMgr::writeAcctRecord: cannot write "
		   	   "to accouting file",2);
			return;
	}
	string typeStr = (recType == NEWSESSION ? "new session" :
			  (recType == CANCELSESSION ? "cancel session" :
			   (recType == CONNECT_REC ? "connect" :
			    (recType == DISCONNECT_REC ?
			     "disconnect" : "undefined record"))));
	time_t t = Misc::currentTime();
	string now = string(ctime(&t)); now.erase(now.end()-1);
	string s;
	acctFile << typeStr << ", " << now << ", " << cname << ", "
		 << Np4d::ip2string(cliIp,s) << ", ";
	acctFile << Forest::fAdr2string(cliAdr,s) << ", ";
	acctFile << Forest::fAdr2string(rtrAdr,s) << "\n";
	acctFile.flush();
}

/** Write a client record to clientFile.
 *  The calling thread is assumed to hold a lock on the client.
 *  @param clx is a non-negative integer
 */
void writeRecord(int clx) {
	if (clx < 0 || clx >= cliTbl->getMaxClients()) return;

	pthread_mutex_lock(&clientFileLock);
	if (dummyRecord == 0) {
		// create dummy record, for padding clientFile
		dummyRecord = new char[RECORD_SIZE];
		for (char *p = dummyRecord; p < dummyRecord+RECORD_SIZE; p++)
			*p = ' ';
		dummyRecord[0] = '-'; dummyRecord[RECORD_SIZE-1] = '\n';
	}
	if (maxRecord == 0) {
		clientFile.seekp(0,ios_base::end);
		maxRecord = clientFile.tellp()/RECORD_SIZE;
	}


	// position file pointer, adding dummy records if needed
	if (clx > maxRecord) {
		clientFile.seekp((maxRecord+1)*RECORD_SIZE);
		while (clx > maxRecord) {
			clientFile.write(dummyRecord,RECORD_SIZE);
			maxRecord++;
		}
	}
	clientFile.seekp(clx*RECORD_SIZE);

	if (cliTbl->validClient(clx)) {
		string s;
		cliTbl->client2string(clx,s);
		s = "+ " + s;
		if (s.length() > RECORD_SIZE) {
			s.erase(RECORD_SIZE-1); s += "\n";
		} else {
			s.erase(s.length()-1);
			int len = RECORD_SIZE - s.length();
			char *p = dummyRecord + s.length();
			s.append(p,len);
		}
		clientFile.write(s.c_str(),RECORD_SIZE);
	} else {
		//s.assign(dummyRecord,RECORD_SIZE);
		clientFile.write(dummyRecord,RECORD_SIZE);
	}
	clientFile.flush();
	maxRecord = max(clx,maxRecord);
	pthread_mutex_unlock(&clientFileLock);
	return;
}

} // ends namespace
