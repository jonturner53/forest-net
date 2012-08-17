/** @file ClientMgr.cpp
 * 
 * @author Logan Stafman
 * @date 2011
 *
 *
 */
#include "ClientMgr.h"
#include "CommonDefs.h"
#include "stdinc.h"
#include "Np4d.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
/** usage:
 *  ClientMgr netMgrAdr rtrAdr ccAdr rtrIp intIp extIp myAdr finTime
 *  usersFile acctFile prefixFile
 *
 *  Command line arguments include the following:
 *  netMgrAdr - The Forest address of a Network Manager
 *  rtrAdr - The Forest address of the router to connect to
 *  CC_Adr - The Forest address of a Comtree Controller
 *  rtrIp - The IP address of the router to connect to
 *  intIp - The IP address used for TCP connections from "internal" hosts
 *  extIp - The IP address used for TCP connections from "external" hosts
 *  myAdr - The Forest address of this Client Manager
 *  finTime - How many seconds this program should run before terminating
 *  users - list of usernames and associated passwords
 *  acctFile - output file keeping track of which clients connect to
 *  this Client Manager
 *  prefixFile - file defining how different remote IP prefixes map to
 *  routers in the forest network
 */

int main(int argc, char ** argv) {
	uint32_t finTime = 0;
	ipa_t rtrIp, intIp, extIp; fAdr_t rtrAdr, myAdr, CC_Adr, netMgrAdr;
	rtrIp = intIp = extIp = 0; rtrAdr = myAdr = CC_Adr = netMgrAdr = 0;
	if(argc != 12 ||
	    (netMgrAdr = Forest::forestAdr(argv[1])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[2])) == 0 ||
	    (CC_Adr = Forest::forestAdr(argv[3])) == 0 ||
	    (rtrIp = Np4d::ipAddress(argv[4])) == 0 ||
	    (intIp = Np4d::ipAddress(argv[5])) == 0 ||
	    (extIp = Np4d::ipAddress(argv[6])) == 0 ||
	    (myAdr = Forest::forestAdr(argv[7])) == 0 ||
	    (sscanf(argv[8],"%d", &finTime)) != 1) {
		fatal("ClientMgr usage: ClientMgr netMgrAdr rtrAdr comtCtlAdr "
		      "rtrIp intIp extIp myAdr finTime usersFile acctFile "
		      "prefixFile");
	}
	if(!init(netMgrAdr,rtrIp,rtrAdr,CC_Adr,intIp, extIp, myAdr,
	         argv[9], argv[10]))
		fatal("init: Failed to initialize ClientMgr");
	if(!readPrefixInfo(argv[11]))
		fatal("readPrefixFile: Failed to read prefixes");
	
	pthread_t run_thread;
	if(pthread_create(&run_thread,NULL,run,(void *) &finTime) != 0) {
		fatal("can't create run thread\n");
	}
	pthread_join(run_thread,NULL);
	return 0;
}

/** Initializes sockets, and acts as a psuedo-constructor
 *  @param nma is the netMgr forest address
 *  @param ri is the IP of the router
 *  @param ra is the forest address of the router
 *  @param cca is the ComtreeController's forest address
 *  @param mi is the IP address of tihs Netmgr
 *  @param ma is the forest address of this NetMgr
 *  @param filename is the file of the usernames file
 *  @param acctFile is the file to write connection records to
 *  @return true on success, false on failure
 */
