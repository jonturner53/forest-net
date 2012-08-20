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
#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>

void* run(void *);		///< main run method, takes in finTime
bool init(fAdr_t,ipa_t,fAdr_t,fAdr_t,ipa_t,ipa_t,fAdr_t,char*,char*); ///< initialize sockets
ipa_t intIp;		///< address for tcp connections from "internal" hosts
ipa_t extIp;		///< address for tcp connections from "external" hosts
ipa_t rtrIp;		///< IP address for forest router
fAdr_t netMgrAdr;	///< Forest address of NetMgr
fAdr_t rtrAdr;		///< Forest address of router
fAdr_t myAdr;		///< Forest address of self
fAdr_t CC_Adr;		///< Forest address of ComtreeController

const static int LISTEN_PORT = 30140; ///< TCP port to listen for avatars on
static const int TPSIZE = 500;
static const int NORESPONSE = (1 << 31);

mysqlpp::Connection * sqlconn; //< pointer to mysql connection
sql_create_3(user_pass,1,3,
  mysqlpp::sql_int, id,
  mysqlpp::sql_varchar, user,
  mysqlpp::sql_varchar, pass)

PacketStoreTs *ps;	///< pointer to packet store
int sock;		///< Forest socket
int tcpSockInt;		///< Listen for avatars on internal ip
int tcpSockExt;		///< Listen for avatars on external ip
int avaSock;		///< Avatar connection socket

pthread_mutex_t sqlLock;

uint64_t seqNum;
map<string,string>* unames; ///< map of known usernames to pwords

int numPrefixes;
struct prefixInfo {
	string prefix;
	fAdr_t rtrAdr;
	ipa_t rtrIp;
};
prefixInfo prefixes[1000];
struct clientStruct {
	fAdr_t fa;//forest address
	fAdr_t ra;//router forest address
	ipa_t rip;//router IP
	string uname;
	string pword;
	int sock;//socket number
	ipa_t aip;//avatar IP
};
int proxyIndex;
map<fAdr_t,Queue*>* proxyQueues;
struct proxyStruct {
	ipa_t pip; //proxy ip
	ipp_t udpPort;//proxy port
	ipp_t tcpPort;//tcp listen port
};

proxyStruct proxies[1000];
struct QueuePair {
	Queue in; Queue out;
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
bool readPrefixInfo(char*); ///< read prefix info from prefix file
bool findCliRtr(ipa_t,fAdr_t&,ipa_t&); ///< gives rtrAdr and rtrIp from prefix

void* handler(void *);
void handleIncoming(int); //deal with incoming packets by sending to appropriate threads
void* acceptAvatars(void *);
int sendAndWait(int,CtlPkt&,Queue&,Queue&);
int sendCtlPkt(CtlPkt&,comt_t,fAdr_t,Queue&,Queue&);
void send(int);		///< send packet to forest router
void requestAvaInfo(ipa_t,ipp_t);///< send ctlpkt asking for client info
int recvFromForest(string&);	///< receive packet from Forest
void connect();		///< connect to forest network
void disconnect();	///< disconnect from forest network
void readUsernames();	///< read filenames and store
void initializeAvatar();///< try to connect to and initialize ClientAvatar
bool checkUser(string&,string&); ///< check if username/pword pair is in sql db
#endif
