/** @file Avatar.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef AVATAR_H
#define AVATAR_H

#include <set>
#include <list>
#include <bitset>
#include "CommonDefs.h"
#include "PacketHeader.h"
#include "PacketStore.h"
#include "HashSet.h"

/** Class that implements a simulated avatar in a virtual world.
 *  
 *  This class implements an avatar in a very simple virtual world.
 *  The world is just a square area that is divided into a set of
 *  grid squares. The avatar wanders randomly around the square,
 *  issuing status reports periodically, indicating where it is,
 *  the direction in which it's moving and its velocity.
 *  The status reports are sent on a multicast group associated
 *  with the the grid square it's currently in. The Avatar also
 *  subscribes to multicasts for all squares within its
 *  "visibility range".
 */ 
class Avatar {
public:
		Avatar(ipa_t, comt_t, comt_t);
		~Avatar();

	bool	init(ipa_t, string&, string&, char*);
	void	run(uint32_t);
	const static int STATUS_REPORT = 1; ///< status report payload code
private:
	const static int UPDATE_PERIOD = 50;  ///< # ms between status updates
	const static int CLIMGR_PORT = 30140; ///< port number for client mgr
	const static int CONTROLLER_PORT = 30130;///< port for remote controller
	const static int NUM_ITEMS = 10;///< # of items in status report
	const static int GRID = 10000;	///< xy extent of one grid square
	const static int MAXNEAR = 1000;///< max # of nearby avatars
	// speeds below are the amount avatar moves during an update period
	const static int SLOW  =100;	///< slow avatar speed
	const static int MEDIUM=250;	///< medium avatar speed
	const static int FAST  =600;	///< fast avatar speed

	// network parameters 
	ipa_t	myIpAdr;		///< IP address of interface
	ipp_t	port;			///< port to connect to Controller on
	uint16_t myPort;		///< port number for all io
	ipa_t	rtrIpAdr;		///< IP address of router
	fAdr_t	myAdr;			///< forest address of host
	fAdr_t	rtrAdr;			///< forest address of router
	fAdr_t	comtCtlAdr;		///< forest address of ComtreeController
	int	sock;			///< socket number for forest network
	int	extSock;		///< listen socket for remote driver
	int	connSock;		///< connect socket for driver

	// comtrees
	comt_t	comt;			///< current comtree
	comt_t	firstComt;		///< lower range of comtrees
	comt_t	lastComt;		///< upper range of comtrees

	// avatar properties
	int	x;			///< x coordinate in virtual world
	int	y;			///< y coordinate in virtual world
	double	direction;		///< direction avatar is facing in deg
	double	deltaDir;		///< change in direction per period
	double	speed;			///< speed moving in UNITS/update period

	// data structures defining walls in virtual world
	int	worldSize;	   	///< # of grid squares per dimension
	int*	walls; 			///< array of walls
	set<int> *visSet;		///< visSet[g] contains multicast groups
					///< for squares visible from g's square

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
	int	seqNum;			///< sequence number of control packet

	// data structures for multicast subscriptions and other avatars
	set<int> *mySubs;		///< multicast group subscriptions
	HashSet *visibleAvatars;	///< set of visible avatars
	HashSet *nearAvatars;		///< set of nearby avatars
					///< (those we receive packets from)
	int numVisible;			///< number of visible avatars
	int numNear;			///< number of nearby avatars

	PacketStore *ps;		///< pointer to packet store

	// private helper methods 
	bool	login(ipa_t,string,string); 	
	bool	setupWalls(const char*);
	int	groupNum(int, int);
	bool	isVis(int, int);
	bool	linesIntersect( double, double, double, double,
				double, double, double, double);

	void	updateStatus(uint32_t);
	void	updateNearby(int);
	void	updateSubs();	
	void	sendStatus(int);	
	
	void	startComtSwitch(comt_t, uint32_t);
	bool	completeComtSwitch(packet, uint32_t);
	void	send2comtCtl(CpTypeIndex,bool=false);
	comt_t	check4command();	
	void	forwardReport(uint32_t,int,int=0);	

	void	subscribeAll();		
	void	unsubscribeAll();		
	void	subscribe(list<int>&);
	void	unsubscribe(list<int>&);

	void	connect();		
	void	disconnect();		
	void	send(int);		
	int	receive();		
};

#endif
