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
		Avatar(ipa_t, ipa_t, fAdr_t, fAdr_t, comt_t);
		~Avatar();

	bool	init();			
	void	run(int); 	
	const static int STATUS_REPORT = 1; ///< status report payload
private:
	const static int SIZE = 1000000;   ///< xy extent of virtual world
	const static int GRID = 200000;	   ///< xy extent of one grid square
	const static int VISRANGE = 60000; ///< how far avatar can see

	const static int UNIT = 1;	///< basic length unit
	const static int SLOW = 8000;	///< slow avatar speed in UNITS/sec
	const static int MEDIUM =25000;	///< medium avatar speed
	const static int FAST = 80000;	///< fast avatar speed

	const static int UPDATE_PERIOD = 50;  ///< # ms between status updates
	
	// network parameters 
	ipa_t	myIpAdr;		///< IP address of interface
	uint16_t myPort;		///< port number for all io
	ipa_t	rtrIpAdr;		///< IP address of router
	fAdr_t	myAdr;			///< forest address of host
	fAdr_t	rtrAdr;			///< forest address of router
	int	sock;			///< socket number
	comt_t	comt;			///< comtree number

	// avatar properties
	int	x;			///< x coordinate in virtual world
	int	y;			///< y coordinate in virtual world
	double	direction;		///< direction avatar is facing in deg
	double	deltaDir;		///< change in direction per period
	double	speed;			///< speed moving in UNITS/sec

	// info on groups and nearby avatars 
	const static int MAXGROUPS = (SIZE/GRID)*(SIZE/GRID);
					///< maximum # of multicast groups
	const static int MAXNEAR = 1000;///< max # of nearby avatars
	UiDlist	*mcGroups;		///< multicast groups subscribed to
	int numNear;			///< number of nearby avatars
	int nextAv;			///< next available avatar index
	UiHashTbl *nearAvatars;		///< set of visible avatars

	PacketStore *ps;		///< pointer to packet store

	// private helper methods 
	int	groupNum(int, int);	///< return group num for given position

	void	updateStatus(int);	///< update avatar position, etc
	void	updateSubscriptions();	///< send multicast subscription packets
	void	updateNearby(int);	///< update set of nearby avatars

	void	sendStatus(int);	///< send status report
	void	connect();		///< send connect packet
	void	disconnect();		///< send disconnect packet
	void	send(int);		///< send packet
	int	receive();		///< return next input packet
};

#endif
