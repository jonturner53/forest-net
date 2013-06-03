/** @file ClientTable.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ClientTable.h"

namespace forest {


/** Constructor for ClientTable, allocates space and initializes table. */
ClientTable::ClientTable(int maxClients, int maxSessions)
			   : maxCli(maxClients), maxSess(maxSessions) {
	svec = new Session[maxSessions+1];
	sessLists = new UiClist(maxSessions);
	sessMap = new IdMap(maxSess);
	cvec = new Client[maxClients+1];
	clients = new UiSetPair(maxClients);
	nameMap = new map<string, int>();
	for (int i = 0; i <= maxSess; i++) svec[i].clx = 0;
}
	
/** Destructor for ClientTable, frees dynamic storage */
ClientTable::~ClientTable() {
	delete [] svec; delete sessLists; delete sessMap;
	delete [] cvec; delete clients; delete nameMap;
	pthread_mutex_destroy(&mapLock);
}

/** Initialize lock and condition variables.  */
bool ClientTable::init() {
        if (pthread_mutex_init(&mapLock,NULL) != 0) return false;
	for (int clx = 1; clx <= maxCli; clx++) {
		cvec[clx].busyBit = false;
		if (pthread_cond_init(&cvec[clx].busyCond,NULL) != 0)
			return false;
	}
	return true;
}

/** Lock a client's table entry.
 *  @param clx is a client index
 *  @return true if able to a lock on the specified client;
 *  return false if the client index does not refer to a valid client
bool ClientTable::lockClient(int clx) {
	lockMap();
	if (!clients->isIn(clx)) {
		unlockMap(); return 0;
	}
	while (cvec[clx].busyBit) { // wait until client's entry is not busy
		pthread_cond_wait(&cvec[clx].busyCond,&mapLock);
		if (!clients->isIn(clx)) {
			pthread_cond_signal(&cvec[clx].busyCond);
			unlockMap(); return false;
		}
	}
	cvec[clx].busyBit = true; // set busyBit to lock client table entry
	unlockMap();
	return true;
}
 */

/** Get a client by name and lock its table entry.
 *  @param cname is a client name
 *  @return the client index associated with the given client name,
 *  or 0 if the name does not match any client; on a successful
 *  return, the table entry for the client is locked; the caller
 *  must release it when done using it
 */
int ClientTable::getClient(string& cname) {
	lockMap();
	map<string, int>::iterator p = nameMap->find(cname);
	if (p == nameMap->end()) { unlockMap(); return 0; }
	int clx = p->second;
	while (cvec[clx].busyBit) { // wait until client's entry is not busy
		pthread_cond_wait(&cvec[clx].busyCond,&mapLock);
		p = nameMap->find(cname);
		if (p == nameMap->end()) {
			pthread_cond_signal(&cvec[clx].busyCond);
			unlockMap(); return 0;
		}
	}
	cvec[clx].busyBit = true; // set busyBit to lock client table entry
	unlockMap();
	return clx;
}

/** Release a previously locked client table entry.
 *  Waiting threads are signalled to let them proceed.
 *  @param clx is the index of a locked comtree.
 */
void ClientTable::releaseClient(int clx) {
	lockMap();
	cvec[clx].busyBit = false;
	pthread_cond_signal(&cvec[clx].busyCond);
	unlockMap();
}

/** Get a session by its forest address and lock its client's table entry.
 *  @param cliAdr is a forest address
 *  @return the session index associated with the given client address,
 *  or 0 if the name does not match any known session; on a successful
 *  return, the table entry for the session's client is locked; the caller
 *  must release it when done using it
 */
int ClientTable::getSession(fAdr_t cliAdr) {
	lockMap();
	int sess = sessMap->getId(key(cliAdr));
	if (sess == 0) { unlockMap(); return 0; }
	int clx = svec[sess].clx;
	while (cvec[clx].busyBit) { // wait until client's entry is not busy
		pthread_cond_wait(&cvec[clx].busyCond,&mapLock);
		sess = sessMap->getId(key(cliAdr));
		if (sess == 0) {
			pthread_cond_signal(&cvec[clx].busyCond);
			unlockMap(); return 0;
		}
		clx = svec[sess].clx;
	}
	cvec[clx].busyBit = true; // set busyBit to lock client table entry
	unlockMap();
	return sess;
}

/** Get the first client in the list of valid clients.
 *  @return the index of the first valid client, or 0 if there is no
 *  valid client; on a successful return, the client is locked
 */
