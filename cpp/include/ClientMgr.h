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

PacketStoreTs *ps;	///< pointer to packet store

Substrate *sub;		///< substrate for handling packets
ClientTable *cliTbl;	///< data about clients and active sessions
Logger *logger;		///< error message logger

ofstream acctFile;	///< stream for writing accounting records

/** Used to identify the type of an accounting record. */
enum acctRecType { UNDEF, NEWSESSION, CONNECT_REC, DISCONNECT_REC };

bool init(ipa_t,ipa_t);
bool bootMe(ipa_t, ipa_t, fAdr_t&, fAdr_t&, fAdr_t&, ipa_t&, ipp_t&, uint64_t&);

void* handler(void *);
bool handleClient(int,CpHandler&);
int handleLogin(int,NetBuffer&,string&);
void handleAdmin(int,NetBuffer&);
bool handleConnDisc(pktx,CtlPkt&,CpHandler&);

bool readRates(NetBuffer&,RateSpec&);

void addClient(int, NetBuffer&);
void removeClient(int, NetBuffer&);
void modPassword(int, NetBuffer&);
void modRealName(int, NetBuffer&);
void modEmail(int, NetBuffer&);
void modDefRates(int, NetBuffer&);
void modTotalRates(int, NetBuffer&);
void showClient(int, NetBuffer&);

void writeAcctRecord(const string&, fAdr_t, ipa_t, fAdr_t, acctRecType); 

} // ends namespace

#endif
