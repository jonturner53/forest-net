/** @file ComtreeRegister.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "ComtreeRegister.h"

namespace forest {


/** Constructor for ComtreeRegister, allocates space and initializes table. */
ComtreeRegister::ComtreeRegister(int maxComtrees) : maxComt(maxComtrees) {
	cvec = new Comtree[maxComt+1];
	comtMap = new IdMap(maxComt);
	maxCtx = 0;
}
	
/** Destructor for ComtreeRegister, frees dynamic storage */
ComtreeRegister::~ComtreeRegister() {
	delete [] cvec; delete comtMap;
	pthread_mutex_destroy(&mapLock);
}

/** Initialize lock and condition variables.  */
bool ComtreeRegister::init() {
        if (pthread_mutex_init(&mapLock,NULL) != 0) return false;
	for (int ctx = 1; ctx <= maxComt; ctx++) {
		cvec[ctx].busyBit = false;
		if (pthread_cond_init(&cvec[ctx].busyCond,NULL) != 0)
			return false;
	}
	return true;
}

/** Get a comtree index for a given comtree.
 *  @param comt is a comtree number
 *  @return the comtree index associated with the given comtree,
 *  or 0 if there is no table entry for the given comtree; on
 *  return, the table entry for the comtree is locked; the caller
 *  must release it when done using it
 */
int ComtreeRegister::getComtIndex(comt_t comt) {
	lockMap();
	int ctx = comtMap->getId(key(comt));
	if (ctx == 0) { unlockMap(); return 0; }
	while (cvec[ctx].busyBit) { // wait until client's entry is not busy
		pthread_cond_wait(&cvec[ctx].busyCond,&mapLock);
		if (ctx == 0) { unlockMap(); return 0; }
	}
	cvec[ctx].busyBit = true; // set busyBit to lock entry
	unlockMap();
	return ctx;
}

/** Release a previously locked comtree register entry.
 *  Waiting threads are signalled to let them proceed.
 *  @param ctx is the index of a locked comtree.
 */
void ComtreeRegister::releaseComtree(int ctx) {
	lockMap();
	cvec[ctx].busyBit = false;
	pthread_cond_signal(&cvec[ctx].busyCond);
	unlockMap();
}

/** Get the first comtree in the list of active comtrees.
 *  @return the index of the first valid comtree, or 0 if there is no
 *  valid comtree; on a successful return, the comtree is locked
 */
ctx ComtreeRegister::firstComtree() {
	lockMap();
	int ctx = comtMap->firstId();
	if (ctx == 0) { unlockMap(); return 0; }
	while (cvec[ctx].busyBit) {
		pthread_cond_wait(&cvec[ctx].busyCond,&mapLock);
		ctx = comtMap->firstId();
		if (ctx == 0) {
			pthread_cond_signal(&cvec[ctx].busyCond);
			unlockMap(); return 0;
		}
	}
	cvec[ctx].busyBit = true;
	unlockMap();
	return ctx;
}

/** Get the index of the next client.
 *  The caller is assumed to have a lock on the current client.
 *  @param ctx is the index of a valid client
 *  @return the index of the next client in the list of valid clients
 *  or 0 if there is no next client; on return, the lock on ctx
 *  is released and the next client is locked.
 */
int ComtreeRegister::nextComtree(int ctx) {
	lockMap();
	int nuCtx = comtMap->nextId(ctx);
	if (nuCtx == 0) {
		cvec[ctx].busyBit = false;
		pthread_cond_signal(&cvec[ctx].busyCond);
		unlockMap();
		return 0;
	}
	while (cvec[nuCtx].busyBit) {
		pthread_cond_wait(&cvec[nuCtx].busyCond,&mapLock);
		nuCtx = comtMap->nextId(ctx);
		if (nuCtx == 0) {
			cvec[ctx].busyBit = false;
			pthread_cond_signal(&cvec[ctx].busyCond);
			pthread_cond_signal(&cvec[nuCtx].busyCond);
			unlockMap();
			return 0;
		}
	}
	cvec[nuCtx].busyBit = true;
	cvec[ctx].busyBit = false;
	pthread_cond_signal(&cvec[ctx].busyCond);
	unlockMap();
	return nuCtx;
}

/** Add a new comtree.
 *  Attempts to add a new comtree to the register. Can fail if the specified
 *  comtree is already in use, or if there is no more space.
 *  On return, the new entry is locked; the caller must
 *  release it when done.
 *  @param comt is the comtree number for the new entry
 *  @param ctx is an optional argument specifying the comtree index
 *  to be used for this new comtree; if ctx == 0, the comtree index is
 *  assigned automatically (this is the default behavior)
 *  @return the index of the new table entry or 0 on failure;
 */
int ComtreeRegister::addComtree(comt_t comt, int ctx) {
	lockMap();
	if (ctx == 0) ctx = comtMap->addPair(key(comt));
	else ctx = comtMap->addPair(key(comt),ctx);
	if (ctx == 0) { unlockMap(); return 0;}
	cvec[ctx].busyBit = true;
	unlockMap();

	cvec[ctx].comt = comt;
	maxCtx = max(ctx,maxCtx);
	return ctx;
}

/** Remove a comtree.
 *  Assumes that the calling thread has already locked the comtree.
 *  The lock is released on return.
 *  @param ctx is the index of the comtree to be deleted
 */
