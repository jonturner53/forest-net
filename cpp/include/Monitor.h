/** @file Monitor.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <set>
#include <list>
#include "Forest.h"
#include "Packet.h"
#include "CtlPkt.h"
#include "NetBuffer.h"
#include "PacketStore.h"
#include "UiHashTbl.h"
#include "Avatar.h"

namespace forest {


/** Monitor observes status reports sent by Avatars in a virtual
 *  world and reports them to a remote client that implements
 *  a graphical display of the moving Avatars.
 */
class Monitor {
public:
		Monitor(ipa_t, ipa_t , int);
		~Monitor();

	bool	init(const string&, const string&); ///< open and setup socket
	void	run(uint32_t); 		///< run avatar
private:
	const static short MON_PORT = 30124;///< port# for connection to GUI
	const static int NUMITEMS = 9;	///< # distinct items in status packet
	const static int GRID = 10000;	///< xy extent of one grid square
	const static int MAX_AVATARS = 1000; ///< max # of avatars to monitor
	const static int MAX_WORLD = 30000; ///< max extent of virtual world
	const static int MAX_VIEW = 1000; ///< max extent of view
	const static int UPDATE_PERIOD = 50;    ///< # ms between status updates
	int worldSize;			///< # of squares in x and y directions
	
	fAdr_t	myAdr;			///< forest address of host
	ipa_t	myIp;			///< IP address to bind to sockets
	fAdr_t	rtrAdr;			///< forest address of router
	ipa_t	rtrIp;			///< IP address of router
	ipp_t	rtrPort;		///< port number of router
	ipa_t	cmIp;			///< IP address of client manager
	fAdr_t	ccAdr;			///< address of ComtCtl

	uint64_t nonce;			///< used when connecting

	int	dgSock;			///< datagram socket number
	int	listenSock;		///< listening socket for remote GUI
	int	connSock;		///< connection socket

	int	cornerX;		///< lower-left corner of current view
	int	cornerY;
	int	viewSize;		///< size of current view


	set<int> *mySubs;		///< current multicast subscriptions

	PacketStore *ps;		///< pointer to packet store

	// data for managing signalling interactions with ComtCtl
	const static uint32_t SWITCH_TIMEOUT = 500000; ///< 500 ms timeout
	const static bool RETRY = true; ///< flag used to signal retry
	enum STATE {
		IDLE, JOINING, LEAVING 
	};				///< state of signaling interaction
	STATE	switchState;		///< current state
	uint32_t switchTimer;		///< time last signalling packet sent
	int	switchCnt;		///< number of attempts so far
	comt_t	nextComt;		///< comtree we're switching to
	comt_t	comt;			///< current comtree number
	int	seqNum;			///< sequence number of control packet

	// private helper methods
	bool	login(const string&, const string&);
	int	groupNum(int, int);

	bool	connect();		
	bool	disconnect();	

	comt_t 	check4command(); 	

	void	sendToRouter(int);
	int 	receiveFromRouter();
	void	forwardReport(int,int);

	void	startComtSwitch(comt_t, uint32_t);
	bool	completeComtSwitch(pktx, uint32_t);
	void	send2comtCtl(CtlPkt::CpType,bool=false);

	void	switchComtrees(int);
	void	updateSubs();
	void	subscribeAll();
	void	unsubscribeAll();
	void	subscribe(list<int>&);
	void	unsubscribe(list<int>&);
};

} // ends namespace

#endif