int ClientTable::firstClient() {
	lockMap();
	int clx = clients->firstIn();
	if (clx == 0) { unlockMap(); return 0; }
	while (cvec[clx].busyBit) {
		pthread_cond_wait(&cvec[clx].busyCond,&mapLock);
		clx = clients->firstIn(); // first client may have changed
		if (clx == 0) {
			pthread_cond_signal(&cvec[clx].busyCond);
			unlockMap(); return 0;
		}
	}
	cvec[clx].busyBit = true;
	unlockMap();
	return clx;
}

/** Get the index of the next client.
 *  The caller is assumed to have a lock on the current client.
 *  @param clx is the index of a valid client
 *  @return the index of the next client in the list of valid clients
 *  or 0 if there is no next client; on return, the lock on clx
 *  is released and the next client is locked.
 */
int ClientTable::nextClient(int clx) {
	lockMap();
	int nuClx = clients->nextIn(clx);
	if (nuClx == 0) {
		cvec[clx].busyBit = false;
		pthread_cond_signal(&cvec[clx].busyCond);
		unlockMap();
		return 0;
	}
	while (cvec[nuClx].busyBit) {
		pthread_cond_wait(&cvec[nuClx].busyCond,&mapLock);
		nuClx = clients->nextIn(clx);
		if (nuClx == 0) {
			cvec[clx].busyBit = false;
			pthread_cond_signal(&cvec[clx].busyCond);
			pthread_cond_signal(&cvec[nuClx].busyCond);
			unlockMap();
			return 0;
		}
	}
	cvec[nuClx].busyBit = true;
	cvec[clx].busyBit = false;
	pthread_cond_signal(&cvec[clx].busyCond);
	unlockMap();
	return nuClx;
}

/** Add a new client.
 *  Attempts to add a new client to the table. Can fail if the specified
 *  user name is already in use, or if there is no more space.
 *  On return, the new client's table entry is locked; the caller must
 *  release it when done.
 *  @param cname is the client name
 *  @param pwd is the client password
 *  @param realName is the client's real world name
 *  @param email is the client's email address
 *  @return the index of the new table entry or 0 on failure;
 *  can fail if cname clashes with an existing name, or there is no space left
 */
int ClientTable::addClient(string& cname, string& pwd, string& realName,
		   string& email, RateSpec& defRates, RateSpec& totalRates) {
	lockMap();
	map<string,int>::iterator p = nameMap->find(cname);
	if (p != nameMap->end()) { unlockMap(); return 0; }
	int clx = clients->firstOut();
	if (clx == 0) { unlockMap(); return 0;}
	nameMap->insert(pair<string,int>(cname,clx));
	clients->swap(clx);
	cvec[clx].busyBit = true;
	unlockMap();

	setClientName(clx,cname); setPassword(clx,pwd);
	setRealName(clx,realName); setEmail(clx,email);
	getDefRates(clx) = defRates;
	getTotalRates(clx) = totalRates;
	getAvailRates(clx) = totalRates;
	cvec[clx].firstSess = cvec[clx].activeCount = 0;

	return clx;
}

/** Remove a client.
 *  Assumes that the calling thread has already locked the client.
 *  The lock is released on return.
 *  @param clx is the index of the client to be deleted
 */
void ClientTable::removeClient(int clx) {
	lockMap();
	nameMap->erase(cvec[clx].cname);
	while (cvec[clx].firstSess != 0)
		removeSession(cvec[clx].firstSess);
	clients->swap(clx);
	cvec[clx].busyBit = false;
	pthread_cond_signal(&cvec[clx].busyCond);
	unlockMap();
}

/** Add a new session.
 *  Attempts to add a new session to an existing client.
 *  @param cliAdr is the forest address assigned to client
 *  @param rtrAdr is the forest address of the router assigned to client
 *  @param clx is the client index of a valid client for which the caller
 *  is assumed to hold a lock
 *  @return the index of the new session or 0 on failure; fails if clx
 *  there is already a session using the specified client address
 */
int ClientTable::addSession(fAdr_t cliAdr, fAdr_t rtrAdr, int clx) {
	lockMap();
	int sess = sessMap->addPair(key(cliAdr));
	if (sess == 0) { unlockMap(); return 0; }
	svec[sess].cliAdr = cliAdr;
	svec[sess].rtrAdr = rtrAdr;
	svec[sess].clx = clx;
	if (cvec[clx].firstSess == 0) {
		cvec[clx].firstSess = sess;
	} else {
		sessLists->join(sess,cvec[clx].firstSess);
	}
	unlockMap();
	
	return sess;
}

/** Remove a session.
 *  Assumes that calling thread holds a lock on the client for this session.
 *  @param sess is the index of the session to be deleted
 */
