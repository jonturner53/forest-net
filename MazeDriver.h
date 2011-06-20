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
		Avatar(ipa_t, ipa_t, ipa_t, fAdr_t, fAdr_t, comt_t,int,char*);
		~Avatar();

	bool	init();			
	void	run(int); 	
	const static int STATUS_REPORT = 1; ///< status report payload
private:
	const static short AV_PORT = 30130; ///< port with which to connect to controler
	int SIZE;	 		  ///< xy extent of virtual world
	const static int GRID = 200000;	 ///< xy extent of one grid square
	char* WALLS;			///< list of walls
	bitset<10000> wallsSet;		///< bitset of the walls
	const static int VISRANGE = 60000; ///< how far avatar can see
	const static int NUMITEMS = 10;///<number of distinct items in a status packet
	const static int UNIT = 1;	///< basic length unit
	const static int SLOW = 8000;	///< slow avatar speed in UNITS/sec
	const static int MEDIUM =25000;	///< medium avatar speed
	const static int FAST = 80000;	///< fast avatar speed

	const static int UPDATE_PERIOD = 50;  ///< # ms between status updates
	
	// network parameters
	ipa_t   extIpAdr;		///< IP address of the controller 
	ipa_t	intIp;
	uint16_t myPort;		///< port number for all io
	ipa_t	rtrIpAdr;		///< IP address of router
	fAdr_t	myAdr;			///< forest address of host
	fAdr_t	rtrAdr;			///< forest address of router
	int	intSock;		///< internal socket number
	int 	extSock;		///< external socket number
	int	connSock;
	comt_t	comt;			///< comtree number
	
	const static int MAX_REPORTS = 1; ///< max # of reports per status pkt
	uint32_t *statPkt;		///< pointer to buffer for status packet
	int	repCnt;			///< number of reports in status packet

	// avatar properties
	int	x;			///< x coordinate in virtual world
	int	y;			///< y coordinate in virtual world
	double	direction;		///< direction avatar is facing in deg
	double	deltaDir;		///< change in direction per period
	double	speed;			///< speed moving in UNITS/sec

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
	
	void	updateStatus(int, int);	///< update avatar position remotely, etc
	void	updateStatus(int);		///< update avatar position randomly, etc
	void	updateSubscriptions();	///< send multicast subscription packets
	void	updateNearby(int);	///< update set of nearby avatars
	void	check4input(int);	///< read input from controller
	bool    isVis(int, int);        ///< return if region2 is visible from region1
        bool    linesIntersect(double,double,double,double,double,double,double,double); ///< tells if line segments intersect

	
	void	updateStatus2cont(int,int); 	///< send buffered status reports to controller
	void	sendStatus(int);		///< send status report
	void	connect();		///< send connect packet
	void	disconnect();		///< send disconnect packet
	void	send(int);		///< send packet
	void	send2cont();		///< send packet to controller
	int	receive();		///< return next input packet
};

#endif
