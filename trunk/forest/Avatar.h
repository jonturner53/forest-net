// Header file for Avatar class, which emulates an avatar
// moving randomly in a very simple virtual world.

#ifndef AVATAR_H
#define AVATAR_H

#include "forest.h"
#include "header.h"
#include "pktStore.h"

class Avatar {
public:
		Avatar(ipa_t, ipa_t, fa_t, fa_t, comt_t);
		~Avatar();

	bool	init();			// open and setup socket
	int	receive();		// return next input packet
	void	send(int);		// send packet
	void	run(bool,uint32_t,uint32_t); // run avatar
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

	// avatar properties
	int	x, y;			// position in virtual world
	int	direction;		// direction avatar is facing in degrees
	int	speed;			// speed moving in UNITS/sec

	// info on groups and nearby avatars
	const static int MAXGROUPS = 25; // maximum # of multicast groups
	const static int MAXNEAR = 200;	// max # of nearby avatars
	hashTbl	mcGroups(MAXGROUPS)	// multicast groups subscribed to
	hashTbl nearAvatars(MAXNEAR)	// set of visible avatars

	pktStore *ps;			// pointer to packet store
};

#endif
