/** \file UiHashTbl.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */
  

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "stdinc.h"

/** Maintains set of (key, value) pairs where key is a 64 bit value and
 *  value is a positive 32 bit integer in a restricted range.
 *  All (key,value) pairs must be fully disjoint; that is no two pairs may
 *  share the same key and no two pairs may share the same value.
 * 
 *  Main methods
 *    lookup - returns value for given key
 *    insert - adds a (key,value) pair
 *    remove - removes the pair for a given key
 * 
 *  The implementation uses a 2-left hash table with eight items
 *  in each bucket. The hash table is configured for a specified
 *  range of values (max of 10^6) with a maximum load factor of
 *  50% to minimize the potential for overloading any bucket.
 */
class UiHashTbl {
public:
		UiHashTbl(int);
		~UiHashTbl();

	int	lookup(uint64_t); 		
	bool	insert(uint64_t, uint32_t); 
	void	remove(uint64_t); 	
	void	clear(); 	
	void	dump();		
private:
	static const int BKT_SIZ = 8;		///< # of items per bucket
	static const int MAXVAL = (1 << 20)-1;	///< largest stored value
	int	n;			///< range of values is 1..n
	int	nb;			///< number of hash buckets per section
	int	bktMsk;			///< mask used to extract bucket index
	int	valMsk;			///< mask used to extract value
	int	fpMsk;			///< mask used to extract fingerprint

	typedef uint32_t bkt_t[BKT_SIZ]; ///< type declaration for buckets
	bkt_t	*bkt;			///< vector of hash backets
	uint64_t *keyVec;		///< vector of keys, indexed by value

	void hashit(uint64_t,int,uint32_t&,uint32_t&);
};

#endif
