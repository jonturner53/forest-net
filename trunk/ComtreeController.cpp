/** @file ComtreeController.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ComtreeController.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "CommonDefs.h"
#include "stdinc.h"
#include "support/UiHashTbl.h"
/** usagstring lnk = temp.at(j+3);:
 * 
 *  Command line arguments include two ip addresses for the
 *  ComtreeController. The first is the IP address that a remote display can
 *  use to connect to the ComtreeController. If this is specified as 127.0.0.1
 *  the ComtreeController listens on the default IP address. The second is the
 *  IP address used by the ComtreeController within the Forest overlay. RtrIp
 *  is the router's IP address, myAdr is the ComtreeController's Forest
 *  address, rtrAdr is the Forest address of the router.
 *
 *  Topology is the name of a file that defines the backbone topology of the
 *  forest network and the topologies of one or more comtrees.
 */

int repCnt, finTime, NUMITEMS;
uint32_t* statPkt;

int main(int argc, char *argv[]) {
        string path;
	ipa_t   extIp;                  ///< IP address for remote UI
        ipa_t   intIp;                  ///< IP address for Forest net
        ipa_t   rtrIp;                  ///< IP address of router
        fAdr_t  myAdr;                  ///< forest address of this host
        fAdr_t  rtrAdr;  	

	if(argc ==  8){	
		extIp  = Np4d::ipAddress(argv[1]);
        	intIp  = Np4d::ipAddress(argv[2]);
        	rtrIp  = Np4d::ipAddress(argv[3]);
        	myAdr  = Forest::forestAdr(argv[4]);
        	rtrAdr = Forest::forestAdr(argv[5]);
		path = argv[7];
		sscanf(argv[6],"%d", &finTime);
	}else
		fatal("ComtreeController extIp intIp rtrIp myAdr rtrAdr topology finTime");
	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress();
        if (extIp == 0) fatal("can't retrieve default IP address");
	
	ComtreeController* cc=new ComtreeController(extIp, intIp, rtrIp, myAdr, rtrAdr);
	if(!cc->init())
		fatal("Init Failure");	
	cc->parse(path);
	cc->run(finTime*1000000);
	exit(0);
	delete cc;
}

// Constructor for ComtreeController, allocates space and initializes private data
ComtreeController::ComtreeController(ipa_t eIp, ipa_t iIp, ipa_t rIp, fAdr_t myA, fAdr_t frIp){
	extIp = eIp;                  ///< IP address for remote UI
        intIp = iIp;                  ///< IP address for Forest net
        rtrIp = rIp;                  ///< IP address of router
        myAdr = myA;                  ///< forest address of this host
        rtrAdr= frIp;  
	NUMITEMS=4;
	repCnt = 0;
	statPkt= new uint32_t[500];
	topology = new vector< vector<string> >();
	int nPkts = 10000;
        ps = new PacketStore(nPkts+1, nPkts+1);
	connSock=-1;
}

ComtreeController::~ComtreeController() {delete statPkt; delete topology; delete ps; }

/** Initialize sockets.
 *  @return true on success, false on failure
 */
bool ComtreeController::init() {
	intSock = Np4d::datagramSocket();
	if (intSock < 0 || 
	    !Np4d::bind4d(intSock,intIp,0) ||
	    !Np4d::nonblock(intSock))
		return false;
	connect(); 		// send initial connect packet
	usleep(1000000);	// 1 second delay provided for use in SPP
				// delay gives SPP linecard the time it needs
				// to setup NAT filters before second packet

	// setup external TCP socket for use by remote GUI
	extSock = Np4d::streamSocket();
	if (extSock < 0) return false;

return 	Np4d::bind4d(extSock,extIp, NM_PORT) &&
	Np4d::listen4d(extSock) &&
	Np4d::nonblock(extSock);
}

/**Parse the Topology file.
*/
void ComtreeController::parse(string filename){
	string line;
	string whitespaces (" \t\n");
	vector<string> builder;
	ifstream ifs (filename.c_str());
	bool wasOpened = 0; 
	if(ifs.is_open())
	{
		wasOpened = 1;
		while (ifs.good()){
			getline(ifs, line);
			string str;
			size_t found;
			//remove # comments
			if(!line.empty()){
				found=line.find_first_of('#');
				if(found!=string::npos && found != 0)
					str = line.substr(0, found);
				else if(found==string::npos)
					str = line;
				else
					str ="";
			}
			size_t a;
			size_t b;
			if(!str.empty()){
				a = str.find_first_not_of(whitespaces);
				b = str.find_last_not_of(whitespaces);
				if(a!=string::npos && b!=string::npos)
					str = str.substr(a, b+1);
				if(str.compare(".")==0){
					topology->push_back(builder);
					builder.clear();
				}else{
					size_t head=0;
					size_t tail=0;
					while(tail < str.size()){
						head = str.find_first_not_of(whitespaces, tail);
						tail = str.find_first_of(whitespaces, head);
						if(tail != string::npos)
							builder.push_back(str.substr(head, tail-head));
						else
							builder.push_back(str.substr(head));
					}	
				}
			}
		}		
	}
	ifs.close();
	if(wasOpened == 0)
		fatal("topology.txt not found");
}