bool init(fAdr_t nma, ipa_t ri, fAdr_t ra, fAdr_t cca,
	ipa_t iip, ipa_t xip, fAdr_t ma, char * filename, char * acctFile) {
	netMgrAdr = nma; rtrIp = ri; rtrAdr = ra;
	CC_Adr = cca; intIp = iip; extIp = xip;
	myAdr = ma; unamesFile = filename; 
	sock = tcpSockInt = tcpSockExt = avaSock = -1;
	unames = new map<string,string>;
	readUsernames();
	int nPkts = 10000;
	ps = new PacketStoreTs(nPkts+1);
	acctFileStream.open(acctFile);
	clients = new map<uint32_t,clientStruct>;
	seqNum = 0;
	proxyIndex = 0;
	proxyQueues = new map<fAdr_t,Queue*>; 
	pool = new ThreadPool[TPSIZE+1];
	threads = new UiSetPair(TPSIZE);
	tMap = new IdMap(TPSIZE);

	//setup sockets
	tcpSockInt = Np4d::streamSocket();
	tcpSockExt = Np4d::streamSocket();
	sock = Np4d::datagramSocket();
	if(sock < 0) return false;
	if(tcpSockInt < 0) return false;
	if(tcpSockExt < 0) return false;
	bool status = Np4d::bind4d(tcpSockInt,intIp,LISTEN_PORT) &&
		      Np4d::bind4d(tcpSockExt,extIp,LISTEN_PORT);
	if (!status) return false;
	status = Np4d::bind4d(sock,intIp,LISTEN_PORT);
	if (!status) return false;
	connect();
	usleep(1000000); //sleep for one second

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr,PTHREAD_STACK_MIN);
	size_t stacksize;
	pthread_attr_getstacksize(&attr,&stacksize);
	cerr << "min stack size=" << PTHREAD_STACK_MIN << endl;
	cerr << "threads in pool have stacksize=" << stacksize << endl;
	if (stacksize != PTHREAD_STACK_MIN)
		fatal("init: can't set stack size");
	for (int t = 1; t <= TPSIZE; t++) {
		if (!pool[t].qp.in.init() || !pool[t].qp.out.init())
			fatal("init: can't initialize thread queues\n");
                if (pthread_create(&pool[t].th,&attr,handler,
		    (void *) &pool[t]) != 0)
			fatal("init: can't create thread pool");
        }
	return Np4d::listen4d(tcpSockInt)
		&& Np4d::nonblock(tcpSockInt)
		&& Np4d::listen4d(tcpSockExt)
		&& Np4d::nonblock(tcpSockExt)
		&& Np4d::nonblock(sock);
}

/** Writes information about connection control packets to a file
 *  @param cp is a control packet with information to write into an accounting file
 */
void writeToAcctFile(CtlPkt cp) {
	if(acctFileStream.good()) {
		string s;
		if(cp.getCpType() ==NEW_CLIENT && cp.getRrType() == POS_REPLY) {
			acctFileStream << Misc::getTimeNs() << " Client "
			  << Forest::fAdr2string(cp.getAttr(CLIENT_ADR),s);
			acctFileStream << " added to router "
			  << Forest::fAdr2string(cp.getAttr(RTR_ADR),s);
			acctFileStream << endl;
		} else if(cp.getCpType() == CLIENT_CONNECT) {
			acctFileStream << Misc::getTimeNs() << " Client "
			  << Forest::fAdr2string(cp.getAttr(CLIENT_ADR),s);
			acctFileStream << " connected to router "
			  << Forest::fAdr2string(cp.getAttr(RTR_ADR),s);
			acctFileStream << endl;
		} else if(cp.getCpType() == CLIENT_DISCONNECT) {
			acctFileStream << Misc::getTimeNs() << " Client "
			  << Forest::fAdr2string(cp.getAttr(CLIENT_ADR),s);
			acctFileStream << " disconnected from router "
			  << Forest::fAdr2string(cp.getAttr(RTR_ADR),s);
			acctFileStream << endl;
		} else {
			acctFileStream << "Unrecognized control packet" << endl;
		}
	} else {
		cerr << "accounting file could not open" << endl;
	}
}


/* Read usernames and passwords from file and save into respective arrays
 * @param filename is the usernames file to read from
 */
void readUsernames() {
	ifstream ifs(unamesFile);
	if (ifs.good()) {
		while(!ifs.eof()) {
			string uname; string pword;
			ifs >> uname;
			ifs >> pword;
			Misc::skipBlank(ifs);
			(*unames)[uname] = pword;
		}
	} else
		fatal("Could not read usernames file");
}

void send(int p) {
	int length = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(sock,(void *) ps->getBuffer(p),length,
		rtrIp, Forest::ROUTER_PORT);
	if (rv == -1) fatal("send: failure in sendto");
	ps->free(p);
}

/** Finds the router address of the router based on IP prefix
 *  @param cliIp is the ip of the client proxy
 *  @param rtrAdr is the address of the router associated with this IP prefix
 *  @return true if there was an IP prefix found, false otherwise
 */
