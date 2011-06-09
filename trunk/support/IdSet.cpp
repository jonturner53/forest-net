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
	free = new UiList(n);
	idList = new UiDlist(n);

	for (int i = 1; i < n; i++) free->addLast(i);
};
	
IdSet::~IdSet() {
	delete ht; delete free; delete idList;
}

/** Get the id for a given key.
 *  If an id is defined for the input key, then that id is returned.
 *  If no id is defined for this key, one is added and returned.
 *  @param key is the key for which an id is required
 *  @return the corresponding id or 0 on failure
 */
int IdSet::getId(uint64_t key) {
	int id = ht->lookup(key);
	if (id == 0) {
		if (free->empty()) return 0;
		id = free->first(); 
		if (!ht->insert(key,free->first())) return 0;
		free->removeFirst();
		idList->addLast(id);
	}
	return id;
}

/** Release the identifier for a given key.
 *  This operation makes the id available to be bound to some other key.
 *  @param key is the key whose id is to be released
 */
void IdSet::releaseId(uint64_t key) {
	int id = ht->lookup(key);
	if (id == 0 || free->member(id)) return;
	ht->remove(key);
	idList->remove(id);
	free->addFirst(id);
}

/** Clear the IdList.
 */
void IdSet::clear() {
	for (int i = idList->first(); i != 0; i = idList->next(i)) {
		ht->remove(ht->getKey(i));
		free->addLast(i);
	}
	idList->clear();
}

/** Add a representation of the IdSet to the end of a string.
 *  @param s is the string to which the IdSet string is appended.
 */
void IdSet::add2string(string& s) const {
	s += "{ ";
	for (int i = firstId(); i != 0; i = nextId(i)) {
		s += "(";
		Misc::addNum2string(s,ht->getKey(i));
		s += ",";
		Misc::addNum2string(s,i);
		s += ") ";
	}	
	s += "}";
}

void IdSet::write(ostream& out) const {
	string s; add2string(s); out << s;
}