/** Run the ComtreeController.
 */
void ComtreeController::run(int finishTime) {

	// check for control packets from Forest side
	// and process them

	// check for requests from remote display
	// and process them
        
	uint32_t now = Misc::getTime();       // free-running microsecond time
	now = Misc::getTime();
	string nums = "1234567890";
	UiHashTbl cr_tbl(100);
	string fAdr[1000];
	int counter[1000];
	int i;
        for(i = 0; i < topology->size(); i++){
		if(topology->at(i).at(0).compare("nodes:")==0){
			int n;
			for(n = 1; n < topology->at(i).size(); n+=5){
				//map zipcodes of all routers to their forest addresses
				string rname = topology->at(i).at(n);
				if(rname.find_last_of(nums) == rname.length())
					rname = rname.substr(rname.find_first_of(nums));
				else
					rname = rname.substr(rname.find_first_of(nums), rname.find_last_of(nums));
				int zip=atoi(rname.c_str());
				fAdr[zip]=topology->at(i).at(n+2);
			}
		}
		if(topology->at(i).at(0).compare("comtrees:")==0){
			vector<string> temp = topology->at(i);
			vector<string> zips;
			int comtree;
			int nextCounter = 1;
			int j;
			for(j = 1; j < temp.size(); j+=4){
				comtree = atoi(temp.at(j).c_str());
				string root = temp.at(j+1);
				size_t head=0;
                                size_t tail=0;
                                        
				head = root.find_first_of(nums);
                                zips.push_back(root.substr(head).c_str());
				//get core routers
				head = 0;
				tail = 0;
				string core = temp.at(j+2);
				
				//get linked routers
				string lnk = temp.at(j+3);
				head = 0;
				size_t period = 0;
				size_t comma = 0;
				tail = 0;
				while(head != string::npos){
					if(comma < tail)
						head = lnk.find_first_of(nums, comma);
					else
						head = lnk.find_first_of(nums, tail);
					if(head != string::npos && period != string::npos || tail == lnk.find_last_of(":", period)){
                                                 string s=lnk.substr(head, period-head+1);
                                                 int zip = atoi(s.c_str());
                                                 uint64_t num = (uint64_t(comtree) << 32) | (uint64_t(zip) & 0xffffffff);
                                                 if(cr_tbl.lookup(num) == 0){
                                                         cr_tbl.insert(num,nextCounter);
                                                         counter[nextCounter++] = 0;
                                   		}
                                        }
					period = lnk.find_first_of(".", head);
					comma = lnk.find_first_of(",", period);
					tail = lnk.find_first_of(":", period);
				}
				zips.clear();
			}
        	}
	}
	
	while(now <= finishTime){
		now = Misc::getTime();	
		int p = rcvFromForest();
		if(p != 0){
			PacketHeader& h = ps->getHeader(p);
			int zipcode = Forest::zipCode(h.getSrcAdr());
			CtlPkt cp;
			int payload_ln = h.getLength()-(Forest::HDR_LENG+sizeof(uint32_t));
			cp.unpack(ps->getPayload(p), payload_ln);
			int comtree = 0;
			//deconstruct comtree/router key
			if(CLIENT_JOIN_COMTREE == cp.getCpType() || 
			    CLIENT_LEAVE_COMTREE == cp.getCpType()){
				fAdr_t avaAdr = h.getSrcAdr();
				comtree = cp.getAttr(COMTREE_NUM);
				CtlPkt cp1, cp2;
       				cp1.setCpType(cp.getCpType());
        			cp1.setRrType(POS_REPLY);
        			cp1.setSeqNum(cp.getSeqNum());

				//send positive reply packet back to the client
				returnToSender(p,cp1.pack(ps->getPayload(p)));	
					
				cp2.setCpType(cp.getCpType());
				cp2.setRrType(REQUEST);
				cp2.setAttr(COMTREE_NUM,comtree);
				cp2.setAttr(PEER_ADR,avaAdr);
				if(CLIENT_JOIN_COMTREE == cp.getCpType())
					cp2.setCpType(ADD_COMTREE_LINK);
				else
					cp2.setCpType(DROP_COMTREE_LINK);
				int len = cp2.pack(ps->getPayload(p));
				h.setDstAdr(Forest::forestAdr(fAdr[zipcode].c_str()));
				h.setSrcAdr(myAdr); h.setLength(Forest::OVERHEAD + len);
				h.setPtype(NET_SIG); h.setComtree(100);
		
				//send join or drop comtree packet to router of the client
				sendToForest(p);
		
				if(comtree!= 0){	
					uint64_t key = (uint64_t(comtree) << 32) | (uint64_t(zipcode) & 0xffffffff);
					int index = cr_tbl.lookup(key);
					if(index != 0){
						if(CLIENT_JOIN_COMTREE == cp.getCpType()) {
							counter[index]++;
						}
						else if(CLIENT_LEAVE_COMTREE == cp.getCpType() && counter[index] > 0) {
        						counter[index]--;
						}
						// add new report to the outgoing status packet
        					statPkt[0] = htonl(comtree); //comtree num 
        					statPkt[1] = htonl(zipcode); //router
						statPkt[2] = htonl(counter[index]); //num clients on router
						statPkt[3] = htonl(now);
						if(connSock >= 0)
							writeToDisplay();
						else
							connect2display();
					}
				}
			}
		}
	}
	disconnect();           // send final disconnect packet
}

