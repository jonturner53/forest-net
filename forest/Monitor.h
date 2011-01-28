// Header file for Monitor class, which emulates an avatar
// moving randomly in a very simple virtual world.

#ifndef MONITOR_H
#define MONITOR_H

#include "forest.h"
#include "header.h"
#include "pktStore.h"
#include "dlist.h"
#include "hashTbl.h"
#include "Avatar.h"

class Monitor {
public:
		Monitor(ipa_t, ipa_t, fAdr_t, fAdr_t, comt_t);
		~Monitor();

	bool	init();			// open and setup socket
	void	run(int); 		// run avatar
	const static int STATUS_REPORT = 1;// identifies status report payload
private:
	const static int SIZE = 1000000;// xy extent of virtual world
	const static int GRID = 200000;	// xy extent of one grid square

	const static int UPDATE_PERIOD = 50;	// # ms between status updates
	const static int MAX_AVATARS = 1000;	// max # of avatars to monitor
	
	ipa_t	myIpAdr;		// IP address of interface
	uint16_t myPort;		// port number for all io
	ipa_t	rtrIpAdr;		// IP address of router
	fAdr_t	myAdr;			// forest address of host
	fAdr_t	rtrAdr;			// forest address of router
	int	sock;			// socket number
	sockaddr_in sa, dsa;		// socket address structures
	comt_t	comt;			// comtree number

	// avatar properties
	struct avatarData {
	fAdr_t	adr;		// forest address of avatar
	int	ts;		// timestamp of latest update
	int	x, y;		// position in virtual world
	int	dir;		// direction avatar is facing in degrees
	int	speed;		// speed moving in UNITS/sec
	int	numNear;	// number of nearby avatars
	} *avData;		

	int nextAvatar;			// index of next row in avData table
	hashTbl *watchedAvatars;	// set of (avatarAddr, row#) pairs

	pktStore *ps;			// pointer to packet store

	// private helper methods
	int	receive();		// return next input packet
	void	send(int);		// send packet
	int	getTime();		// get free-running time value (in us)
	void	updateStatus(int);	// process a status report
	int	groupNum(int, int);	// return group num for given position
	void	subscribeAll();		// subscribe to multicast connections
	void	connect();		// send connect packet
	void	disconnect();		// send disconnect packet
	void	printStatus(int);		// print periodic log
};

#endif