bool findCliRtr(ipa_t cliIp, fAdr_t& rtrAdr, ipa_t& rtrIp) {
	string cip;
	Np4d::ip2string(cliIp,cip);
cerr << "findCliRtr cip=" << cip << endl;
	for(unsigned int i = 0; i < (unsigned int) numPrefixes; ++i) {
		string ip = prefixes[i].prefix;
cerr << prefixes[i].prefix << endl;
		unsigned int j = 0;
		while (j < ip.size() && j < cip.size()) {
			if (ip[j] != '*' && cip[j] != ip[j]) break;
			if (ip[j] == '*' ||
			    j+1 == ip.size() && j+1 == cip.size()) {
				rtrAdr = prefixes[i].rtrAdr;
				rtrIp  = prefixes[i].rtrIp;
string s;
cerr << "returning true " << Forest::fAdr2string(rtrAdr,s) << "\n";
				return true;
			}
			j++;
		}
	}
cerr << "findCliRtr returns false\n";
	return false;
}

/** Reads the prefix file
 *  @param filename is the name of the prefix file
 *  @return true on success, false otherwise
 */
bool readPrefixInfo(char filename[]) {
	ifstream ifs; ifs.open(filename);
	if(ifs.fail()) return false;
	Misc::skipBlank(ifs);
	int i = 0;
	while(!ifs.eof()) {
		string pfix, rtrIpStr; fAdr_t rtrAdr; ipa_t rtrIp;
		ifs >> pfix;
		if(!Forest::readForestAdr(ifs,rtrAdr))
			break;
		ifs >> rtrIpStr;
		rtrIp = Np4d::ipAddress(rtrIpStr.c_str());
		prefixes[i].prefix = pfix;
		prefixes[i].rtrAdr = rtrAdr;
		prefixes[i].rtrIp  = rtrIp;
		Misc::skipBlank(ifs);
		i++;
	}
	numPrefixes = i;
	cout << "read address info for " << numPrefixes << " prefixes" << endl;
	return true;
}

/** run repeatedly tries to initialize any new avatars until the time runs out
 *@param finTime is the length of time that this should run
 *
 */
void* run(void* finishTime) {
	uint64_t now = Misc::getTimeNs();
	uint64_t finTime = *((uint32_t *) finishTime);
	finTime *= 1000000000;
	while (now <= finTime) {
		bool nothing2do = true;
		ipa_t avIp; ipp_t port;
		int avaSock = Np4d::accept4d(tcpSockExt,avIp,port);
		if(avaSock <= 0) avaSock = Np4d::accept4d(tcpSockInt,avIp,port);
		if(avaSock > 0) {
			nothing2do = false;
			int t = threads->firstOut();
			if(t == 0) fatal("ClientMgr::run: out of threads");
			threads->swap(t);
			pool[t].ipa = avIp;
			pool[t].sock = avaSock;
			pool[t].seqNum = ++seqNum;
			pool[t].qp.in.enq(1); //TCP connection set up
		}
		//first receieve packets and send to threads
		string proxyString;
		int p = recvFromForest(proxyString);
		if (p > 0) {
			nothing2do = false;
			handleIncoming(p);	
		} else if(p < 0) {
			//message from proxy
			istringstream iss(proxyString);
			string ipStr;
			ipp_t proxUdp, proxTcp;
			iss >> ipStr >> proxUdp >> proxTcp;
			ipa_t proxIp = Np4d::ipAddress(ipStr.c_str());
			ipa_t rtrIp; fAdr_t rtrAdr;
			if(!findCliRtr(proxIp,rtrAdr,rtrIp)) {
				rtrAdr = prefixes[0].rtrAdr;
				rtrIp  = prefixes[0].rtrIp;
			}
			proxies[proxyIndex].pip = proxIp;
			proxies[proxyIndex].udpPort = proxUdp;
			proxies[proxyIndex].tcpPort = proxTcp;
			if(proxyQueues->find(rtrAdr)==proxyQueues->end()) {
				(*proxyQueues)[rtrAdr] = new Queue(10);
			}
			Queue* q = (proxyQueues->find(rtrAdr))->second;
			q->enq(proxyIndex++);
			//send back, but not as ctl pkt
			string rtrIpStr; Np4d::ip2string(rtrIp,rtrIpStr);
			string rtrAdrStr; Forest::fAdr2string(rtrAdr,rtrAdrStr);
			string buf = rtrIpStr + " " + rtrAdrStr;
			//send that buf
			Np4d::sendto4d(sock,(void*)buf.c_str(),buf.size()+1,proxIp,proxUdp);
		} else {
			ps->free(p);
		}
		//now do some sending
		for (int t = threads->firstIn(); t != 0;
			t = threads->nextIn(t)) {
			if(pool[t].qp.out.empty()) continue;
			nothing2do = false;
			int p1 = pool[t].qp.out.deq();
			if (p1 == 0) { //thread is done
				pool[t].qp.in.reset();
				threads->swap(t);
				close(pool[t].sock);
				continue;
			}
			PacketHeader& h1 = ps->getHeader(p1);
			CtlPkt cp1;
			cp1.unpack(ps->getPayload(p1),
				   h1.getLength()-Forest::OVERHEAD);
			if (cp1.getSeqNum() == 1) {
				// means this is a repeat of a pending
				// outgoing request
				if (tMap->validId(t)) {
					cp1.setSeqNum(tMap->getKey(t));
		 		} else {
					// in this case, reply has arrived
					// so, suppress duplicate request
					ps->free(p1); continue;
				}
			} else {
				if (tMap->validId(t))
					tMap->dropPair(tMap->getKey(t));
				tMap->addPair(seqNum,t);
				cp1.setSeqNum(seqNum++);
			}
			cp1.pack(ps->getPayload(p1));
			h1.payErrUpdate(ps->getBuffer(p1));
			send(p1);
		}
		if(nothing2do && threads->firstIn() == 0) usleep(1000);
		now = Misc::getTimeNs();
	}
	disconnect();
	pthread_exit(NULL);
}

