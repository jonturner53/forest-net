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
	clientLog.open("clientLog",ofstream::app);
	if (!clientLog.good()) {
		logger->log("ClientMgr::init: could not open clientLog "
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
			close(sock);
		} else {
			Packet& p = ps->getPacket(px);
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
 *  @param sock is a socket number for an open stream socket
 *  @return true if the interaction proceeds normally, followed
 *  by a normal close; return false if an error occurs
 */
bool handleClient(int sock,CpHandler& cph) {
	int clx; string clientName;
	NetBuffer buf(sock,1024);

	clx = loginDialog(sock,buf);
	if (clx == 0) return true;
	// clx is locked now

	ClientTable::privileges priv = cliTbl->getPrivileges(clx);
	//if (priv == ClientTable::ADMIN || priv == ClientTable::ROOT) {
	//	adminDialog(sock,cph,clx,buf);
	//} else {
		userDialog(sock,cph,clx,buf);
//	}
	cliTbl->releaseClient(clx);
	return true;
}

/** Carry out login dialog and verify that client/password is correct.
 *  @param sock is an open socket to the client
 *  @param buf is a reference to a NetBuffer bound to the socket
 *  @param return the client's index in the client table, if the operation
 *  succeeds (including password verification), else 0; on a successful
 *  return the client's entry in the table is locked; the caller must
 *  release it when done
 */
int loginDialog(int sock, NetBuffer& buf) {
	string client, pwd, s0, s1, s2, s3;
	int numFailures = 0;
	if (!buf.readWord(s0) || s0 != "Forest-login-v1" ||
	    !buf.nextLine() || !buf.readAlphas(s1)) {
		Np4d::sendString(sock,"misformatted login dialog\n"
				      "overAndOut\n");
		return 0;
	}
	while (true) {
		// process up to 3 login attempts
		// expected form for login
		// Forest-login-v1
		// login: clientName
		// password: clientPassword
		// over
		if (s1 == "login") {
cerr << "login" << endl;
			// process remainder of login dialog
			if (buf.verify(':') && buf.readAlphas(client) &&
			      buf.nextLine() &&
			    buf.readAlphas(s2) && s2 == "password" &&
			      buf.verify(':') && buf.readWord(pwd) &&
			      buf.nextLine() &&
			    buf.readLine(s3) && s3 == "over") {
cerr << "client=" << client << " pwd=" << pwd << endl;
				int clx =cliTbl->getClient(client); // locks clx
cerr << "proceeding clx=" << clx << "\n";
				if (clx == 0) {
					Np4d::sendString(sock,"login failed: "
							 "try again\nover\n");
				} else if (cliTbl->checkPassword(clx,pwd)) {
					Np4d::sendString(sock,"login successful"
							      "\nover\n");
					return clx;
				} else {
					cliTbl->releaseClient(clx);
					Np4d::sendString(sock,"login failed, "
							 "try again\nover\n");
					if (!buf.readAlphas(s1)) {
						Np4d::sendString(sock,
							"misformatted login "
							"dialog\n"
					      		"overAndOut\n");
						return 0;
					}
				}
			}
		} else if (s1 == "newAccount") {
			// attempt to add account
			if (buf.verify(':') && buf.readAlphas(client) &&
			      buf.nextLine() &&
			    buf.readAlphas(s2) && s2 == "password" &&
			      buf.verify(':') && buf.readWord(pwd) &&
			      buf.nextLine() &&
			    buf.readLine(s3) && s3 == "over") {
				int clx = cliTbl->getClient(client); //locks clx
				if (clx != 0) {
					Np4d::sendString(sock,"client name not "
							"available: try again\n"
							"over\n");
					cliTbl->releaseClient(clx);
				}
				clx = cliTbl->addClient(client,pwd,
							ClientTable::STANDARD);
				if (clx != 0) {
					writeClientLog(clx);
					Np4d::sendString(sock,"success\n"
							      "over\n");
					return clx;
				} else {
					Np4d::sendString(sock,"unable to add "
							 "client\nover\n");
					if (!buf.readAlphas(s1)) {
						Np4d::sendString(sock,
							"misformatted login "
							"dialog\n"
					      		"overAndOut\n");
						return 0;
					}
				}
			}
		} else {
			Np4d::sendString(sock,"misformatted login dialog\n"
					      "overAndOut\n");
			return 0;
		}
		numFailures++;
		if (numFailures >= 3) {
			Np4d::sendString(sock,"login failed: you're done\n"
				 	 "overAndOut\n");
			return 0;
		}
	}
}

void userDialog(int sock, CpHandler& cph, int clx, NetBuffer& buf) {
	string cmd, reply;
cerr << "entering userDialog\n";
	while (buf.readAlphas(cmd)) {
cerr << "cmd=" << cmd << endl;
		reply = "success";
		if (cmd == "newSession") {
			newSession(sock,cph,clx,buf,reply);
		} else if (cmd == "getProfile") {
			getProfile(clx,buf,reply);
		} else if (cmd == "updateProfile") {
			updateProfile(clx,buf,reply);
		} else if (cmd == "changePassword") {
			changePassword(clx,buf,reply);
		} else if (cmd == "uploadPhoto") {
			uploadPhoto(sock,clx,buf,reply);
		} else if (cmd == "addComtree" && buf.nextLine()) {
			addComtree(clx,buf,reply);
		} else if (cmd == "over" && buf.nextLine()) {
			// ignore
		} else if (cmd == "overAndOut" && buf.nextLine()) {
			break;
		} else {
			reply = "unrecognized input";
		}
		if (sock == -1) break;
cerr << "sending reply: " << reply << endl;
		reply += "\nover\n";
		Np4d::sendString(sock,reply);
	}
cerr << "terminating" << endl;
}

bool newSession(int sock, CpHandler& cph, int clx,
		NetBuffer& buf, string& reply) {
	// look for new session request - note: client is now locked
	string s1;
	RateSpec rs;
	if (buf.verify(':')) {
		if (buf.readRspec(rs) || !buf.nextLine()) {
			reply = "unrecognized input";
			return false;
		}
	} else {
		if (buf.nextLine()) {
			rs = cliTbl->getDefRates(clx);
		} else {
			reply = "unrecognized input";
			return false;
		}
	}
	if (!buf.readLine(s1) || s1 != "over") {
		reply = "unrecognized input";
		return false;
	}
	// make sure we have sufficient capacity
	bool ok = rs.leq(cliTbl->getAvailRates(clx));
	if (!ok) {
		reply = "session rate exceeds available capacity";
		return true;
	}

	// proceed with new session setup
	CtlPkt repCp;
	ipa_t clientIp = Np4d::getSockIp(sock);
	pktx rpx = cph.newSession(nmAdr, clientIp, rs, repCp);
	if (rpx == 0) {
		reply = "cannot complete login: NetMgr never responded";
		return false;
	}
	if (repCp.mode != CtlPkt::POS_REPLY) {
		reply = "cannot complete login: NetMgr failed (" +
			 repCp.errMsg + ")";
		ps->free(rpx); 
		return false;
	}

	int sess = cliTbl->addSession(repCp.adr1, repCp.adr2, clx);
	if (sess == 0) {
		reply = "cannot complete login: could not create "
			"session record";
		return false;
	}
	cliTbl->setState(sess, ClientTable::PENDING);

	// set session information
	const string& cliName = cliTbl->getClientName(clx);
	cliTbl->setClientIp(sess,clientIp);
	cliTbl->setRouterAdr(sess,repCp.adr2);
	cliTbl->setState(sess,ClientTable::PENDING);
	cliTbl->setStartTime(sess,Misc::currentTime());
	cliTbl->getAvailRates(sess).subtract(rs);

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
	reply = ss.str();

	ps->free(rpx);
	return true;
}

void getProfile(int clx, NetBuffer& buf, string& reply) {
	string s, userName;
cerr << "reading user name\n";
	if (!buf.verify(':') || !buf.readAlphas(userName) || !buf.nextLine()) {
		reply = "could not read user name"; return;
	}
	string myName = cliTbl->getClientName(clx);
cerr << "myName=" << myName << " userName=" << userName << endl;
	int tclx;
	if (userName == myName) {
		tclx = clx;
	} else {
		tclx = cliTbl->getClient(userName);
		if (tclx == 0) {
			reply = "no such user"; return;
		}
		ClientTable::privileges priv = cliTbl->getPrivileges(clx);
		if (priv != ClientTable::ADMIN && priv != ClientTable::ROOT) {
			reply = "this operation requires administrative "
				"privileges";
			cliTbl->releaseClient(tclx);
			return;
		}
		ClientTable::privileges tpriv = cliTbl->getPrivileges(tclx);
		if (priv != ClientTable::ROOT && tpriv == ClientTable::ROOT) {
			reply = "this operation requires root privileges";
			cliTbl->releaseClient(tclx);
			return;
		}
	}
	reply  = "realName: \"" + cliTbl->getRealName(tclx) + "\"\n";
	reply += "email: " + cliTbl->getEmail(tclx) + "\n";
	reply += "defRates: " + cliTbl->getDefRates(tclx).toString(s) + "\n";
	reply += "totalRates: " + cliTbl->getTotalRates(tclx).toString(s) +"\n";
	if (tclx != clx) cliTbl->releaseClient(tclx);
}

void updateProfile(int clx, NetBuffer& buf, string& reply) {
	string s, userName;
cerr << "reading user name\n";
	if (!buf.verify(':') || !buf.readAlphas(userName) || !buf.nextLine()) {
		reply = "could not read user name"; return;
	}

	string myName = cliTbl->getClientName(clx);
cerr << "got (" << userName << ") and I am (" << myName << ")" << endl;
// null value for userName??? even though readAlphas passed??
	int tclx;
	if (userName == myName) {
		tclx = clx;
	} else {
		tclx = cliTbl->getClient(userName);
		if (tclx == 0) {
			reply = "no such user"; return;
		}
		ClientTable::privileges priv = cliTbl->getPrivileges(clx);
		if (priv != ClientTable::ADMIN && priv != ClientTable::ROOT) {
			reply = "this operation requires administrative "
				"privileges";
			cliTbl->releaseClient(tclx);
			return;
		}
		ClientTable::privileges tpriv = cliTbl->getPrivileges(tclx);
		if (priv != ClientTable::ROOT &&
		    (tpriv == ClientTable::ROOT ||
		     tpriv == ClientTable::ADMIN)) {
			reply = "this operation requires root privileges";
			cliTbl->releaseClient(tclx);
			return;
		}
	}
	string item; RateSpec rates;
	ClientTable::privileges priv = cliTbl->getPrivileges(clx);
	while (buf.readAlphas(item)) {
		if (item == "realName" && buf.verify(':') &&
		    buf.readString(s) && buf.nextLine()) {
			cliTbl->setRealName(tclx,s);
		} else if (item == "email" && buf.verify(':') &&
		    buf.readLine(s)) {
			cliTbl->setEmail(tclx,s);
		} else if (item == "defRates" && buf.verify(':') &&
			   buf.readRspec(rates) && buf.nextLine()) {
			if (priv == ClientTable::LIMITED) continue;
			if (rates.leq(cliTbl->getTotalRates(tclx)))
				cliTbl->getDefRates(tclx) = rates;
		} else if (item == "totalRates" && buf.verify(':') &&
			   buf.readRspec(rates) && buf.nextLine()) {
			if (priv == ClientTable::LIMITED) continue;
			if (rates.leq(cliTbl->getTotalRates(tclx)) ||
			    rates.leq(cliTbl->getTotalRates())) {
				cliTbl->getTotalRates(tclx) = rates;
			}
		} else if (item == "over" && buf.nextLine()) {
			reply = "profile updated";
			writeClientLog(tclx);
			break;
		} else if (item == "overAndOut" && buf.nextLine()) {
			reply = "profile updated";
			writeClientLog(tclx);
			break;
		} else {
			reply = "misformatted request (" + item + ")";
			break;
		}
	}
	if (tclx != clx) cliTbl->releaseClient(tclx);
	return;
}

void changePassword(int clx, NetBuffer& buf, string& reply) {
	string s, userName;
	if (!buf.verify(':') || !buf.readAlphas(userName) || !buf.nextLine()) {
		reply = "could not read user name"; return;
	}
	int tclx;
	if (userName == cliTbl->getClientName(clx)) {
		tclx = clx;
	} else {
		tclx = cliTbl->getClient(userName);
		if (tclx == 0) {
			reply = "no such user"; return;
		}
		ClientTable::privileges priv = cliTbl->getPrivileges(clx);
		if (priv != ClientTable::ADMIN && priv != ClientTable::ROOT) {
			reply = "this operation requires administrative "
				"privileges";
			cliTbl->releaseClient(tclx);
			return;
		}
		ClientTable::privileges tpriv = cliTbl->getPrivileges(tclx);
		if (priv != ClientTable::ROOT &&
		    (tpriv == ClientTable::ROOT ||
		     tpriv == ClientTable::ADMIN)) {
			reply = "this operation requires root privileges";
			cliTbl->releaseClient(tclx);
			return;
		}
	}
	string item, pwd;
	while (buf.readAlphas(item)) {
cerr << "item=" << item << "\nbuf=" << buf.toString(s);
		if (item == "password" && buf.verify(':') &&
		    buf.readWord(pwd) && buf.nextLine()) {
			cliTbl->setPassword(tclx,pwd);
		} else if (item == "over" && buf.nextLine()) {
			writeClientLog(tclx); break;
		} else if (item == "overAndOut" && buf.nextLine()) {
			writeClientLog(tclx); break;
		} else {
			reply = "misformatted request (" + item + ")";
			break;
		}
	}
	if (tclx != clx) cliTbl->releaseClient(tclx);
	return;
}

void uploadPhoto(int sock, int clx, NetBuffer& buf, string& reply) {
	int length;
	if (!buf.verify(':') || !buf.readInt(length) || !buf.nextLine()) {
		reply = "cannot read length"; return;
	}
	if (length > 50000) {
		reply = "photo file exceeds 50000 byte limit"; return;
	}
	const string& cname = cliTbl->getClientName(clx);
	ofstream photoFile;
	photoFile.open("clientPhotos/" + cname + ".jpg",ofstream::binary);
	if (!photoFile.good()) {
		reply = "cannot open photo file"; return;
	}
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
		reply = "file transfer incomplete"; return;
	}
	photoFile.close();
	reply = "photo received";
	return;
}

void addComtree(int clx, NetBuffer& buf, string& reply) {
}

/** Handle an administrative login session.
 *  Supports commands for adding clients and modifying parameters.
 */
void adminDialog(int sock, CpHandler& cph, int clx, NetBuffer& buf) {
	string cmd; string reply;

	// add code to reject if non-local connection
	// can still use ssh tunneling for secure remote access
	while (buf.readAlphas(cmd)) {
		reply = "success";
cerr << "cmd=" << cmd << endl;
		if (cmd == "getProfile" && buf.nextLine()) {
			getProfile(clx,buf,reply);
		} else if (cmd == "updateProfile" && buf.nextLine()) {
			updateProfile(clx,buf,reply);
		} else if (cmd == "changePassword" && buf.nextLine()) {
			changePassword(clx,buf,reply);
		} else if (cmd == "addClient" && buf.verify(':')) {
			addClient(buf,reply);
		} else if (cmd == "removeClient" && buf.verify(':')) {
			removeClient(buf,reply);
		} else if (cmd == "modPassword" && buf.verify(':')) {
			ClientTable::privileges priv =
				cliTbl->getPrivileges(clx);
			modPassword(buf,priv,reply);
		} else if (cmd == "modPrivileges" && buf.verify(':')) {
			ClientTable::privileges priv =
				cliTbl->getPrivileges(clx);
			modPrivileges(buf,priv,reply);
		} else if (cmd == "modRealName" && buf.verify(':')) {
			modRealName(buf,reply);
		} else if (cmd == "modEmail" && buf.verify(':')) {
			modEmail(buf,reply);
		} else if (cmd == "modDefRates" && buf.verify(':')) {
			modDefRates(buf,reply);
		} else if (cmd == "modTotalRates" && buf.verify(':')) {
			modTotalRates(buf,reply);
		} else if (cmd == "showClient" && buf.verify(':')) {
			showClient(buf,reply);
		} else if (cmd == "over" && buf.nextLine()) {
			// ignore
		} else if (cmd == "overAndOut") {
			buf.nextLine(); break;
		}
		reply += "\n"; Np4d::sendString(sock,reply);

		if (!buf.nextLine()) break;
	}
}

void addClient(NetBuffer& buf, string& reply) {
	string cname, pwd, realName, email;
	RateSpec defRates, totalRates;

	if (buf.readName(cname) && buf.readWord(pwd)) {
		int clx = cliTbl->addClient(cname,pwd,ClientTable::STANDARD);
		if (clx == 0) {
			reply = "unable to add client";
			return;
		}
		cliTbl->setRealName(clx,"none"); cliTbl->setEmail(clx,"none");
		cliTbl->getDefRates(clx) = cliTbl->getDefRates();
		cliTbl->getTotalRates(clx) = cliTbl->getTotalRates();
		writeClientLog(clx);
		cliTbl->releaseClient(clx);
	}
}

void removeClient(NetBuffer& buf, string& reply) {
	string cname;

	if (buf.readName(cname)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			reply = "no such client";
			return;
		}
		cliTbl->removeClient(clx);
	}
}

