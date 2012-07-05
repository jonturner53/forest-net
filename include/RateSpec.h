/** @file NetInfo.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef RATESPEC_H
#define RATESPEC_H

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
	RateSpec() { set(0); }

	/** Constructor that sets all rate fields to a common value. */
	RateSpec(int r) { set(r); }

	/** Constructor that sets all rate fields to specified values. */
	RateSpec(int bru, int brd, int pru, int prd) {
		set(bru,brd,pru,prd);
	}

	/** Set all rate fields to a single value. */
	void set(int r) {
		bitRateUp = bitRateDown = pktRateUp = pktRateDown = r;
	}

	/** Set all rate fields to specified values. */
	void set(int bru, int brd, int pru, int prd) {
		bitRateUp = bru; bitRateDown = brd;
		pktRateUp = pru; pktRateDown = prd;
	}

	/** Determine if all fields are zero.
	 */
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

	/** Compare another RateSpec to this one. */
	bool leq(RateSpec& rs) {
		return	bitRateUp <= rs.bitRateUp &&
			bitRateDown <= rs.bitRateDown &&
			pktRateUp <= rs.pktRateUp &&
			pktRateDown <= rs.pktRateDown;
	};

	/** Create a string representation of the rate spec. */
	string& toString(string& s) {
		stringstream ss;
		ss << "(" << this->bitRateUp << "," << this->bitRateDown
		   << "," << this->pktRateUp << "," << this->pktRateDown << ")";
		s = ss.str();
		return s;
	}
};

#endif
