/** @file ClientAvatar.h 
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
		Avatar(ipa_t, ipa_t, comt_t, comt_t, ipp_t);
		~Avatar();

	bool	init();
	void	setup(const char*);
	void	run(int);
	void	login(string,string,bool); ///< send login string and get fAddress back
	const static int STATUS_REPORT = 1; ///< status report payload
private:
	const static int GRID = 10000;	///< xy extent of one grid square
	int SIZE;		   	///< xy extent of virtual world
	int* WALLS; 			///< list of walls
	// speeds below are the amount avatar moves during an update period
	const static int SLOW  =100;	///< slow avatar speed
	const static int MEDIUM=250;	///< medium avatar speed
	const static int FAST  =600;	///< fast avatar speed

	const static int UPDATE_PERIOD = 50;  ///< # ms between status updates
	const static int CLIMGR_PORT = 30140; ///< port number for CliMgr.cpp
	const static int CONTROLLER_PORT = 30130;///< port for remote controller
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
	int gridSize;			///< # of grid squares vertically or horizontally
	comt_t	comt;			///< current comtree
	comt_t	comt1;			///< lower range of comtrees
	comt_t	comt2;			///< upper range of comtrees

	// avatar properties
	int	x;			///< x coordinate in virtual world
	int	y;			///< y coordinate in virtual world
	double	direction;		///< direction avatar is facing in deg
	double	deltaDir;		///< change in direction per period
	double	speed;			///< speed moving in UNITS/update period

	uint32_t * statPkt;		///< packet to be sent to Gui
					///< maximum # of multicast groups
	const static int MAXNEAR = 1000;///< max # of nearby avatars
	UiDlist	*mcGroups;		///< multicast groups subscribed to
	int numVisible;			///< number of visible avatars
	int numNear;			///< number of avatars in multicast groups
	int stableNumNear;		///< number of nearby avatars reported
	int stableNumVisible;		///< number of visible avatare reported
	int nextAv;			///< next available avatar index
	bool ** visibility;		///< maps visibility from one region to another
	UiHashTbl *visibleAvatars;	///< set of visible avatars
	UiHashTbl *nearAvatars;		///< set of nearby avatars (receiving multicasts)

	PacketStore *ps;		///< pointer to packet store

	// private helper methods 
	int	groupNum(int, int);	///< return group num for given position
	bool	isVis(int, int);	///< return if region2 is visible from region1
	bool	linesIntersect(double,double,double,double,double,double,double,double); ///< tells if line segments intersect
	void	updateStatus(int);	///< update avatar position, etc
	void	updateSubscriptions();	///< send multicast subscription packets
	void	updateNearby(int);	///< update set of nearby avatars
	
	void	unsubAll();		///< unsubscribe from everything
	void	setupWalls(const char*);///< read walls from wallfile
	void	switchComtree(int);	///< switch this avatar to this comtree
	void	sendCtlPkt2CC(bool,int);///< tell ComtreeController that Avatar is switching comtrees
	void	send2Controller();	///< send statPkt to Gui
	void	check4input(int);	///< check for remote controller input
	void	updateStatus(int,int);	///< update avatar position based on remote input
	void	sendStatus(int);	///< send status report
	void	connect();		///< send connect packet
	void	disconnect();		///< send disconnect packet
	void	send(int);		///< send packet
	int	receive();		///< return next input packet
};

#endif
