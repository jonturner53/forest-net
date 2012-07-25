/** @file RateSpec.java
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

package forest.common;

import java.io.*;
import java.util.*;
import algoLib.misc.*;
import algoLib.dataStructures.basic.*;
import algoLib.dataStructures.graphs.*;

/** This class stores bit rates and packet rates for links in
 *  both directions. Directions are referred to as "up" and "down"
 *  to relate easily to usage in comtrees.
 */
public class RateSpec {
	public int bitRateUp;	///< upstream bit rate on comtree link
	public int bitRateDown;	///< downstream bit rate on comtree link
	public int pktRateUp;	///< upstream packet rate on comtree link
	public int pktRateDown;	///< downstream packet rate on comtree link

	/** Default constructor. */
	public RateSpec() { set(0); }

	/** Constructor that sets all rate fields to a common value. */
	public RateSpec(int r) { set(r); }

	/** Constructor that sets all rate fields to specified values. */
	public RateSpec(int bru, int brd, int pru, int prd) {
		set(bru,brd,pru,prd);
	}

	/** Constructor that sets all rate fields to specified values. */
	public RateSpec(RateSpec rs) {
		set(rs.bitRateUp,rs.bitRateDown,rs.pktRateUp,rs.pktRateDown);
	}

	/** Set all rate fields to a single value. */
	public void set(int r) {
		bitRateUp = bitRateDown = pktRateUp = pktRateDown = r;
	}

	/** Set all rate fields to specified values. */
	public void set(int bru, int brd, int pru, int prd) {
		bitRateUp = bru; bitRateDown = brd;
		pktRateUp = pru; pktRateDown = prd;
	}

	/** Determine if all fields are zero.  */
	public boolean isZero() {
		return 	bitRateUp == 0 && bitRateDown == 0 &&
			pktRateUp == 0 && pktRateDown == 0;
	}

	/** Reverse the up/down direction of the fields */
	public void flip() {
		int x;
		x = bitRateUp; bitRateUp = bitRateDown; bitRateDown = x;
		x = pktRateUp; pktRateUp = pktRateDown; pktRateDown = x;
	}

	/** Copy the fields in another RateSpec to this one. */
	public void copyFrom(RateSpec rs) {
		bitRateUp = rs.bitRateUp; bitRateDown = rs.bitRateDown;
		pktRateUp = rs.pktRateUp; pktRateDown = rs.pktRateDown;
	}

	/** Add the fields in another RateSpec to this one. */
	public void add(RateSpec rs) {
		bitRateUp += rs.bitRateUp; bitRateDown += rs.bitRateDown;
		pktRateUp += rs.pktRateUp; pktRateDown += rs.pktRateDown;
	}

	/** Subtract the fields in another RateSpec from this one. */
	public void subtract(RateSpec rs) {
		bitRateUp -= rs.bitRateUp; bitRateDown -= rs.bitRateDown;
		pktRateUp -= rs.pktRateUp; pktRateDown -= rs.pktRateDown;
	}

	/** Scale the fields in this RateSpec by a constant. */
	public void scale(Double f) {
		bitRateUp =   (int) (f*bitRateUp);
		bitRateDown = (int) (f*bitRateDown);
		pktRateUp =   (int) (f*pktRateUp);
		pktRateDown = (int) (f*pktRateDown);
	}

	/** Negate all rates. */
	public void negate() {
		bitRateUp = -bitRateUp; bitRateDown = -bitRateDown;
		pktRateUp = -pktRateUp; pktRateDown = -pktRateDown;
	}

	/** Compare another RateSpec to this one. */
	public boolean leq(RateSpec rs) {
		return	bitRateUp <= rs.bitRateUp &&
			bitRateDown <= rs.bitRateDown &&
			pktRateUp <= rs.pktRateUp &&
			pktRateDown <= rs.pktRateDown;
	};

	/** Create a String representation of the rate spec. */
	public String toString() {
		return	"(" + bitRateUp + "," + bitRateDown +
			"," + pktRateUp + "," + pktRateDown + ")";
	}
}
