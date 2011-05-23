// Header file for Monitor class, which emulates an avatar
// moving randomly in a very simple virtual world.

#ifndef MONITOR_H
#define MONITOR_H

#include "forest.h"
#include "header.h"
#include "pktStore.h"
#include "UiHashTbl.h"
#include "Avatar.h"

class Monitor {
public:
		Monitor(ipa_t, ipa_t , ipa_t, fAdr_t, fAdr_t);
		~Monitor();

	bool	init(char*);		// open and setup socket
	void	run(int); 		// run avatar
private:
	const static short MON_PORT = 30124;// xy extent of virtual world
	const static int SIZE = 1000000;// xy extent of virtual world
	const static int GRID = 200000;	// xy extent of one grid square

	const static int UPDATE_PERIOD = 50;	// # ms between status updates
	const static int MAX_AVATARS = 1000;	// max # of avatars to monitor
	
	ipa_t	extIp, intIp;		// IP addresses for remote GUI and Forest net
	ipa_t	rtrIp;			// IP address of router
	fAdr_t	myAdr;			// forest address of host
	fAdr_t	rtrAdr;			// forest address of router
	int	extSock, intSock;	// socket numbers
	comt_t	comt;			// comtree number
	bool	logging;		// true if logging requested
	ofstream logFile;		// output stream for optional log file

	const static int MAX_REPORTS = 40; // max # of reports per status pkt
	uint32_t *statPkt;		// pointer to buffer for status packet
	int	repCnt;			// number of reports in status packet

	// avatar properties
	struct avatarData {
	fAdr_t	adr;		// forest address of avatar
	int	ts;		// timestamp of latest update
	int	x, y;		// position in virtual world
	int	dir;		// direction avatar is facing in degrees
	int	speed;		// speed moving in UNITS/sec
	int	numNear;	// number of nearby avatars
	comt_t	comt;		// comtree that avatar appears in
	} *avData;		

	int nextAvatar;			// index of next row in avData table
	UiHashTbl *watchedAvatars;	// set of (avatarAddr, row#) pairs

	pktStore *ps;			// pointer to packet store

	// private helper methods
	void 	check4comtree(); 	// check for new comtree # from GUI
	void	send2gui();		// send status packet to remote GUI
	int 	receiveReport(); 	// return report from forest net
	void	send2router(int);	// send packet to forest router
	int	getTime();		// get free-running time value (in us)
	void	updateStatus(int,int);	// process a status report
	int	groupNum(int, int);	// return group num for given position
	void	updateSubscriptions(comt_t,comt_t);// (sub/unsub)scribe to multicasts
	void	connect();		// send connect packet
	void	disconnect();		// send disconnect packet
	void	printStatus(int);	// print periodic log
};

#endif
