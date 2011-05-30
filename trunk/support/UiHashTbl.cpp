/** @file UiHashTbl.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "UiHashTbl.h"

/** Constructor for hashTbl, allocates space and initializes table.
 *  N1 is the limit on the range of values; it must be less than 2^20.
 */
UiHashTbl::UiHashTbl(int n1) : n(n1) {
	n = min(n, MAXVAL);

	for (nb = 1; 8*nb < n; nb <<= 1) {}
	nb = max(nb,4);
	bktMsk = nb - 1; valMsk = (8*nb) - 1; fpMsk = ~valMsk;

	bkt = new bkt_t[2*nb]; keyVec = new uint64_t[n+1];

	int i, j;
	for (i = 0; i < 2*nb; i++) {
		for (j = 0; j < BKT_SIZ; j++) bkt[i][j] = 0;
	}
};
	
UiHashTbl::~UiHashTbl() {
	delete [] bkt; delete [] keyVec;
}

/** Compute a bucket index and fingerprint, for a given key.
 * 
 *  Hashit uses multiplicative hashing with one of two
 *  different multipliers, after first converting
 *  the 64 bit integer into a 32 bit integer.
 *
 *  @param key is the key to be hashed
 *  @param hf is either 0 or 1 and selects one of two hash functions
 *  @param b is a reference; on return its value is equal to the
 *  hash bucket for the given key
 *  @param fp is a reference; on return its value is equal to the
 *  fingerprint for the given key
 */
void UiHashTbl::hashit(uint64_t key, int hf, uint32_t& b, uint32_t& fp) {
	const uint32_t A0 = 0xa8134c35;
	const uint32_t A1 = 0xe626c2d3;

	uint32_t x, y; uint64_t z;

	x = (key >> 16) & 0xffff0000; x |= (key & 0xffff);
	y = (key >> 48) & 0xffff0000; y |= ((key >> 16) & 0xffff); 
	z = x ^ y;
	z *= (hf == 0 ? A0 : A1);
	b = (z >> 32) & bktMsk;
	fp  = (z >> 29) & fpMsk;
}

/** Perform a lookup in the hash table.
 *  @param key is the keey to be looked up in the table
 *  @return the value stored for the given key, or 0 if there is none.
 */
int UiHashTbl::lookup(uint64_t key) {
	int i; uint32_t b, val, fp;

	// check bucket in the first half of the bucket array
	hashit(key,0,b,fp);
	for (i = 0; i < BKT_SIZ; i++) {
                if ((bkt[b][i] & fpMsk) == fp) {
                        val = bkt[b][i] & valMsk;
                        if (keyVec[val] == key) return val;
                }
        }

	// check bucket in the second half of the bucket array
        hashit(key,1,b,fp); b += nb;
        for (i = 0; i < BKT_SIZ; i++) {
                if ((bkt[b][i] & fpMsk) == fp) {
                        val = bkt[b][i] & valMsk;
                        if (keyVec[val] == key) return val;
                }
        }

}

/** Insert a (key,value) pair into hash table.
 *  No check is made to ensure that there is no conflicting
 *  (key,value) pair.
 *  @param key is the key part of the pair
 *  @param val is the value part of the pair
 *  @return true on success, false on failure.
 */
bool UiHashTbl::insert(uint64_t key, uint32_t val) {
	int i, j0, j1, n0, n1;
	uint32_t b0, b1, fp0, fp1;

	// Count the number of unused items in each bucket
	// and find an unused item in each (if there is one)
	hashit(key,0,b0,fp0);
	n0 = 0;
	for (i = 0; i < BKT_SIZ; i++) {
		if (bkt[b0][i] == 0) { n0++; j0 = i; }
	}
	hashit(key,1,b1,fp1); b1 += nb;
	n1 = 0;
	for (i = 0; i < BKT_SIZ; i++) {
		if (bkt[b1][i] == 0) { n1++; j1 = i; }
	}
	// If no unused entry in either bucket, give up.
	if (n0 + n1 == 0) return false;

	// store the key value in keyVec and add entry in least-loaded bucket
	keyVec[val] = key;
	if (n0 >= n1) bkt[b0][j0] = fp0 | (val & valMsk);
	else bkt[b1][j1] = fp1 | (val & valMsk);
		
	return true;
}

/** Remove a (key, value) pair from the table.
 *  @param key is the key of the pair to be removed
 */
void UiHashTbl::remove(uint64_t key) {
	int i; uint32_t b, val, fp; uint32_t *bucket;

	hashit(key,0,b,fp);
	for (i = 0; i < BKT_SIZ; i++) {
		if ((bkt[b][i] & fpMsk) == fp) {
			val = bkt[b][i] & valMsk;
			if (keyVec[val] == key) {
				bkt[b][i] = 0; return;
			}
		}
	}
	hashit(key,1,b,fp); b += nb;
	for (i = 0; i < BKT_SIZ; i++) {
		if ((bkt[b][i] & fpMsk) == fp) {
			val = bkt[b][i] & valMsk;
			if (keyVec[val] == key) {
				bkt[b][i] = 0; return;
			}
		}
	}
}

/** Print out all key,value pairs stored in the hash table,
 *  along with their bucket index, offset within the bucket
 *  and fingerprint.
 */
void UiHashTbl::dump() {
	int i, j, s; uint32_t vm, val, fp; uint32_t *bucket;

	// Determine amount to shift to right-justify fingerprints
	vm = valMsk; s = 0; while (vm != 0) { vm >>= 1; s++; }

	for (i = 0; i < 2*nb; i++) {
		bucket = &bkt[i][0];
		for (j = 0; j < BKT_SIZ; j++) {
			if (bucket[j] != 0) {
				cout << i << "," << j << ": ";
				val =  bucket[j] & valMsk;
				fp  = (bucket[j] &  fpMsk) >> s;
				cout << keyVec[val] << " " << val << " " << fp;
				cout << endl;
			}
		}
	}
}
