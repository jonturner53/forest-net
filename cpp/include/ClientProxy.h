#ifndef CLIPROXY_H
#define CLIPROXY_H

#include "Forest.h"
#include "PacketHeader.h"
#include "PacketStore.h"
class ClientProxy {
public:
	ClientProxy(ipa_t);
	~ClientProxy();

	bool init(ipa_t);
	void run(uint32_t);

	int recvFromForest();
	int recvFromAvatar();
	void send2cli(int);
	void send(int);	
private:
	static const int LISTEN_PORT = 30140;
	static const int UPDATE_PERIOD = 50;
	static const int npkts = 1000;
	PacketStore *ps;
	ipa_t myIpAdr;
	ipa_t rtrIp;
	fAdr_t rtrAdr;
	ipa_t avIp;
	ipp_t avPort;
	int sock;
	int extSock;
	int avaSock;
};
#endif
