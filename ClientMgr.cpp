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
#include "support/Np4d.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>
/** usage:
 *  fClientMgr netMgrAdr rtrAdr rtrIp myIp myAdr finTime usersFile
 *
 *  Command line arguments include the following:
 *  netMgrAdr - The Forest address of a Network Manager
 *  rtrAdr - The Forest address of the router to connect to
 *  CC_Adr - The Forest address of a Comtree Controller
 *  rtrIp - The IP address of the router to connect to
 *  myIp - The IP address of this Client Manager
 *  myAdr - The Forest address of this Client Manager
 *  finTime - How many seconds this program should run before terminating
 *  unamesFile - List of usernames and associated passwords
 *  acctFile - output file keeping track of which clients connect to this Client Manager
 */

int main(int argc, char ** argv) {
	ipa_t rtrIp, myIp; fAdr_t rtrAdr, myAdr, CC_Adr, netMgrAdr;
	uint32_t finTime;
	if(argc != 10 ||
	    (netMgrAdr = Forest::forestAdr(argv[1])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[2])) == 0 ||
	    (CC_Adr = Forest::forestAdr(argv[3])) == 0 ||
	    (rtrIp = Np4d::ipAddress(argv[4])) == 0 ||
	    (myIp = Np4d::ipAddress(argv[5])) == 0 ||
	    (myAdr = Forest::forestAdr(argv[6])) == 0 ||
	    (sscanf(argv[7],"%d", &finTime)) != 1)
		fatal("ClientMgr usage: fClientMgr netMgrAdr rtrAdr rtrIp myIp myAdr finTime usersFile");
	char * unamesFile = argv[8];
	char * acctFile = argv[9];
	ClientMgr climgr(netMgrAdr,rtrIp,rtrAdr,CC_Adr,myIp, myAdr, unamesFile,acctFile);
	if(!climgr.init()) fatal("ClientMgr::init: Failed to initialize ClientMgr");
	climgr.run(1000000*finTime);
	return 0;
}
// Constructor for ClientMgr, allocates space and initializes private data
ClientMgr::ClientMgr(fAdr_t nma, ipa_t ri, fAdr_t ra, fAdr_t cca, ipa_t mi, fAdr_t ma, char * filename, char * acctFile)
	: netMgrAdr(nma), rtrIp(ri), rtrAdr(ra), CC_Adr(cca), myIp(mi), myAdr(ma), unamesFile(filename) {
	sock = extSock = avaSock = -1;
	unames = new map<string,string>;
	readUsernames();
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1,nPkts+1);
	acctFileStream.open(acctFile);
	clients = new map<uint32_t,clientStruct>;
	highLvlSeqNum = 0;
	lowLvlSeqNum = 0; 
}

ClientMgr::~ClientMgr() { delete unames; }
/** Initializes sockets.
 *  @return true on success, false on failure
 */
bool ClientMgr::init() {
	extSock = Np4d::streamSocket();
	sock = Np4d::datagramSocket();
	if(sock < 0) return false;
	if(extSock < 0) return false;
	bool status = Np4d::bind4d(extSock,myIp,LISTEN_PORT);
	if(!status) return false;
	status = Np4d::bind4d(sock,myIp,0);
	if(!status) return false;
	connect();
	usleep(1000000); //sleep for one second
	return Np4d::listen4d(extSock)
		&& Np4d::nonblock(extSock)
		&& Np4d::nonblock(sock);
}

/** Receieves the login string and the avatar's IP address
 */
void ClientMgr::initializeAvatar() {
	ipa_t avIp; ipp_t avPort;
	avaSock = Np4d::accept4d(extSock, avIp, avPort);
	if(avaSock < 0) return;
	char buf[100];
	Np4d::recvBufBlock(avaSock,buf,100);
	string uname; string pword; string s(buf);
	int space1 = s.find(' ',2);
	int space2 = s.find(' ',space1+1);
	uname = s.substr(2,space1-2);
	pword = s.substr(space1+1,space2-(space1+1));
	string fPortStr = s.substr(space2+1);

cerr << "uname=" << uname << " pword=" << pword << " fPortStr=" << fPortStr << endl;
	ipp_t avForestPort = atoi(fPortStr.c_str());
cerr << "avForestPort=" << avForestPort << endl;
	//o is for old, n is for new
	if(buf[0] == 'o') {
		map<string,string>::iterator it = unames->find(uname);
		if(it != unames->end()) {
			if(it->second != pword) {
				cerr << "incorrect password" << endl;
				return;
			}
		} else {
cerr << "uname=" << uname << endl;
			cerr << "not a known user" << endl;
			return;
		}
	} else {
		//write new uname to file (append)
		ofstream ofs(unamesFile,ios_base::app);
		if(ofs.good()) {
			ofs << uname << " " << pword << endl;
			ofs.close();
		} else
			fatal("couldn't write to file");
	}
	(*clients)[++highLvlSeqNum].uname = uname;
	(*clients)[highLvlSeqNum].pword = pword;
	requestAvaInfo(avIp,avForestPort);
}
/** Requests the router information from a network manager
 *  @param aip is the IP address of the client to receive information about
 */