void modPassword(NetBuffer& buf, ClientTable::privileges priv,
		 string& reply) {
	string cname, pwd;

	if (buf.readName(cname) && buf.readWord(pwd)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			reply = "no such client";
			return;
		}
		ClientTable::privileges cpriv = cliTbl->getPrivileges(clx);
		if ((priv == ClientTable::ROOT  && cpriv !=ClientTable::ROOT) ||
		    (priv == ClientTable::ADMIN && cpriv !=ClientTable::ROOT &&
		     cpriv != ClientTable::ADMIN)) {
			cliTbl->setPassword(clx,pwd);
			writeClientLog(clx);
		} else {
			reply = "rejected request";
		}
		cliTbl->releaseClient(clx);
	}
}

void modPrivileges(NetBuffer& buf, ClientTable::privileges priv, string& reply) {
	string cname, nuprivString;

	if (buf.readName(cname) && buf.readAlphas(nuprivString)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			reply = "no such client";
			return;
		}
		ClientTable::privileges nupriv;
		if (nuprivString == "limited")
			nupriv = ClientTable::LIMITED;
		else if (nuprivString == "standard")
			nupriv = ClientTable::STANDARD;
		else if (nuprivString == "admin")
			nupriv = ClientTable::ADMIN;
		else if (nuprivString == "root")
			nupriv = ClientTable::ROOT;
		else {
			reply = "invalid privilege (" + nuprivString + ")";
			cliTbl->releaseClient(clx);
			return;
		}
	
		ClientTable::privileges cpriv = cliTbl->getPrivileges(clx);
		if (priv == ClientTable::ROOT) {
			if (cpriv != ClientTable::ROOT) {
				cliTbl->setPrivileges(clx,nupriv);
				writeClientLog(clx);
			}
		} else if (priv == ClientTable::ADMIN) {
			if (cpriv !=ClientTable::ROOT &&
		     	    cpriv != ClientTable::ADMIN && 
		     	    nupriv != ClientTable::ADMIN && 
		     	    nupriv != ClientTable::ROOT) {
				cliTbl->setPrivileges(clx,nupriv);
				writeClientLog(clx);
			}
		} else {
			reply = "rejected request";
		}
		cliTbl->releaseClient(clx);
	}
}
	
