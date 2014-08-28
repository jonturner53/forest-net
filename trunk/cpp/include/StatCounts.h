/** @file StatCounts.h
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef RATESTATS
#define RATESTATS

#include "Forest.h"

namespace forest {

/** This class stores bit rate and packet rate statistics.
 */
class StatCounts {
public:
	uint64_t bytesIn;	///< input byte count
	uint64_t bytesOut;	///< output byte count
	uint64_t pktsIn;	///< input packet count
	uint64_t pktsOut;	///< output packet count

	/** Default constructor. */
	StatCounts() {
		bytesIn = bytesOut = pktsIn = pktsOut = 0;
	}

	/** Update the input counts.
 	 *  @param len is the number of bytes to add to the byte count
	 */
	void updateIn(int len) { pktsIn++; bytesIn += len; }

	/** Update the output counts.
 	 *  @param len is the number of bytes to add to the byte count
	 */
	void updateOut(int len) { pktsOut++; bytesOut += len; }

	string toString() const {
		string s = to_string(bytesIn) + " " + to_string(bytesOut) +
		     	   " " + to_string(pktsIn) + " " + to_string(pktsOut);
		return s;
	}
	friend ostream& operator<<(ostream& out, const StatCounts& sc) {
		return (out << sc.toString());
	}
};

} // ends namespace


#endif
