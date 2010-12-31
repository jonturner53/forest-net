// This version of ioProc does no real io. Instead it generates
// packets based on a packet specification read from stdin on
// startup. These are fed to the rest of the router through
// the standard interface, allowing for measurements of the
// throughput of the packet processing code, independent of
// the system IO calls.
//
// The packet spec contains one packet specification per line,
// with spaces separating successive fields. The first field
// is the IP address and port number of the source
// host with a colon separating the two. The second field
// is the wunet packet length. This is followed by the
// packet's vnet field, then its wunet src address and destination
// address. This can optionally be followed by values to
// go into successive payload words. Unspecified payload
// words are padded with zeros.

#include "ioProc.h"

// Constructor for ioProc, allocates space and initializes private data
ioProc::ioProc(ipa_t mip1, ipp_t mp1, lnkTbl *lt1, pktStore *ps1)
		: myIpAdr(mip1) , myPort(mp1), lt(lt1), ps(ps1) {
	// nothing else to do in this case
}

// Null destructor
ioProc::~ioProc() { }


bool ioProc::getPacket(int p, int& pause, ipa_t& srcIp, ipp_t& srcPort) {
// Read an input packet from stdin and return it in p.
// The first field on the line is a pause value that represents the
// number of microseconds to leave before returning the packet.
// This is followed by the source IP address and port number.
// The pause, source IP address and port number are returned
// in the corresponding arguments.
// True is returned on success, false on failure.
        int i, leng, vnet, srcAdr, destAdr, x;
	string typStr;

        misc::skipBlank(cin);
        if (!misc::getNum(cin,pause) ||
	    !misc::getIpAdr(cin,srcIp) || !misc::verify(cin,':') ||
            !misc::getNum(cin,srcPort) ||
            !misc::getNum(cin,leng) ||
            !misc::getWord(cin,typStr) ||
            !misc::getNum(cin,vnet) ||
            !misc::getNum(cin,srcAdr) ||
            !misc::getNum(cin,destAdr)) {
                return false;
        }

        ps->setLeng(p,(uint16_t) leng);
	if (typStr == "data") ps->setPtyp(p,DATA);
	else if (typStr == "subscribe") ps->setPtyp(p,SUBSCRIBE);
	else if (typStr == "unsubscribe") ps->setPtyp(p,UNSUBSCRIBE);
	else return false;
        ps->setVnet(p,(vnet_t) vnet);
        ps->setSrcAdr(p,srcAdr);
        ps->setDstAdr(p,destAdr);
        ps->pack(p);

        uint32_t* bp4 = (uint32_t*) ps->buffer(p);
        for (int i = 4; i < (leng+3)/4; i++) {
                if (misc::getNum(cin,x)) {
                        bp4[i] = htonl(x);
                } else {
                        bp4[i] = 0;
                }
        }
        misc::cflush(cin,'\n');
        return true;
}

bool ioProc::init() {
// Initialize IO by reading the packet specification from stdin.
// Packets are stored in pStore and a list of the packet numbers
// is kept in the pktScript data structure.
        int p; int pause; ipa_t srcIp; ipp_t srcPort;
        // read packets from stdin into packetStore
        nPkts = 0;
        while (nPkts < maxPkts) {
                p = ps->alloc();
                if (p == Null) fatal("ioProc::init: too many packets");
                if (!getPacket(p,pause,srcIp,srcPort)) break;
                pktScript[nPkts].delay = pause;
                pktScript[nPkts].srcIp = srcIp;
                pktScript[nPkts].srcPort = srcPort;
                pktScript[nPkts].p = p;
		nPkts++;
        }
        if (nPkts == 0) return false;
	cPkt = 0;
	return true;
}

int ioProc::receive() { 
// Return next waiting packet or Null if there is none. 
	static bool first = true;
	static struct timeval curTime, prevTime;
	int diff;

	// check that enough time has passed since last packet
	if (first) {
		if (gettimeofday(&prevTime,Null) < 0)
			fatal("ioProc:: gettimeofday failure");
		first = false;
	}
	if (gettimeofday(&curTime,Null) < 0)
		fatal("ioProc:: gettimeofday failure");
	diff = curTime.tv_usec - prevTime.tv_usec;
	if (diff < 0) diff += 1000000;
	if (diff < pktScript[cPkt].delay) return Null;
	prevTime.tv_usec += pktScript[cPkt].delay;
	if (prevTime.tv_usec >= 1000000) {
		prevTime.tv_usec -= 1000000; prevTime.tv_sec += 1;
	}
		
	if (cPkt >= nPkts) {
		return Null;	// to go through script once
		// cPkt = 0;	// to repeat script
	} 
	// return next packet from script
	int p = ps->clone(pktScript[cPkt].p);
	int lnk = lt->lookup(pktScript[cPkt].srcIp, pktScript[cPkt].srcPort);
	if (lnk == 0) { // no matching link
		// so, look for a place-holder with 0 for peerPort
		// this allows hosts to use ephemeral ports
		lnk = lt->lookup(pktScript[cPkt].srcIp, 0);
		if (lnk == 0) { ps->free(p); return Null; }
		lt->setPeerPort(lnk,pktScript[cPkt].srcPort);
	}
	ps->setIoBytes(p,ps->leng(p));
	ps->setInLink(p,lnk);
	cPkt++;
	return p;
}

void ioProc::send(int p, int lnk) {
// Just recycle storage.
	ps->free(p);
}