void handleIncoming(int p) {
	PacketHeader& h = ps->getHeader(p);
	if(h.getPtype() == NET_SIG) {
		CtlPkt cp;
		cp.unpack(ps->getPayload(p),
			h.getLength()-Forest::OVERHEAD);
		if (cp.getCpType() == NEW_CLIENT) {
			writeToAcctFile(cp);
			int t = tMap->getId(cp.getSeqNum());
			if (t != 0) {
				pool[t].seqNum = 0;
				pool[t].qp.in.enq(p);
// might want to print error message in this case
			} else ps->free(p);
		} 
		else if(cp.getRrType() == REQUEST &&
		    (cp.getCpType() == CLIENT_CONNECT ||
		     cp.getCpType() == CLIENT_DISCONNECT)) {
			writeToAcctFile(cp);
			CtlPkt cp2(cp.getCpType(),POS_REPLY,cp.getSeqNum());
			int p1 = ps->alloc();
			if (p1 == 0) fatal("packetstore out of packets!");
			PacketHeader & h1 = ps->getHeader(p1);
			h1.setDstAdr(h.getSrcAdr());
			h1.setSrcAdr(h.getDstAdr());
			h1.setFlags(0); h1.setComtree(Forest::NET_SIG_COMT);
			int len = cp2.pack(ps->getPayload(p1));
			h1.setLength(Forest::OVERHEAD + len);
			h1.setPtype(NET_SIG);
			h1.pack(ps->getBuffer(p1));
			send(p1);
		/*} else if(cp.getRrType() == NEG_REPLY &&
			    h.getSrcAdr()==netMgrAdr) {
			string s;
			cerr << "Negative reply from NetMgr:" << endl
			     << cp.toString(s);
		*/} else {
			string s;
			cerr << "unrecognized ctl pkt\n" << endl
			     << cp.toString(s);
			ps->free(p);
		}
	} else {
		ps->free(p);
	}
}
/*Thread dispatcher method
 * @param tp is the threadpool struct for this specific thread/avatar
 */
