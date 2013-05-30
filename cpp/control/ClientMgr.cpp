/** @file ClientMgr.cpp 
 *
 *  @author Jonathan Turner (based on earlier version by Logan Stafman)
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

	uint64_t nonce;
	if (!bootMe(nmIp, myIp, nmAdr, myAdr, rtrAdr, rtrIp, rtrPort, nonce)) {
		return false;
	}

	sub = new Substrate(myAdr, myIp, rtrAdr, rtrIp, rtrPort, nonce,
			    500,handler,0,Forest::CM_PORT,ps,logger);
	if (!sub->init()) return false;
	sub->setRtrReady(true);

	// read client data and open accounting file
	ifstream ifs("clientData");
	if (!ifs.good() || !cliTbl->read(ifs)) {
		logger->log("ClientMgr::init: could not read clientData "
			    "file",2);
		return false;
	}
	acctFile.open("acctRecords");
	if (!acctFile.good()) {
		logger->log("ClientMgr::init: could not open acctRecords "
			   "file",2);
		return false;
	}
	return true;
}

bool bootMe(ipa_t nmIp, ipa_t myIp, fAdr_t& nmAdr, fAdr_t& myAdr,
	    fAdr_t& rtrAdr, ipa_t& rtrIp, ipp_t& rtrPort, uint64_t& nonce) {

cerr << "starting bootMe\n";
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
string s;
	while (true) {
		int now = Misc::getTime();
		if (now > resendTime) {
cerr << "sending boot request\n";
cerr << p.toString(s);
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
cerr << "got reply\n";
cerr << reply.toString(s);

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

/** Control packet handler.
 *  This method is run as a separate thread and does not terminate
 *  until the process does. It communicates with the main thread
 *  through a pair of queues, passing packet numbers back and forth
 *  across the thread boundary. When a packet number is passed to
 *  a handler, the handler "owns" the corresponding packet and can
 *  read/write it without locking. The handler is required to free any
 *  packets that it no longer needs to the packet store.
 *  When the handler has completed the requested operation, it sends
 *  a 0 value back to the main thread, signalling that it is available
 *  to handle another task.
 *  If the main thread passes a negative number through the queue,
 *  it is interpreted as a negated socket number, for an accepted
 *  stream socket to a remote display application.
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
		} else {
			Packet& p = ps->getPacket(px);
			CtlPkt cp(p.payload(),p.length - Forest::OVERHEAD);
			cp.unpack();
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
 *  @param sock is a socket number for an open stream socket
 *  @return true if the interaction proceeds normally, followed
 *  by a normal close; return false if an error occurs
 */
bool handleClient(int sock,CpHandler& cph) {
	int clx; string clientName;
	NetBuffer buf(sock,1024);
	clx = handleLogin(sock,buf,clientName);
	if (clx == 0) { close(sock); return true; }
	if (clientName == "admin") {
		handleAdmin(sock,buf);
		cliTbl->releaseClient(clx);
		close(sock); return true;
	}
	// process new session request - client is now locked
	string s1;
	if (!buf.readAlphas(s1) || s1 != "newSession") {
		Np4d::sendString(sock,"unrecognized input\noverAndOut\n");
		cliTbl->releaseClient(clx);
		close(sock); return true;
	}
	RateSpec rs;
	if (buf.verify(':')) {
		if (readRates(buf,rs) || !buf.nextLine()) {
			Np4d::sendString(sock,"unrecognized input\n"
					      "overAndOut\n");
			cliTbl->releaseClient(clx);
			close(sock); return true;
		}
	} else {
		if (buf.nextLine()) {
			rs = cliTbl->getDefRates(clx);
		} else {
			cliTbl->releaseClient(clx);
			Np4d::sendString(sock,"unrecognized input\n"
					      "overAndOut\n");
			close(sock); return true;
		}
	}
	if (!buf.readLine(s1) || s1 != "over") {
		cliTbl->releaseClient(clx);
		Np4d::sendString(sock,"unrecognized input\noverAndOut\n");
		close(sock); return true;
	}
	// make sure we have sufficient capacity
	bool ok = rs.leq(cliTbl->getAvailRates(clx));
	if (!ok) {
		Np4d::sendString(sock,"session rate exceeds available "
				      "capacity\noverAndOut\n");
		cliTbl->releaseClient(clx);
		close(sock); return true;
	}

	// proceed with new session setup
	ipa_t clientIp = Np4d::getSockIp(sock);
	CtlPkt repCp;
	pktx reply = cph.newSession(nmAdr, clientIp, rs, repCp);
	if (reply == 0) {
		Np4d::sendString(sock,"cannot complete login: "
				 "NetMgr never responded\noverAndOut\n");
		close(sock);
		cliTbl->releaseClient(clx);
		return false;
	}
	if (repCp.mode != CtlPkt::POS_REPLY) {
		Np4d::sendString(sock,"cannot complete login: "
				 "NetMgr failed (" + repCp.errMsg +
				 ")\noverAndOut\n");
		ps->free(reply); close(sock);
		cliTbl->releaseClient(clx);
		return false;
	}

	int sess = cliTbl->addSession(repCp.adr1, repCp.adr2, clx);
	if (sess == 0) {
		Np4d::sendString(sock,"cannot complete login: "
				 "could not create session record\n"
				 ")\noverAndOut\n");
		cliTbl->releaseClient(clx);
		close(sock); return false;
	}
	cliTbl->setState(sess, ClientTable::PENDING);
	// set session information
	const string& cliName = cliTbl->getClientName(clx);
	cliTbl->setClientIp(sess,clientIp);
	cliTbl->setRouterAdr(sess,repCp.adr2);
	cliTbl->setState(sess,ClientTable::PENDING);
	cliTbl->setStartTime(sess,Misc::currentTime());
	cliTbl->getAvailRates(sess).subtract(rs);
	cliTbl->releaseClient(clx);

	// output accounting record
	writeAcctRecord(cliName, repCp.adr1, clientIp, repCp.adr2, NEWSESSION);

	// send information back to client
	stringstream ss; string s;
	ss << "yourAddress: " << Forest::fAdr2string(repCp.adr1,s) << endl;
	ss << "yourRouter: (" << Np4d::ip2string(repCp.ip1,s)
	   << "," << repCp.port1 << ",";
	ss << Forest::fAdr2string(repCp.adr2,s) << ")\n";
	ss << "comtCtlAddress: " << Forest::fAdr2string(repCp.adr3,s) << endl;
	ss << "connectNonce: " << repCp.nonce << "\noverAndOut\n";
cerr << "client manager response\n" << ss.str();
	Np4d::sendString(sock,ss.str());

	ps->free(reply); close(sock); return true;
}

