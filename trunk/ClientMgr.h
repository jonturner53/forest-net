#ifndef CLIENTMGR_H
#define CLIENTMGR_H

#include "CommonDefs.h"
#include "PacketHeader.h"
#include "PacketStoreTs.h"
#include "Queue.h"
#include "IdMap.h"
#include "UiSetPair.h"
#include <pthread.h> 
#include <map>

void* run(void *);		///< main run method, takes in finTime
bool init(fAdr_t,ipa_t,fAdr_t,fAdr_t,ipa_t,fAdr_t,char*,char*);		///< initialize sockets
ipa_t myIp;		///< IP address for self
ipa_t rtrIp;		///< IP address for forest router
fAdr_t netMgrAdr;	///< Forest address of NetMgr
fAdr_t rtrAdr;		///< Forest address of router
fAdr_t myAdr;		///< Forest address of self
fAdr_t CC_Adr;		///< Forest address of ComtreeController
const static int LISTEN_PORT = 30140; ///< TCP port to listen for avatars on
static const int TPSIZE = 500;
PacketStoreTs *ps;	///< pointer to packet store
int sock;		///< Forest socket
int extSock;		///< Listen for avatars socket
int avaSock;		///< Avatar connection socket
uint64_t seqNum;
map<string,string>* unames; ///< map of known usernames to pwords
struct clientStruct {
	fAdr_t fa;//forest address
	fAdr_t ra;//router forest address
	ipa_t rip;//router IP
	string uname;
	string pword;
	int sock;//socket number
	ipa_t aip;//avatar IP
};
struct QueuePair {
Queue in;
Queue out;
};
int threadCount;
struct ThreadPool {
pthread_t th;
QueuePair qp;
uint64_t seqNum;
uint64_t ts;
ipa_t ipa;
int sock;
};
ThreadPool *pool;
UiSetPair *threads;
IdMap *tMap;
map<uint32_t,clientStruct>* clients; ///< map from IP to clients with info
char * unamesFile;	///< filename of usernames file
ofstream acctFileStream; ///< stream to write accounting to file
void writeToAcctFile(CtlPkt); ///< write contents of ctlpkt to accounting file
void* handler(void *);
void handleIncoming(int); //deal with incoming packets by sending to appropriate threads
void* acceptAvatars(void *);
int sendAndWait(int,CtlPkt&,Queue&,Queue&);
int sendCtlPkt(CtlPkt&,comt_t,fAdr_t,Queue&,Queue&);
void send(int);		///< send packet to forest router
void requestAvaInfo(ipa_t,ipp_t);///< send ctlpkt asking for client info
int recvFromForest();	///< receive packet from Forest
void connect();		///< connect to forest network
void disconnect();	///< disconnect from forest network
void readUsernames();	///< read filenames and store
void initializeAvatar();///< try to connect to and initialize ClientAvatar
;
#endif
