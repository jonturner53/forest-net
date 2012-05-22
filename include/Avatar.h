/** @file Avatar.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef AVATAR_H
#define AVATAR_H

#include "CommonDefs.h"
#include "PacketHeader.h"
#include "PacketStore.h"
#include "UiDlist.h"
#include "UiHashTbl.h"
#include <bitset>
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
		Avatar(ipa_t, ipa_t, comt_t, comt_t);
		~Avatar();

	bool	init();
	void	setup(const char*);
	void	run(int);
	void	login(string,string); 	///< send login string and get forest
					///< address, and router info back
	const static int STATUS_REPORT = 1; ///< status report payload code
private:
	const static int UPDATE_PERIOD = 50;  ///< # ms between status updates
	const static int CLIMGR_PORT = 30140; ///< port number for client mgr
	const static int CONTROLLER_PORT = 30130;///< port for remote controller
	const static int GRID = 10000;	///< xy extent of one grid square
	int	SIZE;		   	///< xy extent of virtual world
	int*	WALLS; 			///< array of walls
	int 	gridSize;		///< # of grid squares vertically 
					///< or horizontally
	// speeds below are the amount avatar moves during an update period
	const static int SLOW  =100;	///< slow avatar speed
	const static int MEDIUM=250;	///< medium avatar speed
	const static int FAST  =600;	///< fast avatar speed

	// network parameters 
	ipa_t	myIpAdr;		///< IP address of interface
	ipa_t	cliMgrIpAdr;		///< IP address of the Client Manager
	ipp_t	port;			///< port to connect to Controller on
	uint16_t myPort;		///< port number for all io
	ipa_t	rtrIpAdr;		///< IP address of router
	fAdr_t	myAdr;			///< forest address of host
	fAdr_t	rtrAdr;			///< forest address of router
	fAdr_t	CC_Adr;			///< forest address of ComtreeController
	int	sock;			///< socket number for forest network
	int	CM_sock;		///< socket number for the ClientMgr
	int	Controller_sock;	///< listen socket for remote Controller
	int	controllerConnSock;	///< connect socket for Controllers

	// comtrees
	comt_t	comt;			///< current comtree
	comt_t	comt1;			///< lower range of comtrees
	comt_t	comt2;			///< upper range of comtrees

	// avatar properties
	int	x;			///< x coordinate in virtual world
	int	y;			///< y coordinate in virtual world
	double	direction;		///< direction avatar is facing in deg
	double	deltaDir;		///< change in direction per period
	double	speed;			///< speed moving in UNITS/update period

	UiDlist	*mcGroups;		///< multicast groups subscribed to
	bool ** visibility;		///< visibility[i][j]=true if region i
					///< can see region j
	const static int MAXNEAR = 1000;///< max # of nearby avatars
	UiHashTbl *visibleAvatars;	///< set of visible avatars
	UiHashTbl *nearAvatars;		///< set of nearby avatars
					///< (those we receive packets from)
	int numVisible;			///< number of visible avatars
	int numNear;			///< number of nearby avatars
	int stableNumNear;		///< number of nearby avatars reported
	int stableNumVisible;		///< number of visible avatars reported

	int	seqNum;			///< sequence number for control packets

	PacketStore *ps;		///< pointer to packet store

	// private helper methods 
	void	setupWalls(const char*);
	int	groupNum(int, int);
	bool	isVis(int, int);
	bool	linesIntersect( double, double, double, double,
				double, double, double, double);
	void	updateStatus(uint32_t,int);
	void	updateSubscriptions();	
	void	updateNearby(int);
	
	void	unsubAll();		
	void	switchComtree(int);	
	void	sendCtlPkt2CC(bool,int);
	void	send2Controller(uint32_t,int,int=0);	
	int	check4input();	
	void	sendStatus(int);	

	void	connect();		
	void	disconnect();		
	void	send(int);		
	int	receive();		
};

#endif
