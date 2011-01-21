// Header file for Avatar class, which emulates an avatar
// moving randomly in a very simple virtual world.

#ifndef AVATAR_H
#define AVATAR_H

#include "forest.h"
#include "header.h"
#include "pktStore.h"
#include "dlist.h"
#include "hashTbl.h"

class Avatar {
public:
		Avatar(ipa_t, ipa_t, fAdr_t, fAdr_t, comt_t);
		~Avatar();

	bool	init();			// open and setup socket
	void	run(int); 		// run avatar
	const static int STATUS_REPORT = 1;// identifies status report payload
private:
	const static int SIZE = 1000000;// xy extent of virtual world
	const static int GRID = 100000;	// xy extent of one grid square
	const static int VISRANGE = 60000; // how far avatar can see

	const static int UNIT = 1;	// basic length unit
	const static int SLOW = 4000;	// slow avatar speed in UNITS/sec
	const static int MEDIUM =12000;	// medium avatar speed
	const static int FAST = 40000;	// fast avatar speed

	const static int UPDATE_PERIOD = 50;	// # ms between status updates
	
	ipa_t	myIpAdr;		// IP address of interface
	uint16_t myPort;		// port number for all io
	ipa_t	rtrIpAdr;		// IP address of router
	fAdr_t	myAdr;			// forest address of host
	fAdr_t	rtrAdr;			// forest address of router
	int	sock;			// socket number
	sockaddr_in sa, dsa;		// socket address structures
	comt_t	comt;			// comtree number

	// avatar properties
	int	x, y;			// position in virtual world
	double	direction;		// direction avatar is facing in degrees
	double	deltaDir;		// change in direction per period
	double	speed;			// speed moving in UNITS/sec

	// info on groups and nearby avatars
	const static int MAXGROUPS = (SIZE/GRID)*(SIZE/GRID);
					// maximum # of multicast groups
	const static int MAXNEAR = 200;	// max # of nearby avatars
	dlist	*mcGroups;		// multicast groups subscribed to
	int numNear;			// number of nearby avatars
	hashTbl *nearAvatars;		// set of visible avatars

	pktStore *ps;			// pointer to packet store

	// private helper methods
	int	receive();		// return next input packet
	void	send(int);		// send packet
	int	getTime();		// get free-running time value (in us)
	void	updateStatus(int);	// update avatar position, etc
	int	groupNum(int, int);	// return group num for given position
	void	updateSubscriptions();	// send multicast subscription packets
	void	updateNearby(int);	// update set of nearby avatars
	void	sendStatus(int);	// send status report
	void	connect();		// send connect packet
	void	disconnect();		// send disconnect packet
};

#endif
