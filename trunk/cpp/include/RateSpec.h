/** @file RateSpec.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef RATESPEC_H
#define RATESPEC_H

#include "Forest.h"

namespace forest {


/** This class stores bit rates and packet rates for links in
 *  both directions. Directions are referred to as "up" and "down"
 *  to relate easily to usage in comtrees.
 */
class RateSpec {
public:
	int	bitRateUp;	///< upstream bit rate on comtree link
	int	bitRateDown;	///< downstream bit rate on comtree link
	int	pktRateUp;	///< upstream packet rate on comtree link
	int	pktRateDown;	///< downstream packet rate on comtree link

	/** Default constructor. */
	RateSpec() { reset(); }

	/** Constructor that sets all rate fields to a common value. */
	RateSpec(int r) { set(r); }

	/** Constructor that sets all rate fields to specified values. */
	RateSpec(int bru, int brd, int pru, int prd) {
		set(bru,brd,pru,prd);
	}

	/** Constructor that sets all rate fields to specified values. */
	RateSpec(RateSpec& rs) {
		(*this) = rs;
	}

	void reset() {
		bitRateUp = -1; bitRateDown = pktRateUp = pktRateDown = 0;
	}

	bool isSet() { return bitRateUp != -1; }

	/** Set all rate fields to a single value. */
	void set(int r) {
		bitRateUp = bitRateDown = pktRateUp = pktRateDown = r;
	}

	/** Set all rate fields to specified values. */
	void set(int bru, int brd, int pru, int prd) {
		bitRateUp = bru; bitRateDown = brd;
		pktRateUp = pru; pktRateDown = prd;
	}

	/** Determine if all fields are zero.  */
	bool isZero() {
		return 	bitRateUp == 0 && bitRateDown == 0 &&
			pktRateUp == 0 && pktRateDown == 0;
	}

	/** Reverse the up/down direction of the fields */
	void flip() {
		int x;
		x = bitRateUp; bitRateUp = bitRateDown; bitRateDown = x;
		x = pktRateUp; pktRateUp = pktRateDown; pktRateDown = x;
	}

	/** Add the fields in another RateSpec to this one. */
	void add(RateSpec& rs) {
		bitRateUp += rs.bitRateUp; bitRateDown += rs.bitRateDown;
		pktRateUp += rs.pktRateUp; pktRateDown += rs.pktRateDown;
	}

	/** Subtract the fields in another RateSpec from this one. */
	void subtract(RateSpec& rs) {
		bitRateUp -= rs.bitRateUp; bitRateDown -= rs.bitRateDown;
		pktRateUp -= rs.pktRateUp; pktRateDown -= rs.pktRateDown;
	}

	/** Negate all rates. */
	void negate() {
		bitRateUp = -bitRateUp; bitRateDown = -bitRateDown;
		pktRateUp = -pktRateUp; pktRateDown = -pktRateDown;
	}

	/** Scale the fields in this RateSpec by a constant. */
	void scale(double f) {
		bitRateUp =   (int) (f*bitRateUp);
		bitRateDown = (int) (f*bitRateDown);
		pktRateUp =   (int) (f*pktRateUp);
		pktRateDown = (int) (f*pktRateDown);
	}

	/** Compare another RateSpec to this for equality. */
	bool equals(RateSpec& rs) {
		return	bitRateUp == rs.bitRateUp &&
			bitRateDown == rs.bitRateDown &&
			pktRateUp == rs.pktRateUp &&
			pktRateDown == rs.pktRateDown;
	};

	/** Compare another RateSpec to this one. */
	bool leq(RateSpec& rs) {
		return	bitRateUp <= rs.bitRateUp &&
			bitRateDown <= rs.bitRateDown &&
			pktRateUp <= rs.pktRateUp &&
			pktRateDown <= rs.pktRateDown;
	};

	bool read(istream&);
	string& toString(string& s) const;
};

} // ends namespace


#endif
