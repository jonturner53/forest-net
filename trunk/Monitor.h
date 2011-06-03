/** @file Monitor.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef MONITOR_H
#define MONITOR_H

#include "CommonDefs.h"
#include "PacketHeader.h"
#include "PacketStore.h"
#include "UiHashTbl.h"
#include "Avatar.h"

/** Monitor observes status reports sent by Avatars in a virtual
 *  world and reports them to a remote client that implements
 *  a graphical display of the moving Avatars.
 */
class Monitor {
public:
		Monitor(ipa_t, ipa_t , ipa_t, fAdr_t, fAdr_t);
		~Monitor();

	bool	init(char*);		///< open and setup socket
	void	run(int); 		///< run avatar
private:
	const static short MON_PORT = 30124;///< port# for connection to GUI
	const static int NUMITEMS = 9;///<number of distinct items in a status packet
	const static int SIZE = 1000000;///< xy extent of virtual world
	const static int GRID = 200000;	///< xy extent of one grid square

	const static int UPDATE_PERIOD = 50;	///< # ms between status updates
	const static int MAX_AVATARS = 1000;	///< max # of avatars to monitor
	
	ipa_t	extIp, intIp;		///< IP addresses for remote GUI and Forest net
	ipa_t	rtrIp;			///< IP address of router
	fAdr_t	myAdr;			///< forest address of host
	fAdr_t	rtrAdr;			///< forest address of router

	int	intSock;		///< internal socket number
	int	extSock;		///< external listening socket
	int	connSock;		///< external connection socket

	comt_t	comt;			///< comtree number

	bool	logging;		///< true if logging requested
	ofstream logFile;		///< output stream for optional log file

	const static int MAX_REPORTS = 40; ///< max # of reports per status pkt
	uint32_t *statPkt;		///< pointer to buffer for status packet
	int	repCnt;			///< number of reports in status packet

	// avatar properties
	struct avatarData {
	fAdr_t	adr;		///< forest address of avatar
	int	ts;		///< timestamp of latest update
	int	x;		///< x coordinate in virtual world
	int	y;		///< y coordinate in virtual world
	int	dir;		///< direction avatar is facing in degrees
	int	speed;		///< speed moving in UNITS/sec
	int	numVisible;	///< number of visible avatars + number of nearby avatars
	int	numNear;	///< number of nearby avatars
	comt_t	comt;		///< comtree that avatar appears in
	} *avData;		

	int nextAvatar;			///< index of next row in avData table
	UiHashTbl *watchedAvatars;	///< set of (avatarAddr, row#) pairs

	PacketStore *ps;		///< pointer to packet store

	// private helper methods
	int	groupNum(int, int);

	void	connect();		
	void	disconnect();	

	void 	check4comtree(); 	
	void	send2gui();	

	int 	receiveReport();
	void	send2router(int);

	void	updateStatus(int,int);
	void	updateSubscriptions(comt_t,comt_t);

	void	writeStatus(int);
};

#endif
