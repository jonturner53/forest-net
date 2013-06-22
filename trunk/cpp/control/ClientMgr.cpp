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
 *  @param sock is a socket number for an open stream socket
 *  @return true if the interaction proceeds normally, followed
 *  by a normal close; return false if an error occurs
 */
bool handleClient(int sock,CpHandler& cph) {
	NetBuffer buf(sock,1024);
	string cmd, reply, clientName;
	bool loggedIn;

	if (!buf.readLine(cmd) || cmd != "Forest-login-v1") {
		Np4d::sendString(sock,"misformatted user dialog\n"
				      "overAndOut\n");
		return false;
	}
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
			cancelSession(buf,clientName,reply);
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

bool newSession(int sock, CpHandler& cph, NetBuffer& buf,
		string& clientName, string& reply) {
	string s1;
	RateSpec rs;
cerr << "new session\n";
	if (buf.verify(':')) {
		if (!buf.readRspec(rs) || !buf.nextLine() ||
		    !buf.readLine(s1)  || s1 != "over") {
cerr << "A\n";
			reply = "misformatted new session request";
			return false;
		}
	} else {
		if (!buf.nextLine() || !buf.readLine(s1)  || s1 != "over") {
cerr << "B\n";
			reply = "misformatted new session request";
			return false;
		}
	}
cerr << "C\n";
	int clx = cliTbl->getClient(clientName);
	if (clx == 0) {
		reply = "cannot access client data(" + clientName + ")";
		return false;
	}
	if (!rs.isSet()) rs = cliTbl->getDefRates(clx);
	// make sure we have sufficient capacity
	if (!rs.leq(cliTbl->getAvailRates(clx))) {
		cliTbl->releaseClient(clx);
		reply = "session rate exceeds available capacity";
		return true;
	}
	cliTbl->releaseClient(clx);
cerr << "D\n";
	// proceed with new session setup
	CtlPkt repCp;
	ipa_t clientIp = Np4d::getSockIp(sock);
	pktx rpx = cph.newSession(nmAdr, clientIp, rs, repCp);
	if (rpx == 0) {
		reply = "cannot setup session: NetMgr never responded";
		return false;
	}
cerr << "E\n";
	if (repCp.mode != CtlPkt::POS_REPLY) {
		reply = "cannot complete login: NetMgr failed (" +
			 repCp.errMsg + ")";
		ps->free(rpx); 
		return false;
	}
cerr << "F\n";
	clx = cliTbl->getClient(clientName);
	if (clx == 0) {
		reply = "cannot update client data(" + clientName + ")";
		return false;
	}
	int sess = cliTbl->addSession(repCp.adr1, repCp.adr2, clx);
	if (sess == 0) {
		cliTbl->releaseClient(clx);
		reply = "cannot complete login: could not create "
			"session record";
		return false;
	}
	cliTbl->setState(sess, ClientTable::PENDING);

cerr << "G\n";
	// set session information
	cliTbl->setClientIp(sess,clientIp);
	cliTbl->setRouterAdr(sess,repCp.adr2);
	cliTbl->setState(sess,ClientTable::PENDING);
	cliTbl->setStartTime(sess,Misc::currentTime());
	cliTbl->getAvailRates(sess).subtract(rs);
	cliTbl->releaseClient(clx);

cerr << "H\n";
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

	ps->free(rpx);
	return true;
}

void getProfile(NetBuffer& buf, string& clientName, string& reply) {
	string s, targetName;
cerr << "reading target name\n";
	if (!buf.verify(':') || !buf.readAlphas(targetName) ||
	    !buf.nextLine() || !buf.readLine(s) || s != "over") {
		reply = "misformatted get profile request"; return;
	}
cerr << "clientName=" << clientName << " targetName=" << targetName << endl;
	int clx = cliTbl->getClient(clientName);
	if (clx == 0) {
		reply = "cannot access client data(" + clientName + ")";
		return;
	}
	int tclx;
	if (targetName == clientName) {
		tclx = clx;
	} else {
		tclx = cliTbl->getClient(targetName);
		if (tclx == 0) {
			cliTbl->releaseClient(clx);
			reply = "no such target client"; return;
		}
		ClientTable::privileges priv = cliTbl->getPrivileges(clx);
		if (priv != ClientTable::ADMIN && priv != ClientTable::ROOT) {
			reply = "this operation requires administrative "
				"privileges";
			cliTbl->releaseClient(clx);
			cliTbl->releaseClient(tclx);
			return;
		}
		ClientTable::privileges tpriv = cliTbl->getPrivileges(tclx);
		if (priv != ClientTable::ROOT && tpriv == ClientTable::ROOT) {
			reply = "this operation requires root privileges";
			cliTbl->releaseClient(clx);
			cliTbl->releaseClient(tclx);
			return;
		}
	}
	reply  = "realName: \"" + cliTbl->getRealName(tclx) + "\"\n";
	reply += "email: " + cliTbl->getEmail(tclx) + "\n";
	reply += "defRates: " + cliTbl->getDefRates(tclx).toString(s) + "\n";
	reply += "totalRates: " + cliTbl->getTotalRates(tclx).toString(s) +"\n";
	cliTbl->releaseClient(clx);
	if (tclx != clx) cliTbl->releaseClient(tclx);
}

void updateProfile(NetBuffer& buf, string& clientName, string& reply) {
	string s, targetName;
cerr << "reading target name\n";
	if (!buf.verify(':') || !buf.readAlphas(targetName) ||
	    !buf.nextLine()) {
		reply = "misformatted updateProfile request"; return;
	}
	int clx = cliTbl->getClient(clientName);
	if (clx == 0) {
		reply = "cannot access client data(" + clientName + ")";
		return;
	}
cerr << "got (" << targetName << ") and I am (" << clientName << ")" << endl;
	int tclx;
	if (targetName == clientName) {
		tclx = clx;
	} else {
		tclx = cliTbl->getClient(targetName);
		if (tclx == 0) {
			cliTbl->releaseClient(clx);
			reply = "no such target client";
			return;
		}
		ClientTable::privileges priv = cliTbl->getPrivileges(clx);
		if (priv != ClientTable::ADMIN && priv != ClientTable::ROOT) {
			reply = "this operation requires administrative "
				"privileges";
			cliTbl->releaseClient(clx);
			cliTbl->releaseClient(tclx);
			return;
		}
		ClientTable::privileges tpriv = cliTbl->getPrivileges(tclx);
		if (priv != ClientTable::ROOT &&
		    (tpriv == ClientTable::ROOT ||
		     tpriv == ClientTable::ADMIN)) {
			reply = "this operation requires root privileges";
			cliTbl->releaseClient(clx);
			cliTbl->releaseClient(tclx);
			return;
		}
	}
	string item; RateSpec rates;
	ClientTable::privileges priv = cliTbl->getPrivileges(clx);
	// release locks while getting profile data
	cliTbl->releaseClient(clx);
	if (tclx != clx) cliTbl->releaseClient(tclx);
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
	tclx = cliTbl->getClient(targetName);
	if (tclx == 0) {
		reply = "could not update target client data";
		return;
	}
	if (realName.length() > 0) cliTbl->setRealName(tclx,realName);
	if (email.length() > 0) cliTbl->setEmail(tclx,email);
	if (priv != ClientTable::LIMITED) {
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

void changePassword(NetBuffer& buf, string& clientName, string& reply) {
	string s, targetName;
	if (!buf.verify(':') || !buf.readAlphas(targetName) || 
	    !buf.nextLine() || !buf.readLine(s) || s != "over") {
		reply = "misformatted change password request"; return;
	}
	int clx = cliTbl->getClient(clientName);
	if (clx == 0) {
		reply = "cannot access client data(" + clientName + ")";
		return;
	}
	int tclx;
	if (targetName == clientName) {
		tclx = clx;
		cliTbl->releaseClient(clx);
	} else {
		tclx = cliTbl->getClient(targetName);
		if (tclx == 0) {
			reply = "no such target"; return;
		}
		ClientTable::privileges priv = cliTbl->getPrivileges(clx);
		ClientTable::privileges tpriv = cliTbl->getPrivileges(tclx);
		cliTbl->releaseClient(clx);
		cliTbl->releaseClient(tclx);
		if (priv != ClientTable::ADMIN && priv != ClientTable::ROOT) {
			reply = "this operation requires administrative "
				"privileges";
			return;
		}
		if (priv != ClientTable::ROOT &&
		    (tpriv == ClientTable::ROOT ||
		     tpriv == ClientTable::ADMIN)) {
			reply = "this operation requires root privileges";
			return;
		}
	}
	string item, pwd;
	while (buf.readAlphas(item)) {
cerr << "item=" << item << "\nbuf=" << buf.toString(s);
		if (item == "password" && buf.verify(':') &&
		    buf.readWord(pwd) && buf.nextLine()) {
			// nothing more for now
		} else if (item == "over" && buf.nextLine()) {
			break;
		} else {
			reply = "misformatted request (" + item + ")";
			return;
		}
	}
	tclx = cliTbl->getClient(targetName);
	if (tclx == 0) {
		reply = "cannot access target client data(" + targetName + ")";
		return;
	}
	cliTbl->setPassword(tclx,pwd);
	writeRecord(tclx);
	cliTbl->releaseClient(tclx);
	return;
}

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

void getSessions(NetBuffer& buf, string& clientName, string& reply) {
	// form a string with one line for each session
	// where each line starts with word "session",
	// followed by the fields defined for each session
	// return this as the value of reply
}

void cancelSession(NetBuffer& buf, string& clientName, string& reply) {
	// check buf for ':' followed by a forest address
	// use the forest address to identify the session
	// send a cancel session control packet to net manager
	// and if that completes successfully, remove session from
	// ClientTable data structure, write account record and return
	// on failure, set reply to indicate the cause of the failure
}

void addComtree(NetBuffer& buf, string& clientName, string& reply) {
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

/*
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
*/

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