void ClientTable::removeSession(int sess) {
	lockMap();
	int clx = svec[sess].clx;
	if (clx == 0) { unlockMap(); return; }
	if (cvec[clx].firstSess == sess) {
		if (sessLists->suc(sess) == sess) {
			cvec[clx].firstSess = 0;
		} else {
			cvec[clx].firstSess = sessLists->suc(sess);
			sessLists->remove(sess);
		}
	} else {
		sessLists->remove(sess);
	}
	sessMap->dropPair(key(svec[sess].cliAdr));
	svec[sess].clx = 0; // used to detect unused entries
	unlockMap();
}

/** Read a client entry from an input stream and initialize its table entry.
 *  The entry must be on a line by itself (possibly with a trailing comment).
 *  An entry consists of a client name, password, real name (in quotes),
 *  an email address and a default rate spec.
 *  @param in is an open input stream
 */
bool ClientTable::readEntry(istream& in) {
	string cname, pwd, realName, email;
	RateSpec defRates, totalRates;

	cerr << "reading entry\n";
        Misc::skipBlank(in);
        if (!Misc::readName(in, cname)      || !Misc::verify(in,',') ||
	    !Misc::readWord(in, pwd)        || !Misc::verify(in,',') || 
	    !Misc::readString(in, realName) || !Misc::verify(in,',') ||
	    !Misc::readWord(in, email)      || !Misc::verify(in,',') ||
	    !defRates.read(in)		    || !Misc::verify(in,',') ||
	    !totalRates.read(in)) { 
                return false;
	}
	Misc::cflush(in,'\n');

	int clx = addClient(cname, pwd, realName, email, defRates, totalRates);
	if (clx == 0) return false;
	releaseClient(clx);
        return true;
}

/** Read client table entries from an input stream.
 *  The first line of the input must contain
 *  an integer, giving the number of entries to be read. The input may
 *  include blank lines and comment lines (any text starting with '#').
 *  The operation may fail if the input is misformatted or if
 *  an entry does not pass some basic sanity checks.
 *  In this case, an error message is printed that identifies the
 *  entry on which a problem was detected
 *  @param in is an open input stream
 *  @return true on success, false on failure
 */
bool ClientTable::read(istream& in) {
	int i = 0;
	while (readEntry(in)) i++;
	cout << "read " << i << " client data entries \n";
        return true;
}

/** Construct a string representation of a client.
 *  This metghod does no locking.
 *  @param clx is the client index of the client to be written
 *  @param s is a reference to a string in which result is returned
 *  @param includeSess is true if sessions are to be included, default=false
 *  @return a reference to s
 */
string& ClientTable::client2string(int clx, string& s, bool includeSess) const {
	string s1;
	s  = getClientName(clx) + ", " + getPassword(clx) + ", \"" +
	     getRealName(clx) + "\", " + getEmail(clx) + ", " + 
	     getDefRates(clx).toString(s1) + ", ";
	s += getTotalRates(clx).toString(s1) + "\n";
	if (includeSess) {
		for (int sess = firstSession(clx); sess != 0;
			 sess = nextSession(sess,clx)) {
			s += session2string(sess, s1);
		}
	}
	return s;
}

/** Construct a string representation of a session.
 *  Does no locking.
 *  @param sess is the client index of the session to be written
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& ClientTable::session2string(int sess, string& s) const {
	string s1;
	s  = Forest::fAdr2string(getClientAdr(sess),s1) + ", "; 
	s += Forest::fAdr2string(getRouterAdr(sess),s1) + ", "; 
	s += getSessRates(sess).toString(s1) + ", ";
	time_t t = getStartTime(sess);
	s += ctime(&t);
	s += "\n";
	return s;
}

/** Create a string representation of the client table
 *  Does no locking.
 *  @param s is a reference to a string in which the result is returned
 *  @param includeSess is true if sessions are to be included, default=false
 *  @return a reference to s
 */
string& ClientTable::toString(string& s, bool includeSess) {
	string s1; s = "";
	for (int clx = firstClient(); clx != 0; clx = nextClient(clx))
                s += client2string(clx,s1,includeSess);
	return s;
}

/** Write the complete client table to an output stream.
 *  Does no locking.
 *  @param out is a reference to an open output stream
 *  @param includeSess is true if sessions are to be included, default=false
 */
void ClientTable::write(ostream& out, bool includeSess) {
	string s;
	for (int clx = firstClient(); clx != 0; clx = nextClient(clx))
                out << client2string(clx,s,includeSess);
}

} // ends namespace

