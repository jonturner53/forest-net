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
	void	releaseId(uint64_t); 
	void	clear(); 	

	// produce readable IdSets
	void	add2string(string&) const;
	void	write(ostream&) const;		
private:
	static const int MAXID = (1 << 20)-1;  ///! largest possible identifier
	int n;				///! largest identifier in this set
	UiHashTbl *ht;			///! hash table to compute mapping
	UiList	*free;			///! available identifiers
	UiDlist *idList;		///! in-use identifiers
};

inline int IdSet::firstId() const { return idList->first(); }

inline int IdSet::lastId() const { return idList->last(); }

inline int IdSet::nextId(int i) const { return idList->next(i); }

#endif
