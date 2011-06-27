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
/** usage:
 *       ComtreeController extIp intIp rtrIp myAdr rtrAdr topology finTime
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
	cerr<<"arg 2: "<<argv[2]<<endl;
	cerr<<"intIp: ";
	 Np4d::writeIpAdr(cerr, intIp);
	cerr<<endl;
	ComtreeController* cc=new ComtreeController(extIp, intIp, rtrIp, myAdr, rtrAdr);
	cerr<<"cc built"<<endl;
	if(!cc->init())
		fatal("Init Failure");	
	cerr<<"initalized..."<<endl;
	cerr<<path<<endl;
	cc->parse(path);
	cerr<<"parsed..."<<endl;
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
	NUMITEMS=3;
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
	cerr<<"intIp: ";
         Np4d::writeIpAdr(cerr, intIp);
	cerr<<"Socket: "<<intSock<<endl;
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
	cerr<<"extIp: ";
         Np4d::writeIpAdr(cerr, extIp);
        cerr<<"Socket: "<<extSock<<endl;

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
						head = str.find_first_not_of(whitespaces, tail);											tail = str.find_first_of(whitespaces, head);
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
	UiHashTbl cr_tbl(100);
	int counter[1000];
	int i;
        for(i = 0; i < topology->size(); i++){
		if(topology->at(i).at(0).compare("comtrees:")==0){
			vector<string> temp = topology->at(i);
			vector<string> routers;
			int comtree;
			int nextCounter = 1;
			string nums = "123456789";
			int j;
			for(j = 1; j < temp.size(); j+=4){
				comtree = atoi(temp.at(j).c_str());
				string root = temp.at(j+1);
				size_t head=0;
                                size_t tail=0;
                                        
				head = root.find_first_of(nums);
                                routers.push_back(root.substr(head).c_str());
				//get core routers
				head = 0;
				tail = 0;
				string core = temp.at(j+2);
				while(head != string::npos){
					head = core.find_first_of(nums, tail);
					tail = core.find_first_not_of(nums, head);
					if(tail != string::npos)
						routers.push_back(core.substr(head, tail-head).c_str());
					else if(head != string::npos)
						routers.push_back(core.substr(head).c_str());
				}
				//get linked routers
				head = 0;
				tail = 0;
				string lnk = temp.at(j+3);
					vector<string> nodes;
					while(head != string::npos){
                                        	head = lnk.find_first_of(",", tail);
						tail = lnk.find_first_of(",", head+1);
						if(tail == 0)
							nodes.push_back(lnk.substr(0, head));
						if(tail != string::npos)
							nodes.push_back(lnk.substr(head+1, tail));
						else
							nodes.push_back(lnk.substr(head + 1));
					}
					head = 0;
					tail = 0;
					int n;
					for(n = 0; n < nodes.size(); n++){
						string ln = nodes.at(n);
						while(tail != string::npos){
							head = ln.find_first_of(nums, tail);		
							int period = ln.find_first_of(".", head);
							tail = ln.find_first_of(":", head);
                                        		if( head != string::npos)
                                                		routers.push_back(nodes.at(n).substr(head, period-head).c_str());
                               			}	 
					}
			
				int r;
                        	for(r = 0; r < routers.size(); r++){
					int zip = atoi(routers.at(r).c_str());
                                	uint64_t num = (uint64_t(comtree) << 32) | (uint64_t(zip) & 0xffffffff);
					if(cr_tbl.lookup(num) == 0){
						cr_tbl.insert(num,nextCounter);
						counter[nextCounter++] = 0;
					}
				}
				routers.clear();
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
			cp.unpack(ps->getPayload(p), 1500);
			//h.write(cerr, ps->getBuffer(p));
			int comtree = 0;
			//deconstruct comtree/router key
			if(CLIENT_JOIN_COMTREE == cp.getCpType() || 
				CLIENT_LEAVE_COMTREE == cp.getCpType()){
					comtree = cp.getAttr(COMTREE_NUM);
					if(CLIENT_JOIN_COMTREE == cp.getCpType())
						cerr<<"JOIN @ "<<now<<endl;
					else
						cerr<<"LEAVE @ "<<now<<endl;
				if(comtree!= 0){	
					uint64_t key = (uint64_t(comtree) << 32) | (uint64_t(zipcode) & 0xffffffff);
					int index = cr_tbl.lookup(key);
					if(index != 0){
						if(CLIENT_JOIN_COMTREE == cp.getCpType())
							counter[index]++;
						if(CLIENT_LEAVE_COMTREE == cp.getCpType() && counter[index]>0)
        						counter[index]--;
						// add new report to the outgoing status packet
						cerr<<"statpkt: "<<comtree<<" "<<zipcode<<" "<<counter[index]<<endl;	
        					statPkt[0] = htonl(comtree); //comtree num 
        					statPkt[1] = htonl(zipcode); //router
						statPkt[2] = htonl(counter[index]); //num clients on router
						while(readFromDisplay() == 0)
							readFromDisplay();
						writeToDisplay();
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
int ComtreeController::readFromDisplay() { 
	if (connSock < 0) {
		connSock = Np4d::accept4d(extSock);
		if (connSock < 0) return 0;
		/**
		if (!nonblock(connSock))
			fatal("ComtreeController::readFromDisplay: cannot "
			      "configure connection socket to be nonblocking");
		*/
	}
	return 1;
	/*
	uint32_t length;
	
	if (!Np4d::readInt32(connSock, length))
		fatal("ComtreeController::readFromDisplay: cannot read "
		      "packet length from remote display");
	
	int p = ps->alloc();
	if (p == 0) fatal("ComtreeController::readFromDisplay: out of packets");

	PacketHeader& h = ps->getHeader(p);
	buffer_t& b = ps->getBuffer(p);

	int nbytes = read(connSock, (char *) &b, length);
	if (nbytes != length)
		fatal("ComtreeController::readFromDisplay: cannot read "
		      "message from remote display");

	h.setSrcAdr(myAdr);

        return p;
	*/
	
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
void ComtreeController::sendToForest(int p) {
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
