#include "stdinc.h"
#include "ClientProxy.h"

int main(int argc, char *argv[]){
	ipa_t cliMgrIpAdr, myIpAdr;
	uint32_t finTime;
	if(!argc == 3 ||
	    (myIpAdr     = Np4d::ipAddress(argv[1])) == 0 ||
	    (cliMgrIpAdr = Np4d::ipAddress(argv[2])) == 0 ||
	    (sscanf(argv[3],"%d",&finTime) != 1))
		fatal("usage: ClientProxy myIpAdr cliMgrIpAdr runTime");
	ClientProxy cp(myIpAdr);
	if(!cp.init(cliMgrIpAdr)) fatal("Failed to init client proxy sockets");
	cp.run(1000000*finTime);
}

ClientProxy::ClientProxy(ipa_t mip) : myIpAdr(mip) {
	ps = new PacketStore(npkts);
}

ClientProxy::~ClientProxy() {}

bool ClientProxy::init(ipa_t cmip) {
	sock = Np4d::datagramSocket();
	if(sock < 0 || !Np4d::bind4d(sock,myIpAdr,0) ||
	   !Np4d::nonblock(sock)) {
		cerr << "failed to initialize Forest socket" << endl;
		return false;
	}
	extSock = Np4d::streamSocket();
	avaSock = -1;
	if(extSock < 0 ||
	   !Np4d::bind4d(extSock,Np4d::myIpAddress(),0) ||
	   !Np4d::listen4d(extSock) || !Np4d::nonblock(extSock)) {
		cerr << "failed to initialize external socket" << endl;
		return false;
	}
	//figure out ports
	string myIpStr; Np4d::ip2string(myIpAdr,myIpStr);
	string sockStr; Util::num2string(Np4d::getSockPort(sock),sockStr);
	string extSockStr; extSockStr = Util::num2string(Np4d::getSockPort(extSock),extSockStr);
	string buf = "    " + myIpStr + " " + sockStr + " " + extSockStr;
	buf[0] = (char)0; buf[1] = (char)0; buf[2] = (char)0; buf[3] = (char)0;
	Np4d::sendto4d(sock,(void*)buf.c_str(),buf.size()+1,cmip,LISTEN_PORT);
	return true;
}

void ClientProxy::run(uint32_t runTime) {
	bool gotCMreply = false;
	uint32_t now,nextTime;
	now = nextTime = Util::getTime();
	while(now <= runTime) {
		if(!gotCMreply) {
			char b[100];
			int nbytes = Np4d::recv4d(sock,&b[0],1500);
			if(nbytes < 0) continue;
			gotCMreply = true;
			istringstream iss(b); string rtrIpStr, rtrAdrStr;
			iss >> rtrIpStr >> rtrAdrStr;
			rtrIp = Np4d::ipAddress(rtrIpStr.c_str());
			rtrAdr = Forest::forestAdr(rtrAdrStr.c_str());
			cout << "got rtr ip: " << rtrIpStr << endl;
			cout << "got rtr adddress: " << rtrAdrStr << endl;
		}
		if(avaSock > 0) {
			int p;
			while((p = recvFromAvatar()) != 0)
				send(p);
			while((p = recvFromForest()) != 0)
				send2cli(p);
		} else {
			avaSock = Np4d::accept4d(extSock,avIp,avPort);
			if(avaSock > 0) {
				if(!Np4d::nonblock(avaSock)) fatal("couldn't make avaSock nonblocking");
			}
		}
		nextTime += 1000*UPDATE_PERIOD;
                now = Misc::getTime();
                useconds_t delay = nextTime - now;
                if (delay < ((uint32_t) 1) << 31) usleep(delay);
                else nextTime = now + 1000*UPDATE_PERIOD;
	}
}

/**
 * Receives information over the forest socket
 *
 *@return 0 if nothing is received, a packet otherwise
 */
int ClientProxy::recvFromForest() {
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

int ClientProxy::recvFromAvatar() {
	int p = ps->alloc();
	if(p == 0) return 0;
	buffer_t &b = ps->getBuffer(p);
	int nbytes = Np4d::recvBuf(avaSock,(char*)&b[0],1500);
	if(nbytes <= 0) { ps->free(p); return 0; }
	ps->unpack(p);
	PacketHeader & h = ps->getHeader(p);
	CtlPkt cp;
	cp.unpack(ps->getPayload(p), h.getLength() - (Forest::OVERHEAD));
	return p;
}

void ClientProxy::send2cli(int p) {
PacketHeader &h = ps->getHeader(p);
string pkt; cerr << h.toString(ps->getBuffer(p),pkt);	
	int length = ps->getHeader(p).getLength();
	ps->pack(p);
	int nb = Np4d::sendBuf(avaSock,(char *) ps->getBuffer(p),length);
	if(nb != length) fatal("send: failure in sendBuf");
	ps->free(p);
}

void ClientProxy::send(int p) {
PacketHeader &h = ps->getHeader(p);
string pkt; cerr << h.toString(ps->getBuffer(p),pkt);
	int length = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(sock,(void *) ps->getBuffer(p),length,
		rtrIp, Forest::ROUTER_PORT);
	if (rv == -1) fatal("send: failure in sendto");
	ps->free(p);
}

