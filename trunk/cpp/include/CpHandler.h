/** @file CpHandler.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef CPHANDLER_H
#define CPHANDLER_H

#include "Forest.h"
#include "Logger.h"
#include "CtlPkt.h"
#include "RateSpec.h"
#include "Queue.h"
#include "PacketStoreTs.h"

/** This class is designed for use with multi-threaded controllers
 *  (NetMgr, ComtCtl, etc) that use control packets to communicate
 *  with routers and other components. It handles retransmission
 *  of control packet requests, on behalf of the thread using it.
 *  Each thread instaniates and initializes its own CpHandler.
 *  The threads use a pair of queues to communicate with a
 *  "main thread" that handles IO and routing of packets to
 *  the appropriate thread.
 */
class CpHandler {
public:
	CpHandler(Queue* q1, Queue* q2, fAdr_t myAdr1, Logger* log,
		  PacketStoreTs *ps1) : inq(q1), outq(q2), myAdr(myAdr1),
		  logger(log), ps(ps1), tunIp(0), tunPort(0) {};

	static const int NORESPONSE = (1 << 31);

	bool getCp(pktx,CtlPkt&);

	pktx clientAddComtree(fAdr_t,int);
	pktx clientDropComtree(fAdr_t,comt_t);
	pktx clientJoinComtree(fAdr_t,comt_t,ipa_t, ipp_t);
	pktx clientLeaveComtree(fAdr_t,comt_t,ipa_t, ipp_t);

	pktx addIface(fAdr_t,int,ipa_t,RateSpec&);
	pktx dropIface(fAdr_t,int);
	pktx modIface(fAdr_t,int,ipa_t,RateSpec&);
	pktx getIface(fAdr_t,int);

	pktx addLink(fAdr_t,ntyp_t,ipa_t,ipp_t,int,int,fAdr_t);
	pktx dropLink(fAdr_t,int);
	pktx modLink(fAdr_t,int,RateSpec&);
	pktx getLink(fAdr_t,int);

	pktx addComtree(fAdr_t,comt_t);
	pktx dropComtree(fAdr_t,comt_t);
	pktx modComtree(fAdr_t,comt_t,int,int);
	pktx getComtree(fAdr_t,comt_t);

	pktx addComtreeLink(fAdr_t,comt_t,int,int=-1);
	pktx addComtreeLink(fAdr_t,comt_t,ipa_t,ipp_t,int=-1);
	pktx dropComtreeLink(fAdr_t,comt_t,int);
	pktx dropComtreeLink(fAdr_t,comt_t,ipa_t,ipp_t);
	pktx modComtreeLink(fAdr_t,comt_t,int,RateSpec&);
	pktx getComtreeLink(fAdr_t,comt_t,int);

	pktx newClient(fAdr_t,ipa_t,ipp_t);
	pktx clientConnect(fAdr_t,fAdr_t,fAdr_t);
	pktx clientDisconnect(fAdr_t,fAdr_t,fAdr_t);

	pktx bootRequest(fAdr_t);
	pktx bootReply(fAdr_t,fAdr_t,fAdr_t);
	pktx bootComplete(fAdr_t);
	pktx bootAbort(fAdr_t);

	int sendCtlPkt(CtlPkt&, fAdr_t);
	bool handleReply(pktx, CtlPkt&, string&, string&);
	void errReply(pktx, CtlPkt&, const char*);

	void setTunnel(ipa_t, ipp_t);

private:
	Queue* inq;		///< from main thread
	Queue* outq;		///< going back to main thread
	fAdr_t myAdr;		///< address of this host
	ipa_t tunIp;		///< ip address for tunnel
	ipp_t tunPort;		///< port number for tunnel
	Logger* logger;		///< for reporting error messages
	PacketStoreTs* ps;	///< thread-safe packet store

	int sendAndWait(pktx, CtlPkt&);
};

inline void CpHandler::setTunnel(ipa_t ip, ipp_t port) {
	tunIp = ip; tunPort = port;
}

#endif