void modRealName(NetBuffer& buf, string& reply) {
	string cname, realName;

	if (buf.readName(cname) && buf.readString(realName)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			reply = "no such client";
			return;
		}
		cliTbl->setRealName(clx,realName);
		writeClientLog(clx);
		cliTbl->releaseClient(clx);
	}
}

void modEmail(NetBuffer& buf, string& reply) {
	string cname, email;

	if (buf.readName(cname) && buf.readWord(email)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			reply = "no such client";
			return;
		}
		cliTbl->setEmail(clx,email);
		writeClientLog(clx);
		cliTbl->releaseClient(clx);
		return;
	}
}

void modDefRates(NetBuffer& buf, string& reply) {
	string cname; RateSpec rates;

	if (buf.readName(cname) && buf.readRspec(rates)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			reply = "no such client";
			return;
		}
		cliTbl->getDefRates(clx) = rates;
		writeClientLog(clx);
		cliTbl->releaseClient(clx);
	}
}

void modTotalRates(NetBuffer& buf, string& reply) {
	string cname; RateSpec rates;

	if (buf.readName(cname) && buf.readRspec(rates)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			reply = "no such client";
			return;
		}
		cliTbl->getTotalRates(clx) = rates;
		writeClientLog(clx);
		cliTbl->releaseClient(clx);
	}
}

void showClient(NetBuffer& buf, string& reply) {
	string cname; 

	if (buf.readName(cname)) {
		int clx = cliTbl->getClient(cname);
		if (clx == 0) {
			reply = "no such client";
			return;
		}
		cliTbl->client2string(clx,reply);
		writeClientLog(clx);
		cliTbl->releaseClient(clx);
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

	if (cp.type == CtlPkt::CLIENT_DISCONNECT) {
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
	acctFile.flush();
}

void writeClientLog(int clx) {
	// should really lock while writing
	if (!clientLog.good()) {
		logger->log("ClientMgr::writeClientLog: cannot write "
		   	   "to client log file",2);
			return;
	}
	string s;
	clientLog << cliTbl->client2string(clx,s);
	clientLog.flush();
}

} // ends namespace