void ClientMgr::requestAvaInfo(ipa_t aip, ipp_t aport) {
	packet p = ps->alloc();
	if(p == 0) fatal("ClientMgr::requestAvaInfo failed to alloc packet");
	CtlPkt cp;
	cp.setRrType(REQUEST); cp.setSeqNum(((++lowLvlSeqNum) << 32)|(highLvlSeqNum & 0xffffffff));
	cp.setCpType(NEW_CLIENT); cp.setAttr(CLIENT_IP,aip);
	cp.setAttr(CLIENT_PORT,aport);
	int len = cp.pack(ps->getPayload(p));
	PacketHeader &h = ps->getHeader(p); h.setLength(Forest::OVERHEAD + len);
	h.setPtype(NET_SIG); h.setFlags(0);
	h.setComtree(100); h.setSrcAdr(myAdr);
	h.setDstAdr(netMgrAdr); h.pack(ps->getBuffer(p));
ps->pack(p);
cerr << "sending new client request to NetMgr\n";
h.write(cerr,ps->getBuffer(p));
	send(p);
}

/** Writes information about connection control packets to a file
 *  @param cp is a control packet with information to write into an accounting file
 */
void ClientMgr::writeToAcctFile(CtlPkt cp) {
	if(acctFileStream.good()) {
		if(cp.getCpType() == NEW_CLIENT && cp.getRrType() == POS_REPLY) {
			acctFileStream << Misc::getTime() << " Client "; Forest::writeForestAdr(acctFileStream,cp.getAttr(CLIENT_ADR));
			acctFileStream << " added to router "; Forest::writeForestAdr(acctFileStream,cp.getAttr(RTR_ADR));
			acctFileStream << endl;
		} else if(cp.getCpType() == CLIENT_CONNECT) {
			acctFileStream << Misc::getTime() << " Client "; Forest::writeForestAdr(acctFileStream,cp.getAttr(CLIENT_ADR));
			acctFileStream << " connected to router "; Forest::writeForestAdr(acctFileStream,cp.getAttr(RTR_ADR));
			acctFileStream << endl;
		} else if(cp.getCpType() == CLIENT_DISCONNECT) {
			acctFileStream << Misc::getTime() << " Client "; Forest::writeForestAdr(acctFileStream,cp.getAttr(CLIENT_ADR));
			acctFileStream << " disconnected from router "; Forest::writeForestAdr(acctFileStream,cp.getAttr(RTR_ADR));
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
void ClientMgr::readUsernames() {
	ifstream ifs(unamesFile);
	if(ifs.good()) {
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

void ClientMgr::send(int p) {
	int length = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(sock,(void *) ps->getBuffer(p),length,
		rtrIp, Forest::ROUTER_PORT);
	if (rv == -1) fatal("ClientMgr::send: failure in sendto");
	ps->free(p);
}
/** run repeatedly tries to initialize any new avatars until the time runs out
 *@param finTime is the length of time that this should run
 *
 */
void ClientMgr::run(int finTime) {
	uint32_t now;
	now = 0;
	while(now <= finTime) {
		now = Misc::getTime();
		if(avaSock < 0) initializeAvatar();
		int p = recvFromForest();
		if(p != 0) {
			CtlPkt cp; cp.unpack(ps->getPayload(p),1500);
			if(cp.getCpType() == CLIENT_CONNECT || cp.getCpType() == CLIENT_DISCONNECT) {
				writeToAcctFile(cp);
				uint64_t highLvlSeq = cp.getSeqNum() & 0xffffffff;
				lowLvlSeqNum = cp.getSeqNum() >> 32;
				cp.setSeqNum((++lowLvlSeqNum << 32)|(highLvlSeq & 0xffffffff));
				cp.setRrType(POS_REPLY);
				int len = cp.pack(ps->getPayload(p));
				PacketHeader &h = ps->getHeader(p);
				h.setLength(Forest::OVERHEAD + len);
				h.setDstAdr(netMgrAdr);
				h.setSrcAdr(myAdr);
				h.pack(ps->getBuffer(p));
				send(p);
					
			} else if(cp.getCpType() == NEW_CLIENT && cp.getRrType() == POS_REPLY) {
				uint64_t highLvlSeq = cp.getSeqNum() & 0xffffffff;
				lowLvlSeqNum = cp.getSeqNum() >> 32;
				writeToAcctFile(cp);
				fAdr_t avaRtrAdr = cp.getAttr(RTR_ADR);
				ipa_t avaRtrIp = cp.getAttr(RTR_IP);
				fAdr_t avaAdr = cp.getAttr(CLIENT_ADR);
				(*clients)[highLvlSeq].ra = avaRtrAdr;
				(*clients)[highLvlSeq].rip = avaRtrIp;
				(*clients)[highLvlSeq].fa = avaAdr;
				Np4d::sendInt(avaSock,avaRtrAdr);
				Np4d::sendInt(avaSock,avaAdr);
				Np4d::sendInt(avaSock,avaRtrIp);
				Np4d::sendInt(avaSock,CC_Adr);
				close(avaSock);
				avaSock = -1;
			}
		}
	}
	disconnect();
}
/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
void ClientMgr::connect() {
        packet p = ps->alloc();
	if(p == 0) fatal("Couldn't allocate space");
        PacketHeader& h = ps->getHeader(p);
        h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
        h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
        send(p);
}
/** Send final disconnect packet to forest router
 */
void ClientMgr::disconnect() {
        packet p = ps->alloc();
        PacketHeader& h = ps->getHeader(p);

        h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
        h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

        send(p);
}
/**
 * Receives information over the forest socket
 *
 *@return 0 if nothing is received, a packet otherwise
 */
int ClientMgr::recvFromForest() {
        int p = ps->alloc();
        if (p == 0) return 0;
        buffer_t& b = ps->getBuffer(p);
        int nbytes = Np4d::recv4d(sock,&b[0],1500);
        if (nbytes < 0) { ps->free(p); return 0; }
        ps->unpack(p);
        PacketHeader& h = ps->getHeader(p);
        CtlPkt cp;
        cp.unpack(ps->getPayload(p), h.getLength() - (Forest::OVERHEAD));
	return p;
}
