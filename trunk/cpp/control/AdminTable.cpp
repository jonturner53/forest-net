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
	nameMap = new HashSet<string, Hash::string>(maxAdm);
	maxAdx = 0;
}
	
/** Destructor for AdminTable, frees dynamic storage */
AdminTable::~AdminTable() { delete [] avec; delete nameMap; }

/** Initialize lock and condition variables.  */
bool AdminTable::init() {
	for (int adx = 1; adx <= maxAdm; adx++)
		avec[adx].busyBit = false;
	return true;
}

/** Get an admin by name and lock its table entry.
 *  @param aname is an admin name
 *  @return the admin index associated with the given admin name,
 *  or 0 if the name does not match any admin; on a successful
 *  return, the table entry for the admin is locked; the caller
 *  must release it when done using it
 */
int AdminTable::getAdmin(const string& aname) {
	unique_lock<mutex> lck(mapLock);
	int adx = nameMap->find(aname);
	if (adx == 0) return 0;
	while (avec[adx].busyBit) { // wait until entry not busy
		avec[adx].busyCond.wait(lck);
		adx = nameMap->find(aname);
		if (adx == 0) return 0;
	}
	avec[adx].busyBit = true; // set busyBit to lock entry
	return adx;
}

/** Release a previously locked admin table entry.
 *  Waiting threads are signalled to let them proceed.
 *  @param adx is the index of a locked comtree.
 */
void AdminTable::releaseAdmin(int adx) {
	unique_lock<mutex> lck(mapLock);
	avec[adx].busyBit = false;
	avec[adx].busyCond.notify_one();
}

/** Get the first admin in the list of valid admins.
 *  @return the index of the first valid admin, or 0 if there is no
 *  valid admin; on a successful return, the admin is locked
 */
int AdminTable::firstAdmin() {
	unique_lock<mutex> lck(mapLock);
	int adx = nameMap->first();
	if (adx == 0) return 0;
	while (avec[adx].busyBit) {
		avec[adx].busyCond.wait(lck);
		adx = nameMap->first(); // first admin may have changed
		if (adx == 0) return 0;
	}
	avec[adx].busyBit = true;
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
	unique_lock<mutex> lck(mapLock);
	int nuAdx = nameMap->next(adx);
	if (nuAdx == 0) {
		avec[adx].busyBit = false;
		avec[adx].busyCond.notify_one();
		return 0;
	}
	while (avec[adx].busyBit) {
		avec[adx].busyCond.wait(lck);
		nuAdx = nameMap->next(adx);
		if (nuAdx == 0) {
			avec[adx].busyBit = false;
			avec[adx].busyCond.notify_one();
			return 0;
		}
	}
	avec[adx].busyBit = true;
	avec[adx].busyBit = false;
	avec[adx].busyCond.notify_one();
	return nuAdx;
}

/** Add a new admin.
 *  Attempts to add a new admin to the table. Can fail if the specified
 *  user name is already in use, or if there is no more space.
 *  On return, the new admin's table entry is locked; the caller must
 *  release it when done.
 *  @param aname is the admin name
 *  @param pwd is the admin password
 *  @param adx is an optional argument; if present and non-zero, the new
 *  entry is assigned the index adx, if that index is not already in use
 *  @return the index of the new table entry or 0 on failure;
 *  can fail if aname clashes with an existing name, or there is no space left
 */
int AdminTable::addAdmin(string& aname, string& pwd, int adx) {
	unique_lock<mutex> lck(mapLock);
	if (nameMap->find(aname) != 0) return 0;
	adx = nameMap->insert(aname,adx);
	if (adx == 0) return 0;
	avec[adx].busyBit = true;

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
	unique_lock<mutex> lck(mapLock);
	nameMap->remove(nameMap->retrieve(adx));
	avec[adx].busyBit = false;
	avec[adx].busyCond.notify_one();
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
        if (Util::verify(in,'+')) {
	        if (!Util::readWord(in, aname)      || !Util::verify(in,',') ||
		    !Util::readWord(in, pwd)        || !Util::verify(in,',') || 
		    !Util::readString(in, realName) || !Util::verify(in,',') ||
		    !Util::readWord(in, email)) {
	                return false;
		}
		Util::nextLine(in);
	} else if (Util::verify(in,'-')) {
		maxAdx = max(adx, maxAdx);
		Util::nextLine(in); return true;
	} else {
		Util::nextLine(in); return false;
	}

	if (addAdmin(aname, pwd, adx) == 0) return false;
	setRealName(adx,realName); setEmail(adx,email);
	releaseAdmin(adx);
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
	int i = 1;
	while (readEntry(in,i)) i++;
	cout << "read " << i-1 << " admin records, producing "
	     << nameMap->size() << "table entries\n";
        return true;
}

/** Create a string representation of the admin table
 *  @param s is a reference to a string in which the result is returned
 *  @param includeSess is true if sessions are to be included, default=false
 *  @return a reference to s
 */
string AdminTable::toString() {
	string s;
	for (int adx = firstAdmin(); adx != 0; adx = nextAdmin(adx))
                s += avec[adx].toString();
	return s;
}

/** Write the complete admin table to an output stream.
 *  @param out is a reference to an open output stream
 */
void AdminTable::write(ostream& out) {
	for (int adx = firstAdmin(); adx != 0; adx = nextAdmin(adx))
                out << avec[adx];
}

} // ends namespace

