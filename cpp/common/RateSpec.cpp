/** @file RateSpec.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RateSpec.h"

namespace forest {


/** Read up to first occurrence of a given character.
 *  @param in is the input stream to read from
 *  @param c  is character to look for
 *  @return c or 0, on end-of-file
 */
bool RateSpec::read(istream& in) {
	return	Util::verify(in,'(') && Util::readNum(in,bitRateUp) &&
		Util::verify(in,',') && Util::readNum(in,bitRateDown) &&
		Util::verify(in,',') && Util::readNum(in,pktRateUp) &&
		Util::verify(in,',') && Util::readNum(in,pktRateDown) &&
		Util::verify(in,')');
}

/** Create a string representation of the rate spec.
 *  @param s is a reference to a string in which result is returned
 *  @param return a reference to s
 */
string RateSpec::toString() const {
	stringstream ss;
	ss << "(" << this->bitRateUp << "," << this->bitRateDown
	   << "," << this->pktRateUp << "," << this->pktRateDown << ")";
	return ss.str();
}

} // ends namespace