/** Carry out login dialog and verify that client/password is correct.
 *  @param sock is an open socket to the client
 *  @param buf is a reference to a NetBuffer bound to the socket
 *  @param clientName is a reference to a string in which the client name
 *  is returned
 *  @param return the client's index in the client table, if the operation
 *  succeeds (including password verification), else 0; on a successful
 *  return the client's entry in the table is locked; the caller must
 *  releae it when done
 */
int handleLogin(int sock, NetBuffer& buf, string& clientName) {
	// process up to 3 login attempts
int numFailures = 0;
	while (true) {
		string client, pwd, s0,s1, s2, s3;
		// expected form for login
		// Forest-login-v1
		// login: clientName
		// password: clientPassword
		// over
		// newSession: (50,1000,25,500)
		// overAndOut
		if (buf.readWord(s0) && s0 == "Forest-login-v1" &&
		    buf.nextLine() &&
		    buf.readAlphas(s1) && s1 == "login" && buf.verify(':') &&
		    buf.readAlphas(client) && buf.nextLine() &&
		    buf.readWord(s2) && s2 == "password" && buf.verify(':') &&
		    buf.readAlphas(pwd) && buf.nextLine() &&
		    buf.readAlphas(s3) && s3 == "over") {
			int clx = cliTbl->getClient(client); // locks entry
			if (clx != 0 && cliTbl->checkPassword(clx,pwd)) {
				Np4d::sendString(sock,"login successful\n"
						      "over\n");
				return clx;
			}
			if (clx != 0) cliTbl->releaseClient(clx);
		} else {
			Np4d::sendString(sock,"misformatted login dialog\n"
					      "overAndOut\n");
			return 0;
		}
		Np4d::sendString(sock,"login failed: try again\nover\n");
		numFailures++;
		if (numFailures >= 3) {
			Np4d::sendString(sock,"login failed: you're done\n"
				 	 "overAndOut\n");
			return 0;
		}
	}
}

bool readRates(NetBuffer& buf, RateSpec& rs) {
	int bru, brd, pru, prd;
	if (buf.verify('(')  &&
	    buf.readInt(bru) && buf.verify(',') &&
	    buf.readInt(brd) && buf.verify(',') &&
	    buf.readInt(pru) && buf.verify(',') &&
	    buf.readInt(prd) && buf.verify(')')) {
		rs.set(bru,brd,pru,prd); return true;
	}
	return false;
}

/** Handle an administrative login session.
 *  Supports commands for adding clients and modifying parameters.
 */
