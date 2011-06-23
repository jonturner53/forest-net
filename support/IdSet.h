/** \file IdSet.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */
  

#ifndef IDSET_H
#define IDSET_H

#include "stdinc.h"

#include "Misc.h"
#include "UiList.h"
#include "UiDlist.h"
#include "UiHashTbl.h"


/** Data structure that assigns small integer identifiers to large keys.
 *  This is useful in contexts where the "natural identifiers"
 *  in an application can vary over a large range, but the number
 *  of distinct identifiers that are in use at one time is relatively
 *  small. This data structure can be used to map the "natural identfiers"
 *  into integers in a restricted range 1..max, where max is specified
 *  when the data structure is initialized.
 */
class IdSet {
public:
		IdSet(int);
		~IdSet();

	// access methods
	int	firstId() const; 	
	int	lastId() const; 	
	int	nextId(int) const; 	

	// define/retrieve/remove (key,id) pairs
	int	getId(uint64_t); 		
	uint64_t getKey(int);
	void	releaseId(uint64_t); 
	void	clear(); 	

	// predicates
	bool	isMapped(uint64_t);
	bool	isAssigned(int);

	// produce readable IdSets
	void	add2string(string&) const;
	void	write(ostream&) const;		
private:
	static const int MAXID = (1 << 20)-1;  ///< largest possible identifier
	int n;				///< largest identifier in this set
	UiHashTbl *ht;			///< hash table to compute mapping
	UiList	*free;			///< available identifiers
	UiDlist *idList;		///< in-use identifiers
};

/** Get the first assigned identifier, in some arbitrary order.
 *  @return number of the first identfier
 */
inline int IdSet::firstId() const { return idList->first(); }

/** Get the last assigned identifier, in some arbitrary order.
 *  @return number of the last identfier
 */
inline int IdSet::lastId() const { return idList->last(); }

/** Get the next assigned identifier, in some arbitrary order.
 *  @param id is an identifer in the set
 *  @return number of the next identfier
 */
inline int IdSet::nextId(int id) const { return idList->next(id); }

/** Determine if a given key has been mapped to an identfier.
 *  @param key is the key to be checked
 *  @return true if the key has been mapped, else false
 */
inline bool isMapped(uint64_t key) { return (ht->lookup(key) != 0); }

/** Determine if a given identifier has been assigned to a key.
 *  @param id is the identifier to be checked
 *  @return true if the key has been mapped, else false
 */
inline bool isAssigned(int id) {
	return (1 <= id && id <= n && idList->member(id));
}

/** Get the key that was mapped to the given identifier */
 *  @param id is the identifier whose key is to be returned
 *  @return the key that maps to id, or 0 if there is none
 */
inline uint64_t getKey(int id) {
	return (isAssigned(id) ? ht->getKey(id) : 0); 
}

#endif