void* handler(void * tp) {
	Queue& inQ = (((ThreadPool *) tp)->qp).in;
	Queue& outQ = (((ThreadPool *) tp)->qp).out;

	while (true) {
		int start = inQ.deq();
		if (start != 1) {
			string s;
			PacketHeader& h = ps->getHeader(start);
			cerr << h.toString(ps->getBuffer(start),s)
			     << "handler: Thread synchronization error, "
				"abandoning this attempt\n";
			outQ.enq(0); //signal completion to main thread
			close(avaSock);
			continue;
		}
		int aip = ((ThreadPool *) tp)->ipa;
		int avaSock = ((ThreadPool *) tp)->sock;
		int seqNum = ((ThreadPool *) tp)->seqNum;
		char buf[100];
		uint32_t port;
	        Np4d::recvBufBlock(avaSock,buf,100);
		istringstream iss(buf);
		string uname, pword, proxy;
		iss >> uname >> pword >> port >> proxy;
		bool needProxy = (proxy=="proxy");
		CtlPkt cp2(NEW_CLIENT,REQUEST,seqNum);
		fAdr_t rtrAdr; ipa_t rtrIp;
		if(!findCliRtr(aip,rtrAdr,rtrIp)) {
			rtrAdr = prefixes[0].rtrAdr;
			rtrIp = prefixes[0].rtrIp;
		}
		proxyStruct pro;
		if(needProxy) {
			int proxyIndex = ((proxyQueues->find(rtrAdr))->second)->deq();
			pro = proxies[proxyIndex];
		}
		cp2.setAttr(CLIENT_IP,(needProxy ? pro.pip : aip));
		cp2.setAttr(CLIENT_PORT,(needProxy ? pro.udpPort : (ipp_t)port));
		int reply = sendCtlPkt(cp2,Forest::NET_SIG_COMT,
					netMgrAdr,inQ,outQ);
		if (reply == NORESPONSE) {
			// abandon this attempt
			string s;
			cerr << "handler: no reply from net manager to\n"
			     << cp2.toString(s);
			Np4d::sendInt(avaSock,-1);
			outQ.enq(0); //signal completion to main thread
			close(avaSock);
			continue;
		}
		//get response
		PacketHeader &h3 = ps->getHeader(reply);
		CtlPkt cp3;
		cp3.unpack(ps->getPayload(reply),
			   h3.getLength()-Forest::OVERHEAD);
		if(cp3.getCpType() == NEW_CLIENT &&
		    cp3.getRrType() == POS_REPLY) {
			fAdr_t rtrAdr = cp3.getAttr(RTR_ADR);
			ipa_t rtrIp = cp3.getAttr(RTR_IP);
			fAdr_t cliAdr = cp3.getAttr(CLIENT_ADR);
			//send via TCP
			Np4d::sendInt(avaSock,rtrAdr);
			Np4d::sendInt(avaSock,cliAdr);
			if(needProxy) {
				Np4d::sendInt(avaSock,pro.pip);
				Np4d::sendInt(avaSock,pro.tcpPort);
				Np4d::sendInt(avaSock,pro.udpPort);
				Np4d::sendInt(avaSock,CC_Adr);
			} else {
				Np4d::sendInt(avaSock,rtrIp);
				Np4d::sendInt(avaSock,CC_Adr);
			}
		} else if(cp3.getCpType() == NEW_CLIENT &&
		    	  cp3.getRrType() == NEG_REPLY) { //send -1 to client
			cerr << "Client could not connect:" << cp3.getErrMsg()
			     << endl;
			Np4d::sendInt(avaSock,-1);
		} else {
			string s;
			cerr << h3.toString(ps->getBuffer(reply),s);
			cerr << cp3.toString(s);
			cerr << "handler: unrecognized ctl pkt\n";
		}
		close(avaSock);
		ps->free(reply);
		outQ.enq(0); //signal completion to main thread
	}
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
void connect() {
        packet p = ps->alloc();
	if(p == 0) fatal("Couldn't allocate space");
        PacketHeader& h = ps->getHeader(p);
        h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
        h.setComtree(Forest::CLIENT_CON_COMT);
	h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
        send(p);
	ps->free(p);
}
/** Send final disconnect packet to forest router
 */
void disconnect() {
        packet p = ps->alloc();
        PacketHeader& h = ps->getHeader(p);

        h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
        h.setComtree(Forest::CLIENT_CON_COMT);
	h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

        send(p);
	ps->free(p);
}
/**
 * Receives information over the forest socket
 *
 *@return 0 if nothing is received, a packet otherwise
 */
int recvFromForest(string& proxyString) {
        int p = ps->alloc();
        if (p == 0) return 0;
        buffer_t& b = ps->getBuffer(p);
        int nbytes = Np4d::recv4d(sock,&b[0],1500);
        if (nbytes < 0) { ps->free(p); return 0; }
	if(b[0] == 0) {//not a forest pkt, but a proxy pkt
		proxyString = (string)((char *) (&b[1]));
		ps->free(p);
		return -1;
	}
        ps->unpack(p);
        PacketHeader& h = ps->getHeader(p);
        CtlPkt cp;
        cp.unpack(ps->getPayload(p), h.getLength() - (Forest::OVERHEAD));
	return p;
}

/** Send a control packet multiple times before giving up.
 *  This method makes a copy of the original and sends the copy
 *  back to the main thread. If no reply is received after 1 second,
 *  it tries again; it makes a total of three attempts before giving up.
 *  @param p is the packet number of the packet to be sent; the header
 *  for p is assumed to be unpacked
 *  @param cp is the control packet structure for p (already unpacked)
 *  @param inQ is the queue coming from the main thread
 *  @param outQ is the queue going back to the main thread
 *  @return the packet number for the reply packet, or -1 if there
 *  was no reply
 */
int sendAndWait(int p, CtlPkt& cp, Queue& inQ, Queue& outQ) {
	PacketHeader& h = ps->getHeader(p);
	h.setSrcAdr(myAdr); ps->pack(p);

	// make copy of packet and send the copy
	int copy = ps->fullCopy(p);
	if (copy == 0) {
		cerr << "sendAndWait: no packets left in packet store\n";
		return NORESPONSE;
	}
	outQ.enq(copy);

	for (int i = 1; i < 3; i++) {
		int reply = inQ.deq(2000000000); // 2 sec timeout
		if (reply == Queue::TIMEOUT) {
			// no reply, make new copy and send
			int retry = ps->fullCopy(p);
			if (retry == 0) {
				cerr << "sendAndWait: no packets "
					"left in packet store\n";
				return NORESPONSE;
			}
			cp.setSeqNum(1);        // tag retry as a repeat
                        cp.pack(ps->getPayload(retry));
                        PacketHeader& hr = ps->getHeader(retry);
                        hr.payErrUpdate(ps->getBuffer(retry));
			outQ.enq(retry);
		} else {
			return reply;
		}
	}
	return NORESPONSE;
}

/** Send a control packet back through the main thread.
 *  The control packet object is assumed to be already initialized.
 *  If it is a reply packet, it is packed into a packet object whose index is
 *  then placed in the outQ. If it is a request packet, it is sent similarly
 *  and then the method waits for a reply. If the reply is not received
 *  after one second, it tries again. After three attempts, it gives up.
 *  @param cp is the pre-formatted control packet
 *  @param comt is the comtree on which it is to be sent
 *  @param dest is the destination address to which it is to be sent
 *  @param inQ is the thread's input queue from the main thread
 *  @param outQ is the thread's output queue back to the main thread
 *  @return the packet number of the reply, when there is one, and 0
 *  on on error, or if there is no reply.
 */
int sendCtlPkt(CtlPkt& cp, comt_t comt, fAdr_t dest, Queue& inQ, Queue& outQ) {
        int p = ps->alloc();
        if (p == 0) {
                cerr << "sendCtlPkt: no packets left in packet store\n";
                return 0;
        }
	// set default seq# for requests, tells main thread to use "next" seq#
	if (cp.getRrType() == REQUEST) cp.setSeqNum(0);
        int plen = cp.pack(ps->getPayload(p));
        if (plen == 0) {
		string s;
                cerr << "sendCtlPkt: packing error\n" << cp.toString(s);
                ps->free(p);
                return 0;
        }
        PacketHeader& h = ps->getHeader(p);
        h.setLength(plen + Forest::OVERHEAD);
        h.setPtype(NET_SIG);
        h.setFlags(0);
        h.setComtree(comt);
        h.setDstAdr(dest);
        h.setSrcAdr(myAdr);
        h.pack(ps->getBuffer(p));

        if (cp.getRrType() != REQUEST) {
                outQ.enq(p); return 0;
        }
        int reply = sendAndWait(p,cp,inQ,outQ);
        ps->free(p);
        return reply;
}
