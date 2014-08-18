/** @file NetMgr.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef NETMGR_H
#define NETMGR_H

#include <thread>
#include <mutex>
#include <chrono>

#include "Forest.h"
#include "Packet.h"
#include "PacketStore.h"
#include "NetInfo.h"
#include "ComtInfo.h"
#include "ListPair.h"
#include "HashSet.h"
#include "Queue.h"
#include "Substrate.h"
#include "NetBuffer.h"
#include "Logger.h"

using std::thread;
using std::mutex;
using namespace chrono;

#ifdef DB_MODE
	#include "DBConnector.h"
#else
	#include "AdminTable.h"
#endif

namespace forest {


/** NetMgr manages a Forest network.
 *  It relays control packets from a remote Console and
 *  provides some autonomous control capabilities. 
 *  
 *  The NetMgr class defines some common data (defined as static) that is
 *  shared by a pool of worker threads and a large collection of methods.
 *  A separate NetMgr object is instantiated for each worker thread,
 *  allowing for per thread data in addition to the shared data.
 *  The NetMgr class is derived from the Controller class that
 *  serves as the interface between the worker threads
 *  and a separate Substrate thread. The Controller class defines a
 *  per-object input queue used to pass packet indexes fromt the Substrate.
 *  It also defines a shared output queue and a per-object thread index.
 *  
 *  The Substrate thread handles network IO and dispatches packets
 *  to/from the worker threads.
 */
class NetMgr : public Controller {
public:
		NetMgr();
		~NetMgr();

	static bool init(const char*, const char*, int);
	static void cleanup();
	static bool runAll();

private:
	static const int numThreads = 100; ///< number of worker threads
	static ipa_t myIp;		///< IP address of this host
	static fAdr_t myAdr;		///< forest address of this host
	static fAdr_t rtrAdr;		///< forest address of router
	static fAdr_t cliMgrAdr;	///< forest address of the client mgr
	static fAdr_t comtCtlAdr;	///< forest address of the comtree ctl
	static int netMgr;		///< node # of net manager in NetInfo
	static int nmRtr;		///< node # of net manager's router

	static Logger *logger;		///< error message logger
	static PacketStore *ps;		///< pointer to packet store
	
	static AdminTable *admTbl;   	///< data about administrators
	
	static NetInfo *net;		///< global view of net topology
	static ComtInfo *comtrees;	///< pre-configured comtrees
	
	static Substrate *sub;		///< substrate for routing control pkts

	static NetMgr *tpool;		///< thread pool
	
	// Information relating client addresses and router addresses
	// This is a temporary expedient and will be replaced later
	static const int maxPrefixes = 1000;
	static int numPrefixes;
	struct prefixInfo {
		string prefix;
		fAdr_t rtrAdr;
	};
	struct clientInfo {
		ipa_t cliIp;
		fAdr_t rtrAdr;
		fAdr_t cliAdr;
		ipa_t rtrIp;
	};
	static prefixInfo prefixes[maxPrefixes+1];
	
#ifdef DB_MODE
	static DBConnector *dbConn; //DB Connector
#else
	static char *dummyRecord;      ///< dummy record for padding admin file
	static int maxRecord;          ///< largest record number in admin file
	
	static fstream adminFile;	///< for reading/updating admin data
	static mutex adminFileLock;	///< only 1 thread can update at a time
#endif
	
	
	static int const RECORD_SIZE = 256; ///< size of an admin file record

	bool	static readPrefixInfo(const char*);

	bool	run();

	pktx	sendRequest(pktx, int, fAdr_t, pktx, const string&);
	pktx	sendRequest(pktx, int, fAdr_t);
	void	sendReply(pktx, int, fAdr_t);
	void	errReply(pktx, CtlPkt&, const string&);
	
	void* 	handler(void *);
	bool 	handleConsole(int);
	bool 	handleConDisc(int,CtlPkt&);
	bool 	handleNewSession(int,CtlPkt&);
	bool 	handleCancelSession(int,CtlPkt&);
	bool 	handleBootLeaf(int,CtlPkt&);
	bool 	handleBootRouter(int,CtlPkt&);
	
	static void writeAdminRecord(int);
/* console code - omit for now
	bool	login(NetBuffer&,string&,string&);
	bool	newAccount(NetBuffer&,string&,string&);
	void	getProfile(NetBuffer&, string&, string&);
	void	updateProfile(NetBuffer&, string&, string&);
	void	changePassword(NetBuffer&, string&, string&);
	
	void	getLinkTable(NetBuffer&, string&, CpHandler&);
	void 	getComtreeTable(NetBuffer&, string&, CpHandler&);
	void 	getIfaceTable(NetBuffer&, string&, CpHandler&);
	void 	getRouteTable(NetBuffer&, string&, CpHandler&);
	
	void	addFilter(NetBuffer&, string&, CpHandler&);
	void	modFilter(NetBuffer&, string&, CpHandler&);
	void	dropFilter(NetBuffer&, string&, CpHandler&);
	void	getFilter(NetBuffer&, string&, CpHandler&);
	void	getFilterSet(NetBuffer&, string&, CpHandler&);
	void 	getLoggedPackets(NetBuffer& buf, string& reply, CpHandler& cph);
	void 	enablePacketLog(NetBuffer& buf, string& reply, CpHandler& cph);
*/
	
	uint64_t generateNonce();
	fAdr_t	setupLeaf(int, pktx, CtlPkt&, int, int, uint64_t,bool=false);
	bool	setupEndpoint(int, int, pktx, CtlPkt&, bool=false);
	bool	setupComtree(int, int, pktx, CtlPkt&, bool=false);
	bool	processReply(pktx, CtlPkt&, pktx, CtlPkt&, const string&);
	
	void	sendToCons(int);	
	int	recvFromCons();
	
	bool	findCliRtr(ipa_t,fAdr_t&); ///< gives the rtrAdr of the prefix
};

} // ends namespace


#endif
