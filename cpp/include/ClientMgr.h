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

// handler tasks
void* handler(void *);
bool handleClient(int,CpHandler&);
bool handleConnDisc(pktx,CtlPkt&,CpHandler&);

// dialogs with client
int loginDialog(int,NetBuffer&);
void adminDialog(int,CpHandler&,int,NetBuffer&);
void userDialog(int,CpHandler&,int,NetBuffer&);

// operations that can be performed by normal clients
bool newSession(int, CpHandler&, int, NetBuffer&, string&);
void getProfile(int, NetBuffer&, string&);
void updateProfile(int, NetBuffer&, string&);
void changePassword(int, NetBuffer&, string&);
void addComtree(int, NetBuffer&, string&);

// privileged operations
void addClient(NetBuffer&, string&);
void removeClient(NetBuffer&, string&);
void modPassword(NetBuffer&, ClientTable::privileges, string&);
void modRealName(NetBuffer&, string&);
void modEmail(NetBuffer&, string&);
void modDefRates(NetBuffer&, string&);
void modTotalRates(NetBuffer&, string&);
void showClient(NetBuffer&, string&);

void writeAcctRecord(const string&, fAdr_t, ipa_t, fAdr_t, acctRecType); 

} // ends namespace

#endif
