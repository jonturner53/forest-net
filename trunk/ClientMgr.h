#ifndef CLIENTMGR_H
#define CLIENTMGR_H

#include "CommonDefs.h"
#include "PacketHeader.h"
#include "PacketStore.h"
#include <map>
class ClientMgr {
public:
	ClientMgr(fAdr_t,ipa_t,fAdr_t,fAdr_t,ipa_t,fAdr_t,char*,char*); ///< Constructor
	~ClientMgr();		///< Destructor
	bool init();		///< initialize sockets
	void run(int);		///< main run method, takes in finTime
	ipa_t myIp;		///< IP address for self
	ipa_t rtrIp;		///< IP address for forest router
	fAdr_t netMgrAdr;	///< Forest address of NetMgr
	fAdr_t rtrAdr;		///< Forest address of router
	fAdr_t myAdr;		///< Forest address of self
	fAdr_t CC_Adr;		///< Forest address of ComtreeController
	const static int LISTEN_PORT = 30140; ///< TCP port to listen for avatars on
	PacketStore *ps;	///< pointer to packet store
	int sock;		///< Forest socket
	int extSock;		///< Listen for avatars socket
	int avaSock;		///< Avatar connection socket
	uint64_t lowLvlSeqNum;
	uint64_t highLvlSeqNum;
	map<string,string>* unames; ///< map of known usernames to pwords
	struct clientStruct {
		fAdr_t fa;//forest address
		fAdr_t ra;//router forest address
		fAdr_t rip;//router IP
		string uname;
		string pword;
	};
	map<uint32_t,clientStruct>* clients; ///< map from IP to clients with info
	char * unamesFile;	///< filename of usernames file
	ofstream acctFileStream; ///< stream to write accounting to file
private:
	void writeToAcctFile(CtlPkt); ///< write contents of ctlpkt to accounting file
	void send(int);		///< send packet to forest router
	bool requestAvaInfo(ipa_t,ipp_t);///< send ctlpkt asking for client info
	int recvFromForest();	///< receive packet from Forest
	void connect();		///< connect to forest network
	void disconnect();	///< disconnect from forest network
	void readUsernames();	///< read filenames and store
	bool initializeAvatar();///< try to connect to and initialize ClientAvatar
};
#endif