void handleAdmin(int sock, NetBuffer& buf) {
	string cmd;
	while (true) {
		Np4d::sendString(sock,"admin: ");
		buf.readAlphas(cmd);
		if (cmd == "addClient" && buf.verify(':')) {
			addClient(sock,buf);
		} else if (cmd == "removeClient" && buf.verify(':')) {
			removeClient(sock,buf);
		} else if (cmd == "modPassword" && buf.verify(':')) {
			modPassword(sock,buf);
		} else if (cmd == "modRealName" && buf.verify(':')) {
			modRealName(sock,buf);
		} else if (cmd == "modEmail" && buf.verify(':')) {
			modEmail(sock,buf);
		} else if (cmd == "modDefRates" && buf.verify(':')) {
			modDefRates(sock,buf);
		} else if (cmd == "modTotalRates" && buf.verify(':')) {
			modTotalRates(sock,buf);
		} else if (cmd == "showClient" && buf.verify(':')) {
			showClient(sock,buf);
		} else if (cmd == "quit") {
			buf.nextLine(); break;
		}
		if (!buf.nextLine()) break;
	}
}

void addClient(int sock, NetBuffer& buf) {
	string cname, pwd, realName, email;
	RateSpec defRates, totalRates;

	if (buf.readName(cname) && buf.readWord(pwd) && 
	    buf.readString(realName) && buf.readWord(email) &&
	    readRates(buf,defRates) && readRates(buf,totalRates)) {
		int clx = cliTbl->addClient(cname,pwd,realName,email,
					    defRates,totalRates);
		if (clx == 0) {
			Np4d::sendString(sock,"unable to add client\n");
			return;
		}
		cliTbl->getDefRates(clx) = defRates;
		cliTbl->getTotalRates(clx) = totalRates;
		cliTbl->releaseClient(clx);
		return;
	}
}

void removeClient(int sock, NetBuffer& buf) {
	string cname;

	if (buf.readName(cname)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			Np4d::sendString(sock,"no such client\n");
			return;
		}
		cliTbl->removeClient(clx);
		return;
	}
}

void modPassword(int sock, NetBuffer& buf) {
	string cname, pwd;

	if (buf.readName(cname) && buf.readWord(pwd)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			Np4d::sendString(sock,"no such client\n");
			return;
		}
		cliTbl->setPassword(clx,pwd);
		cliTbl->releaseClient(clx);
		return;
	}
}
	
void modRealName(int sock, NetBuffer& buf) {
	string cname, realName;

	if (buf.readName(cname) && buf.readString(realName)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			Np4d::sendString(sock,"no such client\n");
			return;
		}
		cliTbl->setRealName(clx,realName);
		cliTbl->releaseClient(clx);
		return;
	}
}

void modEmail(int sock, NetBuffer& buf) {
	string cname, email;

	if (buf.readName(cname) && buf.readWord(email)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			Np4d::sendString(sock,"no such client\n");
			return;
		}
		cliTbl->setEmail(clx,email);
		cliTbl->releaseClient(clx);
		return;
	}
}

void modDefRates(int sock, NetBuffer& buf) {
	string cname; RateSpec rates;

	if (buf.readName(cname) && readRates(buf,rates)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			Np4d::sendString(sock,"no such client\n");
			return;
		}
		cliTbl->getDefRates(clx) = rates;
		cliTbl->releaseClient(clx);
		return;
	}
}

void modTotalRates(int sock, NetBuffer& buf) {
	string cname; RateSpec rates;

	if (buf.readName(cname) && readRates(buf,rates)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			Np4d::sendString(sock,"no such client\n");
			return;
		}
		cliTbl->getTotalRates(clx) = rates;
		cliTbl->releaseClient(clx);
		return;
	}
}

void showClient(int sock, NetBuffer& buf) {
	string cname; 

	if (buf.readName(cname)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			Np4d::sendString(sock,"no such client\n");
			return;
		}
		string s;
		cliTbl->client2string(clx,s);
		cliTbl->releaseClient(clx);
		Np4d::sendString(sock,s);
		return;
	}
}
	
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

	if (cp.type == CtlPkt::CLIENT_DISCONNECT) 
		cliTbl->removeSession(sess);

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
	if (!acctFile.good()) {
		logger->log("ClientMgr::writeAcctRecord: cannot write "
		   	   "to accouting file",2);
			return;
	}
	string typeStr = (recType == NEWSESSION ? "new session" :
			  (recType == CONNECT_REC ? "connect" :
			   (recType == DISCONNECT_REC ?
			    "disconnect" : "undefined record")));
	time_t t = Misc::currentTime();
	string now = string(ctime(&t)); now.erase(now.end()-1);
	string s;
	acctFile << typeStr << ", " << now << ", " << cname << ", "
		 << Np4d::ip2string(cliIp,s) << ", ";
	acctFile << Forest::fAdr2string(cliAdr,s) << ", ";
	acctFile << Forest::fAdr2string(rtrAdr,s) << "\n";
}

} // ends namespace
