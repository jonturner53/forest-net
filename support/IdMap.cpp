/** @file IdMap.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "IdMap.h"

/** Constructor for IdMap, allocates space and initializes table.
 *  N1 is the limit on the range of values; it must be less than 2^20.
 */
IdMap::IdMap(int n1) : n(n1) {
	if (n > MAXID) fatal("IdMap::IdMap: specified size too large");
	ht = new UiHashTbl(n);
	ids = new UiSetPair(n);
};
	
IdMap::~IdMap() { delete ht; delete ids; }

/** Add a new key->id pair.
 *  @param key is the key for which an id is required
 *  @return the new id or 0 if the key is already mapped or the
 *  operation fails
 */
int IdMap::addPair(uint64_t key) {
	if (validKey(key) || (ids->firstOut() == 0)) return 0;
	int id = ids->firstOut(); 
	if (!ht->insert(key,id)) return 0;
	ids->swap(id);
	return id;
}

/** Add a new key->id pair.
 *  @param key is the key for which an id is required
 *  @param id is the requested id that the key is to be mapped to
 *  @return the new id or 0 if the key is already mapped or the
 *  id is already in use or the operation fails
 */
int IdMap::addPair(uint64_t key, int id) {
	if (validKey(key) || validId(id)) return 0;
	if (!ht->insert(key,id)) return 0;
	ids->swap(id);
	return id;
}

/** Remove a pair from the mapping.
 *  This operation makes the id available to be bound to some other key.
 *  @param key is the key whose id is to be released
 */
void IdMap::dropPair(uint64_t key) {
	int id = ht->lookup(key);
	if (id == 0) return;
	ht->remove(key);
	ids->swap(id);
}

/** Clear the IdMap.  */
void IdMap::clear() {
	for (int i = firstId(); i != 0; i = firstId()) ids->swap(i);
}

/** Create a string representation of the IdMap.
 *  @param s is the string in which the
 */
string& IdMap::toString(string& s) const {
	s = "{ ";
	for (int i = firstId(); i != 0; i = nextId(i)) {
		string s1, s2;
		s = s + "(" + Misc::num2string(ht->getKey(i),s1)
		      + "," + Misc::num2string(i,s2) + ") ";
	}	
	s += "}";
}

void IdMap::write(ostream& out) const {
	string s; out << toString(s);
}
