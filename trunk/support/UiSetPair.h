/** \file UiSetPair.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */
  

#ifndef UISETPAIR_H
#define UISETPAIR_H

#include "stdinc.h"

#include "Misc.h"

/** Data structure that represents a pair of integer sets.
 *  The integer values are stored in a limited range 1..n
 *  and each integer is always in one of the two sets.
 *  The sets are referred to as "in" and "out" can can
 *  be accessed using the provided methods.
 *  The only way to modify the data structure is to
 *  move an item from one set to the other.
 */
class UiSetPair {
public:
		UiSetPair(int);
		~UiSetPair();

	// predicates
	bool	isIn(int) const;
	bool	isOut(int) const;

	// iteration methods
	int	firstIn() const; 	
	int	firstOut() const; 	
	int	lastIn() const; 	
	int	lastOut() const; 	
	int	nextIn(int) const; 	
	int	nextOut(int) const; 	
	int	prevIn(int) const; 	
	int	prevOut(int) const; 	

	// getters
	int	getNumIn() const;
	int	getNumOut() const;

	// modifiers
	void	swap(int);

	// produce string representation
	string&	toString(string&) const;
private:
	int n;			///< largest integer in set pair
	int numIn;		///< number of elements in in-set
	int numOut;		///< number of elements in out-set

	int inHead;		///< first value in the in-set
	int inTail;		///< last value in the in-set
	int outHead;		///< first value in the out-set
	int outTail;		///< last value in the out-set

	int *nxt;		///< nxt[i] defines next value after i
	int *prv;		///< prv[i] defines value preceding i
};

/** Determine if an integer belongs to the "in-set".
 *  @param is is an integer in the range of values supported by the object
 *  @param return true if i is a member of the "in-set", else false.
 */
inline bool UiSetPair::isIn(int i) const {
	return 1 <= i && i <= n && (nxt[i] > 0 || i == inTail);
}

/** Determine if an integer belongs to the "out-set".
 *  @param is is an integer in the range of values supported by the object
 *  @param return true if i is a member of the "out-set", else false.
 */
inline bool UiSetPair::isOut(int i) const {
	return 1 <= i && i <= n && (nxt[i] < 0 || i == outTail);
}

inline int UiSetPair::getNumIn() const { return numIn; }
inline int UiSetPair::getNumOut() const { return numOut; }

/** Get the first int in the in-set.
 *  @return the first value on the in-set or 0 if the list is empty.
 */
inline int UiSetPair::firstIn() const { return inHead; }

/** Get the first int in the out-set.
 *  @return the first value on the out-set or 0 if the list is empty.
 */
inline int UiSetPair::firstOut() const { return outHead; }

/** Get the last int in the in-set.
 *  @return the last value on the in-set or 0 if the list is empty.
 */
inline int UiSetPair::lastIn() const { return inTail; }

/** Get the first int in the out-set.
 *  @return the last value on the out-set or 0 if the list is empty.
 */
inline int UiSetPair::lastOut() const { return outTail; }

/** Get the next value in the inlist.
 *  @param i is the "current" value
 *  @return the next value on the in-set or 0 if no more values
 */
inline int UiSetPair::nextIn(int i) const {
	return (0 <= i && i <= n && nxt[i] > 0 ? nxt[i] : 0);
}

/** Get the next value in the outlist.
 *  @param i is the "current" value
 *  @return the next value on the out-set or 0 if no more values
 */
inline int UiSetPair::nextOut(int i) const {
	return (0 <= i && i <= n && nxt[i] < 0 ? -nxt[i] : 0);
}

/** Get the previous value in the inlist.
 *  @param i is the "current" value
 *  @return the previous value on the in-set or 0 if no more values
 */
inline int UiSetPair::prevIn(int i) const {
	return (0 <= i && i <= n && prv[i] > 0 ? prv[i] : 0);
}

/** Get the previous value in the outlist.
 *  @param i is the "current" value
 *  @return the previous value on the out-set or 0 if no more values
 */
inline int UiSetPair::prevOut(int i) const {
	return (0 <= i && i <= n && prv[i] < 0 ? -prv[i] : 0);
}

#endif
