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

namespace forest {


/** This class is designed for use with multi-threaded controllers
 *  (NetMgr, ComtCtl, etc) that use control packets to communicate
 *  with routers and other components. It handles retransmission
 *  of control packet requests, on behalf of the thread using it.
 *  Each thread instantiates and initializes its own CpHandler.
 *  The threads use a pair of queues to communicate with a
 *  "main thread" that handles IO and routing of packets to
 *  the appropriate thread.
 */
class CpHandler {
public:
	CpHandler(Queue* q1, Queue* q2, fAdr_t myAdr1, Logger* log,
		  PacketStoreTs *ps1) : inq(q1), outq(q2), myAdr(myAdr1),
		  tunIp(0), tunPort(0), ps(ps1), logger(log) {}

	static const int NORESPONSE = (1 << 31);

	pktx clientAddComtree(fAdr_t,int,CtlPkt&);
	pktx clientDropComtree(fAdr_t,comt_t,CtlPkt&);
	pktx clientJoinComtree(fAdr_t,comt_t,ipa_t, ipp_t,CtlPkt&);
	pktx clientLeaveComtree(fAdr_t,comt_t,ipa_t, ipp_t,CtlPkt&);

	pktx addIface(fAdr_t,int,ipa_t,RateSpec&,CtlPkt&);
	pktx dropIface(fAdr_t,int,CtlPkt&);
	pktx modIface(fAdr_t,int,ipa_t,RateSpec&,CtlPkt&);
	pktx getIface(fAdr_t,int,CtlPkt&);

	pktx addLink(fAdr_t,Forest::ntyp_t,int,int,ipa_t,ipp_t,fAdr_t,uint64_t,CtlPkt&);
	pktx addLink(fAdr_t,Forest::ntyp_t,int,uint64_t,CtlPkt&);
	pktx dropLink(fAdr_t,int,fAdr_t,CtlPkt&);
	pktx modLink(fAdr_t,int,RateSpec&,CtlPkt&);
	pktx getLink(fAdr_t,int,CtlPkt&);

	pktx addComtree(fAdr_t,comt_t,CtlPkt&);
	pktx dropComtree(fAdr_t,comt_t,CtlPkt&);
	pktx modComtree(fAdr_t,comt_t,int,int,CtlPkt&);
	pktx getComtree(fAdr_t,comt_t,CtlPkt&);

	pktx addComtreeLink(fAdr_t,comt_t,int,int,CtlPkt&);
	pktx addComtreeLink(fAdr_t,comt_t,fAdr_t,CtlPkt&);
	pktx dropComtreeLink(fAdr_t,comt_t,int,fAdr_t,CtlPkt&);
	pktx modComtreeLink(fAdr_t,comt_t,int,RateSpec&,CtlPkt&);
	pktx getComtreeLink(fAdr_t,comt_t,int,CtlPkt&);

	pktx newSession(fAdr_t,ipa_t,RateSpec&,CtlPkt&);
	pktx clientConnect(fAdr_t,fAdr_t,fAdr_t,CtlPkt&);
	pktx clientDisconnect(fAdr_t,fAdr_t,fAdr_t,CtlPkt&);

	pktx setLeafRange(fAdr_t,fAdr_t,fAdr_t,CtlPkt&);
	pktx configLeaf(fAdr_t,fAdr_t,fAdr_t,ipa_t,ipp_t,uint64_t,CtlPkt&);
	pktx bootRouter(fAdr_t,CtlPkt&);
	pktx bootLeaf(fAdr_t,CtlPkt&);
	pktx bootComplete(fAdr_t,CtlPkt&);
	pktx bootAbort(fAdr_t,CtlPkt&);
	
    pktx getLinkSet(fAdr_t,CtlPkt&,int,int); //Feng and Doowon

	int sendRequest(CtlPkt&, fAdr_t, CtlPkt&);
	bool handleReply(pktx, CtlPkt&, string&, string&);
	void sendReply(CtlPkt&, fAdr_t);
	void errReply(pktx, CtlPkt&, const string&);

	void setTunnel(ipa_t, ipp_t);

private:
	Queue* inq;		///< from main thread
	Queue* outq;		///< going back to main thread
	fAdr_t myAdr;		///< address of this host
	ipa_t tunIp;		///< ip address for tunnel
	ipp_t tunPort;		///< port number for tunnel
	PacketStoreTs* ps;	///< thread-safe packet store
	Logger* logger;		///< for reporting error messages

	int sendAndWait(pktx, CtlPkt&);
};

inline void CpHandler::setTunnel(ipa_t ip, ipp_t port) {
	tunIp = ip; tunPort = port;
}

} // ends namespace


#endif