void ComtreeRegister::removeComtree(int ctx) {
	lockMap();
	comtMap->dropPair(key(getComtree(ctx)));
	cvec[ctx].busyBit = false;
	pthread_cond_signal(&cvec[ctx].busyCond);
	unlockMap();
}

/** Read a comtree record from an input file and initialize its table entry.
 *  A record includes ...
 *  @param in is an open file stream
 *  @param ctx is an optional argument specifying the comtree index for
 *  this entry; if zero, a comtree index is selected automatically
 *  (this is the default behavior)
 */
bool ComtreeRegister::readEntry(istream& in, int ctx) {
	comt_t comt, string owner, pwd, cfgString, axsString;
	fAdr_t rootZip, super;
	int repInterval; time_t start;

	ss << getComtree(ctx) << ", " << getOwner(ctx) << ", ";
	ss << Forest::fAdr2string(getRootZip(ctx),s) << ", ";
	ss << Forest::fAdr2string(getSuper(ctx),s) << ", ";

	if (!in.good()) return false;
        if (Misc::verify(in,'+')) {
	        if (!Misc::readNum(in, comt) ||
		    !Misc::verify(in,',') ||
		    !Misc::readWord(in, owner)  ||
		    !Misc::verify(in,',') || 
		    !Forest::readForestAdr(in, rootZip) ||
		    !Misc::verify(in,',') || 
		    !Forest::readForestAdr(in, super) ||
		    !Misc::verify(in,',') || 
		    !Misc::readWord(in, cfgString) ||
		    !Misc::verify(in,',') || 
		    !Misc::readString(in, axsString) ||
		    !Misc::verify(in,',') ||
		    !Misc::readWord(in, pwd) ||
		    !Misc::verify(in,',') ||
		    !Misc::readNum(in, repInterval) ||
		    !Misc::verify(in,',') ||
		    !Misc::readNum(in, start)) {
	                return false;
		}
		Misc::cflush(in,'\n');
	} else if (Misc::verify(in,'-')) {
		maxCtx = max(ctx, maxCtx);
		Misc::cflush(in,'\n'); return true;
	} else {
		Misc::cflush(in,'\n'); return false;
	}

	Forest::ConfigMode cfg;
	     if (cfgString == "static")		cfg = STATIC;
	else if (cfgString == "leafAdjust")	cfg = LEAFADJUST;
	else if (cfgString == "stepAdjust")	cfg = STEPADJUST;
	else					cfg = NUL_CFG;

	Forest::AccessMethod axs;
	     if (axsString == "open")		axs = OPEN;
	else if (axsString == "byPermission")	axs = BYPERMISSION;
	else if (axsString == "byPassword")	axs = BYPASSWORD;
	else					axs = NUL_AXS;
	
	if (addComtree(comt, ctx) == 0) return false;
	setOwner(ctx,owner);
	setRootZip(ctx,rootZip); setSuper(ctx,super);
	setConfigMode(ctx) = cfg; setAccessMethod(ctx) = axs;
	setPassword(ctx,password);
	setReportInterval(ctx) = repInterval; setStartTime(ctx) = start;
	releaseComtree(ctx);
        return true;
}

/** Read comtree register entries from an input stream.
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
bool ComtreeRegister::read(istream& in) {
	int i = 0;
	while (readEntry(in,i)) i++;
	cout << "read " << i << " client records, producing "
	     << clients->getNumIn() << "table entries\n";
        return true;
}

/** Construct a string representation of a comtree entry.
 *  This metghod does no locking.
 *  @param ctx is the comtree index of the entry
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& ComtreeRegister::comtree2string(int ctx, string& s) const {
	stringstream ss;
	ss << getComtree(ctx) << ", " << getOwner(ctx) << ", ";
	ss << Forest::fAdr2string(getRootZip(ctx),s) << ", ";
	ss << Forest::fAdr2string(getSuper(ctx),s) << ", ";

	Forest::ConfigMode cfg = getConfigMode(ctx);
	switch (cfg) {
	case STATIC:	ss << "static"; break;
	case LEAFADJUST: ss << "leafAdjust"; break;
	case STEPADJUST: ss << "stepAdjust"; break;
	default: ss << "-";
	}
	ss << ", ";

	Forest::AccessMethod axs = getAccessMethod(ctx);
	switch (axs) {
	case OPEN:	ss << "open"; break;
	case BYPERMISSION: ss << "byPermission"; break;
	case BYPASSWORD: ss << "byPassword"; break;
	default: ss << "-";
	}
	ss << ", ";

	ss << getPassword(ctx) << ", " << getReportInterval(ctx) << ", "
	   << getStartTime(ctx) << endl;

	s = ss.str();
	return s;
}

/** Create a string representation of the comtree register.
 *  @param s is a reference to a string in which the result is returned
 *  @return a reference to s
 */
string& ComtreeRegister::toString(string& s) {
	string s1; s = "";
	for (int ctx = firstComtree(); ctx != 0; ctx = nextComtree(ctx))
                s += client2string(ctx,s1);
	return s;
}

/** Write the complete client table to an output stream.
 *  Does no locking.
 *  @param out is a reference to an open output stream
 */
void ComtreeRegister::write(ostream& out) {
	string s;
	for (int ctx = firstComtree(); ctx != 0; ctx = nextComtree(ctx))
                out << client2string(ctx,s);
}

} // ends namespace

