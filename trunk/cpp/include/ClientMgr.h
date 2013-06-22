#ifndef CLIENTMGR_H
#define CLIENTMGR_H

#include "Forest.h"
#include "Packet.h"
#include "CtlPkt.h"
#include "Substrate.h"
#include "PacketStoreTs.h"
#include "ClientTable.h"
#include "NetBuffer.h"
#include "Queue.h"
#include "CpHandler.h"
//#include "IdMap.h"
//#include "UiSetPair.h"
#include <pthread.h> 
#include <map>

namespace forest {

fAdr_t myAdr;		///< Forest address of self
ipa_t myIp;		///< address of interface to use
fAdr_t rtrAdr;		///< Forest address of router
ipa_t rtrIp;		///< IP address for forest router
ipp_t rtrPort;		///< port number for forest router
ipa_t nmIp;		///< IP address of NetMgr
fAdr_t nmAdr;		///< Forest address of NetMgr
char *dummyRecord;	///< dummy record for padding client file
int maxRecord;		///< largest record number in client file

PacketStoreTs *ps;	///< pointer to packet store

Substrate *sub;		///< substrate for handling packets
ClientTable *cliTbl;	///< data about clients and active sessions
Logger *logger;		///< error message logger

ofstream acctFile;	///< stream for writing accounting records
fstream clientFile;	///< stream for reading/updating client data
pthread_mutex_t clientFileLock; ///< so only one thread can update at a time

/** Used to identify the type of an accounting record. */
enum acctRecType { UNDEF, NEWSESSION, CONNECT_REC, DISCONNECT_REC };

static int const RECORD_SIZE = 256; ///< size of a client file record

bool init(ipa_t,ipa_t);
bool bootMe(ipa_t, ipa_t, fAdr_t&, fAdr_t&, fAdr_t&, ipa_t&, ipp_t&, uint64_t&);

// handler tasks
void* handler(void *);
bool handleClient(int,CpHandler&);
bool handleConnDisc(pktx,CtlPkt&,CpHandler&);

// methods that implement requests from remote clients
bool login(NetBuffer&,string&,string&);
bool newAccount(NetBuffer&,string&,string&);
bool newSession(int, CpHandler&, NetBuffer&, string&, string&);
void getProfile(NetBuffer&, string&, string&);
void updateProfile(NetBuffer&, string&, string&);
void changePassword(NetBuffer&, string&, string&);
void uploadPhoto(int, NetBuffer&, string&, string&);
void getSessions(NetBuffer&, string&, string&);
void cancelSession(NetBuffer&, string&, string&);
void addComtree(NetBuffer&, string&, string&);

void writeRecord(int);
void writeAcctRecord(const string&, fAdr_t, ipa_t, fAdr_t, acctRecType); 

} // ends namespace

#endif
