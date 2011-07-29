/** @file IdSet.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "IdSet.h"

/** Constructor for IdSet, allocates space and initializes table.
 *  N1 is the limit on the range of values; it must be less than 2^20.
 */
IdSet::IdSet(int n1) : n(n1) {
	if (n > MAXID) fatal("IdSet::IdSet: specified size too large");
	ht = new UiHashTbl(n);
	ids = new UiSetPair(n);
};
	
IdSet::~IdSet() { delete ht; delete ids; }

/** Add a new key->id mapping.
 *  @param key is the key for which an id is required
 *  @return the new id or 0 if the key is already mapped or the
 *  operation fails
 */
int IdSet::addId(uint64_t key) {
	if (isMapped(key) || (ids->firstOut() == 0)) return 0;
	int id = ids->firstOut(); 
	if (!ht->insert(key,id)) return 0;
	ids->swap(id);
	return id;
}

/** Release the identifier for a given key.
 *  This operation makes the id available to be bound to some other key.
 *  @param key is the key whose id is to be released
 */
void IdSet::releaseId(uint64_t key) {
	int id = ht->lookup(key);
	if (id == 0) return;
	ht->remove(key);
	ids->swap(id);
}

/** Clear the IdSet.  */
void IdSet::clear() {
	for (int i = firstId(); i != 0; i = firstId()) ids->swap(i);
}

/** Create a string representation of the IdSet.
 *  @param s is the string in which the
 */
string& IdSet::toString(string& s) const {
	s = "{ ";
	for (int i = firstId(); i != 0; i = nextId(i)) {
		string s1, s2;
		s = s + "(" + Misc::num2string(ht->getKey(i),s1)
		      + "," + Misc::num2string(i,s2) + ") ";
	}	
	s += "}";
}

void IdSet::write(ostream& out) const {
	string s; out << toString(s);
}
