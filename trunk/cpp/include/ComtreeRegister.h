/** @file ComtreeRegister.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef CLIENTREGISTER_H
#define CLIENTREGISTER_H

#include <map>
#include "Forest.h"
#include "RateSpec.h"
#include "UiClist.h"
#include "UiSetPair.h"
#include "IdMap.h"

namespace forest {

/** Class that implements a register of information about comtrees,
 *  for use by ClientMgr.
 *
 *  Entries are accessed using a "comtree index", which
 *  can be obtained using the getComtIndex() method. This also
 *  locks the comtree's table entry to permit exclusive access
 *  to the data for that comtree.
 *  Other methods also lock table entries.
 */
class ComtreeRegister {
public:
		ComtreeRegister(int,int);
		~ComtreeRegister();
	bool	init();

	bool	validComtree(int) const;

	// iteration methods
	int	firstComtree();
	int	nextComtree(int);

	// access routines for comtree info
	bool	isLocked(int) const;
	int	getComtIndex(comt_t);		
	void	releaseComtree(int);
	comt_t	getComtree(int);		
	const string& getOwner(int) const;
	fAdr_t	getSuper(int) const;
	Forest::ConfigMode getConfigMode(int) const;
	Forest::AccessMethod getAccessMethod(int) const;
	int	getRepInterval(int) const;
	time_t	getStartTime(int) const;

	// add/remove/modify table entries
	int	addComtree(comt_t, string&);
	void	removeComtree(int);

	void	setOwner(int, const string&);
	void	setSuper(int, fAdr_t);
	void	setConfigMode(int, Forest::ConfigMode);		
	void	setAccessMethod(int, Forest::ConfigMode);		
	void	setRepInterval(int, int);
	void	setStartTime(int, time_t);

	// input/output of table contents 
	bool 	readEntry(istream&, int=0);
	bool 	read(istream&);
	string&	toString(string&);
	string&	comtree2string(int, string&) const;
	void 	write(ostream&);

	// locking/unlocking the internal maps
	// (client name-to-index, client address-to-session)
	void	lockMap();
	void	unlockMap();
private:
	int	maxComt;		///< max number of comtrees
	int	maxCtx;			///< largest defined ctx

	struct Comtree { 		///< comtree register entry
	comt_t	comtree;		///< comtree number
	string	owner;			///< owner's client name
	string	password;		///< password used for access
	fAdr_t	supervisor;		///< server that supervises comtree
	RateSpec defBbRates;		///< default rates for backbone links
	RateSpec defLeafRates;		///< default rates for access links
	Forest::ConfigMode cfgMode;	///< backbone rate configuration mode
	Forest::AccessMethod axsMethod;	///< access method for joining
	int	reportInterval;		///< how often to report status (sec)
	time_t	start;			///< time when comtree started
	bool	busyBit;		///< set for a busy client
	pthread_cond_t busyCond;	///< used to wait for a busy client
	};

	Comtree *cvec;			///< vector of comtree structs

	IdMap *comtMap;			///< maps comtree number to index

	pthread_mutex_t mapLock;	///< must hold during add/remove ops
					///< and while locking comtree

	/** helper functions */
	uint64_t key(comt_t) const;
	bool 	readEntry(istream&);
	int	fileSize();

};

inline bool ComtreeRegister::validComtIndex(int ctx) const {
	return comtMap->validId(ctx);
}

inline int ComtreeRegister::getNumComtrees() const {
	return comtMap->size();
}

inline int ComtreeRegister::getMaxComtrees() const { return maxComt; }

inline int ComtreeRegister::getMaxClx() const { return maxClx; }

/** Get the comtree number of a given entry.
 *  @param ctx is the comtree index for some comtree
 *  @return a the actual comtree number
 */
inline comt_t ComtreeRegister::getComtree(int ctx) const {
	return cvec[ctx].comtree;
}

/** Get the owner of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @return a const reference to a string naming the owner
 */
inline const string& ComtreeRegister::getOwner(int ctx) const {
	return cvec[ctx].owner;
}

/** Get the password for a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @return a const reference to a string for the password
 */
inline const string& ComtreeRegister::getPassword(int ctx) const {
	return cvec[ctx].password;
}

/** Get the supervisor of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @return the forest address of the supervisor of the comtree
 */
inline fAdr_t ComtreeRegister::getSuper(int ctx) const {
	return cvec[ctx].supervisor;
}

/** Get the configuration mode of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @return the configuration mode of the comtree
 */
inline Forest::ConfigMode ComtreeRegister::getConfigMode(int ctx) const {
	return cvec[ctx].configMode;
}

/** Get the access method of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @return the access method of the comtree
 */
inline Forest::AccessMethod ComtreeRegister::getAccessMethod(int ctx) const {
	return cvec[ctx].accessMethod;
}

/** Get the reporting interval of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @return the reporting interval of the comtree
 */
inline int ComtreeRegister::getRepInterval(int ctx) const {
	return cvec[ctx].repInterval;
}

/** Get the start time of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @return the start time of the comtree (in seconds since the epoch)
 */
inline time_t ComtreeRegister::getStartTime(int ctx) const {
	return cvec[ctx].repInterval;
}

inline bool ComtreeRegister::isLocked(int ctx) const {
	return cvec[ctx].busyBit;
}

/** Set the owner of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @param owner is a string naming the owner of the comtree
 */
inline void ComtreeRegister::setOwner(int ctx, const string& owner) const {
	return cvec[ctx].owner = owner;
}

/** Set the password for a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @param pwd is the new password
 */
inline void ComtreeRegister::setPassword(int ctx, const string& pwd) const {
	return cvec[ctx].password = pwd;
}

/** Set the supervisor of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @param super is the forest address of the supervisor of the comtree
 */
inline void ComtreeRegister::setSuper(int ctx, fAdr_t super) const {
	return cvec[ctx].supervisor = super;
}

/** Set the configuration mode of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @return the configuration mode of the comtree
 */
inline void ComtreeRegister::setConfigMode(int ctx, Forest::ConfigMode cfg) const {
	return cvec[ctx].configMode = cfg;
}

/** Set the access method of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @param axs is the access method of the comtree
 */
inline void ComtreeRegister::setConfigMode(int ctx, Forest::AccessMethod axs) {
	return cvec[ctx].accessMethod = axs;
}

/** Set the reporting interval of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @param interval is the reporting interval of the comtree
 */
inline void ComtreeRegister::setRepInterval(int ctx, int interval) {
	return cvec[ctx].repInterval = interval;
}

/** Get the start time of a comtree.
 *  @param ctx is the comtree index for some comtree
 *  @param t is the start time of the comtree (in seconds since the epoch)
 */
inline void ComtreeRegister::setStartTime(int ctx, time_t t) {
	return cvec[ctx].repInterval = t;
}

/** Compute key for use with comtMap.
 *  @param comt is a comtree number
 *  @return a 64 bit hash key
 */
inline uint64_t ComtreeRegister::key(comt_t comt) const {
        return (uint64_t(comt) << 32) | comt;
}

/** Lock the client table.
 *  This method is meant primarily for internal use.
 *  Applications should normally just lock single clients using the
 *  methods provided for that purpose. Use extreme caution when using
 *  this method directly.
 */
inline void ComtreeRegister::lockMap() { pthread_mutex_lock(&mapLock); }

/** Unlock the client table.
 */
inline void ComtreeRegister::unlockMap() { pthread_mutex_unlock(&mapLock); }

} // ends namespace

#endif
