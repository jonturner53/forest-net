/** @file AdminTable.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "AdminTable.h"

namespace forest {


/** Constructor for AdminTable, allocates space and initializes table. */
AdminTable::AdminTable(int maxAdmins) : maxAdm(maxAdmins) {
	avec = new Admin[maxAdm+1];
	admins = new UiSetPair(maxAdm);
	nameMap = new map<string, int>();
	maxAdx = 0;
}
	
/** Destructor for AdminTable, frees dynamic storage */
AdminTable::~AdminTable() {
	delete [] avec; delete admins; delete nameMap;
	pthread_mutex_destroy(&mapLock);
}

/** Initialize lock and condition variables.  */
bool AdminTable::init() {
        if (pthread_mutex_init(&mapLock,NULL) != 0) return false;
	for (int adx = 1; adx <= maxAdm; adx++) {
		avec[adx].busyBit = false;
		if (pthread_cond_init(&avec[adx].busyCond,NULL) != 0)
			return false;
	}
	return true;
}

/** Lock an admin's table entry.
 *  @param adx is an admin index
 *  @return true if able to a lock on the specified admin;
 *  return false if the admin index does not refer to a valid admin
bool AdminTable::lockAdmin(int adx) {
	lockMap();
	if (!admins->isIn(adx)) {
		unlockMap(); return 0;
	}
	while (avec[adx].busyBit) { // wait until admin's entry is not busy
		pthread_cond_wait(&avec[adx].busyCond,&mapLock);
		if (!admins->isIn(adx)) {
			pthread_cond_signal(&avec[adx].busyCond);
			unlockMap(); return false;
		}
	}
	avec[adx].busyBit = true; // set busyBit to lock admin table entry
	unlockMap();
	return true;
}
 */

/** Get an admin by name and lock its table entry.
 *  @param aname is an admin name
 *  @return the admin index associated with the given admin name,
 *  or 0 if the name does not match any admin; on a successful
 *  return, the table entry for the admin is locked; the caller
 *  must release it when done using it
 */
int AdminTable::getAdmin(const string& aname) {
	lockMap();
	map<string, int>::iterator p = nameMap->find(aname);
	if (p == nameMap->end()) { unlockMap(); return 0; }
	int adx = p->second;
	while (avec[adx].busyBit) { // wait until admin's entry is not busy
		pthread_cond_wait(&avec[adx].busyCond,&mapLock);
		p = nameMap->find(aname);
		if (p == nameMap->end()) {
			pthread_cond_signal(&avec[adx].busyCond);
			unlockMap(); return 0;
		}
	}
	avec[adx].busyBit = true; // set busyBit to lock admin table entry
	unlockMap();
	return adx;
}

/** Release a previously locked admin table entry.
 *  Waiting threads are signalled to let them proceed.
 *  @param adx is the index of a locked comtree.
 */
void AdminTable::releaseAdmin(int adx) {
	lockMap();
	avec[adx].busyBit = false;
	pthread_cond_signal(&avec[adx].busyCond);
	unlockMap();
}

/** Get the first admin in the list of valid admins.
 *  @return the index of the first valid admin, or 0 if there is no
 *  valid admin; on a successful return, the admin is locked
 */
int AdminTable::firstAdmin() {
	lockMap();
	int adx = admins->firstIn();
	if (adx == 0) { unlockMap(); return 0; }
	while (avec[adx].busyBit) {
		pthread_cond_wait(&avec[adx].busyCond,&mapLock);
		adx = admins->firstIn(); // first admin may have changed
		if (adx == 0) {
			pthread_cond_signal(&avec[adx].busyCond);
			unlockMap(); return 0;
		}
	}
	avec[adx].busyBit = true;
	unlockMap();
	return adx;
}

/** Get the index of the next admin.
 *  The caller is assumed to have a lock on the current admin.
 *  @param adx is the index of a valid admin
 *  @return the index of the next admin in the list of valid admins
 *  or 0 if there is no next admin; on return, the lock on adx
 *  is released and the next admin is locked.
 */
int AdminTable::nextAdmin(int adx) {
	lockMap();
	int nuAdx = admins->nextIn(adx);
	if (nuAdx == 0) {
		avec[adx].busyBit = false;
		pthread_cond_signal(&avec[adx].busyCond);
		unlockMap();
		return 0;
	}
	while (avec[nuAdx].busyBit) {
		pthread_cond_wait(&avec[nuAdx].busyCond,&mapLock);
		nuAdx = admins->nextIn(adx);
		if (nuAdx == 0) {
			avec[adx].busyBit = false;
			pthread_cond_signal(&avec[adx].busyCond);
			pthread_cond_signal(&avec[nuAdx].busyCond);
			unlockMap();
			return 0;
		}
	}
	avec[nuAdx].busyBit = true;
	avec[adx].busyBit = false;
	pthread_cond_signal(&avec[adx].busyCond);
	unlockMap();
	return nuAdx;
}