/** Check for next message from the remote UI.
 *  @return a packet number with a formatted control packet on success,
 *  0 on failure
 */
void ComtreeController::connect2display() { 
	if (connSock < 0) {
		connSock = Np4d::accept4d(extSock);
		if (connSock < 0) return;
		if(!Np4d::nonblock(connSock)) return;

	}
}

/** Write a packet to the socket for the user interface.
 */
void ComtreeController::writeToDisplay() {
	int nbytes = NUMITEMS*sizeof(uint32_t);
        char* p = (char *) statPkt;
        while (nbytes > 0) {
                int n = write(connSock, (void *) p, nbytes);
                //cerr<<"nbytes: "<<nbytes<<" n written: "<<n<<endl;
		if (n < 0) fatal("Monitor::send2gui: failure in write");
                p += n; nbytes -= n;
        }
}

/** Check for next packet from the Forest network.
 *  @return next report packet or 0, if no report has been received.
 */
int ComtreeController::rcvFromForest() { 
	int p = ps->alloc();
	if (p == 0) return 0;
        buffer_t& b = ps->getBuffer(p);
        int nbytes = Np4d::recv4d(intSock,&b[0],1500);
        if (nbytes < 0) { ps->free(p); return 0; }
	//cerr<<"nbytes: "<<nbytes<<endl;
	ps->unpack(p);

	PacketHeader& h = ps->getHeader(p);
	CtlPkt cp;
	cp.unpack(ps->getPayload(p), h.getLength() - (Forest::HDR_LENG + 4));

        return p;
}

/** Send packet to Forest router.
 */
void ComtreeController::sendToForest(packet p) {
	PacketHeader& h = ps->getHeader(p);
	buffer_t& buf = ps->getBuffer(p);
	int leng = ps->getHeader(p).getLength();
	ps->pack(p);
	int rv = Np4d::sendto4d(intSock,(void *) &buf,leng,
		    	      	rtrIp,Forest::ROUTER_PORT);
	if (rv == -1) fatal("ComtreeController::sendToForest: failure in sendto");

	/* add later
	save packet in re-send heap with timer set to
	retransmission time
	*/
}

/** Send packet back to sender.
 *  Update the length, flip the addresses and pack the buffer.
 *  @param p is the packet number
 *  @param paylen is the length of the payload in bytes
 */
void ComtreeController::returnToSender(packet p, int paylen) {
        PacketHeader& h = ps->getHeader(p);
        h.setLength(Forest::HDR_LENG + paylen + sizeof(uint32_t));
	
	buffer_t& buf = ps->getBuffer(p);
	int leng = ps->getHeader(p).getLength();
        
	fAdr_t temp = h.getDstAdr();
        h.setDstAdr(h.getSrcAdr());
        h.setSrcAdr(temp);

        ps->pack(p);
	
	int rv = Np4d::sendto4d(intSock,(void *) &buf,leng, rtrIp,Forest::ROUTER_PORT);
	if (rv == -1) fatal("ComtreeController::sendToForest: failure in sendto");
}

/** Send initial connect packet to forest router
 *  Uses comtree 1, which is for user signalling.
 */
void ComtreeController::connect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);
	h.setLength(4*(5+1)); h.setPtype(CONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);
	sendToForest(p);
}

/** Send final disconnect packet to forest router.
 */
void ComtreeController::disconnect() {
	packet p = ps->alloc();
	PacketHeader& h = ps->getHeader(p);

	h.setLength(4*(5+1)); h.setPtype(DISCONNECT); h.setFlags(0);
	h.setComtree(1); h.setSrcAdr(myAdr); h.setDstAdr(rtrAdr);

	sendToForest(p);
}
