// Header file for hashTbl class.
//
// Maintains set of (key, value) pairs where key is a 64 bit value and
// value is a positive 32 bit integer in a restricted range.
// All (key,value) pairs must be disjoint; that is no two pairs may
// share the same key and no two pairs may share the same value.
//
// Main methods
//   lookup - returns value for given key
//   insert - adds a (key,value) pair
//   remove - removes the pair for a given key
//
// The implementation uses a 2-left hash table with eight items
// in each bucket. The hash table is configured for a specified
// range of values (max of 10^6) with a maximum load factor of
// 50% to minimize the potential for overloading any bucket.

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "stdinc.h"

class hashTbl {
public:
		hashTbl(int);
		~hashTbl();

	int	lookup(uint64_t); 		// lookup entry
	bool	insert(uint64_t, uint32_t); 	// add an entry
	void	remove(uint64_t); 		// remove an entry
	void	dump();				// print hash table contents
private:
	static const int BKT_SIZ = 8;		// # of items per bucket
	static const int MAXVAL = (1 << 20)-1;	// largest stored value
	int	n;			// range of values is 1..n
	int	nb;			// number of hash buckets per section
	int	bktMsk;			// mask used to extract bucket index
	int	valMsk;			// mask used to extract value
	int	fpMsk;			// mask used to extract fingerprint

	typedef uint32_t bkt_t[BKT_SIZ]; // type declaration for buckets
	bkt_t	*bkt;			// vector of hash backets
	uint64_t *keyVec;		// vector of keys, indexed by value

	void hashit(uint64_t,int,uint32_t&,uint32_t&); // hash function
	int chkBkt(uint32_t, uint32_t, uint64_t); // check bucket for match
};

#endif
