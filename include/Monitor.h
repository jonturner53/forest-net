/** @file Monitor.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <set>
#include <list>
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
		Monitor(ipa_t, ipa_t , ipa_t, fAdr_t, fAdr_t, int);
		~Monitor();

	bool	init();			///< open and setup socket
	void	run(int); 		///< run avatar
private:
	const static short MON_PORT = 30124;///< port# for connection to GUI
	const static int NUMITEMS = 9;	///< # distinct items in status packet
	const static int GRID = 10000;	///< xy extent of one grid square
	const static int MAX_AVATARS = 1000; ///< max # of avatars to monitor
	const static int MAX_WORLD = 30000; ///< max extent of virtual world
	const static int MAX_VIEW = 1000; ///< max extent of view
	const static int UPDATE_PERIOD = 50;    ///< # ms between status updates
	int worldSize;			///< # of squares in x and y directions
	
	ipa_t	extIp;			///< local IP used for remote GUI
	ipa_t	intIp;			///< local IP used for Forest
	ipa_t	rtrIp;			///< IP address of router
	fAdr_t	myAdr;			///< forest address of host
	fAdr_t	rtrAdr;			///< forest address of router

	int	intSock;		///< internal socket number
	int	extSock;		///< external listening socket
	int	connSock;		///< external connection socket
	int	cornerX;		///< lower-left corner of current view
	int	cornerY;
	int	viewSize;		///< size of current view
	comt_t	comt;			///< current comtree number

	set<int> *mySubs;		///< current multicast subscriptions

	PacketStore *ps;		///< pointer to packet store

	// private helper methods
	int	groupNum(int, int);

	void	connect();		
	void	disconnect();	

	void 	check4command(); 	

	int 	receiveReport();
	void	forwardReport(int,int);

	void	send2router(int);

	void	switchComtrees(int);
	void	updateSubs();
	void	subscribeAll();
	void	unsubscribeAll();
	void	subscribe(list<int>&);
	void	unsubscribe(list<int>&);
};

#endif
