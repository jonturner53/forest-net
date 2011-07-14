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
 *  Command line arguments include two ip addresses.
 *  The first is the IP address is that of the NetMgr which ClientMgr sends CtlPkts to.
 */

int main(int argc, char ** argv) {
	ipa_t rtrIp, myIp; fAdr_t rtrAdr, myAdr, CC_Adr, netMgrAdr;
	uint32_t finTime;
	if(argc != 9 ||
	    (netMgrAdr = Forest::forestAdr(argv[1])) == 0 ||
	    (rtrAdr = Forest::forestAdr(argv[2])) == 0 ||
	    (CC_Adr = Forest::forestAdr(argv[3])) == 0 ||
	    (rtrIp = Np4d::ipAddress(argv[4])) == 0 ||
	    (myIp = Np4d::ipAddress(argv[5])) == 0 ||
	    (myAdr = Forest::forestAdr(argv[6])) == 0 ||
	    (sscanf(argv[7],"%d", &finTime)) != 1)
		fatal("ClientMgr usage: fClientMgr netMgrAdr rtrAdr rtrIp myIp myAdr finTime usersFile");
	char * unamesFile = argv[8];
	ClientMgr climgr(netMgrAdr,rtrIp,rtrAdr,CC_Adr,myIp, myAdr, unamesFile);
	if(!climgr.init()) fatal("ClientMgr::init: Failed to initialize ClientMgr");
	climgr.run(1000000*finTime);
	return 0;
}
// Constructor for ClientMgr, allocates space and initializes private data
ClientMgr::ClientMgr(fAdr_t nma, ipa_t ri, fAdr_t ra, fAdr_t cca, ipa_t mi, fAdr_t ma, char * filename)
	: netMgrAdr(nma), rtrIp(ri), rtrAdr(ra), CC_Adr(cca), myIp(mi), myAdr(ma), unamesFile(filename) {
	sock = extSock = avaSock = -1;
	unames = new map<string,string>;
	readUsernames();
	int nPkts = 10000;
	ps = new PacketStore(nPkts+1,nPkts+1);
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
		&& Np4d::nonblock(extSock);
}

void ClientMgr::initializeAvatar() {
	ipa_t avIp; ipp_t avPort;
	avaSock = Np4d::accept4d(extSock, avIp, avPort);
	if(avaSock < 0) return;
	//receive uname + pword
	char buf[100];
	Np4d::recvBufBlock(avaSock,buf,100);
	string uname; string pword; string s(buf);
	uname = s.substr(2,s.find(' ',2)-2);
	pword = s.substr(3+uname.size(),s.size()-uname.size());
	//o is for old, n is for new
	if(buf[0] == 'o') {
		map<string,string>::iterator it = unames->find(uname);
		if(it != unames->end()) {
			if(it->second != pword) {
				cerr << "incorrect password" << endl;
				return;
			}
		} else {
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
	//send avIp to NetMgr
	packet p = ps->alloc();
	if(p == 0) fatal("ClientMgr::initializeAvatar failed to alloc packet");
	CtlPkt cp;
	cp.setRrType(REQUEST); cp.setSeqNum(1);
	cp.setCpType(NEW_CLIENT); cp.setAttr(CLIENT_IP,avIp);
	int len = cp.pack(ps->getPayload(p));
	PacketHeader &h = ps->getHeader(p); h.setLength(Forest::OVERHEAD + len);
	h.setPtype(CLIENT_SIG); h.setFlags(0);
	h.setComtree(100); h.setSrcAdr(myAdr);
	h.setDstAdr(netMgrAdr); h.pack(ps->getBuffer(p));
	send(p);
	//receive information back from NetMgr
	int rp = recvFromForest();
	cp.unpack(ps->getPayload(rp),1500);
	fAdr_t avaRtrAdr, avaAdr; ipa_t avaRtrIp;
	if(cp.getCpType() == NEW_CLIENT) {
		avaRtrAdr = cp.getAttr(RTR_ADR);
		avaRtrIp = cp.getAttr(RTR_IP);
		avaAdr = cp.getAttr(CLIENT_ADR);
	}
	
	//send forest info to avatar
	Np4d::sendIntBlock(avaSock,avaRtrAdr);
	Np4d::sendIntBlock(avaSock,avaAdr);
	Np4d::sendIntBlock(avaSock,avaRtrIp);
	Np4d::sendIntBlock(avaSock,CC_Adr);
	close(avaSock);
}
/* Read usernames and passwords from file and save into respective arrays
 * @param filename is the usernames file to read from
 */
void ClientMgr::readUsernames() {
	ifstream ifs(unamesFile);
	if(ifs.good()) {
		string line;
		while(getline(ifs,line)) {
			string uname = line.substr(0,line.find_first_of(' '));
			string pword = line.substr(uname.size()+1,line.size()-uname.size()+1);
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
		initializeAvatar();
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