/** Add a new admin.
 *  Attempts to add a new admin to the table. Can fail if the specified
 *  user name is already in use, or if there is no more space.
 *  On return, the new admin's table entry is locked; the caller must
 *  release it when done.
 *  @param aname is the admin name
 *  @param pwd is the admin password
 *  @param adx is an optional argument specifying the admin index
 *  to be used for this new admin; if adx == 0, the admin index is
 *  assigned automatically (this is the default behavior)
 *  @return the index of the new table entry or 0 on failure;
 *  can fail if aname clashes with an existing name, or there is no space left
 */
int AdminTable::addAdmin(string& aname, string& pwd, int adx) {
	lockMap();
	map<string,int>::iterator p = nameMap->find(aname);
	if (p != nameMap->end()) { unlockMap(); return 0; }
	if (adx != 0) {
		if (admins->isIn(adx)) adx = 0;
	} else {
		adx = admins->firstOut();
	}
	if (adx == 0) { unlockMap(); return 0;}
	nameMap->insert(pair<string,int>(aname,adx));
	admins->swap(adx);
	avec[adx].busyBit = true;
	unlockMap();

	setAdminName(adx,aname); setPassword(adx,pwd);
	setRealName(adx,"noname"); setEmail(adx,"nomail");

	maxAdx = max(adx,maxAdx);
	return adx;
}

/** Remove an admin.
 *  Assumes that the calling thread has already locked the admin.
 *  The lock is released on return.
 *  @param adx is the index of the admin to be deleted
 */
void AdminTable::removeAdmin(int adx) {
	lockMap();
	nameMap->erase(avec[adx].aname);
	admins->swap(adx);
	avec[adx].busyBit = false;
	pthread_cond_signal(&avec[adx].busyCond);
	unlockMap();
}

/** Read an admin record from an input file and initialize its table entry.
 *  A record includes an admin name, password, real name (in quotes),
 *  an email address, a default rate spec and a total rate spec.
 *  @param in is an open file stream
 *  @param adx is an optional argument specifying the admin index for
 *  this entry; if zero, an admin index is selected automatically
 *  (this is the default behavior)
 */
bool AdminTable::readEntry(istream& in, int adx) {
	string aname, pwd, realName, email;

	if (!in.good()) return false;
        if (Misc::verify(in,'+')) {
	        if (!Misc::readName(in, aname)      || !Misc::verify(in,',') ||
		    !Misc::readWord(in, pwd)        || !Misc::verify(in,',') || 
		    !Misc::readString(in, realName) || !Misc::verify(in,',') ||
		    !Misc::readWord(in, email)      || !Misc::verify(in,',')) {
	                return false;
		}
		Misc::cflush(in,'\n');
	} else if (Misc::verify(in,'-')) {
		maxAdx = max(adx, maxAdx);
		Misc::cflush(in,'\n'); return true;
	} else {
		Misc::cflush(in,'\n'); return false;
	}

	if (addAdmin(aname, pwd, adx) == 0) return false;
	setRealName(adx,realName); setEmail(adx,email);
        return true;
}

/** Read admin table entries from an input stream.
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
bool AdminTable::read(istream& in) {
	int i = 0;
	while (readEntry(in,i)) i++;
	cout << "read " << i << " admin records, producing "
	     << admins->getNumIn() << "table entries\n";
        return true;
}

/** Construct a string representation of an admin.
 *  This metghod does no locking.
 *  @param adx is the admin index of the admin to be written
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& AdminTable::admin2string(int adx, string& s) const {
	string s1;
	s  = getAdminName(adx) + ", " + getPassword(adx) + ", ";
	s += ", \"" + getRealName(adx) + "\", " + getEmail(adx) + "\n";
	return s;
}


/** Create a string representation of the admin table
 *  @param s is a reference to a string in which the result is returned
 *  @param includeSess is true if sessions are to be included, default=false
 *  @return a reference to s
 */
string& AdminTable::toString(string& s) {
	string s1; s = "";
	for (int adx = firstAdmin(); adx != 0; adx = nextAdmin(adx))
                s += admin2string(adx,s1);
	return s;
}

/** Write the complete admin table to an output stream.
 *  @param out is a reference to an open output stream
 */
void AdminTable::write(ostream& out) {
	string s;
	for (int adx = firstAdmin(); adx != 0; adx = nextAdmin(adx))
                out << admin2string(adx,s);
}

} // ends namespace

